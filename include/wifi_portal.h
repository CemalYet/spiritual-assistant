#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <cstddef>

namespace WiFiPortal
{
    // Portal configuration
    constexpr const char *AP_SSID = "SpiritualAssistant-Setup";
    constexpr const char *AP_PASSWORD = "12345678"; // Minimum 8 chars for WPA2
    constexpr int AP_CHANNEL = 1;
    constexpr int AP_MAX_CONNECTIONS = 4;
    constexpr unsigned long PORTAL_TIMEOUT = 600000; // 10 minutes timeout (increased from 5 to prevent connection timeouts)

    // Portal lifecycle
    bool start();
    void stop();
    bool isActive();
    void handle();

    // Credential callbacks
    bool hasNewCredentials();
    void getNewCredentials(char *ssidBuffer, size_t ssidSize, char *passBuffer, size_t passSize);
    void clearCredentials();
}
