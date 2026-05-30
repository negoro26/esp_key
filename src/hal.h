/**
 * @file hal.h (src/)
 * @brief Hardware Abstraction Layer for ESP32.
 *
 * Pin definitions and timing constants for the verified
 * encoder hardware. Only included by ESP32-targeted files
 * (encoder_glue.cpp, main.cpp) — never by native builds.
 */
#pragma once
#include <stdint.h>
#include <Arduino.h>

/* VERIFIED PHYSICAL LAYER — DO NOT CHANGE */
#define ENC_CLK_PIN          21   /* Confirmed CW/CCW signal A */
#define ENC_DT_PIN           22   /* Confirmed direction signal B */
#define ENC_SW_PIN           17   /* Active LOW push switch */
#define ENC_DEBOUNCE_US      50   /* Validated via stress test */
#define ENC_POLL_INTERVAL_US 100  /* Aggressive polling for security */

void hal_init();
