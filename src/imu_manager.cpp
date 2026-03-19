#include "imu_manager.h"
#include "tft_config.h"

#include <SensorQMI8658.hpp>
#include <Wire.h>

static SensorQMI8658 qmi;
static bool initialized = false;
static bool wakeOnMotionArmed = false;

static constexpr uint8_t WOM_THRESHOLD_MG = 80;
static constexpr uint8_t REG_CTRL1 = 0x02;
static constexpr uint8_t REG_STATUS0 = 0x2E;
static constexpr uint8_t REG_STATUS1 = 0x2F;

static bool writeRawReg(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static bool readRawReg(uint8_t reg, uint8_t &value)
{
    Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
        return false;

    if (Wire.requestFrom(static_cast<int>(QMI8658_L_SLAVE_ADDRESS), 1) != 1)
        return false;

    value = static_cast<uint8_t>(Wire.read());
    return true;
}

static bool applyLatchMode()
{
    // CTRL1=0x60: keep register auto-increment enabled and apply latch-style INT behavior
    // expected by the existing wake flow (event stays asserted until status read).
    if (!writeRawReg(REG_CTRL1, 0x60))
        return false;

    uint8_t verify = 0;
    if (!readRawReg(REG_CTRL1, verify))
        return false;

    return verify == 0x60;
}

namespace ImuManager
{

    bool init()
    {
        if (initialized)
            return true;

        if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, I2C_SDA, I2C_SCL))
        {
            Serial.printf("[IMU] QMI8658 not found at 0x%02X\n", IMU_ADDR);
            return false;
        }

        Serial.printf("[IMU] QMI8658 OK — chip ID: 0x%02X\n", qmi.getChipID());
        initialized = true;

        if (!applyLatchMode())
            Serial.println("[IMU] WARNING: Failed to apply CTRL1 latch mode (0x60)");

        qmi.powerDown();
        return true;
    }

    bool isAvailable()
    {
        return initialized;
    }

    bool clearWakeStatus()
    {
        if (!initialized)
            return false;

        // update() reads STATUS_INT/STATUS0/STATUS1. Also read STATUS0/1 directly to
        // mirror the known-good wake-clear sequence used in field testing.
        qmi.update();

        uint8_t dummy = 0;
        readRawReg(REG_STATUS0, dummy);
        readRawReg(REG_STATUS1, dummy);
        return true;
    }

    bool armWakeOnMotion()
    {
        if (!initialized)
            return false;

        clearWakeStatus();

        // Keep INT1 idle HIGH so a motion event can pull the shared GPIO0 line LOW.
        // This avoids entering sleep with the wake condition already active.
        const int rc = qmi.configWakeOnMotion(
            WOM_THRESHOLD_MG,
            SensorQMI8658::ACC_ODR_LOWPOWER_128Hz,
            SensorQMI8658::INTERRUPT_PIN_1,
            1,
            0x20);

        if (rc != 0)
        {
            Serial.printf("[IMU] Wake-on-motion arm failed (rc=%d)\n", rc);
            wakeOnMotionArmed = false;
            return false;
        }

        // configWakeOnMotion performs a sensor reset; re-apply latch mode after arm.
        if (!applyLatchMode())
            Serial.println("[IMU] WARNING: Failed to re-apply CTRL1 latch mode after WoM arm");

        wakeOnMotionArmed = true;
        return true;
    }

    void disarmAfterWake()
    {
        if (!initialized)
            return;

        clearWakeStatus();

        if (wakeOnMotionArmed)
            qmi.powerDown();

        wakeOnMotionArmed = false;
    }

} // namespace ImuManager
