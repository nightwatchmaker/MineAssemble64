CC := clang
LD := ld.lld


CFLAGS := --target=x86_64-pc-win32 -ffreestanding -fshort-wchar -mno-red-zone
CFLAGS += -fno-stack-protector -fno-stack-check -fno-asynchronous-unwind-tables
CFLAGS += -Wall -Wextra -Werror -Iuefi
LDFLAGS := /subsystem:efi_application /entry:efi_main /nodefaultlib

.PHONY: all image run smoke-test clean

OVMF_CODE ?= /usr/share/edk2/x64/OVMF_CODE.4m.fd
OVMF_VARS ?= /usr/share/edk2/x64/OVMF_VARS.4m.fd

all: BOOTX64.EFI

image: BOOTX64.EFI
	dd if=/dev/zero of=mineassemble64.img bs=1M count=64 status=none
	parted -s mineassemble64.img mklabel gpt mkpart ESP fat32 1MiB 100% set 1 esp on
	mformat -i mineassemble64.img@@1048576 -F -v MINE64
	mmd -i mineassemble64.img@@1048576 ::/EFI ::/EFI/BOOT
	mcopy -i mineassemble64.img@@1048576 BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI


entry.obj: entry.s
	$(CC) --target=x86_64-pc-win32 -c -o $@ $<

main.obj: main.c uefi.h
	$(CC) $(CFLAGS) -c -o $@ $<

reference.obj: ../src/reference.c
	$(CC) $(CFLAGS) -Wno-sign-compare -c -o $@ $<

assets.s: ../src/textures.asm ../src/vga.asm gen_assets.py
	python3 gen_assets.py

assets.obj: assets.s
	$(CC) --target=x86_64-pc-win32 -c -o $@ $<

game.obj: game.s
	$(CC) --target=x86_64-pc-win32 -c -o $@ $<

BOOTX64.EFI: entry.obj main.obj reference.obj assets.obj game.obj
	$(LD) -flavor link $(LDFLAGS) /out:$@ $^

run: image
	cp $(OVMF_VARS) mineassemble64-vars.fd
	qemu-system-x86_64 -machine q35 -m 128M -drive if=pflash,format=raw,unit=0,readonly=on,file=$(OVMF_CODE) -drive if=pflash,format=raw,unit=1,file=mineassemble64-vars.fd -drive format=raw,file=mineassemble64.img
	rm -f mineassemble64-vars.fd

smoke-test: image qemu_smoke.py
	python3 qemu_smoke.py

clean:
	rm -f *.obj assets.s BOOTX64.EFI mineassemble64.img mineassemble64-vars.fd
