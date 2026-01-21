#pragma once

#include "prayer_types.h"

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

    // --- Adhan Settings ---
    // Per-prayer adhan enable/disable (Sunrise is always false)
    bool getAdhanEnabled(PrayerType prayer);
    bool setAdhanEnabled(PrayerType prayer, bool enabled);

    // Volume (0-100 percent)
    uint8_t getVolume();
    bool setVolume(uint8_t volume);

    // Get volume mapped to hardware range (0-21)
    uint8_t getHardwareVolume();

    // --- Location Settings ---
    double getLatitude();
    double getLongitude();
    bool setLocation(double latitude, double longitude);

    const char *getCityName();
    bool setCityName(const char *name);

    int32_t getDiyanetId();
    bool setDiyanetId(int32_t id);

    // --- Action Flags ---
    // Check if prayer times need recalculation (method or location changed)
    bool needsRecalculation();
    void clearRecalculationFlag();

    // Check if WiFi needs reconnection (credentials changed)
    bool needsWiFiReconnect();
    void clearWiFiReconnectFlag();
}
