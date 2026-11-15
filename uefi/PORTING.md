# x64 UEFI port map

The legacy 32-bit BIOS build remains under `src/`. The UEFI build deliberately
uses separate x86-64 Microsoft-ABI objects under `uefi/`.

| Legacy BIOS assembly | x64 UEFI replacement |
|---|---|
| `init.asm` / `main.asm` | `entry.s`, `main.c` and the PE/COFF linker entry |
| `globals.asm` | data and BSS symbols in `game.s` |
| `cmath.asm` | x87-backed `sinf`, `cosf`, and SSE `absf` in `game.s` |
| `player.asm` | `set_pos`, `set_view`, and player globals in `game.s` |
| `world.asm` | `init_world`, `get_block`, `set_block` in `game.s` |
| `input.asm` | x64 `handle_input` and `handle_collision` in `game.s`, with UEFI polling in `main.c` |
| `graphics.asm` | ray setup, face/texture/color routines, reticle, and frame renderer in `game.s` |
| `splash.asm` | `show_splash` in `game.s`; bitmap is converted by `gen_assets.py` |
| `textures.asm` / VGA palette | `assets.s` generated from the original data at build time |
| `vga.asm` | GOP framebuffer setup/presentation in `main.c` |
| `interrupts.asm` | UEFI console input polling, Boot Services timing, and GOP services |

The BIOS-only PIC, IDT, IRQ, VGA-port, and mode-0x13 symbols are intentionally
not exported by the UEFI object because those hardware contracts do not exist
for a UEFI application. The replacement behavior is exercised by
`make uefi-smoke-test`, which boots the generated ESP under OVMF.
