/**
 * @file ui_page_home.h
 * @brief Screen 0 — Main Dashboard
 *
 * Layout (480x320 landscape):
 *   Status bar (h=22) → Separator (h=1) → Greeting (h=16) →
 *   Hero area (flex col, countdown) → Prayer strip (h=50, bottom) → Nav dots
 */

#ifndef UI_PAGE_HOME_H
#define UI_PAGE_HOME_H

#include <lvgl.h>
#include <cstdint>

namespace UiPageHome
{
    // Build and return the screen object (call once at startup)
    lv_obj_t *create();

    lv_obj_t *getScreen();

    // ── Data setters (called from ui_state_reader) ──────────────────

    // Countdown seconds until next prayer — updates lbl_countdown
    void setCountdown(uint32_t secondsToNext);

    // Next prayer name in Turkish (e.g. "Yatsı")
    void setNextPrayerName(const char *name);

    // All 6 prayer times for the strip ("05:12" etc.)
    void setPrayerTimes(const char *fajr, const char *sunrise,
                        const char *dhuhr, const char *asr,
                        const char *maghrib, const char *isha);

    // Which column is active (0=İmsak … 5=Yatsı, -1=none)
    void setActivePrayerIndex(int8_t idx);

    // Progress 0–100 for the active glow bar
    void setActivePrayerProgress(uint8_t pct);

    // Greeting left part: greeting string, right part: date/ramadan string
    // e.g. ("Hayırlı Geceler —", "Ramazan 17. Gün")
    void setGreeting(const char *left, const char *right);

    // Iftar/Sahur pill — pass text like "İftara Kaldı 02:31", or visible=false to hide
    void setIftarDelta(bool visible, const char *text);

    // Status bar left text and date abbreviation
    void setStatusBarCity(const char *city, const char *dateAbbrev);

    // WiFi bars (0–3) and battery (0–100, charging flag)
    void setMuted(bool muted);
    void setWifi(uint8_t bars);

} // namespace UiPageHome

#endif // UI_PAGE_HOME_H
