#pragma once

#include <stdint.h>
#include "pin_fsm_interface.h"

// Forward declaration to avoid full include in header, reducing dependencies
class TFT_eSPI;

/**
 * @brief Presentation layer for rendering FSM states and TOTP codes to the TFT display.
 * Designed to be non-blocking and prevent screen flicker through partial redraws.
 */
class DisplayManager {
public:
    /**
     * @brief Construct a new Display Manager object
     * @param tft Reference to an instantiated TFT_eSPI object
     */
    DisplayManager(TFT_eSPI& tft);

    /**
     * @brief Initialize display hardware.
     */
    void begin();

    /**
     * @brief Non-blocking render update. Only redraws when state or values change.
     * 
     * @param state The current FsmState.
     * @param currentDigitIndex The active digit index (0-3).
     * @param currentDigitValue The dial value (0-9).
     * @param totpCode The generated TOTP string (only used in UNLOCKED state).
     */
    void update(FsmState state, uint8_t currentDigitIndex, uint8_t currentDigitValue, const char* totpCode = nullptr);

private:
    TFT_eSPI& _tft;
    
    // State tracking variables to prevent blocking full-screen redraws
    FsmState _lastState = (FsmState)0xFFFFFFFF;
    uint8_t _lastDigitIndex = 0xFF;
    uint8_t _lastDigitValue = 0xFF;
};
