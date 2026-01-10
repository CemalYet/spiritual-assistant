#pragma once

namespace SettingsManager
{
    // Initialize settings manager
    bool init();

    // Prayer calculation method (1-15)
    int getPrayerMethod();
    bool setPrayerMethod(int method);

    // Get method name for display
    const char *getMethodName(int method);

    // List of available methods for UI
    struct MethodInfo
    {
        int id;
        const char *name;
    };

    // Returns array of available methods (null-terminated)
    const MethodInfo *getAvailableMethods();
    int getMethodCount();

    // --- Action Flags ---
    // Check if prayer times need recalculation (method or location changed)
    bool needsRecalculation();
    void clearRecalculationFlag();

    // Check if WiFi needs reconnection (credentials changed)
    bool needsWiFiReconnect();
    void clearWiFiReconnectFlag();
}
