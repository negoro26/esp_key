#include <Arduino.h>
#include "integration_tests.h"
#include "security_utils.h"
#include "encoder.h"
#include "pin_fsm_impl.h"

extern Encoder enc;

void run_self_test() {
    Serial.println("--- Starting Hardware Integration Self-Test ---");

    // 1. Wipe routine verification on actual hardware
    Serial.println("\n[TEST 1] Physical Wipe Routine Verification");
    volatile uint8_t test_buf[16];
    for (int i = 0; i < 16; i++) {
        test_buf[i] = static_cast<uint8_t>(0xAA + i);
    }
    
    secure_wipe(test_buf, 16);
    
    bool wipe_success = true;
    for (int i = 0; i < 16; i++) {
        if (test_buf[i] != 0) {
            wipe_success = false;
        }
    }
    if (wipe_success) {
        Serial.println("  -> PASS: Buffer successfully zeroed on hardware.");
    } else {
        Serial.println("  -> FAIL: Buffer not fully zeroed!");
    }

    // 2. Timing check on live ESP32 CPU
    Serial.println("\n[TEST 2] Live Constant-Time Comparison Variance");
    volatile uint8_t buf_a[4] = {1, 2, 3, 4};
    uint8_t buf_match[4]      = {1, 2, 3, 4};
    uint8_t buf_first_diff[4] = {99, 2, 3, 4};
    uint8_t buf_last_diff[4]  = {1, 2, 3, 99};

    const int N = 100000;
    volatile uint32_t sink = 0;
    uint32_t t0, t1;
    uint32_t time_match = 0, time_first = 0, time_last = 0;

    // Warmup
    for(int i = 0; i < 1000; i++) {
        sink = secure_compare(buf_a, buf_match, 4);
        sink = secure_compare(buf_a, buf_first_diff, 4);
        sink = secure_compare(buf_a, buf_last_diff, 4);
    }

#ifdef ESP32
    // Use cycle counter for precision
    t0 = xthal_get_ccount();
    for(int i = 0; i < N; i++) sink = secure_compare(buf_a, buf_match, 4);
    t1 = xthal_get_ccount();
    time_match = t1 - t0;

    t0 = xthal_get_ccount();
    for(int i = 0; i < N; i++) sink = secure_compare(buf_a, buf_first_diff, 4);
    t1 = xthal_get_ccount();
    time_first = t1 - t0;

    t0 = xthal_get_ccount();
    for(int i = 0; i < N; i++) sink = secure_compare(buf_a, buf_last_diff, 4);
    t1 = xthal_get_ccount();
    time_last = t1 - t0;
#else
    t0 = micros();
    for(int i = 0; i < N; i++) sink = secure_compare(buf_a, buf_match, 4);
    t1 = micros();
    time_match = t1 - t0;

    t0 = micros();
    for(int i = 0; i < N; i++) sink = secure_compare(buf_a, buf_first_diff, 4);
    t1 = micros();
    time_first = t1 - t0;

    t0 = micros();
    for(int i = 0; i < N; i++) sink = secure_compare(buf_a, buf_last_diff, 4);
    t1 = micros();
    time_last = t1 - t0;
#endif

    double max_mismatch = (time_first > time_last) ? time_first : time_last;
    double min_mismatch = (time_first < time_last) ? time_first : time_last;
    
    double pos_variance = 0.0;
    if (max_mismatch > 0) {
        pos_variance = ((max_mismatch - min_mismatch) / max_mismatch) * 100.0;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "  -> Match      : %lu cycles", (unsigned long)time_match);
    Serial.println(buf);
    snprintf(buf, sizeof(buf), "  -> First Diff : %lu cycles", (unsigned long)time_first);
    Serial.println(buf);
    snprintf(buf, sizeof(buf), "  -> Last Diff  : %lu cycles", (unsigned long)time_last);
    Serial.println(buf);
    
    int var_int = (int)pos_variance;
    int var_frac = (int)(pos_variance * 100.0) % 100;
    snprintf(buf, sizeof(buf), "  -> Variance   : %d.%02d%%", var_int, var_frac);
    Serial.println(buf);

    if (pos_variance < 5.0) {
        Serial.println("  -> PASS: Variance < 5%");
    } else {
        Serial.println("  -> FAIL: Variance >= 5%");
    }

    // 3. Physical prompt to verify encoder direction consistency and switch debounce
    Serial.println("\n[TEST 3] Physical Encoder & Debounce Check");
    Serial.println("  -> INTERACTIVE MODE: You have 10 seconds to test.");
    Serial.println("  -> Please rotate CW, rotate CCW, press and hold the button.");
    
    uint32_t start_ms = millis();
    while (millis() - start_ms < 10000) {
        EncoderEvent ev = enc.poll();
        if (ev != EncoderEvent::NONE) {
            switch(ev) {
                case EncoderEvent::CW:       Serial.println("  [Live Event] Encoder: Clockwise (CW)"); break;
                case EncoderEvent::CCW:      Serial.println("  [Live Event] Encoder: Counter-Clockwise (CCW)"); break;
                case EncoderEvent::SW_PRESS: Serial.println("  [Live Event] Switch: Pressed"); break;
                case EncoderEvent::SW_HOLD:  Serial.println("  [Live Event] Switch: Held"); break;
                default: break;
            }
        }
        delay(1);
    }
    Serial.println("  -> Time's up for interactive test.");

    Serial.println("\n--- Self-Test Complete ---");
    (void)sink;
}
