/* mytl/tlmain.c — echo trustlet command loop.
 *
 * tlMain(tci, len) is called once by the McLib startup (start.S) with the TCI
 * buffer the normal world passed to mcOpenSession(). We own both ends, so the
 * TCI protocol is whatever we define here:
 *
 *   tci[0] = command id         (NW writes; TA replies with RSP_FLAG OR'd in)
 *   tci[1] = status / length    (TA writes status; some cmds use it as a length)
 *   tci[2..] = payload words
 *
 * The loop: wait for the NW notification, service the command in place, flip the
 * response flag, notify the NW back.
 */
#include <stdint.h>

extern uint32_t tlapi_wait(uint32_t timeout);
extern void     tlapi_notify(void);

#define TLAPI_INFINITE  0xffffffffu
#define RSP_FLAG        0x80000000u

enum {
    CMD_ECHO = 1,   /* upper-case tci[1] bytes of payload at &tci[2]             */
    CMD_ADD  = 2,   /* tci[2] = tci[2] + tci[3]                                  */
    CMD_INFO = 3,   /* tci[2] = magic, tci[3] = received tci_len                 */
};

#define MYTL_MAGIC 0x6d79746cu       /* "mytl" */

/* an initialized global forces a non-empty .data segment (KeyMint has one too) */
uint32_t mytl_magic = MYTL_MAGIC;

void tlMain(volatile uint32_t *tci, uint32_t len)
{
    for (;;) {
        tlapi_wait(TLAPI_INFINITE);

        uint32_t cmd    = tci[0] & ~RSP_FLAG;
        uint32_t status = 0;

        switch (cmd) {
        case CMD_ECHO: {
            volatile uint8_t *p = (volatile uint8_t *)&tci[2];
            uint32_t n = tci[1];
            uint32_t max = (len > 8) ? (len - 8) : 0;   /* payload bytes available */
            if (n > max) n = max;
            for (uint32_t i = 0; i < n; i++) {
                uint8_t c = p[i];
                if (c >= 'a' && c <= 'z')
                    p[i] = (uint8_t)(c - 32);
            }
            status = n;     /* report how many bytes were processed (after clamping) */
            break;
        }
        case CMD_ADD:
            tci[2] = tci[2] + tci[3];
            break;
        case CMD_INFO:
            tci[2] = mytl_magic;
            tci[3] = len;
            break;
        default:
            status = 0xffffffffu;       /* unknown command */
        }

        tci[0] = cmd | RSP_FLAG;
        tci[1] = status;
        tlapi_notify();
    }
}
