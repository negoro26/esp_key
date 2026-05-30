/**
 * @file encoder.h
 * @brief Encoder Driver Subsystem
 * 
 * Handles non-blocking quadrature decoding and switch debouncing.
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include "hal.h"

/**
 * @brief Enumeration of all possible encoder events.
 */
enum EncoderEvent {
    ENC_NONE,           // No event
    ENC_CW,             // Clockwise rotation step
    ENC_CCW,            // Counter-clockwise rotation step
    ENC_SW_PRESSED,     // Switch pressed down (debounced)
    ENC_SW_HELD,        // Switch held down
    ENC_SW_RELEASED     // Switch released
};

/**
 * @brief Initialize encoder state.
 * Must be called after hal_init().
 */
void encoder_init();

/**
 * @brief Poll the encoder hardware for state changes.
 * 
 * Threat model: Blocking delays or missed edges could allow timing attacks
 * or unreliable input leading to lockouts.
 * Mitigation: Non-blocking micros() polling with strict debounce and edge
 * rejection logic.
 * 
 * @return The latest EncoderEvent detected, or ENC_NONE if no action.
 */
EncoderEvent encoder_poll();

/**
 * @brief Override internal encoder state for unit testing.
 */
void encoder_mock_input(int clk, int dt, int sw);

#endif // ENCODER_H
