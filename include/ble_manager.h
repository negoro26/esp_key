#pragma once

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <BleKeyboard.h>
#endif

/**
 * @brief Manages BLE HID Keyboard Emulation.
 * strictly adheres to the air-gap constraint by deferring radio initialization.
 */
class BleManager {
public:
    /**
     * @brief Construct a new Ble Manager.
     * Does NOT initialize the BLE radio stack to preserve air-gap.
     */
    BleManager();

    /**
     * @brief Initializes the BLE radio stack.
     * MUST only be called when the FSM is in the UNLOCKED state.
     */
    void begin();

    /**
     * @brief Emulates keyboard typing of the TOTP code.
     * Checks for an active central connection, types the code, and presses Return.
     * 
     * @param totpCode The 6-digit null-terminated TOTP string.
     * @return true if sent, false if disconnected or invalid.
     */
    bool typeCode(const char* totpCode);

    /**
     * @brief Polls connected peers for the BLE Current Time Service (CTS) 
     * and synchronizes the internal ESP32 clock.
     * @return true if successfully synchronized from peer.
     */
    bool syncTimeFromPeer();

    /**
     * @brief Check if BLE is connected
     */
    bool isConnected();

private:
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    // Statically allocated to avoid dynamic heap allocation, 
    // but its constructor does not fire up the radio.
    BleKeyboard _bleKeyboard;
#endif
};
