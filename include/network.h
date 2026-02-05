#pragma once

namespace Network
{
    // Initialize network subsystem
    // If skipHardcodedCredentials is true, only use stored NVS credentials (for FORCE_AP_PORTAL)
    void init(bool skipHardcodedCredentials = false);

    // Connect to WiFi (uses stored credentials or starts portal)
    bool connectWiFi();

    // Sync time via NTP
    void syncTime();

    // Handle portal in main loop (call frequently)
    void handlePortal();

    // Check connection status
    bool isConnected();

    // Disconnect WiFi (for power saving)
    void disconnect();

    // Check if portal is active
    bool isPortalActive();

    // Stop portal (for offline mode transition)
    void stopPortal();

    // Start portal for settings (no restart needed)
    void startPortal();

    // Returns true if portal opened after failed connection
    bool isRetryConnection();

    // Returns number of failed connection attempts
    int getConnectionAttempts();

    // Returns true if portal just closed with successful WiFi connection
    bool didPortalConnectWiFi();

    // Clear the portal-connected-wifi flag
    void clearPortalConnectFlag();
}
