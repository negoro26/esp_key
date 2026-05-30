/**
 * @file pin_fsm_interface.h
 * @brief Pure C++ PIN FSM interface. ZERO Arduino dependencies.
 *
 * Defines the state machine states and abstract interface for the
 * PIN entry finite state machine. Concrete implementations inherit
 * from IPinFsm and inject ISerial + IEncoder dependencies.
 */
#pragma once
#include "encoder_interface.h"

/**
 * @brief All possible states of the PIN entry FSM.
 *
 * LOCKED       - Initial state, awaiting first interaction
 * DIGIT_SELECT - User is rotating encoder to choose a digit value
 * DIGIT_CONFIRM- Momentary state: digit confirmed, buffered, index advanced
 * VERIFY       - All digits entered, comparing against stored PIN
 * UNLOCKED     - PIN matched, system unlocked
 * WIPE_TRIGGER - 3 failures reached, secrets being destroyed
 * HALTED       - Terminal state after wipe, no further input accepted
 */
enum class FsmState : uint32_t {
    LOCKED        = 0x11111111,
    LOCKED_INV    = 0xEEEEEEEE,
    DIGIT_SELECT  = 0x22222222,
    DIGIT_SELECT_INV = 0xDDDDDDDD,
    DIGIT_CONFIRM = 0x33333333,
    DIGIT_CONFIRM_INV = 0xCCCCCCCC,
    VERIFY        = 0x44444444,
    VERIFY_INV    = 0xBBBBBBBB,
    UNLOCKED      = 0x55555555,
    UNLOCKED_INV  = 0xAAAAAAAA,
    WIPE_TRIGGER  = 0x66666666,
    WIPE_TRIGGER_INV = 0x99999999,
    HALTED        = 0x77777777,
    HALTED_INV    = 0x88888888,
    UNPROVISIONED = 0x0F0F0F0F,
    UNPROVISIONED_INV = 0xF0F0F0F0
};

inline FsmState operator~(FsmState a) {
    return static_cast<FsmState>(~static_cast<uint32_t>(a));
}

class IPinFsm {
public:
    virtual ~IPinFsm() = default;

    /** @brief Reset FSM to initial state (LOCKED if provisioned, else UNPROVISIONED). */
    virtual void reset() = 0;

    /** @brief Force FSM state, e.g., to UNPROVISIONED. */
    virtual void setState(FsmState state) = 0;

    /** @brief Configure the correct PIN. */
    virtual void setStoredPin(const uint8_t* pin) = 0;

    /** @brief Feed an encoder event into the FSM for processing. */
    virtual void processEvent(EncoderEvent event) = 0;

    /** @brief Return the current FSM state. */
    virtual FsmState getState() const = 0;

    /** @brief Return the current digit index (0-3). */
    virtual uint8_t getCurrentDigitIndex() const = 0;

    /** @brief Return the current dial value (0-9). */
    virtual uint8_t getCurrentDigitValue() const = 0;
};
