#include "../include/ble_manager.h"
#include <stdio.h>
#include <NimBLEDevice.h>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)

BleManager::BleManager() : _bleKeyboard("ESP Key TOTP", "Custom", 100) {
    // Constructor initializes the object but DOES NOT initialize the BT radio.
}

void BleManager::begin() {
    // This is the explicit trigger to turn on the BLE stack and radio
    _bleKeyboard.begin();
}

bool BleManager::typeCode(const char* totpCode) {
    if (!totpCode) return false;

    if (_bleKeyboard.isConnected()) {
        _bleKeyboard.print(totpCode);
        _bleKeyboard.write(KEY_RETURN);
        return true;
    }
    return false;
}

bool BleManager::isConnected() {
    return _bleKeyboard.isConnected();
}

bool BleManager::syncTimeFromPeer() {
    if (!_bleKeyboard.isConnected()) return false;

    NimBLEServer* pServer = NimBLEDevice::getServer();
    if (!pServer) return false;

    std::vector<uint16_t> peerIds = pServer->getPeerDevices();
    if (peerIds.empty()) return false;

    uint16_t conn_id = peerIds[0];
    NimBLEConnInfo peerInfo = pServer->getPeerInfo(conn_id);
    
    // Check if client already exists for this peer
    NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(peerInfo.getAddress());
    
    bool created = false;
    if (!pClient) {
        pClient = NimBLEDevice::createClient();
        created = true;
        if (!pClient->connect(peerInfo.getAddress())) {
            NimBLEDevice::deleteClient(pClient);
            return false;
        }
    }

    bool synced = false;
    NimBLERemoteService* pSvc = pClient->getService(NimBLEUUID((uint16_t)0x1805));
    if (pSvc) {
        NimBLERemoteCharacteristic* pChr = pSvc->getCharacteristic(NimBLEUUID((uint16_t)0x2A2B));
        if (pChr && pChr->canRead()) {
            std::string val = pChr->readValue();
            if (val.length() >= 10) {
                uint16_t year = val[0] | (val[1] << 8);
                uint8_t month = val[2];
                uint8_t day = val[3];
                uint8_t hour = val[4];
                uint8_t min = val[5];
                uint8_t sec = val[6];
                
                struct tm t = {0};
                t.tm_year = year - 1900;
                t.tm_mon = month - 1;
                t.tm_mday = day;
                t.tm_hour = hour;
                t.tm_min = min;
                t.tm_sec = sec;
                
                time_t unixTime = mktime(&t);
                struct timeval tv = { .tv_sec = (long)unixTime, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                synced = true;
            }
        }
    }

    if (created) {
        NimBLEDevice::deleteClient(pClient);
    }
    
    return synced;
}

#else

// Mock implementations for native builds to avoid breaking tests
BleManager::BleManager() {}
void BleManager::begin() {}
bool BleManager::typeCode(const char* totpCode) {
    (void)totpCode;
    return false; 
}
bool BleManager::isConnected() { return false; }
bool BleManager::syncTimeFromPeer() { return false; }

#endif
