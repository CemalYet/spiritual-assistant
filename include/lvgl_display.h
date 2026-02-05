/**
 * @file lvgl_display.h
 * @brief LVGL Display Driver - Main display system for ESP32
 *
 * Replaces TftDisplay completely. Uses LovyanGFX + LVGL for all screens.
 */

#ifndef LVGL_DISPLAY_H
#define LVGL_DISPLAY_H

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

    /// Update status icons (WiFi, NTP, Adhan)
    void updateStatus();

} // namespace LvglDisplay

#endif // LVGL_DISPLAY_H
