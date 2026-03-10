#pragma once

#include <cstdint>

// TcaExpander — shared TCA9554 accessor with shadow registers.
// All modules must use this to prevent pin clobbering.

namespace TcaExpander
{
    bool init();
    void setPinMode(uint8_t pin, uint8_t mode);
    void writePin(uint8_t pin, uint8_t value);
    uint8_t readPin(uint8_t pin);
    uint8_t readAll();
}
