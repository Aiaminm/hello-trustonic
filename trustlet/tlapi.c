/* tlapi.c — tlApi wrappers: invoke the gateway as f(cmd, args...). */
#include <stdint.h>
#include "tlapi.h"

/* mclfTextHeader.mcLibEntry: 32-bit gateway pointer at VA 0x108c, loader-filled */
#define GW ((uintptr_t)*(const volatile uint32_t *)0x108cUL)

void tlApiLibStartup(void)
{
    ((void (*)(uint32_t, uint32_t, uint32_t))GW)(TLAPI_CMD_MCLIB_INIT, 0, 0);
}

tlApiResult_t tlApiGetIDs(uint32_t propset, const char *name, void *out)
{
    return ((tlApiResult_t (*)(uint32_t, uint32_t, const char *, void *))GW)
            (TLAPI_CMD_GET_IDS, propset, name, out);
}

tlApiResult_t tlApiWaitNotification(uint32_t timeout)
{
    return ((tlApiResult_t (*)(uint32_t, uint32_t))GW)(0x6, timeout);
}

tlApiResult_t tlApiNotify(void)
{
    return ((tlApiResult_t (*)(uint32_t))GW)(0x7);
}

tlApiResult_t tlApiCrAbort(tlApiCrSession_t sessionHandle)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t))GW)(0xd, sessionHandle);
}

tlApiResult_t tlApiGenerateKeyPair(tlApiKeyPair_t * keyPair, tlApiKeyPairType_t keyPairType, size_t len)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiKeyPair_t *, tlApiKeyPairType_t, size_t))GW)(0xf, keyPair, keyPairType, len);
}

tlApiResult_t tlApiCipherInitWithData(tlApiCrSession_t *pSessionHandle, tlApiCipherAlg_t alg, tlApiCipherMode_t mode, const tlApiKey_t *key, const uint8_t *buffer, size_t bufferLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t *, tlApiCipherAlg_t, tlApiCipherMode_t, const tlApiKey_t *, const uint8_t *, size_t))GW)(0x10, pSessionHandle, alg, mode, key, buffer, bufferLen);
}

tlApiResult_t tlApiCipherUpdate(tlApiCrSession_t sessionHandle, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t, const uint8_t *, size_t, uint8_t *, size_t *))GW)(0x11, sessionHandle, srcData, srcLen, destData, destLen);
}

tlApiResult_t tlApiSignatureInitWithData(tlApiCrSession_t *pSessionHandle, const tlApiKey_t *key, tlApiSigMode_t mode, tlApiSigAlg_t alg, const uint8_t *buffer, size_t bufferLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t *, const tlApiKey_t *, tlApiSigMode_t, tlApiSigAlg_t, const uint8_t *, size_t))GW)(0x13, pSessionHandle, key, mode, alg, buffer, bufferLen);
}

tlApiResult_t tlApiCipherDoFinal(tlApiCrSession_t sessionHandle, const uint8_t *srcData, size_t srcLen, uint8_t *destData, size_t *destLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t, const uint8_t *, size_t, uint8_t *, size_t *))GW)(0x12, sessionHandle, srcData, srcLen, destData, destLen);
}

tlApiResult_t tlApiSignatureUpdate(tlApiCrSession_t sessionHandle, const uint8_t *message, size_t messageLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t, const uint8_t *, size_t))GW)(0x14, sessionHandle, message, messageLen);
}

tlApiResult_t tlApiSignatureSign(tlApiCrSession_t sessionHandle, const uint8_t *message, size_t messageLen, uint8_t *signature, size_t *signatureLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t, const uint8_t *, size_t, uint8_t *, size_t *))GW)(0x15, sessionHandle, message, messageLen, signature, signatureLen);
}

tlApiResult_t tlApiSignatureVerify(tlApiCrSession_t sessionHandle, const uint8_t *message, size_t messageLen, const uint8_t *signature, size_t signatureLen, bool *validity)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t, const uint8_t *, size_t, const uint8_t *, size_t, bool *))GW)(0x16, sessionHandle, message, messageLen, signature, signatureLen, validity);
}

tlApiResult_t tlApiMessageDigestInitWithData(tlApiCrSession_t *pSessionHandle, tlApiMdAlg_t alg, const uint8_t *buffer, const uint8_t lengthOfDataHashedPreviously[8])
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t *, tlApiMdAlg_t, const uint8_t *, const uint8_t *))GW)(0x17, pSessionHandle, alg, buffer, lengthOfDataHashedPreviously);
}

tlApiResult_t tlApiMessageDigestUpdate(tlApiCrSession_t sessionHandle, const uint8_t *message, size_t messageLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t, const uint8_t *, size_t))GW)(0x18, sessionHandle, message, messageLen);
}

tlApiResult_t tlApiMessageDigestDoFinal(tlApiCrSession_t sessionHandle, const uint8_t *message, size_t messageLen, uint8_t *hash, size_t *hashLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiCrSession_t, const uint8_t *, size_t, uint8_t *, size_t *))GW)(0x19, sessionHandle, message, messageLen, hash, hashLen);
}

tlApiResult_t tlApiRandomGenerateData(tlApiRngAlg_t alg, uint8_t *randomBuffer, size_t *randomLen)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiRngAlg_t, uint8_t *, size_t *))GW)(0xe, alg, randomBuffer, randomLen);
}

void * tlApiMalloc(uint32_t size, uint32_t hint)
{
    return ((void * (*)(uint32_t, uint32_t, uint32_t))GW)(0x1c, size, hint);
}

void * tlApiRealloc(void* buffer, uint32_t newSize)
{
    return ((void * (*)(uint32_t, void*, uint32_t))GW)(0x1d, buffer, newSize);
}

void tlApiFree(void *buffer)
{
    ((void (*)(uint32_t, void *))GW)(0x1e, buffer);
}

tlApiResult_t tlApiGetVersion(uint32_t *tlApiVersion)
{
    return ((tlApiResult_t (*)(uint32_t, uint32_t *))GW)(0x1, tlApiVersion);
}

tlApiResult_t tlApiGetMobicoreVersion(mcVersionInfo_t * mcVersionInfo)
{
    return ((tlApiResult_t (*)(uint32_t, mcVersionInfo_t *))GW)(0x2, mcVersionInfo);
}

void tlApiExit(uint32_t exitCode)
{
    ((void (*)(uint32_t, uint32_t))GW)(0x4, exitCode);
}

tlApiResult_t tlApiGetSuid(mcSuid_t *suid)
{
    return ((tlApiResult_t (*)(uint32_t, mcSuid_t *))GW)(0xb, suid);
}

tlApiResult_t tlApiGetVirtMemType(uint32_t *type, addr_t addr, uint32_t size)
{
    return ((tlApiResult_t (*)(uint32_t, uint32_t *, addr_t, uint32_t))GW)(0x1a, type, addr, size);
}

tlApiResult_t tlApiWrapObjectExt(const void *src, size_t plainLen, size_t encryptedLen, void *dest, size_t *destLen, mcSoContext_t context, mcSoLifeTime_t lifetime, const tlApiSpTrustletId_t *consumer, uint32_t flags)
{
    return ((tlApiResult_t (*)(uint32_t, const void *, size_t, size_t, void *, size_t *, mcSoContext_t, mcSoLifeTime_t, const tlApiSpTrustletId_t *, uint32_t))GW)(0x9, src, plainLen, encryptedLen, dest, destLen, context, lifetime, consumer, flags);
}

tlApiResult_t tlApiUnwrapObjectExt(void *src, size_t srcLen, void *dest, size_t *destLen, uint32_t flags)
{
    return ((tlApiResult_t (*)(uint32_t, void *, size_t, void *, size_t *, uint32_t))GW)(0xa, src, srcLen, dest, destLen, flags);
}

tlApiResult_t tlApiDeriveKey(const void *salt, size_t saltLen, void *dest, size_t destLen, mcSoContext_t context, mcSoLifeTime_t lifetime)
{
    return ((tlApiResult_t (*)(uint32_t, const void *, size_t, void *, size_t, mcSoContext_t, mcSoLifeTime_t))GW)(0x1b, salt, saltLen, dest, destLen, context, lifetime);
}

tlApiResult_t tlApiEndorse(const void *msg, size_t msgLen, void *dst, size_t *dstLen, mcScope_t scope)
{
    return ((tlApiResult_t (*)(uint32_t, const void *, size_t, void *, size_t *, mcScope_t))GW)(0x55, msg, msgLen, dst, dstLen, scope);
}

tlApiResult_t tlApiGetSecureTimestamp(timestamp_ptr pTimestamp)
{
    return ((tlApiResult_t (*)(uint32_t, timestamp_ptr))GW)(0x62, pTimestamp);
}

tlApiResult_t tlApiDrmProcessContent(uint8_t sHandle, tlApiDrmDecryptContext_t decryptCtx, uint8_t *input, tlApiDrmInputSegmentDescriptor_t inputDesc, uint16_t processMode, uint8_t *output)
{
    return ((tlApiResult_t (*)(uint32_t, uint8_t, tlApiDrmDecryptContext_t, uint8_t *, tlApiDrmInputSegmentDescriptor_t, uint16_t, uint8_t *))GW)(0x5c, sHandle, decryptCtx, input, inputDesc, processMode, output);
}

tlApiResult_t tlApiDrmOpenSession(uint8_t *sHandle)
{
    return ((tlApiResult_t (*)(uint32_t, uint8_t *))GW)(0x5d, sHandle);
}

tlApiResult_t tlApiDrmCloseSession(uint8_t sHandle)
{
    return ((tlApiResult_t (*)(uint32_t, uint8_t))GW)(0x5e, sHandle);
}

tlApiResult_t tlApiDrmCheckLink(uint8_t sHandle, tlApiDrmLink_t link)
{
    return ((tlApiResult_t (*)(uint32_t, uint8_t, tlApiDrmLink_t))GW)(0x5f, sHandle, link);
}

tlApiResult_t tlApiTuiSetImage(tlApiTuiImage_ptr image, tlApiTuiCoordinates_t coordinates)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiTuiImage_ptr, tlApiTuiCoordinates_t))GW)(0x59, image, coordinates);
}

tlApiResult_t tlApiTuiGetTouchEvent(tlApiTuiTouchEvent_ptr touchEvent)
{
    return ((tlApiResult_t (*)(uint32_t, tlApiTuiTouchEvent_ptr))GW)(0x5a, touchEvent);
}

/* The plain *Init calls have no dispatch id of their own; real TAs forward to the
 * corresponding *InitWithData with no preloaded data. */
tlApiResult_t tlApiCipherInit(tlApiCrSession_t *pSessionHandle, tlApiCipherAlg_t alg, tlApiCipherMode_t mode, const tlApiKey_t *key)
{
    return tlApiCipherInitWithData(pSessionHandle, alg, mode, key, NULL, 0);
}

tlApiResult_t tlApiSignatureInit(tlApiCrSession_t *pSessionHandle, const tlApiKey_t *key, tlApiSigMode_t mode, tlApiSigAlg_t alg)
{
    return tlApiSignatureInitWithData(pSessionHandle, key, mode, alg, NULL, 0);
}

tlApiResult_t tlApiMessageDigestInit(tlApiCrSession_t *pSessionHandle, tlApiMdAlg_t algorithm)
{
    return tlApiMessageDigestInitWithData(pSessionHandle, algorithm, NULL, NULL);
}
