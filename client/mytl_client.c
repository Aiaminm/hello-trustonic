/* mytl_client.c — normal-world driver for the mytl echo trustlet.
 *
 * Opens a MobiCore session to UUID 0a0a0a0a…0001, then exercises the three
 * commands the TA implements over the shared TCI buffer:
 *   CMD_ECHO  - upper-cases a string
 *   CMD_ADD   - adds two u32s
 *   CMD_INFO  - returns the TA magic + the tci_len the TA was given
 *
 * Build with the Android NDK (Bionic), link libMcClient:
 *   $NDK/.../aarch64-linux-android<API>-clang mytl_client.c -lMcClient -o mytl_client
 * Run on device (needs mcDriverDaemon up and the .tlbin in /vendor/app/mcRegistry).
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "MobiCoreDriverApi.h"

#define TCI_LEN   0x1000u
#define RSP_FLAG  0x80000000u
enum { CMD_ECHO = 1, CMD_ADD = 2, CMD_INFO = 3 };

static const mcUuid_t MYTL_UUID = {
    { 0x0a,0x0a,0x0a,0x0a, 0,0,0,0, 0,0,0,0, 0,0,0,0x01 }
};

static mcSessionHandle_t sess;
static volatile uint32_t *t;     /* TCI as u32 words */

/* send one command, wait for the reply, return the TA status word (tci[1]) */
static int call(uint32_t cmd)
{
    mcResult_t r;
    t[0] = cmd;
    if ((r = mcNotify(&sess)) != MC_DRV_OK) {
        printf("  mcNotify: 0x%08x\n", r); return -1;
    }
    if ((r = mcWaitNotification(&sess, MC_INFINITE_TIMEOUT)) != MC_DRV_OK) {
        printf("  mcWaitNotification: 0x%08x\n", r); return -1;
    }
    if (t[0] != (cmd | RSP_FLAG))
        printf("  warn: reply marker tci[0]=0x%08x (expected 0x%08x)\n", t[0], cmd | RSP_FLAG);
    return (int)t[1];
}

int main(void)
{
    mcResult_t r;
    uint8_t *tci = NULL;

    if ((r = mcOpenDevice(MC_DEVICE_ID_DEFAULT)) != MC_DRV_OK) {
        printf("mcOpenDevice: 0x%08x\n", r); return 1;
    }
    if ((r = mcMallocWsm(MC_DEVICE_ID_DEFAULT, 0, TCI_LEN, &tci, 0)) != MC_DRV_OK) {
        printf("mcMallocWsm: 0x%08x\n", r); return 1;
    }
    sess.deviceId = MC_DEVICE_ID_DEFAULT;
    if ((r = mcOpenSession(&sess, &MYTL_UUID, tci, TCI_LEN)) != MC_DRV_OK) {
        printf("mcOpenSession: 0x%08x  <- TA failed to load/start\n", r);
        return 1;
    }
    printf("session opened: id=%u\n", sess.sessionId);
    t = (volatile uint32_t *)tci;

    /* CMD_ECHO */
    const char *msg = "hello trustzone";
    strcpy((char *)&t[2], msg);
    t[1] = (uint32_t)strlen(msg);
    int st = call(CMD_ECHO);
    printf("ECHO: status=%d  result=\"%s\"\n", st, (char *)&t[2]);

    /* CMD_ADD */
    t[2] = 40; t[3] = 2;
    st = call(CMD_ADD);
    printf("ADD:  status=%d  40+2=%u\n", st, t[2]);

    /* CMD_INFO */
    st = call(CMD_INFO);
    printf("INFO: status=%d  magic=0x%08x  ta_tci_len=%u\n", st, t[2], t[3]);

    mcCloseSession(&sess);
    mcFreeWsm(MC_DEVICE_ID_DEFAULT, tci);
    mcCloseDevice(MC_DEVICE_ID_DEFAULT);
    return 0;
}
