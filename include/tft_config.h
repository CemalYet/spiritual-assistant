#ifndef TFT_CONFIG_H
#define TFT_CONFIG_H

// ===========================================
// TFT_eSPI User Setup for ILI9341 + XPT2046
// ESP32-S3 DevKitC-1
// ===========================================

// Driver
#define ILI9341_DRIVER

// Screen dimensions
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// ESP32-S3 Pin assignments
#define TFT_CS 10
#define TFT_RST 11
#define TFT_DC 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_MISO 15

// Touch controller pins
#define TOUCH_CS 9

// Use HSPI (SPI2) for TFT - avoid conflicts with Flash
#define USE_HSPI_PORT

// Fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

// Smooth fonts from SPIFFS/LittleFS
#define SMOOTH_FONT

// SPI frequency
#define SPI_FREQUENCY 40000000 // 40MHz for stability
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

#endif // TFT_CONFIG_H
