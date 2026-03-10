#pragma once

#include <cstdint>

// PmuManager — AXP2101 PMIC driver.
// See hardware/SCHEMATIC_REFERENCE.md for rail map & charging config.

namespace PmuManager
{
    bool init();

    void enableButtonBatteryCharge();
    void enableBatteryMonitoring();
    int getBatteryPercent();      // 0–100, or -1 if no battery
    uint16_t getBatteryVoltage(); // millivolts
    bool isCharging();
    bool isVbusPresent();
    void setSpeakerAmpEnabled(bool enabled);
    void setCodecPowerEnabled(bool enabled);
    bool writeDataBuffer(uint8_t *data, uint8_t size);
    bool readDataBuffer(uint8_t *data, uint8_t size);
}
