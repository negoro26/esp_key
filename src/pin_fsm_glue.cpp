/**
 * @file pin_fsm_glue.cpp
 * @brief Arduino HAL binding for PinFsm.
 *
 * Provides ArduinoSerial (concrete ISerial) wrapping Serial,
 * and instantiates the global PinFsm with injected dependencies.
 *
 * This file is EXCLUDED from native builds via build_src_filter.
 */
#include <Arduino.h>
#include "pin_fsm_impl.h"
#include "encoder.h"

class ArduinoSerial : public ISerial {
public:
    void print(const char* str) override {
        Serial.print(str);
    }
    void println(const char* str) override {
        Serial.println(str);
    }
    void print(char c) override {
        Serial.print(c);
    }
};

static ArduinoSerial hwSerial;
extern Encoder enc;
PinFsm fsm(hwSerial, enc);
