#define XPOWERS_CHIP_AXP2101 // Must precede XPowersLib.h

#include "pmu_manager.h"
#include "tft_config.h"
#include "tca_expander.h"

#include <XPowersLib.h>
#include <Wire.h>

static constexpr uint16_t ALDO1_VOLTAGE_MV = 3300;
static constexpr uint16_t BTN_BATT_CHARGE_MV = 3300;
static constexpr uint16_t SYS_POWERDOWN_MV = 2600;

static XPowersPMU pmu;
static bool initialized = false;

namespace PmuManager
{

    bool init()
    {
        if (initialized)
            return true;

        if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL))
        {
            Serial.printf("[PMU] AXP2101 not found at 0x%02X\n", PMU_ADDR);
            return false;
        }

        // Disable unused rails (see hardware/SCHEMATIC_REFERENCE.md)
        pmu.disableDC2();
        pmu.disableDC3();
        pmu.disableDC4();
        pmu.disableDC5();
        pmu.disableALDO2();
        pmu.disableALDO3();
        pmu.disableALDO4();
        pmu.disableBLDO1();
        pmu.disableBLDO2();
        pmu.disableCPUSLDO();
        pmu.disableDLDO1();
        pmu.disableDLDO2();

        // ALDO1 → LVCC3V3 (ES8311 codec)
        pmu.setALDO1Voltage(ALDO1_VOLTAGE_MV);
        pmu.enableALDO1();

        // VBUS limits
        pmu.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
        pmu.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);
        pmu.setSysPowerDownVoltage(SYS_POWERDOWN_MV);

        // No battery temp sensor on board — must disable or charging fails
        pmu.disableTSPinMeasure();

        // Charging parameters
        pmu.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
        pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
        pmu.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
        pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);
        pmu.setChargingLedMode(XPOWERS_CHG_LED_OFF);

        enableBatteryMonitoring();
        enableButtonBatteryCharge();

        // Power key timing
        pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
        pmu.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

        pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        pmu.clearIrqStatus();

        // Speaker amp OFF by default
        TcaExpander::setPinMode(EXIO_PA_CTRL, OUTPUT);
        TcaExpander::writePin(EXIO_PA_CTRL, LOW);

        initialized = true;
        Serial.println("[PMU] AXP2101 OK");
        return true;
    }

    void enableButtonBatteryCharge()
    {
        pmu.enableButtonBatteryCharge();
        pmu.setButtonBatteryChargeVoltage(BTN_BATT_CHARGE_MV);
    }

    void enableBatteryMonitoring()
    {
        pmu.enableBattDetection();
        pmu.enableBattVoltageMeasure();
        pmu.enableVbusVoltageMeasure();
        pmu.enableSystemVoltageMeasure();
    }

    int getBatteryPercent()
    {
        if (!initialized)
            return -1;
        return pmu.getBatteryPercent();
    }

    uint16_t getBatteryVoltage()
    {
        if (!initialized)
            return 0;
        return pmu.getBattVoltage();
    }

    bool isCharging()
    {
        if (!initialized)
            return false;
        return pmu.isCharging();
    }

    bool isVbusPresent()
    {
        if (!initialized)
            return false;
        return pmu.isVbusIn();
    }

    void setSpeakerAmpEnabled(bool enabled)
    {
        TcaExpander::writePin(EXIO_PA_CTRL, enabled ? HIGH : LOW);
    }

    void setCodecPowerEnabled(bool enabled)
    {
        if (!initialized)
            return;
        if (enabled)
            pmu.enableALDO1();
        else
            pmu.disableALDO1();
    }

    bool writeDataBuffer(uint8_t *data, uint8_t size)
    {
        if (!initialized || size > XPOWERS_AXP2101_DATA_BUFFER_SIZE)
            return false;
        return pmu.writeDataBuffer(data, size);
    }

    bool readDataBuffer(uint8_t *data, uint8_t size)
    {
        if (!initialized || size > XPOWERS_AXP2101_DATA_BUFFER_SIZE)
            return false;
        return pmu.readDataBuffer(data, size);
    }

} // namespace PmuManager
