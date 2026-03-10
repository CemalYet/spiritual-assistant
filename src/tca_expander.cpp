#include "tca_expander.h"
#include "tft_config.h"
#include <TCA9554.h>
#include <Arduino.h>

namespace
{
    TCA9554 tca(TCA9554_ADDR);

    constexpr uint8_t PIN_COUNT = 8;

    uint8_t shadowDirection = 0xFF; // TCA9554 default: all INPUT
    uint8_t shadowOutput = 0x00;
    bool initialized = false;
}

namespace TcaExpander
{
    bool init()
    {
        if (initialized)
            return true;

        if (!tca.begin())
        {
            Serial.println("[TcaExpander] ERROR: TCA9554 not found at 0x20");
            return false;
        }

        // Read current chip state into shadow registers
        shadowDirection = 0xFF;
        shadowOutput = 0x00;

        initialized = true;
        Serial.println("[TcaExpander] OK — TCA9554 initialized");
        return true;
    }

    void setPinMode(uint8_t pin, uint8_t mode)
    {
        if (pin >= PIN_COUNT || !initialized)
            return;

        tca.pinMode1(pin, mode);

        if (mode == OUTPUT)
            shadowDirection &= ~(1 << pin);
        else
            shadowDirection |= (1 << pin);
    }

    void writePin(uint8_t pin, uint8_t value)
    {
        if (pin >= PIN_COUNT || !initialized)
            return;

        if (value)
            shadowOutput |= (1 << pin);
        else
            shadowOutput &= ~(1 << pin);

        tca.write1(pin, value);
    }

    uint8_t readPin(uint8_t pin)
    {
        if (pin >= PIN_COUNT || !initialized)
            return 0;
        return tca.read1(pin);
    }

    uint8_t readAll()
    {
        if (!initialized)
            return 0;
        return tca.read8();
    }
}
