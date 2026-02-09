/**
 * @file lvgl_display.h
 * @brief LVGL Display Driver - Main display system for ESP32
 *
 * Replaces TftDisplay completely. Uses LovyanGFX + LVGL for all screens.
 */

#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

#include <stdint.h>

namespace LvglDisplay
{
    /// Initialize LVGL and display hardware
    /// Call this ONCE at startup before any other display functions
    bool begin();

    /// LVGL tick handler - call from main loop (handles timers, animations)
    void loop();

    /// Initialize home screen UI (prayer data set by main.cpp)
    void showPrayerScreen();

    /// Update time display (called every minute)
    void updateTime();

    /// Force date update (called after portal closes or time sync)
    void updateDate();

    /// Update status icons (WiFi, NTP, Adhan)
    void updateStatus();

    /// Format prayer date string (e.g. "1 Subat Cumartesi")
    /// dayOffset: 0 = today, 1 = tomorrow
    /// Returns pointer to static buffer (valid until next call)
    const char *formatPrayerDate(int dayOffset = 0);

} // namespace LvglDisplay

#endif // LVGL_DISPLAY_H
