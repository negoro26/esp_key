#include "../include/system_time_provider.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

#include <sys/time.h>
#include <time.h>
#include <stddef.h>

uint64_t SystemTimeProvider::getUnixTime() const {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec;
}

bool SystemTimeProvider::isSynced() const {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // If time is less than year 2020 (approx 1577836800 seconds), it's not synced.
    // 1577836800 is a safe threshold to determine if we are still stuck at epoch 0 (1970).
    return tv.tv_sec > 1577836800;
}

#else

uint64_t SystemTimeProvider::getUnixTime() const {
    return 1716000000; // Mock time for native tests
}

bool SystemTimeProvider::isSynced() const {
    return true; // Always synced for native tests
}

#endif
