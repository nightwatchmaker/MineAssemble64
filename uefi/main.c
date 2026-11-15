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

static void print(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out, CHAR16 *text) {
    if (out && out->output_string)
        out->output_string(out, text);
}

static int locate_gop(EFI_SYSTEM_TABLE *system_table) {
    EFI_STATUS status = system_table->boot_services->locate_protocol(
        gop_guid, 0, (void **)&g_gop);
    return status == EFI_SUCCESS && g_gop && g_gop->mode;
}

/* This is the UEFI replacement for the old mode-0x13 VGA setup. */
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
    return g_framebuffer != 0 && g_framebuffer_width != 0 && g_framebuffer_height != 0;
}

static void draw_gop_smoke_test(void) {
    uint32_t y;
    for (y = 0; y < g_framebuffer_height; ++y) {
        uint32_t x;
        for (x = 0; x < g_framebuffer_width; ++x) {
            uint32_t red = (x * 255U) / g_framebuffer_width;
            uint32_t blue = (y * 255U) / g_framebuffer_height;
            g_framebuffer[y * g_framebuffer_stride + x] = (blue << 16) | (red << 8) | 0x20U;
        }
    }

    /* A high-contrast block makes stride/pixel writes obvious in firmware. */
    for (y = 0; y < 64 && y < g_framebuffer_height; ++y) {
        uint32_t x;
        for (x = 0; x < 256 && x < g_framebuffer_width; ++x)
            g_framebuffer[y * g_framebuffer_stride + x] = 0x00f0f0f0U;
    }
}

/* Exposed for the x64 renderer port. Pixels are native GOP 32-bit pixels. */
uint32_t *uefi_framebuffer(uint32_t *width, uint32_t *height, uint32_t *stride) {
    if (width) *width = g_framebuffer_width;
    if (height) *height = g_framebuffer_height;
    if (stride) *stride = g_framebuffer_stride;
    return g_framebuffer;
}

EFI_STATUS uefi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    (void)image_handle;

    print(system_table->con_out, (CHAR16 *)L"MineAssemble x64 UEFI port\r\n");
    if (!init_graphics(system_table)) {
        print(system_table->con_out, (CHAR16 *)L"GOP framebuffer unavailable\r\n");
        return 1;
    }

    draw_gop_smoke_test();

    print(system_table->con_out, (CHAR16 *)L"GOP ready; framebuffer smoke test drawn\r\n");
    print(system_table->con_out, (CHAR16 *)L"This entry point replaces GRUB, VGA mode 0x13, PIC, and IDT setup.\r\n");

    for (;;) {
        if (system_table->boot_services->stall)
            system_table->boot_services->stall(1000000);
    }
}
