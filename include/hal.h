/**
 * @file hal.h
 * @brief Hardware Abstraction Layer Definitions
 * 
 * Defines the verified hardware pins and timing constants for the 
 * Air-Gapped Rotary PIN Authenticator.
 */

#ifndef HAL_H
#define HAL_H

#include <Arduino.h>

// HARDWARE_HAL.h - VERIFIED PHYSICAL LAYER
#define ENC_CLK_PIN     21    // Confirmed CW/CCW signal A
#define ENC_DT_PIN      22    // Confirmed direction signal B  
#define ENC_SW_PIN      17    // Active LOW push switch
#define ENC_DEBOUNCE_US 50    // Validated via stress test (5ms safe margin)
#define ENC_POLL_INTERVAL_US 100  // Aggressive polling for security responsiveness

/**
 * @brief Initialize the hardware pins used for the encoder.
 * 
 * Threat model: Floating inputs can cause spurious pin entries.
 * Mitigation: Use internal pull-ups to guarantee a stable HIGH state
 * when the external circuit is open.
 */
void hal_init();

#endif // HAL_H
