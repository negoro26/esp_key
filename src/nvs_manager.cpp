#include "../include/nvs_manager.h"
#include <string.h>

#include <Preferences.h>
static Preferences prefs;

NvsManager::NvsManager() {}

void NvsManager::begin() {
    prefs.begin("esp_key", false);
}

bool NvsManager::hasCredentials() {
    return prefs.getBool("provisioned", false);
}

bool NvsManager::saveCredentials(const uint8_t* pin, const char* secret) {
    if (!pin || !secret) return false;
    prefs.putBytes("pin", pin, 4);
    
    char paddedSecret[64] = {0};
    strncpy(paddedSecret, secret, sizeof(paddedSecret) - 1);
    prefs.putBytes("secret", paddedSecret, 64);
    
    prefs.putBool("provisioned", true);
    return true;
}

bool NvsManager::loadCredentials(uint8_t* pin, char* secret, size_t secretMaxLen) {
    if (!hasCredentials()) return false;
    prefs.getBytes("pin", pin, 4);
    
    char paddedSecret[64] = {0};
    size_t len = prefs.getBytes("secret", paddedSecret, 64);
    if (len == 0) return false;
    
    strncpy(secret, paddedSecret, secretMaxLen - 1);
    secret[secretMaxLen - 1] = '\0';
    return true;
}

void NvsManager::eraseAll() {
    uint8_t dummyPin[4] = {0, 0, 0, 0};
    uint8_t dummySecret[64] = {0};
    
    prefs.putBytes("pin", dummyPin, 4);
    prefs.putBytes("secret", dummySecret, 64);
    prefs.putBool("provisioned", false);
    
    prefs.clear();
}
