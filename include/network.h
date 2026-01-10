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

    // Handle settings server in main loop (call frequently)
    void handleSettingsServer();

    // Start mDNS with settings hostname
    bool startMDNS();

    // Start settings web server
    void startSettingsServer();

    // Check connection status
    bool isConnected();

    // Check if portal is active
    bool isPortalActive();

    // Check if settings server is active (portal completed successfully)
    bool isSettingsActive();

    // Returns true if portal opened after failed connection
    bool isRetryConnection();

    // Returns number of failed connection attempts
    int getConnectionAttempts();
}
