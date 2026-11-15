CC := clang
LD := ld.lld


CFLAGS := --target=x86_64-pc-win32 -ffreestanding -fshort-wchar -mno-red-zone
CFLAGS += -fno-stack-protector -fno-stack-check -fno-asynchronous-unwind-tables
CFLAGS += -Wall -Wextra -Werror -Iuefi
LDFLAGS := /subsystem:efi_application /entry:efi_main /nodefaultlib

.PHONY: all image clean

all: BOOTX64.EFI

image: BOOTX64.EFI
	dd if=/dev/zero of=mineassemble64.img bs=1M count=16 status=none
	mkfs.fat -F 32 mineassemble64.img >/dev/null
	mmd -i mineassemble64.img ::/EFI ::/EFI/BOOT
	mcopy -i mineassemble64.img BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI


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

clean:
	rm -f *.obj assets.s BOOTX64.EFI mineassemble64.img
