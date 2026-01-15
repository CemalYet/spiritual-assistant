#pragma once

namespace Network
{
    // Initialize network subsystem
    void init();

    // Connect to WiFi (uses stored credentials or starts portal)
    bool connectWiFi();

    // Sync time via NTP
    void syncTime();

    // Handle portal in main loop (call frequently)
    void handlePortal();

    // Check connection status
    bool isConnected();

    // Check if portal is active
    bool isPortalActive();

    // Returns true if portal opened after failed connection
    bool isRetryConnection();

    // Returns number of failed connection attempts
    int getConnectionAttempts();
}
