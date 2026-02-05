/**
 * @file ui_home.h
 * @brief Premium LVGL UI - Main Display System
 * Replaces TftDisplay completely - handles all screens
 */

#ifndef UI_HOME_H
#define UI_HOME_H

#include <lvgl.h>

namespace UiHome
{
    // ═══════════════════════════════════════════════════════════════
    // CORE FUNCTIONS
    // ═══════════════════════════════════════════════════════════════
    void init(); // Initialize home screen
    void loop(); // Call from main loop (lv_timer_handler)

    // ═══════════════════════════════════════════════════════════════
    // HOME SCREEN UPDATES
    // ═══════════════════════════════════════════════════════════════
    void setTime(int hour, int minute);
    void setDate(const char *date); // Turkish date string
    void setCalculationMethod(const char *method);
    void setNextPrayer(const char *name, const char *time);
    void setWifiConnected(bool connected, int rssi = -50); // WiFi status with RSSI for color
    void setNtpSynced(bool synced);                        // NTP time sync status (checkmark icon)
    void setAdhanAvailable(bool available);                // Show crossed speaker if adhan file missing
    bool isMuted();
    void setMuted(bool muted);

    // Navigation
    using NavCallback = void (*)(int page); // 0=Home, 1=Prayer, 2=Menu
    void setNavCallback(NavCallback cb);
    void setActiveNav(int page);

    // ═══════════════════════════════════════════════════════════════
    // PRAYER TIMES PAGE (Cami icon)
    // ═══════════════════════════════════════════════════════════════
    struct PrayerTimesData
    {
        const char *fajr;
        const char *sunrise;
        const char *dhuhr;
        const char *asr;
        const char *maghrib;
        const char *isha;
        int nextPrayerIndex; // 0-5, -1 if none
    };
    void setPrayerTimes(const PrayerTimesData &data);
    void showPrayerTimesPage();
    void showHomePage();

    // ═══════════════════════════════════════════════════════════════
    // STATUS SCREENS (replaces TftDisplay)
    // ═══════════════════════════════════════════════════════════════
    void showConnecting(const char *ssid);                                     // WiFi connecting
    void showPortal(const char *apName, const char *password, const char *ip); // Portal mode
    void showMessage(const char *line1, const char *line2 = nullptr);          // Generic message
    void showError(const char *line1, const char *line2 = nullptr);            // Error with red X
    void showHome();                                                           // Switch to home screen

} // namespace UiHome

#endif // UI_HOME_H
