#include <unity.h>
#include <string.h>
#include <stdint.h>
#include "../../include/totp_engine.h"

// Mock Time Provider
class MockTimeProvider : public ITimeProvider {
public:
    uint64_t mockTime = 0;
    uint64_t getUnixTime() const override {
        return mockTime;
    }
};

// Mock mbedtls implementation for native tests
#if !defined(ESP32) && !defined(ARDUINO_ARCH_ESP32)
#define MBEDTLS_MD_SHA1 1
typedef struct { int dummy; } mbedtls_md_info_t;

static const mbedtls_md_info_t mock_sha1_info = {0};

extern "C" {

const mbedtls_md_info_t* mbedtls_md_info_from_type(int type) {
    if (type == MBEDTLS_MD_SHA1) return &mock_sha1_info;
    return nullptr;
}

// RFC 6238 pre-computed HMACs for secret "12345678901234567890" (20 bytes)
struct TestVector {
    uint64_t t;
    const uint8_t mac[20];
};

static const TestVector mock_vectors[] = {
    // T=1 (time 59)
    {1, {0x75,0xa4,0x8a,0x19,0xd4,0xcb,0xe1,0x00,0x64,0x4e,0x8a,0xc1,0x39,0x7e,0xea,0x74,0x7a,0x2d,0x33,0xab}},
    // T=37037036 (time 1111111109)
    {37037036, {0x27,0x8c,0x02,0xe5,0x36,0x10,0xf8,0x4c,0x40,0xbd,0x91,0x35,0xac,0xd4,0x10,0x10,0x12,0x41,0x0a,0x14}},
    // T=37037037 (time 1111111111)
    {37037037, {0xb0,0x09,0x2b,0x21,0xd0,0x48,0xaf,0x20,0x9d,0xa0,0xa1,0xdd,0xd4,0x98,0xad,0xe8,0xa7,0x94,0x87,0xed}},
    // T=41152263 (time 1234567890)
    {41152263, {0x90,0x7c,0xd1,0xa9,0x11,0x65,0x64,0xec,0xb9,0xd5,0xd1,0x78,0x03,0x25,0xf2,0x46,0x17,0x3f,0xe7,0x03}},
    // T=66666666 (time 2000000000)
    {66666666, {0x25,0xa3,0x26,0xd3,0x1f,0xc3,0x66,0x24,0x4c,0xad,0x05,0x49,0x76,0x02,0x0c,0x7b,0x56,0xb1,0x3d,0x5f}},
    // T=666666666 (time 20000000000)
    {666666666, {0xab,0x07,0xe9,0x7e,0x2c,0x12,0x78,0x76,0x9d,0xbc,0xd7,0x57,0x83,0xaa,0xbd,0xe7,0x5e,0xd8,0x55,0x0a}}
};

int mbedtls_md_hmac(const mbedtls_md_info_t *md_info,
                    const unsigned char *key, size_t keylen,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output) {
    if (md_info != &mock_sha1_info) return -1;
    if (keylen != 20 || memcmp(key, "12345678901234567890", 20) != 0) return -2;
    if (ilen != 8) return -3;
    
    uint64_t t = 0;
    for(int i=0; i<8; i++) t = (t << 8) | input[i];

    for (const auto& vec : mock_vectors) {
        if (vec.t == t) {
            memcpy(output, vec.mac, 20);
            return 0;
        }
    }
    return -4;
}

} // extern "C"
#endif

void setUp(void) {}
void tearDown(void) {}

void test_totp_rfc6238_vectors(void) {
    MockTimeProvider timeProv;
    char totpCode[7];
    // ASCII "12345678901234567890", which is Base32 "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"
    const uint8_t secret[] = "12345678901234567890";
    
    timeProv.mockTime = 59;
    TEST_ASSERT_TRUE(TotpEngine::generateCode(secret, 20, timeProv, totpCode));
    TEST_ASSERT_EQUAL_STRING("287082", totpCode);

    timeProv.mockTime = 1111111109;
    TEST_ASSERT_TRUE(TotpEngine::generateCode(secret, 20, timeProv, totpCode));
    TEST_ASSERT_EQUAL_STRING("081804", totpCode);

    timeProv.mockTime = 1111111111;
    TEST_ASSERT_TRUE(TotpEngine::generateCode(secret, 20, timeProv, totpCode));
    TEST_ASSERT_EQUAL_STRING("050471", totpCode);

    timeProv.mockTime = 1234567890;
    TEST_ASSERT_TRUE(TotpEngine::generateCode(secret, 20, timeProv, totpCode));
    TEST_ASSERT_EQUAL_STRING("005924", totpCode);

    timeProv.mockTime = 2000000000;
    TEST_ASSERT_TRUE(TotpEngine::generateCode(secret, 20, timeProv, totpCode));
    TEST_ASSERT_EQUAL_STRING("279037", totpCode);

    timeProv.mockTime = 20000000000;
    TEST_ASSERT_TRUE(TotpEngine::generateCode(secret, 20, timeProv, totpCode));
    TEST_ASSERT_EQUAL_STRING("353130", totpCode);
}

void test_totp_bad_args(void) {
    MockTimeProvider timeProv;
    char totpCode[7];
    const uint8_t secret[] = "12345678901234567890";
    
    TEST_ASSERT_FALSE(TotpEngine::generateCode(nullptr, 20, timeProv, totpCode));
    TEST_ASSERT_FALSE(TotpEngine::generateCode(secret, 0, timeProv, totpCode));
    TEST_ASSERT_FALSE(TotpEngine::generateCode(secret, 20, timeProv, nullptr));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_totp_rfc6238_vectors);
    RUN_TEST(test_totp_bad_args);
    return UNITY_END();
}
