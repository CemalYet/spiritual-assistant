/**
 * @file ui_page_status.h
 * @brief Status Screens - Connecting, Portal, Message, Error
 */

#ifndef UI_PAGE_STATUS_H
#define UI_PAGE_STATUS_H

#include <lvgl.h>

namespace UiPageStatus
{
    // Show "Connecting to WiFi..." screen
    void showConnecting(const char *ssid);

    // Show WiFi portal configuration screen
    void showPortal(const char *apName, const char *password, const char *ip);

    // Show generic message (1 or 2 lines)
    void showMessage(const char *line1, const char *line2 = nullptr);

    // Show error message with red X icon
    void showError(const char *line1, const char *line2 = nullptr);

} // namespace UiPageStatus

#endif // UI_PAGE_STATUS_H
