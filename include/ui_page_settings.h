/**
 * @file ui_page_settings.h
 * @brief Settings Page - Volume Control, WiFi Button
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

    // Set callback for WiFi button press
    void setAdvancedCallback(AdvancedCallback cb);

    // Volume control (1-5)
    void setVolumeLevel(int level);
    int getVolumeLevel();

    // WiFi button state (handles button style + portal overlay internally)
    void setWiFiButtonState(WifiState state, const char *ip = nullptr);

} // namespace UiPageSettings

#endif // UI_PAGE_SETTINGS_H
