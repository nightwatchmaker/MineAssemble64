# x86-64 UEFI application entry shim.
# UEFI uses the Microsoft x64 calling convention: image handle in RCX,
# system table in RDX, and 32 bytes of caller-provided shadow space.

.text
.globl efi_main
.def efi_main;
.scl 2;
.type 32;
.endef

.extern uefi_main

.text
efi_main:
    subq $40, %rsp              # shadow space + 16-byte call alignment
    call uefi_main
    addq $40, %rsp
    retq
