#pragma once
#include <stdint.h>

/**
 * @brief Pure virtual interface for providing UNIX time.
 * Allows decoupling time acquisition from TOTP calculation for native testing.
 */
class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;
    
    /**
     * @brief Get the current UNIX time.
     * @return uint64_t Seconds since Jan 1, 1970 (Epoch).
     */
    virtual uint64_t getUnixTime() const = 0;
};
