#!/usr/bin/env python3
"""Audit the UEFI object exports against the ported assembly surface."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parent
objects = [root / "entry.obj", root / "main.obj", root / "reference.obj", root / "assets.obj", root / "game.obj"]
output = subprocess.check_output(["llvm-nm", *map(str, objects)], text=True)
symbols = set(re.findall(r"\b[0-9A-Fa-f]+\s+[A-Za-z]\s+(\S+)$", output, re.MULTILINE))
expected = {
    "efi_main", "sinf", "cosf", "absf", "init_world", "get_block", "set_block",
    "set_pos", "set_view", "handle_input", "handle_collision", "ray_dir",
    "face_normal", "tex_index", "ray_color", "reticle_color", "show_splash",
    "draw_frame", "palette", "tex_grass", "tex_dirt", "tex_grass_side",
    "splash_bitmap", "world", "playerPos", "pitch", "yaw", "keys",
}
missing = sorted(expected - symbols)
if missing:
    raise SystemExit("missing UEFI exports: " + ", ".join(missing))

bios_only = {
    "start", "main", "init_idt", "init_pic", "init_input", "init_time",
    "set_timer_frequency", "set_irq_handler", "clear_irq_handler", "enable_irq",
    "init_vga", "irq0_end", "irq1_end",
}
leaked = sorted(bios_only & symbols)
if leaked:
    raise SystemExit("BIOS-only exports leaked into UEFI objects: " + ", ".join(leaked))

print(f"UEFI symbol audit passed: {len(expected)} required exports, no BIOS-only leaks")
