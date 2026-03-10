/**
 * @file ui_page_settings.h
 * @brief Settings Page — Brightness, Volume, Toggles, WiFi Connect
 *
 * Layout matches the HTML mockup: 4 sections (Ekran, Ses, Güç & Mod, Bağlantı)
 * with slider rows, toggle rows, and a QR-nav button.
 */

#ifndef UI_PAGE_SETTINGS_H
#define UI_PAGE_SETTINGS_H

#include <lvgl.h>
#include "app_state.h" // WifiState

namespace UiPageSettings
{
    using AdvancedCallback = void (*)();

    // Create/recreate settings screen
    lv_obj_t *create();

    // Get the screen object
    lv_obj_t *getScreen();

    // Set callback for WiFi / "Telefondan Yönet" button press
    void setAdvancedCallback(AdvancedCallback cb);

    // Volume control (0-100 percentage)
    void setVolumeLevel(int pct);
    int getVolumeLevel();

    // Brightness control (0-100%)
    void setBrightnessLevel(int pct);

    // Sync toggle visuals from AppState / SettingsManager
    void syncToggles();

    // WiFi button state (handles button style + portal screen internally)
    void setWiFiButtonState(WifiState state, const char *ip = nullptr);

    // Get portal screen (page 3) — returns nullptr if portal not active
    lv_obj_t *getPortalScreen();

    // Power mode selector (syncs visual to current SettingsManager value)
    void updatePowerModeUI();

    // Returns true if (x,y) screen coordinate hits a slider track
    bool isSliderHit(int16_t x, int16_t y);

    // Status bar updates (wifi/battery icons)
    void setStatusBarCity(const char *city, const char *dateAbbrev);
    void setWifi(uint8_t bars);
    void setBattery(uint8_t pct, bool charging);

} // namespace UiPageSettings

#endif // UI_PAGE_SETTINGS_H
