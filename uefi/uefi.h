#ifndef MINEASSEMBLE_UEFI_H
#define MINEASSEMBLE_UEFI_H

#include <stdint.h>

typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef uint16_t CHAR16;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;
typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;
typedef struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;
typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_TEXT_OUTPUT_STRING)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, CHAR16 *);
typedef EFI_STATUS (*EFI_TEXT_CLEAR_SCREEN)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *);
typedef EFI_STATUS (*EFI_TEXT_RESET)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, uint8_t);
typedef EFI_STATUS (*EFI_TEXT_SET_ATTRIBUTE)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, uint64_t);
typedef EFI_STATUS (*EFI_LOCATE_PROTOCOL)(const void *, void *, void **);
typedef EFI_STATUS (*EFI_STALL)(uint64_t);
typedef EFI_STATUS (*EFI_GET_NEXT_MONOTONIC_COUNT)(uint64_t *);
typedef EFI_STATUS (*EFI_GET_MEMORY_MAP)(uint64_t *, void *, uint64_t *, uint64_t *, uint32_t *);
typedef EFI_STATUS (*EFI_ALLOCATE_POOL)(uint32_t, uint64_t, void **);
typedef EFI_STATUS (*EFI_FREE_POOL)(void *);

typedef struct {
    uint16_t scan_code;
    CHAR16 unicode_char;
} EFI_INPUT_KEY;

typedef EFI_STATUS (*EFI_INPUT_READ_KEY)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *, EFI_INPUT_KEY *);

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET reset;
    EFI_TEXT_OUTPUT_STRING output_string;
    EFI_TEXT_OUTPUT_STRING test_string;
    EFI_TEXT_OUTPUT_STRING query_mode;
    EFI_TEXT_OUTPUT_STRING set_mode;
    EFI_TEXT_SET_ATTRIBUTE set_attribute;
    EFI_TEXT_CLEAR_SCREEN clear_screen;
};

struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    void *reset;
    EFI_INPUT_READ_KEY read_key_stroke;
    void *wait_for_key;
};

struct EFI_GRAPHICS_OUTPUT_MODE_INFORMATION {
    uint32_t version;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_format;
    uint32_t pixel_information[4];
    uint32_t pixels_per_scan_line;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE {
    uint32_t max_mode;
    uint32_t mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    uint64_t info_size;
    EFI_PHYSICAL_ADDRESS framebuffer_base;
    uint64_t framebuffer_size;
};

struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_STATUS (*query_mode)(EFI_GRAPHICS_OUTPUT_PROTOCOL *, uint32_t, uint64_t *, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **);
    EFI_STATUS (*set_mode)(EFI_GRAPHICS_OUTPUT_PROTOCOL *, uint32_t);
    void (*blt)(void);
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
};

struct EFI_BOOT_SERVICES {
    char _header[0x18];
    void *raise_tpl;
    void *restore_tpl;
    void *allocate_pages;
    void *free_pages;
    EFI_GET_MEMORY_MAP get_memory_map;
    EFI_ALLOCATE_POOL allocate_pool;
    EFI_FREE_POOL free_pool;
    void *create_event;
    void *set_timer;
    void *wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    void *handle_protocol;
    void *reserved;
    void *register_protocol_notify;
    void *locate_handle;
    void *locate_device_path;
    void *install_configuration_table;
    void *load_image;
    void *start_image;
    void *exit;
    void *unload_image;
    void *exit_boot_services;
    EFI_GET_NEXT_MONOTONIC_COUNT get_next_monotonic_count;
    EFI_STALL stall;
    void *set_watchdog_timer;
    void *connect_controller;
    void *disconnect_controller;
    void *open_protocol;
    void *close_protocol;
    void *open_protocol_information;
    void *protocols_per_handle;
    void *locate_handle_buffer;
    EFI_LOCATE_PROTOCOL locate_protocol;
};

struct EFI_SYSTEM_TABLE {
    char _reserved_0[0x18];
    CHAR16 *firmware_vendor;
    uint32_t firmware_revision;
    EFI_HANDLE console_in_handle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *con_in;
    EFI_HANDLE console_out_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *con_out;
    void *standard_error_handle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *std_err;
    EFI_SYSTEM_TABLE *runtime_services;
    EFI_BOOT_SERVICES *boot_services;
};

#define EFI_SUCCESS 0
#define EFI_NOT_READY 0x8000000000000006ULL
#define EFI_ALLOCATE_ANY_PAGES 0
#define EFI_LOADER_DATA 2
#define EFI_GRAPHICS_PIXEL_FORMAT_RGB 0
#define EFI_GRAPHICS_PIXEL_FORMAT_BGR 1

extern const uint8_t gop_guid[16];

#endif
