/* runtime.c — McLib runtime: tlApi gateway + stubs, C entry (ta_start). */
#include <stdint.h>
#include "mcLoadFormat.h"
#include "tlapi.h"

#define TEXT_HDR ((const volatile mclfTextHeader_t *)(0x1000UL + MCLF_HEADER_SIZE_V23))

/* tlApi gateway: cmd in x0, args x1..x3 */
static uint32_t gw(unsigned long cmd, unsigned long a1, unsigned long a2, unsigned long a3)
{
    unsigned long (*g)(unsigned long, unsigned long, unsigned long, unsigned long)
        = (void *)(unsigned long)TEXT_HDR->mcLibEntry;
    return (uint32_t)g(cmd, a1, a2, a3);
}

uint32_t tlapi_wait(uint32_t timeout) { return gw(6, timeout, 0, 0); }   /* waitNotification */
void     tlapi_notify(void)           { gw(7, 0, 0, 0); }                /* notify */

int tlapi_get_property_uuid(uint32_t propset, const char *name, void *out16)
{
    return (int)gw(0x2b, propset, (unsigned long)name, (unsigned long)out16);  /* TEE_GetPropertyAsUUID */
}

/* kernel writes {tci_ptr, tci_len} into the last 8 bytes of bss before entry */
extern uint32_t __tci_desc[2];

__attribute__((noreturn)) void ta_start(void)
{
    gw(0x80000000, 0, 0, 0);   /* startup / ready */
    ta_main((volatile uint32_t *)(unsigned long)__tci_desc[0], __tci_desc[1]);
    for (;;) { }
}
