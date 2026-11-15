#include "uefi.h"

const uint8_t gop_guid[16] = {
    0xde, 0xa9, 0x42, 0x90, 0xdc, 0x23, 0x38, 0x4a,
    0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a
};

static EFI_GRAPHICS_OUTPUT_PROTOCOL *g_gop;
static uint32_t *g_framebuffer;
static uint32_t g_framebuffer_width;
static uint32_t g_framebuffer_height;
static uint32_t g_framebuffer_stride;
static uint32_t g_pixel_format;

extern uint8_t *vga;
extern uint8_t keys[128];
extern uint8_t palette[768];
extern float dPitch;
extern float dYaw;
extern void init_world(void);
extern void show_splash(void);
extern void handle_input(void);
extern void update(float dt);
extern void draw_frame(void);

static void print(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out, CHAR16 *text) {
    if (out && out->output_string)
        out->output_string(out, text);
}

static int locate_gop(EFI_SYSTEM_TABLE *system_table) {
    EFI_STATUS status = system_table->boot_services->locate_protocol(
        gop_guid, 0, (void **)&g_gop);
    return status == EFI_SUCCESS && g_gop && g_gop->mode;
}

/* UEFI replacement for the old mode-0x13 VGA setup. */
static int init_graphics(EFI_SYSTEM_TABLE *system_table) {
    if (!locate_gop(system_table))
        return 0;

    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = g_gop->mode;
    if (!mode->info || mode->info->pixel_format > EFI_GRAPHICS_PIXEL_FORMAT_BGR)
        return 0;

    g_framebuffer = (uint32_t *)(uintptr_t)mode->framebuffer_base;
    g_framebuffer_width = mode->info->horizontal_resolution;
    g_framebuffer_height = mode->info->vertical_resolution;
    g_framebuffer_stride = mode->info->pixels_per_scan_line;
    g_pixel_format = mode->info->pixel_format;
    return g_framebuffer != 0 && g_framebuffer_width != 0 && g_framebuffer_height != 0;
}

static void poll_input(EFI_SYSTEM_TABLE *system_table) {
    EFI_INPUT_KEY key;
    while (system_table->con_in->read_key_stroke(system_table->con_in, &key) == EFI_SUCCESS) {
        uint32_t code = 0;
        uint8_t release_event = 0;
        if (key.unicode_char == 'w' || key.unicode_char == 'W') code = 0x11;
        if (key.unicode_char == 'a' || key.unicode_char == 'A') code = 0x1e;
        if (key.unicode_char == 's' || key.unicode_char == 'S') code = 0x1f;
        if (key.unicode_char == 'd' || key.unicode_char == 'D') code = 0x20;
        if (key.unicode_char == 'q' || key.unicode_char == 'Q') { code = 0x10; release_event = 1; }
        if (key.unicode_char == 'e' || key.unicode_char == 'E') { code = 0x12; release_event = 1; }
        if (key.unicode_char == 'i' || key.unicode_char == 'I') { dPitch = 1.0f; code = 0; }
        if (key.unicode_char == 'k' || key.unicode_char == 'K') { dPitch = -1.0f; code = 0; }
        if (key.unicode_char == 'j' || key.unicode_char == 'J') { dYaw = 1.0f; code = 0; }
        if (key.unicode_char == 'l' || key.unicode_char == 'L') { dYaw = -1.0f; code = 0; }
        if (key.unicode_char == ' ') code = 0x39;
        if (key.unicode_char == 0x1b) { code = 0x01; release_event = 1; }
        if (key.scan_code == 1) { dPitch = 1.0f; code = 0; }
        if (key.scan_code == 2) { dPitch = -1.0f; code = 0; }
        if (key.scan_code == 3) { dYaw = -1.0f; code = 0; }
        if (key.scan_code == 4) { dYaw = 1.0f; code = 0; }
        if (code != 0)
            keys[code] = release_event ? 0x80 : 0x81;
    }
}

static void present_indexed(void) {
    /* Scale the legacy 320x200 logical framebuffer to the active GOP mode.
       This also clears the firmware's console pixels outside the old VGA
       rectangle instead of leaving the boot manager visible behind the game. */
    uint32_t y;
    for (y = 0; y < g_framebuffer_height; ++y) {
        uint32_t source_y = (y * 200U) / g_framebuffer_height;
        uint32_t x;
        for (x = 0; x < g_framebuffer_width; ++x) {
            uint32_t source_x = (x * 320U) / g_framebuffer_width;
            uint32_t index = vga[source_y * 320U + source_x];
            uint32_t red = palette[index * 3U] << 2;
            uint32_t green = palette[index * 3U + 1U] << 2;
            uint32_t blue = palette[index * 3U + 2U] << 2;
            uint32_t pixel = g_pixel_format == EFI_GRAPHICS_PIXEL_FORMAT_BGR
                ? (red << 16) | (green << 8) | blue
                : (blue << 16) | (green << 8) | red;
            g_framebuffer[y * g_framebuffer_stride + x] = pixel;
        }
    }
}

static void clear_input_pulse(void) {
    keys[0x11] = 0;
    keys[0x1e] = 0;
    keys[0x1f] = 0;
    keys[0x20] = 0;
    keys[0x48] = 0;
    keys[0x4b] = 0;
    keys[0x4d] = 0;
    keys[0x50] = 0;
    dPitch = 0.0f;
    dYaw = 0.0f;
}

static float frame_delta(EFI_SYSTEM_TABLE *system_table, uint64_t *last_ticks) {
    uint64_t now;
    if (!system_table->boot_services->get_next_monotonic_count ||
        system_table->boot_services->get_next_monotonic_count(&now) != EFI_SUCCESS)
        return 0.016f;
    if (*last_ticks == 0) {
        *last_ticks = now;
        return 0.016f;
    }
    uint64_t elapsed = now - *last_ticks;
    *last_ticks = now;
    float dt = (float)elapsed / 10000000.0f;
    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.100f) dt = 0.100f;
    return dt;
}

EFI_STATUS uefi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    (void)image_handle;
    uint64_t last_ticks = 0;

    print(system_table->con_out, (CHAR16 *)L"MineAssemble x64 UEFI port\r\n");
    if (!init_graphics(system_table)) {
        print(system_table->con_out, (CHAR16 *)L"GOP framebuffer unavailable\r\n");
        return 1;
    }

    show_splash();
    present_indexed();
    if (system_table->boot_services->stall)
        system_table->boot_services->stall(500000);
    init_world();
    print(system_table->con_out, (CHAR16 *)L"GOP ready; x64 UEFI game loop active\r\n");

    for (;;) {
        poll_input(system_table);
        handle_input();
        update(frame_delta(system_table, &last_ticks));
        draw_frame();
        present_indexed();
        clear_input_pulse();
        if (system_table->boot_services->stall)
            system_table->boot_services->stall(16000);
    }
}
