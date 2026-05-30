#pragma once
#include "nvs_interface.h"

/**
 * @brief Manages Non-Volatile Storage (NVS) for provisioning.
 */
class NvsManager : public INvsManager {
public:
    NvsManager();
    
    /** @brief Initialize the NVS preferences. */
    void begin() override;
    
    /** @brief Returns true if credentials exist in NVS. */
    bool hasCredentials() override;
    
    /** 
     * @brief Saves the 4-digit PIN and the base32 secret. 
     * Base32 secret string must be null terminated.
     */
    bool saveCredentials(const uint8_t* pin, const char* secret) override;
    
    /**
     * @brief Loads the 4-digit PIN and the base32 secret into the provided buffers.
     */
    bool loadCredentials(uint8_t* pin, char* secret, size_t secretMaxLen) override;
    
    /** @brief Erases all stored credentials (used during secure wipe). */
    void eraseAll() override;
};
