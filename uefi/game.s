.intel_syntax noprefix

.section .data
.align 4
_fltused:       .long 0
vga:            .quad render_buffer
half_pi:        .float 1.57079632679
neg_half_pi:    .float -1.57079632679
zero:           .float 0.0
hFov:           .float 90.0
colTolerance:   .float 0.1
playerHeight:   .float 1.8
dPitch:         .float 0.0
dYaw:           .float 0.0
velocity:       .float 0.0, 0.0, 0.0
worldSX:        .long 32
worldSY:        .long 32
worldSZ:        .long 32
halfWorldSX:    .long 16
halfWorldSY:    .long 16
halfWorldSZ:    .long 16
sunDir:         .float 1.0, 3.0, 1.0

.section .bss
.align 16
playerPos:      .zero 12
pitch:          .zero 4
pitchS:         .zero 4
pitchC:         .zero 4
yaw:            .zero 4
yawS:           .zero 4
yawC:           .zero 4
keys:           .zero 128
world:          .zero 32768
render_buffer:  .zero 64000

.section .text

.globl _fltused, vga, half_pi, neg_half_pi, zero, hFov, dPitch, dYaw, velocity
.globl colTolerance, playerHeight, playerPos, pitch, pitchS, pitchC
.globl yaw, yawS, yawC, keys, world, worldSX, worldSY, worldSZ, sunDir
.globl render_buffer
.globl sinf, cosf, absf, set_pos, set_view
.globl init_world, get_block, set_block
.globl handle_input, handle_collision
.globl ray_dir, face_normal, tex_index, ray_color, reticle_color, show_splash, draw_frame
.extern handle_key
.extern raytrace
.extern splash_bitmap

/* MS x64 float ABI: XMM0 contains the argument and result. */
sinf:
    sub rsp, 24
    movss [rsp+16], xmm0
    fld dword ptr [rsp+16]
    fsin
    fstp dword ptr [rsp+16]
    movss xmm0, [rsp+16]
    add rsp, 24
    ret

cosf:
    sub rsp, 24
    movss [rsp+16], xmm0
    fld dword ptr [rsp+16]
    fcos
    fstp dword ptr [rsp+16]
    movss xmm0, [rsp+16]
    add rsp, 24
    ret

absf:
    andps xmm0, XMMWORD PTR [abs_mask]
    ret

set_pos:
    movss dword ptr [playerPos], xmm0
    movss dword ptr [playerPos+4], xmm1
    movss dword ptr [playerPos+8], xmm2
    ret

set_view:
    movss dword ptr [pitch], xmm0
    movss dword ptr [yaw], xmm1
    movss xmm2, dword ptr [pitch]
    maxss xmm2, dword ptr [neg_half_pi]
    minss xmm2, dword ptr [half_pi]
    movss dword ptr [pitch], xmm2
    movss xmm0, xmm2
    call sinf
    movss dword ptr [pitchS], xmm0
    movss xmm0, xmm2
    call cosf
    movss dword ptr [pitchC], xmm0
    movss xmm0, dword ptr [yaw]
    call sinf
    movss dword ptr [yawS], xmm0
    movss xmm0, dword ptr [yaw]
    call cosf
    movss dword ptr [yawC], xmm0
    ret

/* void init_world(void) */
init_world:
    push rdi
    sub rsp, 32
    lea rdi, world
    xor eax, eax
    mov ecx, 32768
    rep stosb                         /* RDI is volatile in MS ABI */
    xor r10d, r10d                    /* x */
.init_x:
    xor r11d, r11d                    /* y */
.init_y:
    xor edx, edx                      /* z */
.init_z:
    mov eax, r10d
    imul eax, 1024
    mov r8d, r11d
    imul r8d, 32
    add eax, r8d
    add eax, edx
    cmp r11d, 16
    jae .air
    mov byte ptr [world+rax], 1
    jmp .next_z
.air:
    mov byte ptr [world+rax], 0
.next_z:
    inc edx
    cmp edx, 32
    jb .init_z
    inc r11d
    cmp r11d, 32
    jb .init_y
    inc r10d
    cmp r10d, 32
    jb .init_x
    movss xmm0, dword ptr [halfWorldSX]
    cvtsi2ss xmm0, dword ptr [halfWorldSX]
    movss xmm1, dword ptr [halfWorldSY]
    cvtsi2ss xmm1, dword ptr [halfWorldSY]
    addss xmm1, dword ptr [playerHeight]
    movss xmm2, dword ptr [halfWorldSZ]
    cvtsi2ss xmm2, dword ptr [halfWorldSZ]
    call set_pos
    xorps xmm0, xmm0
    movaps xmm1, xmm0
    call set_view
    add rsp, 32
    pop rdi
    ret

/* uint8_t get_block(int x,int y,int z) */
get_block:
    imul ecx, 1024
    imul edx, 32
    add ecx, edx
    add ecx, r8d
    movzx eax, byte ptr [world+rcx]
    ret

/* void set_block(int x,int y,int z,uint8_t type) */
set_block:
    imul ecx, 1024
    imul edx, 32
    add ecx, edx
    add ecx, r8d
    mov byte ptr [world+rcx], r9b
    ret

handle_input:
    sub rsp, 40
    mov ecx, 0x48
    call handle_key
    mov ecx, 0x50
    call handle_key
    mov ecx, 0x4b
    call handle_key
    mov ecx, 0x4d
    call handle_key
    mov ecx, 0x39
    call handle_key
    mov ecx, 0x10
    call handle_key
    mov ecx, 0x12
    call handle_key
    mov ecx, 0x01
    call handle_key
    add rsp, 40
    ret

/* void handle_collision(vec3 *pos, vec3 *velocity) */
handle_collision:
    sub rsp, 72
    mov qword ptr [rsp+64], rdx
    mov r8, rsp
    call raytrace
    cmp dword ptr [rsp], 1
    jne .collision_done
    movss xmm0, dword ptr [rsp+28]
    comiss xmm0, dword ptr [colTolerance]
    jae .collision_done
    mov r10, qword ptr [rsp+64]
    cmp dword ptr [rsp+16], 0
    je .no_x
    mov dword ptr [r10], 0
.no_x:
    cmp dword ptr [rsp+20], 0
    je .no_y
    mov dword ptr [r10+4], 0
.no_y:
    cmp dword ptr [rsp+24], 0
    je .collision_done
    mov dword ptr [r10+8], 0
.collision_done:
    add rsp, 72
    ret

/* ray_dir(vec3 *out, int x, int y) */
ray_dir:
    sub rsp, 56
    movdqu [rsp], xmm6
    movdqu [rsp+16], xmm7
    mov eax, edx
    sub eax, 160
    cvtsi2ss xmm0, eax
    divss xmm0, dword ptr [screen_half_width]  /* clipX */
    mov eax, 100
    sub eax, r8d
    cvtsi2ss xmm1, eax
    divss xmm1, dword ptr [screen_half_height] /* clipY */

    movss xmm2, dword ptr [fov]
    movss xmm3, dword ptr [yawC]
    movss xmm4, dword ptr [yawS]
    movss xmm5, dword ptr [pitchS]

    /* x = aspect*fov*yawC*clipX + fov*yawS*pitchS*clipY - pitchC*yawS */
    movss xmm6, xmm2
    mulss xmm6, xmm3
    mulss xmm6, xmm0
    mulss xmm6, dword ptr [aspect]
    movss xmm7, xmm2
    mulss xmm7, xmm4
    mulss xmm7, xmm5
    mulss xmm7, xmm1
    addss xmm6, xmm7
    movss xmm7, dword ptr [pitchC]
    mulss xmm7, xmm4
    subss xmm6, xmm7
    movss dword ptr [rcx], xmm6

    /* y = fov*pitchC*clipY + pitchS */
    movss xmm6, xmm2
    mulss xmm6, dword ptr [pitchC]
    mulss xmm6, xmm1
    addss xmm6, xmm5
    movss dword ptr [rcx+4], xmm6

    /* z = -aspect*fov*yawS*clipX + fov*yawC*pitchS*clipY - pitchC*yawC */
    movss xmm6, xmm2
    mulss xmm6, xmm4
    mulss xmm6, xmm0
    mulss xmm6, dword ptr [neg_aspect]
    movss xmm7, xmm2
    mulss xmm7, xmm3
    mulss xmm7, xmm5
    mulss xmm7, xmm1
    addss xmm6, xmm7
    movss xmm7, dword ptr [pitchC]
    mulss xmm7, xmm3
    subss xmm6, xmm7
    movss dword ptr [rcx+8], xmm6
    movdqu xmm6, [rsp]
    movdqu xmm7, [rsp+16]
    add rsp, 56
    ret

face_normal:
    mov dword ptr [rdx], 0
    mov dword ptr [r8], 0
    mov dword ptr [r9], 0
    cmp ecx, 0
    jne .face_right
    mov dword ptr [rdx], -1
    ret
.face_right:
    cmp ecx, 1
    jne .face_bottom
    mov dword ptr [rdx], 1
    ret
.face_bottom:
    cmp ecx, 2
    jne .face_top
    mov dword ptr [r8], -1
    ret
.face_top:
    cmp ecx, 3
    jne .face_back
    mov dword ptr [r8], 1
    ret
.face_back:
    cmp ecx, 4
    jne .face_front
    mov dword ptr [r9], -1
    ret
.face_front:
    mov dword ptr [r9], 1
    ret

/* int tex_index(vec3 *pos, int face) */
tex_index:
    /* Select the two fractional coordinates for the face. */
    cmp edx, 0
    je .u_z_v_y
    cmp edx, 1
    je .u_z_v_y
    cmp edx, 2
    je .u_x_v_z
    cmp edx, 3
    je .u_x_v_z
    movss xmm0, dword ptr [rcx]
    movss xmm1, dword ptr [rcx+4]
    jmp .coords
.u_z_v_y:
    movss xmm0, dword ptr [rcx+8]
    movss xmm1, dword ptr [rcx+4]
    jmp .coords
.u_x_v_z:
    movss xmm0, dword ptr [rcx]
    movss xmm1, dword ptr [rcx+8]
.coords:
    mulss xmm0, dword ptr [texture_scale]
    mulss xmm1, dword ptr [texture_scale]
    cvttss2si eax, xmm0
    cvttss2si edx, xmm1
    and eax, 15
    and edx, 15
    sub edx, 15
    neg edx
    imul eax, 16
    add eax, edx
    ret

/* uint8_t ray_color(int x,int y,int z,vec3 *pos,int tex,int face) */
ray_color:
    sub rsp, 120
    mov dword ptr [rsp+80], ecx
    mov dword ptr [rsp+84], edx
    mov dword ptr [rsp+88], r8d
    mov qword ptr [rsp+96], r9
    mov eax, dword ptr [rsp+160]
    mov dword ptr [rsp+104], eax
    mov eax, dword ptr [rsp+168]
    mov dword ptr [rsp+108], eax

    /* Trace toward the sun and select the dark half of the texture if blocked. */
    mov rcx, qword ptr [rsp+96]
    lea rdx, sunDir
    lea r8, [rsp+32]
    call raytrace
    cmp dword ptr [rsp+32], 0
    je .lit
    add dword ptr [rsp+104], 256
.lit:
    mov eax, dword ptr [rsp+108]
    cmp eax, 2
    je .dirt_texture
    cmp eax, 3
    je .grass_texture

    /* Side blocks at the world ceiling show the grass edge. */
    cmp dword ptr [rsp+84], 31
    je .grass_side_texture
    mov ecx, dword ptr [rsp+80]
    mov edx, dword ptr [rsp+84]
    inc edx
    mov r8d, dword ptr [rsp+88]
    call get_block
    cmp eax, 1
    je .dirt_texture
.grass_side_texture:
    lea r10, tex_grass_side
    jmp .texture_selected
.dirt_texture:
    lea r10, tex_dirt
    jmp .texture_selected
.grass_texture:
    lea r10, tex_grass
.texture_selected:
    mov eax, dword ptr [rsp+104]
    movzx eax, byte ptr [r10+rax]
    add rsp, 120
    ret

/* byte reticle_color(byte color): return an indexed inverse-color accent. */
reticle_color:
    push r10
    push r11
    and ecx, 255
    mov eax, ecx
    imul eax, 3
    lea r10, palette
    movzx edx, byte ptr [r10+rax]
    movzx r8d, byte ptr [r10+rax+1]
    movzx r9d, byte ptr [r10+rax+2]
    cmp r9d, 63
    je .reticle_sky
    cmp r8d, edx
    jg .reticle_grass
    mov eax, 254
    jmp .reticle_done_color
.reticle_grass:
    mov eax, 253
    jmp .reticle_done_color
.reticle_sky:
    mov eax, 255
.reticle_done_color:
    pop r11
    pop r10
    ret

/* Copy the original 320x200 splash bitmap into the logical render buffer. */
show_splash:
    push rsi
    push rdi
    lea rsi, splash_bitmap
    lea rdi, render_buffer
    mov ecx, 320 * 200
    rep movsb
    pop rdi
    pop rsi
    ret

/* Render the original 320x200 logical target into vga. */
draw_frame:
    push r12
    push r13
    push r14
    sub rsp, 88
    xor r11d, r11d                    /* y */
    lea r12, [rsp+32]                /* ray direction */
    lea r13, render_buffer
.row:
    xor r14d, r14d                    /* x */
.pixel:
    mov rcx, r12
    mov edx, r14d
    mov r8d, r11d
    call ray_dir
    mov dword ptr [rsp+72], r11d
    mov rcx, qword ptr [playerPos]
    mov qword ptr [rsp+48], rcx
    mov ecx, dword ptr [playerPos+8]
    mov dword ptr [rsp+56], ecx
    lea rcx, [rsp+48]
    mov rdx, r12
    xor r8d, r8d
    call raytrace
    mov r11d, dword ptr [rsp+72]
    mov byte ptr [rsp+64], al
    mov eax, r11d
    imul eax, 320
    add eax, r14d
    mov dword ptr [rsp+68], eax
    mov edx, dword ptr [rsp+68]
    movzx eax, byte ptr [rsp+64]
    mov byte ptr [r13+rdx], al

    /* Restore the original 6-pixel crosshair at the logical screen center. */
    cmp r14d, 157
    jle .vertical_reticle
    cmp r14d, 163
    jge .vertical_reticle
    cmp r11d, 100
    jne .vertical_reticle
    movzx ecx, byte ptr [rsp+64]
    call reticle_color
    mov edx, dword ptr [rsp+68]
    mov byte ptr [r13+rdx], al
    jmp .reticle_done
.vertical_reticle:
    cmp r11d, 97
    jle .reticle_done
    cmp r11d, 103
    jge .reticle_done
    cmp r14d, 160
    jne .reticle_done
    movzx ecx, byte ptr [rsp+64]
    call reticle_color
    mov edx, dword ptr [rsp+68]
    mov byte ptr [r13+rdx], al
.reticle_done:
    inc r14d
    cmp r14d, 320
    jb .pixel
    inc r11d
    cmp r11d, 200
    jb .row
    add rsp, 88
    pop r14
    pop r13
    pop r12
    ret

.section .rdata
.align 16
abs_mask:           .long 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff
screen_half_width:  .float 160.0
screen_half_height: .float 100.0
fov:                .float 0.625
aspect:             .float 1.6
neg_aspect:         .float -1.6
neg_one:            .float -1.0
texture_scale:      .float 15.0
