/**
 * @file encoder.h (src/)
 * @brief Concrete Encoder class inheriting IEncoder.
 *
 * Uses callback-based HAL injection to avoid direct Arduino
 * dependencies in the class definition. Hardware binding is
 * done in encoder_glue.cpp.
 */
#pragma once
#include <stdint.h>
#include "../include/encoder_interface.h"

struct EncoderCallbacks {
    unsigned long (*micros_fn)();
    unsigned long (*millis_fn)();
    int (*read_clk_fn)();
    int (*read_dt_fn)();
    int (*read_sw_fn)();
};

class Encoder : public IEncoder {
public:
    Encoder(EncoderCallbacks cb, uint32_t debounce_us = 50);
    void begin() override;
    EncoderEvent poll() override;
private:
    EncoderCallbacks hw;
    uint32_t debounceUs;
    int lastCLK;
    int lastSW;
    unsigned long lastEdgeTime;
    unsigned long lastSwEdgeTime;
    bool swActive;
    bool swHoldTriggered;
};
