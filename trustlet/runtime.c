/* runtime.c — C entry: startup handshake, hand the TCI buffer to tlMain.
 * start.S sets the stack and calls ta_start.
 */
#include <stdint.h>
#include "tlapi.h"

/* kernel writes {tci_ptr, tci_len} into the last 8 bytes of bss before entry */
extern uint32_t __tci_desc[2];

__attribute__((noreturn)) void ta_start(void)
{
    tlApiLibStartup();
    tlMain((addr_t)(unsigned long)__tci_desc[0], __tci_desc[1]);
    for (;;) { }
}
