#include "hal.h"

/**
 * @brief Initialize hardware pins for the encoder.
 * Threat model addressed: Floating pins leading to false state transitions.
 * We strictly use internal pull-ups.
 */
void hal_init() {
    pinMode(ENC_CLK_PIN, INPUT_PULLUP);
    pinMode(ENC_DT_PIN, INPUT_PULLUP);
    pinMode(ENC_SW_PIN, INPUT_PULLUP);
}
