#!/usr/bin/env python3
"""Boot the UEFI image under OVMF and assert that it writes a framebuffer."""
from pathlib import Path
import json
import os
import shutil
import socket
import subprocess
import time

root = Path(__file__).resolve().parent
keep_frames = os.environ.get("KEEP_SMOKE_FRAMES") == "1"
code = Path(os.environ.get("OVMF_CODE", "/usr/share/edk2/x64/OVMF_CODE.4m.fd"))
vars_template = Path(os.environ.get("OVMF_VARS", "/usr/share/edk2/x64/OVMF_VARS.4m.fd"))
if not code.exists() or not vars_template.exists():
    raise SystemExit("OVMF firmware files not found")

vars_copy = root / "smoke-vars.fd"
qmp_path = root / "smoke-qmp.sock"
screen_path = root / "smoke-screen.ppm"
splash_path = root / "smoke-splash.ppm"
game_path = root / "smoke-game.ppm"
view_path = root / "smoke-view.ppm"
for path in (vars_copy, qmp_path, screen_path, splash_path, game_path, view_path):
    path.unlink(missing_ok=True)
shutil.copyfile(vars_template, vars_copy)

command = [
    "qemu-system-x86_64", "-machine", "q35", "-m", "128M",
    "-display", "none", "-serial", "none",
    "-drive", f"if=pflash,format=raw,unit=0,readonly=on,file={code}",
    "-drive", f"if=pflash,format=raw,unit=1,file={vars_copy}",
    "-drive", f"format=raw,file={root / 'mineassemble64.img'}",
    "-qmp", f"unix:{qmp_path},server=on,wait=off",
]
process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

try:
    deadline = time.monotonic() + 8
    while not qmp_path.exists() and time.monotonic() < deadline:
        if process.poll() is not None:
            break
        time.sleep(0.1)
    if not qmp_path.exists():
        raise RuntimeError("QEMU did not create its QMP socket")

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as qmp:
        qmp.settimeout(4)
        qmp.connect(str(qmp_path))
        qmp.recv(4096)  # greeting

        def command_qmp(request_id, name, arguments=None):
            request = {"execute": name, "id": request_id}
            if arguments is not None:
                request["arguments"] = arguments
            qmp.sendall((json.dumps(request) + "\r\n").encode())
            data = b""
            while b'"id": ' + str(request_id).encode() not in data:
                data += qmp.recv(65536)
            return data

        command_qmp(1, "qmp_capabilities")
        time.sleep(1)
        command_qmp(2, "screendump", {"filename": str(splash_path)})
        command_qmp(3, "send-key", {"keys": [{"type": "qcode", "data": "enter"}]})
        time.sleep(8)
        command_qmp(4, "screendump", {"filename": str(screen_path)})
        for request_id, key in enumerate(("w",) * 10, 5):
            command_qmp(request_id, "send-key", {"keys": [{"type": "qcode", "data": key}]})
        time.sleep(1)
        command_qmp(20, "screendump", {"filename": str(game_path)})
        for request_id, key in enumerate(("k",) * 10, 21):
            command_qmp(request_id, "send-key", {"keys": [{"type": "qcode", "data": key}]})
        time.sleep(1)
        command_qmp(31, "screendump", {"filename": str(view_path)})
        command_qmp(32, "quit")

    process.wait(timeout=4)
    splash = splash_path.read_bytes()
    before = screen_path.read_bytes()
    game = game_path.read_bytes()
    view = view_path.read_bytes()

    marker = game.find(b"\n255\n")
    splash_marker = splash.find(b"\n255\n")
    before_marker = before.find(b"\n255\n")
    view_marker = view.find(b"\n255\n")
    if marker < 0 or splash_marker < 0 or before_marker < 0 or view_marker < 0:
        raise RuntimeError("QEMU did not produce valid PPM screenshots")
    pixels = game[marker + len(b"\n255\n"):]
    splash_pixels = splash[splash_marker + len(b"\n255\n"):]
    before_pixels = before[before_marker + len(b"\n255\n"):]
    view_pixels = view[view_marker + len(b"\n255\n"):]

    nonzero = sum(1 for byte in pixels if byte)
    if nonzero == 0:
        raise RuntimeError("UEFI framebuffer screenshot was completely black")
    changed = sum(1 for before, after in zip(splash_pixels, pixels) if before != after)
    input_changed = sum(1 for old, new in zip(before_pixels, pixels) if old != new)
    view_changed = sum(1 for old, new in zip(pixels, view_pixels) if old != new)
    if view_changed == 0:
        raise RuntimeError("QEMU camera input did not change the rendered game state")
    print(f"UEFI smoke test passed: {nonzero} non-black bytes, {changed} splash changes, {input_changed} movement changes, {view_changed} camera changes")
finally:
    if process.poll() is None:
        process.kill()
        process.wait()
    for path in (vars_copy, qmp_path):
        path.unlink(missing_ok=True)
    if not keep_frames:
        for path in (screen_path, splash_path, game_path, view_path):
            path.unlink(missing_ok=True)
