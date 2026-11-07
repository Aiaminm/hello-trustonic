/* mytl/start.S — minimal from-scratch Kinibi/MobiCore trustlet runtime.
 *
 * Contract (reverse-engineered — see notes/trustlet-abi.md):
 *   - MCLF header `entry` points here (_start); kernel enters at EL0.
 *   - tlApi gateway = *(uint32_t*)0x108c  (mclfTextHeader.mcLibEntry, loader-filled).
 *     Call as gw(cmd, arg1, arg2) with cmd in w0: 6=waitNotification(timeout),
 *     7=notify, 0x80000000=startup-ready.
 *   - The kernel writes a {uint32_t tci_ptr; uint32_t tci_len;} descriptor into the
 *     last 8 bytes of bss (__tci_desc == bss_end-8) before entering the TA.
 *
 * `hint #38/#34` are bti jc / bti c (NOPs where BTI is off) — defensive landing pads.
 */
    .arch armv8-a
    .text

    .global _start
    .type   _start, %function
_start:
    hint    #38                     /* bti jc : entered by the kernel */
    /* install our own stack (top of a bss array) before touching the stack */
    adrp    x0, __stack_top
    add     x0, x0, :lo12:__stack_top
    mov     sp, x0

    /* tlApi startup/ready: gw(0x80000000, 0, 0) */
    mov     w1, #0
    mov     w2, #0
    movz    w0, #0x0000
    movk    w0, #0x8000, lsl #16    /* w0 = 0x80000000 */
    bl      gw_enter

    /* fetch kernel-filled TCI descriptor: w0 = tci_ptr, w1 = tci_len */
    adrp    x9, __tci_desc
    add     x9, x9, :lo12:__tci_desc
    ldp     w0, w1, [x9]
    bl      tlMain
0:  b       0b                      /* tlMain must not return */
    .size   _start, .-_start

    /* uint32_t tlapi_wait(uint32_t timeout) */
    .global tlapi_wait
    .type   tlapi_wait, %function
tlapi_wait:
    hint    #34                     /* bti c */
    mov     w1, w0                  /* timeout -> arg1 */
    mov     w0, #6                  /* cmd 6 = wait notification */
    b       gw_enter
    .size   tlapi_wait, .-tlapi_wait

    /* void tlapi_notify(void) */
    .global tlapi_notify
    .type   tlapi_notify, %function
tlapi_notify:
    hint    #34                     /* bti c */
    mov     w0, #7                  /* cmd 7 = notify */
    b       gw_enter
    .size   tlapi_notify, .-tlapi_notify

    /* int tlapi_get_property_uuid(uint32_t propset, const char *name, void *out16)
     * -> gw(0x2b, propset, name, out). cmd 0x2b = TEE_GetPropertyAsUUID (writes 16 B).
     * Returns the gateway status in w0. */
    .global tlapi_get_property_uuid
    .type   tlapi_get_property_uuid, %function
tlapi_get_property_uuid:
    hint    #34                     /* bti c */
    mov     x3, x2                  /* out     -> arg3 */
    mov     x2, x1                  /* name    -> arg2 */
    mov     w1, w0                  /* propset -> arg1 (zero-extended) */
    mov     w0, #0x2b               /* cmd 0x2b = get property as UUID */
    b       gw_enter
    .size   tlapi_get_property_uuid, .-tlapi_get_property_uuid

    /* tail-call the gateway at *(u32*)0x108c; lr is preserved so the gateway's
     * ret returns to our caller (same shape as the real McLib stubs). */
    .type   gw_enter, %function
gw_enter:
    hint    #34                     /* bti c */
    movz    x16, #0x108c
    ldr     w16, [x16]              /* 32-bit gateway address */
    br      x16
    .size   gw_enter, .-gw_enter
