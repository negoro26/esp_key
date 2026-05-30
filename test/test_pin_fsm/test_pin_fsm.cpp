/**
 * @file test_pin_fsm.cpp
 * @brief Native tests for PinFsm — compiles the REAL pin_fsm.cpp.
 *
 * MockSerial and MockEncoder are injected to observe FSM behaviour
 * without any hardware. The FSM itself is NEVER mocked; we test the
 * real state machine logic end-to-end.
 *
 * Test coverage:
 * - Initial state after reset
 * - CW/CCW digit selection with wrap-around
 * - Full correct PIN entry → UNLOCKED
 * - Incorrect PIN → error, retry
 * - 3 consecutive failures → WIPE_TRIGGER → HALTED
 * - HALTED absorbs all further input
 * - Attempt counter correctness
 * - Serial output verification
 */
#include <unity.h>
#include <string.h>
#include "../../include/encoder_interface.h"
#include "../../include/serial_interface.h"
#include "../../include/pin_fsm_interface.h"
#include "../../src/pin_fsm_impl.h"

/* ================================================================== */
/*  MockSerial — captures output in static ring buffer, no heap       */
/* ================================================================== */

class MockSerial : public ISerial {
public:
    static const int MAX_LINES = 64;
    static const int MAX_LINE_LEN = 80;

    char lines[MAX_LINES][MAX_LINE_LEN];
    int  lineCount;
    /* Scratch buffer for building a line from print() calls before println(). */
    char scratch[MAX_LINE_LEN];
    int  scratchPos;

    MockSerial() : lineCount(0), scratchPos(0) {
        clear();
    }

    void clear() {
        for (int i = 0; i < MAX_LINES; i++) {
            lines[i][0] = '\0';
        }
        lineCount = 0;
        scratchPos = 0;
        scratch[0] = '\0';
    }

    void print(const char* str) override {
        /* Append to scratch buffer */
        int len = 0;
        while (str[len] != '\0') len++;
        for (int i = 0; i < len && scratchPos < MAX_LINE_LEN - 1; i++) {
            scratch[scratchPos++] = str[i];
        }
        scratch[scratchPos] = '\0';
    }

    void print(char c) override {
        if (scratchPos < MAX_LINE_LEN - 1) {
            scratch[scratchPos++] = c;
            scratch[scratchPos] = '\0';
        }
    }

    void println(const char* str) override {
        /* Flush scratch + str as a complete line */
        if (lineCount >= MAX_LINES) return;

        /* Copy scratch into line */
        int pos = 0;
        for (int i = 0; i < scratchPos && pos < MAX_LINE_LEN - 1; i++) {
            lines[lineCount][pos++] = scratch[i];
        }
        /* Append str */
        int len = 0;
        while (str[len] != '\0') len++;
        for (int i = 0; i < len && pos < MAX_LINE_LEN - 1; i++) {
            lines[lineCount][pos++] = str[i];
        }
        lines[lineCount][pos] = '\0';
        lineCount++;

        /* Reset scratch */
        scratchPos = 0;
        scratch[0] = '\0';
    }

    bool containsLine(const char* expected) const {
        for (int i = 0; i < lineCount; i++) {
            if (strcmp(lines[i], expected) == 0) {
                return true;
            }
        }
        return false;
    }
};

/* ================================================================== */
/*  MockEncoder — injectable event queue, no heap                     */
/* ================================================================== */

class MockEncoder : public IEncoder {
public:
    static const int MAX_EVENTS = 32;
    EncoderEvent eventQueue[MAX_EVENTS];
    int queueLen;
    int queuePos;

    MockEncoder() : queueLen(0), queuePos(0) {}

    void begin() override {}

    EncoderEvent poll() override {
        if (queuePos < queueLen) {
            return eventQueue[queuePos++];
        }
        return EncoderEvent::NONE;
    }

    void clearQueue() {
        queueLen = 0;
        queuePos = 0;
    }

    void enqueue(EncoderEvent ev) {
        if (queueLen < MAX_EVENTS) {
            eventQueue[queueLen++] = ev;
        }
    }
};

/* ================================================================== */
/*  Test fixtures                                                     */
/* ================================================================== */

static MockSerial   mockSerial;
static MockEncoder  mockEncoder;
static PinFsm       fsm(mockSerial, mockEncoder);

void setUp(void) {
    mockSerial.clear();
    mockEncoder.clearQueue();
    uint8_t pin[4] = {1, 2, 3, 4};
    fsm.setStoredPin(pin);
    fsm.setState(FsmState::LOCKED);
    fsm.reset();
    mockSerial.clear();   /* Clear the output from reset() itself */
}

void tearDown(void) {
    /* Nothing to free — zero dynamic allocation */
}

/* ================================================================== */
/*  Helper: feed N CW events to select a digit value                  */
/* ================================================================== */
static void rotateCW(int n) {
    for (int i = 0; i < n; i++) {
        fsm.processEvent(EncoderEvent::CW);
    }
}

static void rotateCCW(int n) {
    for (int i = 0; i < n; i++) {
        fsm.processEvent(EncoderEvent::CCW);
    }
}

static void confirmDigit() {
    fsm.processEvent(EncoderEvent::SW_PRESS);
}

/** Enter a 4-digit PIN by rotating CW from 0 for each digit. */
static void enterPin(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
    uint8_t digits[4] = {d0, d1, d2, d3};
    for (int i = 0; i < 4; i++) {
        rotateCW(digits[i]);
        confirmDigit();
    }
}

/* ================================================================== */
/*  Tests                                                             */
/* ================================================================== */

void test_reset_transitions_to_digit_select(void) {
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_cw_stays_in_digit_select(void) {
    fsm.processEvent(EncoderEvent::CW);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_ccw_stays_in_digit_select(void) {
    fsm.processEvent(EncoderEvent::CCW);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_digit_wrap_around_cw(void) {
    /* Rotate CW 10 times from 0 → should wrap back to 0,
       then one more press confirms digit 0.
       After 1 digit confirmed, still DIGIT_SELECT for next. */
    for (int i = 0; i < 10; i++) {
        fsm.processEvent(EncoderEvent::CW);
    }
    confirmDigit();
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_digit_wrap_around_ccw(void) {
    /* From 0, CCW once → wraps to 9. Confirm it. */
    rotateCCW(1);
    confirmDigit();
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_ccw_pin_entry(void) {
    /*
     * Enter PIN {1, 2, 3, 4} using CCW rotation from 0.
     * CCW(9) from 0 wraps to 9, then 8, 7, ... 1. So CCW(9) = digit 1.
     * CCW(8) from 0 wraps to 9, 8, ... 2. So CCW(8) = digit 2.
     * CCW(7) from 0 wraps to 9, 8, ... 3. So CCW(7) = digit 3.
     * CCW(6) from 0 wraps to 9, 8, ... 4. So CCW(6) = digit 4.
     */
    rotateCCW(9); confirmDigit();  /* digit 1 */
    rotateCCW(8); confirmDigit();  /* digit 2 */
    rotateCCW(7); confirmDigit();  /* digit 3 */
    rotateCCW(6); confirmDigit();  /* digit 4 */
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::UNLOCKED),
                      static_cast<int>(fsm.getState()));
}

void test_single_digit_confirm_advances(void) {
    rotateCW(5);
    confirmDigit();
    /* After confirming 1st digit, still in DIGIT_SELECT for 2nd */
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_correct_pin_unlocks(void) {
    /* Stored PIN is {1, 2, 3, 4} */
    enterPin(1, 2, 3, 4);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::UNLOCKED),
                      static_cast<int>(fsm.getState()));
}

void test_correct_pin_prints_success(void) {
    enterPin(1, 2, 3, 4);
    TEST_ASSERT_TRUE(mockSerial.containsLine("[SUCCESS] System Unlocked."));
}

void test_correct_pin_resets_attempts(void) {
    /* First fail to increment counter */
    enterPin(0, 0, 0, 0);
    TEST_ASSERT_EQUAL(1, fsm.getFailedAttempts());

    /* Now enter correct PIN */
    enterPin(1, 2, 3, 4);
    TEST_ASSERT_EQUAL(0, fsm.getFailedAttempts());
}

void test_incorrect_pin_error_message(void) {
    enterPin(9, 9, 9, 9);
    TEST_ASSERT_TRUE(mockSerial.containsLine("[ERROR] Invalid PIN"));
}

void test_incorrect_pin_returns_to_digit_select(void) {
    enterPin(0, 0, 0, 0);
    /* After failure, LOCKED auto-transitions to DIGIT_SELECT */
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_failed_attempt_counter_increments(void) {
    TEST_ASSERT_EQUAL(0, fsm.getFailedAttempts());

    enterPin(0, 0, 0, 0);
    TEST_ASSERT_EQUAL(1, fsm.getFailedAttempts());

    enterPin(0, 0, 0, 0);
    TEST_ASSERT_EQUAL(2, fsm.getFailedAttempts());
}

void test_three_failures_triggers_wipe(void) {
    enterPin(0, 0, 0, 0);  /* fail 1 */
    enterPin(0, 0, 0, 0);  /* fail 2 */
    enterPin(0, 0, 0, 0);  /* fail 3 → WIPE */

    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::HALTED),
                      static_cast<int>(fsm.getState()));
}

void test_wipe_prints_message(void) {
    enterPin(0, 0, 0, 0);
    enterPin(0, 0, 0, 0);
    mockSerial.clear();
    enterPin(0, 0, 0, 0);

    TEST_ASSERT_TRUE(mockSerial.containsLine("WIPE EXECUTED"));
}

void test_halted_absorbs_all_events(void) {
    enterPin(0, 0, 0, 0);
    enterPin(0, 0, 0, 0);
    enterPin(0, 0, 0, 0);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::HALTED),
                      static_cast<int>(fsm.getState()));

    /* Try every event type — state must remain HALTED */
    fsm.processEvent(EncoderEvent::CW);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::HALTED),
                      static_cast<int>(fsm.getState()));

    fsm.processEvent(EncoderEvent::CCW);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::HALTED),
                      static_cast<int>(fsm.getState()));

    fsm.processEvent(EncoderEvent::SW_PRESS);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::HALTED),
                      static_cast<int>(fsm.getState()));

    fsm.processEvent(EncoderEvent::SW_HOLD);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::HALTED),
                      static_cast<int>(fsm.getState()));
}

void test_state_indicator_format_initial(void) {
    /* After reset, indicator should show: [_...] */
    fsm.reset();
    TEST_ASSERT_TRUE(mockSerial.containsLine("[_...]"));
}

void test_state_indicator_format_after_one_digit(void) {
    rotateCW(1);
    confirmDigit();
    /* After 1 digit confirmed: [*_..] */
    TEST_ASSERT_TRUE(mockSerial.containsLine("[*_..]"));
}

void test_none_event_does_nothing(void) {
    FsmState before = fsm.getState();
    fsm.processEvent(EncoderEvent::NONE);
    TEST_ASSERT_EQUAL(static_cast<int>(before),
                      static_cast<int>(fsm.getState()));
}

void test_sw_release_ignored_in_digit_select(void) {
    fsm.processEvent(EncoderEvent::SW_RELEASE);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

void test_sw_hold_ignored_in_digit_select(void) {
    fsm.processEvent(EncoderEvent::SW_HOLD);
    TEST_ASSERT_EQUAL(static_cast<int>(FsmState::DIGIT_SELECT),
                      static_cast<int>(fsm.getState()));
}

/* ================================================================== */
/*  Main                                                              */
/* ================================================================== */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    RUN_TEST(test_reset_transitions_to_digit_select);
    RUN_TEST(test_cw_stays_in_digit_select);
    RUN_TEST(test_ccw_stays_in_digit_select);
    RUN_TEST(test_digit_wrap_around_cw);
    RUN_TEST(test_digit_wrap_around_ccw);
    RUN_TEST(test_ccw_pin_entry);
    RUN_TEST(test_single_digit_confirm_advances);
    RUN_TEST(test_correct_pin_unlocks);
    RUN_TEST(test_correct_pin_prints_success);
    RUN_TEST(test_correct_pin_resets_attempts);
    RUN_TEST(test_incorrect_pin_error_message);
    RUN_TEST(test_incorrect_pin_returns_to_digit_select);
    RUN_TEST(test_failed_attempt_counter_increments);
    RUN_TEST(test_three_failures_triggers_wipe);
    RUN_TEST(test_wipe_prints_message);
    RUN_TEST(test_halted_absorbs_all_events);
    RUN_TEST(test_state_indicator_format_initial);
    RUN_TEST(test_state_indicator_format_after_one_digit);
    RUN_TEST(test_none_event_does_nothing);
    RUN_TEST(test_sw_release_ignored_in_digit_select);
    RUN_TEST(test_sw_hold_ignored_in_digit_select);

    return UNITY_END();
}
