/**
 * @file pin_fsm_impl.h
 * @brief Concrete PinFsm class inheriting IPinFsm.
 *
 * Dependencies are injected via ISerial& and IEncoder& references.
 * All mutable state is declared as static class members to enforce
 * single-instance semantics and avoid dynamic allocation.
 *
 * Security notes:
 * - failedAttempts is volatile to prevent compiler optimization of
 *   the security-critical attempt counter.
 * - enteredPin buffer uses volatile uint8_t to ensure zeroization
 *   writes are not optimized away.
 * - PIN length is a compile-time constant (PIN_LENGTH = 4).
 */
#pragma once
#include <stdint.h>
#include "../include/pin_fsm_interface.h"
#include "../include/serial_interface.h"
#include "../include/security_utils.h"

class PinFsm : public IPinFsm {
public:
    static const uint8_t PIN_LENGTH = 4;

    /**
     * @brief Construct PinFsm with injected serial and encoder.
     * Does NOT call reset() — caller must invoke reset() after
     * hardware is initialized.
     */
    PinFsm(ISerial& serial, IEncoder& encoder);

    void reset() override;
    void setState(FsmState state) override;
    void setStoredPin(const uint8_t* pin) override;
    void processEvent(EncoderEvent event) override;
    FsmState getState() const override;
    uint8_t getCurrentDigitIndex() const override;
    uint8_t getCurrentDigitValue() const override;

    /** @brief Read the volatile failed attempt counter (for testing). */
    uint8_t getFailedAttempts() const;

private:
    ISerial& serialOut;
    IEncoder& encoderIn;

    /* All mutable FSM state is static to enforce single-instance,
       zero-dynamic-allocation semantics. */
    static FsmState              currentState;
    static volatile uint8_t      failedAttempts;
    static volatile uint8_t      enteredPin[PIN_LENGTH];
    static uint8_t               storedPin[PIN_LENGTH];
    static uint8_t               currentDigitIndex;
    static uint8_t               currentDigitValue;

    void transitionTo(FsmState newState);
    void zeroizeBuffers();
    uint32_t verifyPin() const;
    void printStateIndicator() const;
};
