#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <cstddef>

namespace WiFiPortal
{
    // Portal configuration
    constexpr const char *AP_SSID = "AdhanSettings";
    constexpr const char *AP_PASSWORD = "12345678"; // Minimum 8 chars for WPA2
    constexpr int AP_CHANNEL = 6;                   // Channel 6 - less crowded, better compatibility
    constexpr int AP_MAX_CONNECTIONS = 4;
    constexpr unsigned long PORTAL_TIMEOUT = 600000; // 10 minutes timeout (increased from 5 to prevent connection timeouts)

    // Portal lifecycle
    bool start();
    void stop();
    bool isActive();
    void handle();

    // Connection test result (call after hasNewCredentials() returns true)
    bool isConnectionSuccess(); // Returns true if WiFi test connection succeeded

    // Offline mode flag (set when user chooses offline in portal)
    bool isOfflineModeRequested();
    void clearOfflineModeFlag();

    // Credential callbacks
    bool hasNewCredentials();
    void getNewCredentials(char *ssidBuffer, size_t ssidSize, char *passBuffer, size_t passSize);
    void clearCredentials();
}
