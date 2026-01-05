# ESP32-S3 Spiritual Assistant

A modern C++17 prayer times assistant for ESP32-S3 with WiFi connectivity, NTP time synchronization, and LCD display support.

## Features

- ✅ **Automatic Prayer Times** - Computes daily prayer times offline (Adhan)
- ✅ **Turkey (Diyanet) Support** - Tries abdus.dev (Diyanet) first, then falls back to offline
- ✅ **WiFi Connectivity** - Connects to your home WiFi network
- ✅ **NTP Time Sync** - Automatically synchronizes time from internet
- ✅ **LCD Display** - 16x2 I2C LCD with custom mosque and speaker icons
- ✅ **Real-time Adhan Notifications** - Alerts at exact prayer times
- ✅ **Low Power** - Checks prayer times only once per day at midnight
- ✅ **Optimized** - Modern C++17 with memory-efficient code

## Hardware

- **Board**: ESP32-S3-WROOM-1-N16R8
  - 16MB Flash (Quad SPI)
  - 8MB PSRAM (Octal SPI)
  - 240MHz CPU
- **Display**: 16x2 I2C LCD (optional, address 0x27 or 0x3F)
- **Connection**: WiFi (2.4GHz)

## Pin Configuration

| LCD Pin | ESP32-S3 Pin |
|---------|--------------|
| SDA     | GPIO 21      |
| SCL     | GPIO 22      |
| VCC     | 5V           |
| GND     | GND          |

## Installation

1. **Install PlatformIO**
   ```bash
   pip install platformio
   ```

2. **Clone Repository**
   ```bash
   git clone <your-repo-url>
   cd SpiritialAsistant
   ```

3. **Configure WiFi**
   
   Edit `include/config.h`:
   ```cpp
   constexpr std::string_view WIFI_SSID = "YourWiFiName";
   constexpr std::string_view WIFI_PASS = "YourPassword";
   ```

4. **Configure Location** (optional)
   
   Edit `include/config.h`:
   ```cpp
   constexpr std::string_view CITY = "YourCity";
   constexpr std::string_view COUNTRY = "YourCountry";
   ```

5. **Enable LCD** (if connected)
   
   Edit `include/config.h`:
   ```cpp
   constexpr bool LCD_ENABLED = true;
   ```

6. **Build and Upload**
   ```bash
   platformio run --target upload
   ```

7. **Monitor Serial Output**
   ```bash
   platformio device monitor
   ```

## Configuration Options

### Prayer Calculation Method
In `include/config.h`:
```cpp
constexpr int PRAYER_METHOD = 13;  // 13 = Diyanet (Turkey)
```

Available methods:
- 1: University of Islamic Sciences, Karachi
- 2: Islamic Society of North America
- 3: Muslim World League
- 4: Umm Al-Qura University, Makkah
- 5: Egyptian General Authority of Survey
- 13: Diyanet (Turkey)

### Timezone
In `include/config.h`:
```cpp
constexpr long GMT_OFFSET_SEC = 3600;      // GMT+1 (Belgium)
constexpr int DAYLIGHT_OFFSET_SEC = 3600;  // DST offset
```

### LCD Address
In `include/config.h`:
```cpp
constexpr uint8_t LCD_ADDRESS = 0x27;  // Try 0x3F if not working
```

## Prayer Times Display

### Serial Monitor Output
```
========================================
  ESP32-S3 SPIRITUAL ASSISTANT v2.0
========================================
Flash: 16 MB | PSRAM: ACTIVE (7 MB)
========================================

[WiFi] Connected! IP: 192.168.0.208
[NTP] Time synced: 10:10:16

=== PRAYER TIMES UPDATED ===
Imsak   : 06:41
Ogle    : 12:47
Ikindi  : 14:28
Aksam   : 16:48
Yatsi   : 18:36
============================

[Info] Next prayer: Ogle at 12:47

🕌 === ADHAN TIME: Ogle === 🕌
```

### LCD Display Format
```
Line 1: 🕌 14:30 Ikindi
Line 2: 🔊 Aksam 16:48
```

## Project Structure

```
SpiritialAsistant/
├── src/
│   └── main.cpp          # Main application code
├── include/              # Header files (empty)
├── lib/                  # Custom libraries (empty)
├── test/                 # Unit tests (empty)
├── platformio.ini        # PlatformIO configuration
├── .gitignore           # Git ignore rules
└── README.md            # This file
```

## Technical Details

### Memory Usage
- **RAM**: 14% (45KB / 320KB)
- **Flash**: 13.6% (891KB / 6.5MB)
- **PSRAM**: 7MB available

### Libraries Used
- ArduinoJson 7.4.2
- LiquidCrystal_I2C 1.1.4
- HTTPClient (built-in)
- WiFi (built-in)

### Code Features
- Modern C++17 with `std::optional`, `std::string_view`, `std::array`
- Type-safe enum classes
- Constexpr compile-time optimizations
- Zero-cost abstractions
- Manual time formatting (10x faster than snprintf)
- Minute-change detection (60x fewer checks)
- Daily API fetch (24x fewer calls)

## Troubleshooting

### WiFi not connecting
- Check SSID and password spelling
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Move closer to router

### LCD not working
- Check I2C address (try 0x3F instead of 0x27)
- Verify wiring (SDA=GPIO21, SCL=GPIO22)
- Test with I2C scanner sketch

### Wrong prayer times
- Verify city and country names
- Check timezone settings (GMT_OFFSET_SEC)
- Try different prayer calculation method

### HTTP Error 302
- Already fixed - using HTTPS
- Check internet connection

## License

MIT License - feel free to use and modify!

## Author

Created with ❤️ for the Muslim community

## Acknowledgments

- Prayer times (Turkey/Diyanet) by [abdus.dev](https://prayertimes.api.abdus.dev/)
- Built with [PlatformIO](https://platformio.org/)
- Powered by ESP32-S3
