/* start.S — entry stub: set SP, hand off to ta_start (runtime.c). */
    .arch armv8-a
    .text

    .global _start
    .type   _start, %function
_start:
    bti     jc
    adrp    x0, __stack_top
    add     x0, x0, :lo12:__stack_top
    mov     sp, x0
    bl      ta_start
0:  b       0b                      /* ta_start must not return */
    .size   _start, .-_start
