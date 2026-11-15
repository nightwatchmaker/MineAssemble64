; x86-64 UEFI application entry shim.
; UEFI uses the Microsoft x64 calling convention: image handle in RCX,
; system table in RDX, and 32 bytes of caller-provided shadow space.

bits 64
default rel

global efi_main
extern uefi_main

section .text

efi_main:
    sub rsp, 40                 ; shadow space + 16-byte call alignment
    call uefi_main
    add rsp, 40
    ret
