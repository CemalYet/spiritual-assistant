#pragma once

#include "prayer_types.h"
#include <cstdint>

enum class PowerMode : uint8_t
{
    ALWAYS_ON = 0,
    SCREEN_OFF = 1
};

namespace SettingsManager
{
    // Initialize settings manager
    bool init();

    // Prayer calculation method (1-15)
    int getPrayerMethod();
    bool setPrayerMethod(int method);

    // Get method name for display
    const char *getMethodName(int method);
    const char *getMethodShortName(int method); // Short name for compact display

    // List of available methods for UI
    struct MethodInfo
    {
        int id;
        const char *name;
        const char *shortName; // Compact name (e.g., "Diyanet" instead of "Turkey (Diyanet)")
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

    // Mute state (persisted)
    bool getMuted();
    bool setMuted(bool muted);

    // --- Connection Mode ---
    // "wifi" = online mode, "offline" = manual time
    const char *getConnectionMode();
    bool setConnectionMode(const char *mode);
    bool isOfflineMode();

    // --- Location Settings ---
    double getLatitude();
    double getLongitude();
    bool setLocation(double latitude, double longitude);

    const char *getCityName();
    const char *getShortCityName(); // City name without parentheses
    bool setCityName(const char *name);

    int32_t getDiyanetId();
    bool setDiyanetId(int32_t id);

    // --- Action Flags ---
    bool needsRecalculation();
    void clearRecalculationFlag();

    // --- Power Mode ---
    PowerMode getPowerMode();
    bool setPowerMode(PowerMode mode);

    // --- Timezone ---
    const char *getTimezone();
    bool setTimezone(const char *posixTz);
}
