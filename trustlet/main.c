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
#define TEE_PROPSET_TEE_IMPLEMENTATION 0xfffffffdu

enum {
    CMD_ECHO     = 1,   /* upper-case tci[1] bytes of payload at &tci[2]         */
    CMD_ADD      = 2,   /* tci[2] = tci[2] + tci[3]                              */
    CMD_INFO     = 3,   /* tci[2] = magic, tci[3] = received tci_len             */
    CMD_DEVICEID = 4,   /* 16-byte gpd.tee.deviceID UUID into &tci[2]            */
    CMD_RAND     = 5,   /* tci[1] bytes of secure-world random into &tci[2]      */
    CMD_SHA256   = 6,   /* SHA-256 of tci[1] input bytes -> 32 bytes at &tci[2]  */
    CMD_VERSION  = 7,   /* mcVersionInfo_t into &tci[2]                          */
    CMD_TIME     = 8,   /* 64-bit secure timestamp into tci[2],tci[3]           */
    CMD_LOG      = 9,   /* tlApiLogPrintf the payload string                    */
};

#define TA_MAGIC   0x00007461u     /* "ta" */

/* initialized global: forces a non-empty .data segment */
uint32_t ta_magic = TA_MAGIC;

void tlMain(const addr_t tciBuffer, const uint32_t tciBufferLen)
{
    volatile uint32_t *tci = (volatile uint32_t *)tciBuffer;
    uint32_t len = tciBufferLen;

    for (;;) {
        tlApiWaitNotification(TLAPI_INFINITE);

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
            int r = tlApiGetIDs(TEE_PROPSET_TEE_IMPLEMENTATION,
                                "gpd.tee.deviceID", buf);
            volatile uint8_t *p = (volatile uint8_t *)&tci[2];
            for (uint32_t i = 0; i < 16; i++) p[i] = buf[i];
            status = (uint32_t)r;   /* GP result (0 = success) */
            break;
        }
        case CMD_RAND: {
            uint32_t n = tci[1];
            uint32_t cap = (len > 8) ? (len - 8) : 0;
            if (n > cap) n = cap;
            size_t outlen = n;
            tlApiResult_t r = tlApiRandomGenerateData(TLAPI_ALG_SECURE_RANDOM,
                                                      (uint8_t *)&tci[2], &outlen);
            status = (r == TLAPI_OK) ? (uint32_t)outlen : r;
            break;
        }
        case CMD_SHA256: {
            uint32_t n = tci[1];
            uint32_t cap = (len > 8) ? (len - 8) : 0;
            if (n > cap) n = cap;
            uint8_t hash[32];
            size_t hlen = sizeof(hash);
            tlApiCrSession_t md;
            tlApiResult_t r = tlApiMessageDigestInit(&md, TLAPI_ALG_SHA256);
            if (r == TLAPI_OK)
                r = tlApiMessageDigestDoFinal(md, (const uint8_t *)&tci[2], n, hash, &hlen);
            if (r == TLAPI_OK) {
                volatile uint8_t *p = (volatile uint8_t *)&tci[2];
                for (uint32_t i = 0; i < hlen; i++) p[i] = hash[i];
            }
            status = (r == TLAPI_OK) ? (uint32_t)hlen : r;
            break;
        }
        case CMD_VERSION: {
            mcVersionInfo_t vi;
            tlApiResult_t r = tlApiGetMobicoreVersion(&vi);
            if (r == TLAPI_OK) {
                volatile uint8_t *p = (volatile uint8_t *)&tci[2];
                const uint8_t *s = (const uint8_t *)&vi;
                for (uint32_t i = 0; i < sizeof(vi); i++) p[i] = s[i];
            }
            status = r;
            break;
        }
        case CMD_TIME: {
            timestamp_t ts = 0;
            tlApiResult_t r = tlApiGetSecureTimestamp(&ts);
            if (r == TLAPI_OK) { tci[2] = (uint32_t)ts; tci[3] = (uint32_t)(ts >> 32); }
            status = r;
            break;
        }
        case CMD_LOG:
            tlApiLogPrintf("ta: %s", (const char *)&tci[2]);
            status = 0;
            break;
        default:
            status = 0xffffffffu;       /* unknown command */
        }

        tci[0] = cmd | RSP_FLAG;
        tci[1] = status;
        tlApiNotify();
    }
}
