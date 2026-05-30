/**
 * @file test_encoder.cpp
 * @brief Native tests for the IEncoder interface mock.
 *
 * Validates that MockEncoder correctly returns injected events
 * and auto-resets to NONE, proving the interface contract.
 */
#include <unity.h>
#include "../../include/encoder_interface.h"

class MockEncoder : public IEncoder {
public:
    EncoderEvent nextEvent = EncoderEvent::NONE;

    void begin() override {
    }

    EncoderEvent poll() override {
        EncoderEvent ev = nextEvent;
        nextEvent = EncoderEvent::NONE;
        return ev;
    }
};

MockEncoder mockEnc;

void setUp(void) {}
void tearDown(void) {}

void test_mock_encoder_returns_event(void) {
    mockEnc.nextEvent = EncoderEvent::CW;
    TEST_ASSERT_EQUAL(static_cast<int>(EncoderEvent::CW), static_cast<int>(mockEnc.poll()));
    TEST_ASSERT_EQUAL(static_cast<int>(EncoderEvent::NONE), static_cast<int>(mockEnc.poll()));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_mock_encoder_returns_event);
    return UNITY_END();
}
