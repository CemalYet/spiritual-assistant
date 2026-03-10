#ifndef TFT_CONFIG_H
#define TFT_CONFIG_H

// See hardware/SCHEMATIC_REFERENCE.md for full board pin map.

// Display (QSPI)
#define QSPI_CS 12
#define QSPI_CLK 5
#define QSPI_D0 1
#define QSPI_D1 2
#define QSPI_D2 3
#define QSPI_D3 4

// Screen dimensions (native panel portrait — rotation handled by LVGL sw_rotate)
#define TFT_WIDTH 320
#define TFT_HEIGHT 480

// Backlight
#define TFT_BL 6

// I2C bus (shared by 6 devices)
#define I2C_SDA 8
#define I2C_SCL 7

// Touch
#define TOUCH_ADDR 0x3B
#define TOUCH_RST -1

// I2S Audio
#define I2S_BCLK 13
#define I2S_LRC 15
#define I2S_DOUT 16
#define I2S_MCLK 44

// ES8311 codec
#define CODEC_ADDR 0x18

// TCA9554 IO expander
#define TCA9554_ADDR 0x20

// AXP2101 PMIC
#define PMU_ADDR 0x34

// PCF85063 RTC
#define RTC_ADDR 0x51

// QMI8658 IMU
#define IMU_ADDR 0x6B

// TCA9554 EXIO pin map
#define EXIO_CAM_PWDN 0 // Camera power down
#define EXIO_LCD_RST 1  // Display reset
#define EXIO_TP_INT 2   // Touch interrupt
#define EXIO_SD_CS 3    // SD card chip select
#define EXIO_RTC_INT 4  // PCF85063 alarm interrupt
#define EXIO_AXP_IRQ 5  // AXP2101 PMU interrupt
#define EXIO_SYS_OUT 6  // PWR button (via BSS138)
#define EXIO_PA_CTRL 7  // Speaker amp enable

// GPIO0 — shared: BOOT button + QMI8658 INT1
#define WAKE_PIN GPIO_NUM_0

#endif // TFT_CONFIG_H
