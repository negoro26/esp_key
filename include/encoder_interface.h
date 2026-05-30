#pragma once

enum class EncoderEvent { NONE, CW, CCW, SW_PRESS, SW_RELEASE, SW_HOLD };

class IEncoder {
public:
    virtual ~IEncoder() = default;
    virtual void begin() = 0;
    virtual EncoderEvent poll() = 0;
};
