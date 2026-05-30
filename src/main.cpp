/**
 * @file main.cpp
 * @brief ESP Key Phase 2 — Main application loop.
 *
 * Wires together HAL, Encoder, and PinFsm on the ESP32 target.
 * Provides serial command interface for diagnostics.
 */
#include <Arduino.h>
#include "hal.h"
#include "encoder.h"
#include "pin_fsm_impl.h"
#include "integration_tests.h"
#include <TFT_eSPI.h>
#include "display_manager.h"
#include "ble_manager.h"
#include "totp_engine.h"
#include "system_time_provider.h"
#include "nvs_manager.h"
#include "security_utils.h"

extern Encoder enc;
extern PinFsm fsm;

TFT_eSPI tft;
DisplayManager displayManager(tft);
BleManager bleManager;
NvsManager nvsManager;

static SystemTimeProvider sysTime;
static char currentTotp[7] = "------";
static char storedSecret[64] = {0};
static bool bleTyped = false;

static bool verboseMode = false;

void process_serial_commands();

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    Serial.println("==========================================");
    Serial.println("ESP Key - Phase 2");
    Serial.println("==========================================");

    hal_init();
    enc.begin();
    displayManager.begin();

    nvsManager.begin();
    volatile bool hasCreds = nvsManager.hasCredentials();
    if (hasCreds == false && hasCreds != true) {
        fsm.setState(FsmState::UNPROVISIONED);
    } else {
        uint8_t pin[4];
        if (nvsManager.loadCredentials(pin, storedSecret, sizeof(storedSecret))) {
            fsm.setStoredPin(pin);
        } else {
            fsm.setState(FsmState::UNPROVISIONED);
        }
    }

    fsm.reset();
}

void loop() {
    EncoderEvent event = enc.poll();

    if (event != EncoderEvent::NONE) {
        if (verboseMode) {
            switch(event) {
                case EncoderEvent::CW:          Serial.println("[ENC] Clockwise"); break;
                case EncoderEvent::CCW:         Serial.println("[ENC] Counter-Clockwise"); break;
                case EncoderEvent::SW_PRESS:    Serial.println("[SW] Pressed"); break;
                case EncoderEvent::SW_HOLD:     Serial.println("[SW] Held"); break;
                default: break;
            }
        }
        fsm.processEvent(event);
    }

    FsmState currentState = fsm.getState();
    static FsmState lastState = FsmState::LOCKED;

    if (currentState != lastState) {
        volatile FsmState vState = currentState;
        if (vState == FsmState::UNLOCKED && ~vState == FsmState::UNLOCKED_INV) {
            // STRICT AIR-GAP CONSTRAINT: Radio is only initialized here.
            bleManager.begin();
            bleTyped = false;
        } else if (vState == FsmState::WIPE_TRIGGER && ~vState == FsmState::WIPE_TRIGGER_INV) {
            nvsManager.eraseAll();
            Serial.println("NVS Erased!");
        } else {
            // Clear TOTP if locked/halted
            snprintf(currentTotp, sizeof(currentTotp), "------");
            bleTyped = false;
        }
        lastState = currentState;
    }

    if (currentState == FsmState::UNLOCKED) {
        bool synced = sysTime.isSynced();
        
        if (!synced && bleManager.isConnected()) {
            synced = bleManager.syncTimeFromPeer();
        }

        if (synced) {
            uint8_t decodedSecret[32];
            size_t decodedLen = base32_decode(storedSecret, decodedSecret, sizeof(decodedSecret));
            if (decodedLen == 0) {
                snprintf(currentTotp, sizeof(currentTotp), "ERR_B3");
            } else {
                if (!TotpEngine::generateCode(decodedSecret, decodedLen, sysTime, currentTotp)) {
                    snprintf(currentTotp, sizeof(currentTotp), "ERR_CR");
                } else if (!bleTyped && bleManager.typeCode(currentTotp)) {
                    bleTyped = true; // Prevents spamming keystrokes
                    if (verboseMode) Serial.println("TOTP typed via BLE");
                }
            }
        } else {
            snprintf(currentTotp, sizeof(currentTotp), "SYNC");
        }
    }

    displayManager.update(currentState, fsm.getCurrentDigitIndex(), fsm.getCurrentDigitValue(), currentTotp);

    process_serial_commands();
}

void process_serial_commands() {
    static char cmdBuf[128];
    static int cmdPos = 0;

    while (Serial.available() > 0) {
        char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (cmdPos == 0) continue;
            cmdBuf[cmdPos] = '\0';

            /* Convert to uppercase in-place */
            for (int i = 0; i < cmdPos; i++) {
                if (cmdBuf[i] >= 'a' && cmdBuf[i] <= 'z') {
                    cmdBuf[i] = static_cast<char>(cmdBuf[i] - 32);
                }
            }

            /* Trim trailing whitespace */
            while (cmdPos > 0 && (cmdBuf[cmdPos - 1] == ' ' || cmdBuf[cmdPos - 1] == '\t')) {
                cmdBuf[--cmdPos] = '\0';
            }

            if (strcmp(cmdBuf, "TEST") == 0) {
                run_self_test();
            } else if (strcmp(cmdBuf, "STATE") == 0) {
                FsmState currentState = fsm.getState();
                Serial.print("Current State: ");
                switch(currentState) {
                    case FsmState::LOCKED:        Serial.print("LOCKED "); break;
                    case FsmState::DIGIT_SELECT:  Serial.print("DIGIT_SELECT "); break;
                    case FsmState::DIGIT_CONFIRM: Serial.print("DIGIT_CONFIRM "); break;
                    case FsmState::VERIFY:        Serial.print("VERIFY "); break;
                    case FsmState::UNLOCKED:      Serial.print("UNLOCKED "); break;
                    case FsmState::WIPE_TRIGGER:  Serial.print("WIPE_TRIGGER "); break;
                    case FsmState::HALTED:        Serial.print("HALTED "); break;
                    case FsmState::UNPROVISIONED: Serial.print("UNPROVISIONED "); break;
                    default:                      Serial.print("INVALID/INV "); break;
                }
                char hexBuf[16];
                snprintf(hexBuf, sizeof(hexBuf), "(0x%08lX)", (unsigned long)currentState);
                Serial.println(hexBuf);
            } else if (strncmp(cmdBuf, "PROVISION ", 10) == 0) {
                volatile FsmState vState = fsm.getState();
                if (vState == FsmState::UNPROVISIONED && ~vState == FsmState::UNPROVISIONED_INV) {
                    if (cmdPos > 15) {
                        uint8_t newPin[4];
                        for(int i=0; i<4; i++) newPin[i] = cmdBuf[10+i] - '0';
                        char* newSecret = cmdBuf + 15;
                        
                        if (strlen(newSecret) >= sizeof(storedSecret)) {
                            Serial.println("Error: Secret too long. Buffer overflow prevented.");
                        } else {
                            if (nvsManager.saveCredentials(newPin, newSecret)) {
                                Serial.println("Provisioning successful! Rebooting FSM...");
                                if (nvsManager.loadCredentials(newPin, storedSecret, sizeof(storedSecret))) {
                                    fsm.setStoredPin(newPin);
                                    fsm.setState(FsmState::LOCKED);
                                    fsm.reset();
                                }
                            }
                        }
                    }
                } else {
                    Serial.println("Error: Device already provisioned.");
                }
            } else if (strncmp(cmdBuf, "TIME ", 5) == 0) {
                long unixTime = atol(cmdBuf + 5);
                struct timeval tv = { .tv_sec = (long)unixTime, .tv_usec = 0 };
                settimeofday(&tv, NULL);
                Serial.print("Time synchronized via CLI to: ");
                Serial.println(unixTime);
            } else if (strcmp(cmdBuf, "VERBOSE ON") == 0) {
                verboseMode = true;
                Serial.println("Verbose logging ENABLED");
            } else if (strcmp(cmdBuf, "VERBOSE OFF") == 0) {
                verboseMode = false;
                Serial.println("Verbose logging DISABLED");
            } else if (strcmp(cmdBuf, "RESET") == 0) {
                Serial.println("Performing simulated power-cycle wipe...");
                fsm.reset();
            }

            cmdPos = 0;
        } else if (cmdPos < 127) {
            cmdBuf[cmdPos++] = c;
        }
    }
}
