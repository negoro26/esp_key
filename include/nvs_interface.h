#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Pure virtual interface for Non-Volatile Storage (NVS).
 */
class INvsManager {
public:
    virtual ~INvsManager() = default;
    
    /** @brief Initialize the NVS preferences. */
    virtual void begin() = 0;
    
    /** @brief Returns true if credentials exist in NVS. */
    virtual bool hasCredentials() = 0;
    
    /** 
     * @brief Saves the 4-digit PIN and the base32 secret. 
     */
    virtual bool saveCredentials(const uint8_t* pin, const char* secret) = 0;
    
    /**
     * @brief Loads the 4-digit PIN and the base32 secret into the provided buffers.
     */
    virtual bool loadCredentials(uint8_t* pin, char* secret, size_t secretMaxLen) = 0;
    
    /** @brief Erases all stored credentials (used during secure wipe). */
    virtual void eraseAll() = 0;
};
