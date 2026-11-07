/* ta_client.c — normal-world driver for the ta echo trustlet.
 * Opens a MobiCore session to the TA and exercises its commands over the TCI buffer.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "MobiCoreDriverApi.h"

#define TCI_LEN   0x1000u
#define RSP_FLAG  0x80000000u
enum { CMD_ECHO = 1, CMD_ADD = 2, CMD_INFO = 3, CMD_DEVICEID = 4 };

static const mcUuid_t TA_UUID = {
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
    if ((r = mcOpenSession(&sess, &TA_UUID, tci, TCI_LEN)) != MC_DRV_OK) {
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

    /* CMD_DEVICEID — gpd.tee.deviceID (16-byte UUID) */
    st = call(CMD_DEVICEID);
    const uint8_t *id = (const uint8_t *)&t[2];
    printf("DEVID: status=0x%08x  gpd.tee.deviceID=", (unsigned)st);
    for (int i = 0; i < 16; i++) printf("%02x%s", id[i], (i==3||i==5||i==7||i==9)?"-":"");
    printf("\n");

    mcCloseSession(&sess);
    mcFreeWsm(MC_DEVICE_ID_DEFAULT, tci);
    mcCloseDevice(MC_DEVICE_ID_DEFAULT);
    return 0;
}
