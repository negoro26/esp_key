/**
 * @file pin_fsm.cpp
 * @brief Full PIN FSM implementation. ZERO Arduino calls.
 *
 * All output is routed through the injected ISerial interface.
 * PIN verification uses constant-time comparison to resist
 * timing side-channel attacks.
 *
 * Threat model:
 * - Brute force: Volatile attempt counter + wipe after 3 failures.
 * - Timing attack: Constant-time compare iterates all digits
 *   regardless of mismatch position.
 * - Memory forensics: Buffers are zeroized immediately after
 *   verification and on wipe.
 * - Compiler optimization: volatile qualifiers prevent elision
 *   of security-critical stores.
 */
#include "pin_fsm_impl.h"

/* ------------------------------------------------------------------ */
/*  Static member definitions                                         */
/* ------------------------------------------------------------------ */
FsmState              PinFsm::currentState      = FsmState::LOCKED;
volatile uint8_t      PinFsm::failedAttempts     = 0;
volatile uint8_t      PinFsm::enteredPin[PinFsm::PIN_LENGTH] = {0, 0, 0, 0};
uint8_t               PinFsm::currentDigitIndex  = 0;
uint8_t               PinFsm::currentDigitValue  = 0;
uint8_t               PinFsm::storedPin[PinFsm::PIN_LENGTH] = {0, 0, 0, 0};

/* ------------------------------------------------------------------ */
/*  Construction / Reset                                              */
/* ------------------------------------------------------------------ */

PinFsm::PinFsm(ISerial& serial, IEncoder& encoder)
    : serialOut(serial), encoderIn(encoder)
{
    /* Constructor intentionally does NOT call reset().
       Caller must invoke reset() after hardware is ready,
       to avoid printing to an uninitialised serial port. */
}

void PinFsm::reset() {
    failedAttempts = 0;
    zeroizeBuffers();
    if (currentState != FsmState::UNPROVISIONED) {
        transitionTo(FsmState::LOCKED);
    }
}

void PinFsm::setState(FsmState state) {
    currentState = state;
}

void PinFsm::setStoredPin(const uint8_t* pin) {
    if (pin) {
        for (uint8_t i = 0; i < PIN_LENGTH; i++) {
            storedPin[i] = pin[i];
        }
    }
}

void PinFsm::zeroizeBuffers() {
    secure_wipe(enteredPin, PIN_LENGTH);
    currentDigitIndex = 0;
    currentDigitValue = 0;
}

/* ------------------------------------------------------------------ */
/*  State queries                                                     */
/* ------------------------------------------------------------------ */

FsmState PinFsm::getState() const {
    return currentState;
}

uint8_t PinFsm::getFailedAttempts() const {
    return failedAttempts;
}

/* ------------------------------------------------------------------ */
/*  State transitions                                                 */
/* ------------------------------------------------------------------ */

void PinFsm::transitionTo(FsmState newState) {
    currentState = newState;

    switch (newState) {
    case FsmState::LOCKED:
        /* LOCKED is an entry gate; immediately advance to DIGIT_SELECT
           so the user can begin entering a PIN. */
        currentDigitIndex = 0;
        currentDigitValue = 0;
        currentState = FsmState::DIGIT_SELECT;
        printStateIndicator();
        break;

    case FsmState::WIPE_TRIGGER:
        serialOut.println("WIPE EXECUTED");
        zeroizeBuffers();
        failedAttempts = 0;
        currentState = FsmState::HALTED;
        break;

    case FsmState::UNLOCKED:
        serialOut.println("[SUCCESS] System Unlocked.");
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Event processing                                                  */
/* ------------------------------------------------------------------ */

void PinFsm::processEvent(EncoderEvent event) {
    /* HALTED and UNPROVISIONED are absorbing states. No input escapes. */
    if (currentState == FsmState::HALTED || currentState == FsmState::UNPROVISIONED) {
        return;
    }

    /* ---------- DIGIT_SELECT ---------- */
    if (currentState == FsmState::DIGIT_SELECT) {
        if (event == EncoderEvent::CW) {
            currentDigitValue = static_cast<uint8_t>(
                (currentDigitValue + 1) % 10);
            printStateIndicator();
        } else if (event == EncoderEvent::CCW) {
            currentDigitValue = (currentDigitValue == 0)
                ? 9
                : static_cast<uint8_t>(currentDigitValue - 1);
            printStateIndicator();
        } else if (event == EncoderEvent::SW_PRESS) {
            transitionTo(FsmState::DIGIT_CONFIRM);
        }
    }

    /* ---------- DIGIT_CONFIRM ---------- */
    if (currentState == FsmState::DIGIT_CONFIRM) {
        enteredPin[currentDigitIndex] = currentDigitValue;
        currentDigitIndex++;
        currentDigitValue = 0;

        if (currentDigitIndex >= PIN_LENGTH) {
            transitionTo(FsmState::VERIFY);
        } else {
            currentState = FsmState::DIGIT_SELECT;
            printStateIndicator();
        }
    }

    /* ---------- VERIFY ---------- */
    if (currentState == FsmState::VERIFY) {
        uint32_t result = verifyPin();

        /* Securely wipe PIN buffer immediately after comparison.
           Uses volatile writes + asm barrier to defeat DSE. */
        secure_wipe(enteredPin, PIN_LENGTH);
        currentDigitIndex = 0;
        currentDigitValue = 0;

        if (result == SECURITY_MATCH_MAGIC) {
            failedAttempts = 0;
            transitionTo(FsmState::UNLOCKED);
        } else {
            failedAttempts++;
            if (failedAttempts >= 3) {
                transitionTo(FsmState::WIPE_TRIGGER);
            } else {
                serialOut.println("[ERROR] Invalid PIN");
                transitionTo(FsmState::LOCKED);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  PIN verification — constant-time comparison                       */
/* ------------------------------------------------------------------ */

uint32_t PinFsm::verifyPin() const {
    /*
     * Delegates to the standalone secure_compare() utility.
     *
     * Returns SECURITY_MATCH_MAGIC (0x5A5A5A5A) on match,
     *         SECURITY_FAIL_MAGIC  (0xA5A5A5A5) on mismatch.
     *
     * See include/security_utils.h for full threat model documentation
     * (timing side-channel + fault injection / voltage glitching).
     */
    return secure_compare(enteredPin, storedPin, PIN_LENGTH);
}

/* ------------------------------------------------------------------ */
/*  Serial display helpers                                            */
/* ------------------------------------------------------------------ */

void PinFsm::printStateIndicator() const {
    /*
     * Renders: [* * _ . ]
     *   *  = confirmed digit (masked)
     *   _  = current digit being selected
     *   .  = future digit slot
     *
     * Uses only ISerial::print(char) and ISerial::println(const char*)
     * to avoid any dynamic allocation or snprintf.
     */
    serialOut.print('[');
    for (uint8_t i = 0; i < PIN_LENGTH; i++) {
        if (i < currentDigitIndex) {
            serialOut.print('*');
        } else if (i == currentDigitIndex) {
            serialOut.print('_');
        } else {
            serialOut.print('.');
        }
    }
    serialOut.println("]");
}

uint8_t PinFsm::getCurrentDigitIndex() const {
    return currentDigitIndex;
}

uint8_t PinFsm::getCurrentDigitValue() const {
    return currentDigitValue;
}
