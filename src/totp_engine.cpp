#include "../include/totp_engine.h"
#include <stdio.h>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <mbedtls/md.h>
#else
// Mock mbedtls HMAC API for native tests
#define MBEDTLS_MD_SHA1 1
typedef struct { int dummy; } mbedtls_md_info_t;

extern "C" {
    __attribute__((weak)) const mbedtls_md_info_t* mbedtls_md_info_from_type(int type) {
        (void)type;
        return nullptr;
    }
    __attribute__((weak)) int mbedtls_md_hmac(const mbedtls_md_info_t *md_info,
                        const unsigned char *key, size_t keylen,
                        const unsigned char *input, size_t ilen,
                        unsigned char *output) {
        (void)md_info; (void)key; (void)keylen; (void)input; (void)ilen; (void)output;
        return -1;
    }
}
#endif

bool TotpEngine::generateCode(const uint8_t* secret, uint8_t secretLen,
                              const ITimeProvider& timeProv, char* outCode) {
    if (!secret || !outCode || secretLen == 0) return false;

    // RFC 6238 defines 30-second time steps
    uint64_t t = timeProv.getUnixTime() / 30;

    // Convert time to 8-byte big-endian array for HMAC input
    uint8_t timeBytes[8];
    for (int i = 7; i >= 0; i--) {
        timeBytes[i] = static_cast<uint8_t>(t & 0xFF);
        t >>= 8;
    }

    uint8_t mac[20];
    
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (!md_info) return false;

    int ret = mbedtls_md_hmac(md_info, secret, secretLen, timeBytes, 8, mac);
    if (ret != 0) return false;

    // RFC 4226 Dynamic Truncation
    int offset = mac[19] & 0x0F;
    uint32_t bin_code = (mac[offset]  & 0x7F) << 24
                      | (mac[offset+1] & 0xFF) << 16
                      | (mac[offset+2] & 0xFF) <<  8
                      | (mac[offset+3] & 0xFF);

    // TOTP length is 6 digits
    uint32_t totp = bin_code % 1000000;
    
    // Format to 6 digits with leading zeros (zero dynamic allocation)
    snprintf(outCode, 7, "%06u", (unsigned int)totp);
    
    return true;
}
