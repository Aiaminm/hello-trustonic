/* main.c — echo trustlet command loop.
 *
 * TCI protocol (TA-defined; the NW client matches it):
 *   tci[0]   = command id      (TA replies with RSP_FLAG set)
 *   tci[1]   = status / length
 *   tci[2..] = payload words
 */
#include <stdint.h>
#include "tlapi.h"

#define RSP_FLAG        0x80000000u

enum {
    CMD_ECHO     = 1,   /* upper-case tci[1] bytes of payload at &tci[2]         */
    CMD_ADD      = 2,   /* tci[2] = tci[2] + tci[3]                              */
    CMD_INFO     = 3,   /* tci[2] = magic, tci[3] = received tci_len             */
    CMD_DEVICEID = 4,   /* 16-byte gpd.tee.deviceID UUID into &tci[2]            */
};

#define TA_MAGIC   0x00007461u     /* "ta" */

/* initialized global: forces a non-empty .data segment */
uint32_t ta_magic = TA_MAGIC;

void ta_main(volatile uint32_t *tci, uint32_t len)
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
            tci[2] = ta_magic;
            tci[3] = len;
            break;
        case CMD_DEVICEID: {
            /* gpd.tee.deviceID: 16-byte TEE_UUID via a local buffer, then into the TCI */
            uint8_t buf[16];
            for (uint32_t i = 0; i < sizeof(buf); i++) buf[i] = 0;
            int r = tlapi_get_property_uuid(TEE_PROPSET_TEE_IMPLEMENTATION,
                                            "gpd.tee.deviceID", buf);
            volatile uint8_t *p = (volatile uint8_t *)&tci[2];
            for (uint32_t i = 0; i < 16; i++) p[i] = buf[i];
            status = (uint32_t)r;   /* GP result (0 = success) */
            break;
        }
        default:
            status = 0xffffffffu;       /* unknown command */
        }

        tci[0] = cmd | RSP_FLAG;
        tci[1] = status;
        tlapi_notify();
    }
}
