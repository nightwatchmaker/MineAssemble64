CC := clang
LD := ld.lld


CFLAGS := --target=x86_64-pc-win32 -ffreestanding -fshort-wchar -mno-red-zone
CFLAGS += -fno-stack-protector -fno-stack-check -fno-asynchronous-unwind-tables
CFLAGS += -Wall -Wextra -Werror -Iuefi
LDFLAGS := /subsystem:efi_application /entry:efi_main /nodefaultlib

.PHONY: all clean

all: BOOTX64.EFI


entry.obj: entry.s
	$(CC) --target=x86_64-pc-win32 -c -o $@ $<

main.obj: main.c uefi.h
	$(CC) $(CFLAGS) -c -o $@ $<

reference.obj: ../src/reference.c
	$(CC) $(CFLAGS) -Wno-sign-compare -c -o $@ $<

game.obj: game.s
	$(CC) --target=x86_64-pc-win32 -c -o $@ $<

BOOTX64.EFI: entry.obj main.obj reference.obj game.obj
	$(LD) -flavor link $(LDFLAGS) /out:$@ $^

clean:
	rm -f *.obj BOOTX64.EFI
