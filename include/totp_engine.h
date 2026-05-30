#pragma once
#include <stdint.h>
#include "time_interface.h"

/**
 * @brief Standalone TOTP generation module according to RFC 6238.
 * Decoupled from PIN FSM.
 */
class TotpEngine {
public:
    /**
     * @brief Generates a 6-digit TOTP code according to RFC 6238 (HMAC-SHA1).
     * 
     * @param secret    Pointer to the decoded secret bytes.
     * @param secretLen Length of the secret bytes.
     * @param timeProv  Time provider to get current UNIX time.
     * @param outCode   Buffer of at least 7 bytes to store the null-terminated code.
     * @return true on success, false on error (e.g., mbedtls failure or bad args).
     */
    static bool generateCode(const uint8_t* secret, uint8_t secretLen,
                             const ITimeProvider& timeProv, char* outCode);
};
