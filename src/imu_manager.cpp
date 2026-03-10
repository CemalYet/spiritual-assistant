#include "imu_manager.h"
#include "tft_config.h"

#include <SensorQMI8658.hpp>
#include <Wire.h>

static SensorQMI8658 qmi;
static bool initialized = false;

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
        qmi.powerDown();
        return true;
    }

    bool isAvailable()
    {
        return initialized;
    }

} // namespace ImuManager
