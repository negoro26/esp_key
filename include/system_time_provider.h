#pragma once

#include "time_interface.h"

/**
 * @brief Provides time from the ESP32's internal <sys/time.h>.
 * Inherits from ITimeProvider for injection into the TotpEngine.
 */
class SystemTimeProvider : public ITimeProvider {
public:
    /**
     * @brief Gets the current UNIX timestamp.
     * @return uint64_t seconds since Unix epoch.
     */
    uint64_t getUnixTime() const override;

    /**
     * @brief Checks if the internal time has been synchronized.
     * @return true if the system time is significantly past epoch 0 (1970).
     */
    bool isSynced() const;
};
