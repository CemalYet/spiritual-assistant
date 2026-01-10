#include "settings_manager.h"
#include "config.h"
#include <Preferences.h>
#include <Arduino.h>

namespace SettingsManager
{
    static Preferences preferences;
    constexpr const char *NAMESPACE = "settings";
    constexpr const char *KEY_PRAYER_METHOD = "prayerMethod";

    // Cached value to avoid frequent NVS reads
    static int cachedPrayerMethod = -1;

    // Action flags
    static volatile bool flagRecalculation = false;
    static volatile bool flagWiFiReconnect = false;

    // Available calculation methods
    static const MethodInfo methods[] = {
        {1, "Karachi"},
        {2, "ISNA (North America)"},
        {3, "MWL (Muslim World League)"},
        {4, "Umm al-Qura (Makkah)"},
        {5, "Egyptian"},
        {6, "Gulf"},
        {7, "Tehran"},
        {8, "Dubai"},
        {9, "Kuwait"},
        {10, "Qatar"},
        {11, "Singapore"},
        {12, "France (UOIF)"},
        {13, "Turkey (Diyanet)"},
        {14, "Russia"},
        {15, "Moonsighting Committee"},
        {0, nullptr} // Terminator
    };

    bool init()
    {
        // Load cached value
        preferences.begin(NAMESPACE, true);
        cachedPrayerMethod = preferences.getInt(KEY_PRAYER_METHOD, Config::PRAYER_METHOD);
        preferences.end();

        Serial.printf("[Settings] Initialized - Prayer Method: %d (%s)\n",
                      cachedPrayerMethod, getMethodName(cachedPrayerMethod));
        return true;
    }

    int getPrayerMethod()
    {
        if (cachedPrayerMethod < 0)
        {
            // Not initialized, read from NVS
            preferences.begin(NAMESPACE, true);
            cachedPrayerMethod = preferences.getInt(KEY_PRAYER_METHOD, Config::PRAYER_METHOD);
            preferences.end();
        }
        return cachedPrayerMethod;
    }

    bool setPrayerMethod(int method)
    {
        // Validate method ID
        if (method < 1 || method > 15)
        {
            Serial.printf("[Settings] Invalid prayer method: %d\n", method);
            return false;
        }

        if (!preferences.begin(NAMESPACE, false))
        {
            Serial.println("[Settings] ERROR: Failed to open NVS namespace!");
            return false;
        }

        bool success = preferences.putInt(KEY_PRAYER_METHOD, method) > 0;
        preferences.end();

        if (success)
        {
            cachedPrayerMethod = method;
            flagRecalculation = true; // Trigger recalculation in main loop
            Serial.printf("[Settings] Prayer method saved: %d (%s)\n",
                          method, getMethodName(method));
        }
        else
        {
            Serial.println("[Settings] Failed to save prayer method!");
        }

        return success;
    }

    const char *getMethodName(int method)
    {
        for (const auto &m : methods)
        {
            if (m.id == method)
            {
                return m.name;
            }
        }
        return "Unknown";
    }

    const MethodInfo *getAvailableMethods()
    {
        return methods;
    }

    int getMethodCount()
    {
        return 15; // Methods 1-15
    }

    // --- Action Flag Implementation ---
    bool needsRecalculation()
    {
        return flagRecalculation;
    }

    void clearRecalculationFlag()
    {
        flagRecalculation = false;
    }

    bool needsWiFiReconnect()
    {
        return flagWiFiReconnect;
    }

    void clearWiFiReconnectFlag()
    {
        flagWiFiReconnect = false;
    }
}
