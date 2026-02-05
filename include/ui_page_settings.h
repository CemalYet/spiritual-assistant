/**
 * @file ui_page_settings.h
 * @brief Settings Page - Volume Control, WiFi Button
 */

#ifndef UI_PAGE_SETTINGS_H
#define UI_PAGE_SETTINGS_H

#include <lvgl.h>

namespace UiPageSettings
{
    // WiFi button states
    enum class WiFiButtonState
    {
        DISCONNECTED, // Blue - "Ayarlar"
        CONNECTING,   // Yellow - "Baglaniyor..."
        CONNECTED,    // Green - "Bagli: IP"
        FAILED,       // Red - "Baglanamadi"
        PORTAL        // Cyan - AP info display
    };

    using AdvancedCallback = void (*)();

    // Create/recreate settings screen
    lv_obj_t *create();

    // Get the screen object
    lv_obj_t *getScreen();

    // Set callback for WiFi button press
    void setAdvancedCallback(AdvancedCallback cb);

    // Volume control (1-5)
    void setVolumeLevel(int level);
    int getVolumeLevel();

    // WiFi button state
    void setWiFiButtonState(WiFiButtonState state, const char *ip = nullptr);
    void updateWiFiButton(bool connected, const char *ip);

} // namespace UiPageSettings

#endif // UI_PAGE_SETTINGS_H
