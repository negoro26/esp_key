#include "../include/encoder_interface.h"
#include "encoder.h"

#define LOW_VAL 0

Encoder::Encoder(EncoderCallbacks cb, uint32_t debounce_us) : 
    hw(cb),
    debounceUs(debounce_us),
    lastCLK(1), 
    lastSW(1), 
    lastEdgeTime(0), 
    lastSwEdgeTime(0),
    swActive(false),
    swHoldTriggered(false) 
{
}

void Encoder::begin() {
    lastCLK = hw.read_clk_fn();
    lastSW = hw.read_sw_fn();
    lastEdgeTime = hw.micros_fn();
    lastSwEdgeTime = hw.millis_fn();
}

EncoderEvent Encoder::poll() {
    unsigned long nowUs = hw.micros_fn();
    unsigned long nowMs = hw.millis_fn();

    // 1. Quadrature Decode
    int currentCLK = hw.read_clk_fn();
    if (currentCLK != lastCLK && (nowUs - lastEdgeTime) > debounceUs) {
        int dt_val = hw.read_dt_fn();
        bool direction = (dt_val != currentCLK); 
        lastCLK = currentCLK;
        lastEdgeTime = nowUs;
        return direction ? EncoderEvent::CW : EncoderEvent::CCW;
    }
    lastCLK = currentCLK;

    // 2. Switch Decode
    int currentSW = hw.read_sw_fn();
    EncoderEvent eventToReturn = EncoderEvent::NONE;

    if (currentSW != lastSW) {
        if ((nowMs - lastSwEdgeTime) > 50) { // 50ms debounce
            lastSW = currentSW;
            lastSwEdgeTime = nowMs;
            
            // Reject any switch event occurring <100ms after last valid encoder edge
            if ((nowUs - lastEdgeTime) > 100000) { 
                if (currentSW == LOW_VAL) {
                    swActive = true;
                    swHoldTriggered = false;
                } else {
                    // Released
                    if (swActive && !swHoldTriggered) {
                        eventToReturn = EncoderEvent::SW_PRESS;
                    }
                    swActive = false;
                }
            } else {
                swActive = false; // Rejected due to closeness to rotation
            }
        }
    } else {
        if (swActive && currentSW == LOW_VAL && !swHoldTriggered) {
            if ((nowMs - lastSwEdgeTime) > 500) { // 500ms hold threshold
                swHoldTriggered = true;
                eventToReturn = EncoderEvent::SW_HOLD;
            }
        }
    }

    return eventToReturn;
}
