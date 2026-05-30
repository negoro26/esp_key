/**
 * @file serial_interface.h
 * @brief Pure C++ serial output interface. ZERO Arduino dependencies.
 *
 * All firmware output is routed through this interface, allowing
 * injection of mock implementations for native testing.
 */
#pragma once

class ISerial {
public:
    virtual ~ISerial() = default;
    virtual void print(const char* str) = 0;
    virtual void println(const char* str) = 0;
    virtual void print(char c) = 0;
};
