/**
 * @file lvgl_display.h
 * @brief LVGL Display Driver - Main display system for ESP32
 *
 * Replaces TftDisplay completely. Uses Arduino_GFX + LVGL for all screens.
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

    /// Format prayer date string (e.g. "1 Subat Cumartesi")
    /// dayOffset: 0 = today, 1 = tomorrow
    /// Returns pointer to static buffer (valid until next call)
    const char *formatPrayerDate(int dayOffset = 0);

    void setBacklight(uint8_t brightness);
    void displayOff();
    void displayOn();

    /// True while a swipe gesture is being processed (suppress button clicks)
    bool isGestureActive();

    /// Navigate to portal screen (page 3)
    void goToPortalPage();

} // namespace LvglDisplay

#endif // LVGL_DISPLAY_H
