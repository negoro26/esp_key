#include "display_manager.h"
#include <TFT_eSPI.h>
#include <stdio.h>

DisplayManager::DisplayManager(TFT_eSPI& tft) : _tft(tft) {}

void DisplayManager::begin() {
    _tft.init();
    _tft.setRotation(1); // Landscape
    _tft.fillScreen(TFT_BLACK);
}

void DisplayManager::update(FsmState state, uint8_t currentDigitIndex, uint8_t currentDigitValue, const char* totpCode) {
    bool stateChanged = (state != _lastState);
    bool indexChanged = (currentDigitIndex != _lastDigitIndex);
    bool valueChanged = (currentDigitValue != _lastDigitValue);

    if (!stateChanged && !indexChanged && !valueChanged) {
        return; // Non-blocking exit if nothing changed
    }

    _lastState = state;
    _lastDigitIndex = currentDigitIndex;
    _lastDigitValue = currentDigitValue;

    if (stateChanged) {
        // Handle full-screen redraws only upon state transitions
        if (state == FsmState::LOCKED || state == FsmState::DIGIT_SELECT || state == FsmState::DIGIT_CONFIRM) {
            _tft.fillScreen(TFT_BLACK);
            _tft.setTextColor(TFT_WHITE, TFT_BLACK);
            _tft.setTextSize(2);
            _tft.setCursor(10, 10);
            _tft.println("ENTER PIN");
        } else if (state == FsmState::VERIFY) {
            _tft.fillScreen(TFT_BLACK);
            _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            _tft.setTextSize(3);
            _tft.setCursor(10, 50);
            _tft.println("VERIFYING...");
        } else if (state == FsmState::UNLOCKED) {
            _tft.fillScreen(TFT_GREEN);
            _tft.setTextColor(TFT_BLACK, TFT_GREEN);
            _tft.setTextSize(3);
            _tft.setCursor(10, 10);
            _tft.println("ACCESS GRANTED");
            _tft.setCursor(10, 50);
            _tft.setTextSize(2);
            
            if (totpCode && strcmp(totpCode, "SYNC") == 0) {
                _tft.setTextColor(TFT_RED, TFT_BLACK);
                _tft.println("TIME DESYNCED");
                _tft.setTextSize(3);
                _tft.setCursor(10, 80);
                _tft.println("SYNC REQ");
            } else {
                _tft.println("TOTP CODE:");
                _tft.setTextSize(5);
                _tft.setCursor(10, 80);
                _tft.println(totpCode ? totpCode : "------");
            }
        } else if (state == FsmState::WIPE_TRIGGER || state == FsmState::HALTED) {
            _tft.fillScreen(TFT_RED);
            _tft.setTextColor(TFT_WHITE, TFT_RED);
            _tft.setTextSize(3);
            _tft.setCursor(10, 50);
            _tft.println("WIPE");
            _tft.setCursor(10, 80);
            _tft.println("EXECUTED");
        } else if (state == FsmState::UNPROVISIONED) {
            _tft.fillScreen(TFT_ORANGE);
            _tft.setTextColor(TFT_BLACK, TFT_ORANGE);
            _tft.setTextSize(2);
            _tft.setCursor(10, 50);
            _tft.println("AWAITING");
            _tft.setCursor(10, 80);
            _tft.println("PROVISIONING");
        }
    }

    _lastState = state;
    _lastDigitIndex = currentDigitIndex;
    _lastDigitValue = currentDigitValue;

    // Dynamic partial updates for PIN entry (no echo security constraint)
    if (state == FsmState::LOCKED || state == FsmState::DIGIT_SELECT || state == FsmState::DIGIT_CONFIRM) {
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);
        
        // Render PIN mask: [***_]
        if (stateChanged || indexChanged) {
            _tft.setTextSize(4);
            _tft.setCursor(10, 50);
            _tft.print("[");
            for (int i = 0; i < 4; i++) {
                if (i < currentDigitIndex) _tft.print("*");
                else if (i == currentDigitIndex) _tft.print("_");
                else _tft.print(" ");
            }
            _tft.print("]  "); // Padding to clear trailing artifacts
        }

        // Render current dial value separately
        if (stateChanged || valueChanged) {
            _tft.setTextSize(3);
            _tft.setCursor(10, 100);
            if (state == FsmState::DIGIT_SELECT) {
                _tft.print("Dial: ");
                _tft.print(currentDigitValue);
                _tft.print("  "); // Clear trailing
            } else if (state == FsmState::DIGIT_CONFIRM) {
                _tft.print("Confirm: ");
                _tft.print(currentDigitValue);
                _tft.print("  "); // Clear trailing
            } else {
                // Clear the dial line completely if just locked
                _tft.print("             "); 
            }
        }
    }
}
