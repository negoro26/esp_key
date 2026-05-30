/**
 * @file encoder_glue.cpp
 * @brief Arduino HAL binding for the Encoder.
 *
 * Maps Arduino hardware functions to the EncoderCallbacks struct
 * and instantiates the global Encoder with verified pin assignments.
 *
 * This file is EXCLUDED from native builds via build_src_filter.
 */
#include <Arduino.h>
#include "hal.h"
#include "encoder.h"

static unsigned long ard_micros() { return micros(); }
static unsigned long ard_millis() { return millis(); }
static int ard_read_clk() { return digitalRead(ENC_CLK_PIN); }
static int ard_read_dt() { return digitalRead(ENC_DT_PIN); }
static int ard_read_sw() { return digitalRead(ENC_SW_PIN); }

static EncoderCallbacks ard_cb = {
    ard_micros,
    ard_millis,
    ard_read_clk,
    ard_read_dt,
    ard_read_sw
};

Encoder enc(ard_cb, ENC_DEBOUNCE_US);
