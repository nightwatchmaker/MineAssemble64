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
code = Path(os.environ.get("OVMF_CODE", "/usr/share/edk2/x64/OVMF_CODE.4m.fd"))
vars_template = Path(os.environ.get("OVMF_VARS", "/usr/share/edk2/x64/OVMF_VARS.4m.fd"))
if not code.exists() or not vars_template.exists():
    raise SystemExit("OVMF firmware files not found")

vars_copy = root / "smoke-vars.fd"
qmp_path = root / "smoke-qmp.sock"
screen_path = root / "smoke-screen.ppm"
for path in (vars_copy, qmp_path, screen_path):
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
        time.sleep(8)
        command_qmp(2, "screendump", {"filename": str(screen_path)})
        command_qmp(3, "quit")

    process.wait(timeout=4)
    data = screen_path.read_bytes()
    marker = data.find(b"\n255\n")
    if marker < 0:
        raise RuntimeError("QEMU did not produce a valid PPM screenshot")
    pixels = data[marker + len(b"\n255\n"):]
    nonzero = sum(1 for byte in pixels if byte)
    if nonzero == 0:
        raise RuntimeError("UEFI framebuffer screenshot was completely black")
    print(f"UEFI smoke test passed: {nonzero} non-black framebuffer bytes")
finally:
    if process.poll() is None:
        process.kill()
        process.wait()
    for path in (vars_copy, qmp_path, screen_path):
        path.unlink(missing_ok=True)
