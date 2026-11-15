#!/usr/bin/env python3
"""Convert the original NASM byte assets to COFF-compatible GAS syntax."""
from pathlib import Path
import re

root = Path(__file__).parent.parent
out = Path(__file__).parent / "assets.s"
blocks = {"tex_grass": [], "tex_dirt": [], "tex_grass_side": [], "palette": []}
active = None

for source in (root / "src" / "textures.asm", root / "src" / "vga.asm"):
    for line in source.read_text().splitlines():
        stripped = line.split(";", 1)[0].strip()
        if not stripped:
            continue
        label = stripped.split(None, 1)[0].rstrip(":")
        if label in blocks:
            active = label
            remainder = stripped[len(label):].strip()
            if remainder.startswith("db"):
                stripped = remainder
            else:
                continue
        elif not stripped.startswith("db"):
            active = None
        if active is None or not stripped.startswith("db"):
            continue
        values = stripped[2:].strip()
        for token in values.split(","):
            token = token.strip()
            if token:
                blocks[active].append(int(token, 16) if token.lower().startswith("0x") else int(token, 10))

splash = []
for line in (root / "src" / "splash.asm").read_text().splitlines():
    for bitmap_line in re.findall(r'"([01]+)"', line):
        splash.extend(ord(pixel) - ord("0") for pixel in bitmap_line)
if len(splash) != 320 * 200:
    raise SystemExit(f"unexpected splash size: {len(splash)}")

with out.open("w") as stream:
    stream.write(".section .data\n")
    for name, values in blocks.items():
        stream.write(f".globl {name}\n{name}:\n")
        for offset in range(0, len(values), 16):
            stream.write("    .byte " + ", ".join(str(v) for v in values[offset:offset + 16]) + "\n")
    stream.write(".globl splash_bitmap\nsplash_bitmap:\n")
    for offset in range(0, len(splash), 16):
        stream.write("    .byte " + ", ".join(str(v) for v in splash[offset:offset + 16]) + "\n")
