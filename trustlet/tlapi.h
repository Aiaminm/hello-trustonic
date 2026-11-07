/* tlapi.h — runtime/TA interface: the TA calls tlapi_*; the runtime calls ta_main. */
#ifndef TLAPI_H
#define TLAPI_H
#include <stdint.h>

#define TLAPI_INFINITE                 0xffffffffu
#define TEE_PROPSET_TEE_IMPLEMENTATION 0xfffffffdu

uint32_t tlapi_wait(uint32_t timeout);
void     tlapi_notify(void);
int      tlapi_get_property_uuid(uint32_t propset, const char *name, void *out16);

void ta_main(volatile uint32_t *tci, uint32_t len);

#endif /* TLAPI_H */
