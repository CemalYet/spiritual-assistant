#include "settings_manager.h"
#include "config.h"
#include <Preferences.h>
#include <Arduino.h>

namespace SettingsManager
{
    static Preferences preferences;
    constexpr const char *NAMESPACE = "settings";
    constexpr const char *KEY_PRAYER_METHOD = "prayerMethod";
    constexpr const char *KEY_VOLUME = "volume";
    // Adhan enable keys (one per prayer, excluding Sunrise)
    constexpr const char *KEY_ADHAN_FAJR = "adhanFajr";
    constexpr const char *KEY_ADHAN_DHUHR = "adhanDhuhr";
    constexpr const char *KEY_ADHAN_ASR = "adhanAsr";
    constexpr const char *KEY_ADHAN_MAGHRIB = "adhanMaghrib";
    constexpr const char *KEY_ADHAN_ISHA = "adhanIsha";

    // Location keys
    constexpr const char *KEY_LATITUDE = "latitude";
    constexpr const char *KEY_LONGITUDE = "longitude";
    constexpr const char *KEY_CITY_NAME = "cityName";
    constexpr const char *KEY_DIYANET_ID = "diyanetId";

    // Cached values to avoid frequent NVS reads
    static int cachedPrayerMethod = -1;
    static int8_t cachedVolume = -1;
    static int8_t cachedAdhanEnabled[6] = {-1, -1, -1, -1, -1, -1}; // -1 = not loaded

    // Location cache (static buffers - no heap)
    static double cachedLatitude = NAN;
    static double cachedLongitude = NAN;
    static char cachedCityName[96] = {0}; // "City, State, Country" format
    static int32_t cachedDiyanetId = -1;
    static bool locationLoaded = false;

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
        // Load cached values
        preferences.begin(NAMESPACE, true);
        cachedPrayerMethod = preferences.getInt(KEY_PRAYER_METHOD, Config::PRAYER_METHOD);
        cachedVolume = preferences.getUChar(KEY_VOLUME, 80); // Default 80%

        // Load adhan enabled states (default: all enabled except Sunrise)
        cachedAdhanEnabled[idx(PrayerType::Fajr)] = preferences.getBool(KEY_ADHAN_FAJR, true) ? 1 : 0;
        cachedAdhanEnabled[idx(PrayerType::Sunrise)] = 0; // Sunrise never plays adhan
        cachedAdhanEnabled[idx(PrayerType::Dhuhr)] = preferences.getBool(KEY_ADHAN_DHUHR, true) ? 1 : 0;
        cachedAdhanEnabled[idx(PrayerType::Asr)] = preferences.getBool(KEY_ADHAN_ASR, true) ? 1 : 0;
        cachedAdhanEnabled[idx(PrayerType::Maghrib)] = preferences.getBool(KEY_ADHAN_MAGHRIB, true) ? 1 : 0;
        cachedAdhanEnabled[idx(PrayerType::Isha)] = preferences.getBool(KEY_ADHAN_ISHA, true) ? 1 : 0;
        preferences.end();

        Serial.printf("[Settings] Initialized - Method: %d (%s), Volume: %d%%\n",
                      cachedPrayerMethod, getMethodName(cachedPrayerMethod), cachedVolume);
        Serial.printf("[Settings] Adhan: Fajr=%d, Dhuhr=%d, Asr=%d, Maghrib=%d, Isha=%d\n",
                      cachedAdhanEnabled[0], cachedAdhanEnabled[2], cachedAdhanEnabled[3],
                      cachedAdhanEnabled[4], cachedAdhanEnabled[5]);
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

        // Skip if method hasn't changed (no recalculation needed)
        if (method == cachedPrayerMethod)
        {
            Serial.printf("[Settings] Prayer method unchanged: %d\n", method);
            return true;
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
            flagRecalculation = true; // Trigger recalculation only when method actually changes
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

    // --- Adhan Settings Implementation ---

    static const char *getAdhanKey(PrayerType prayer)
    {
        switch (prayer)
        {
        case PrayerType::Fajr:
            return KEY_ADHAN_FAJR;
        case PrayerType::Dhuhr:
            return KEY_ADHAN_DHUHR;
        case PrayerType::Asr:
            return KEY_ADHAN_ASR;
        case PrayerType::Maghrib:
            return KEY_ADHAN_MAGHRIB;
        case PrayerType::Isha:
            return KEY_ADHAN_ISHA;
        default:
            return nullptr;
        }
    }

    bool getAdhanEnabled(PrayerType prayer)
    {
        // Sunrise NEVER plays adhan
        if (prayer == PrayerType::Sunrise)
            return false;

        uint8_t index = idx(prayer);
        if (index >= 6)
            return false;

        if (cachedAdhanEnabled[index] < 0)
        {
            // Load from NVS
            const char *key = getAdhanKey(prayer);
            if (key)
            {
                preferences.begin(NAMESPACE, true);
                cachedAdhanEnabled[index] = preferences.getBool(key, true) ? 1 : 0;
                preferences.end();
            }
        }

        return cachedAdhanEnabled[index] == 1;
    }

    bool setAdhanEnabled(PrayerType prayer, bool enabled)
    {
        // Cannot enable Sunrise adhan
        if (prayer == PrayerType::Sunrise)
            return false;

        const char *key = getAdhanKey(prayer);
        if (!key)
            return false;

        if (!preferences.begin(NAMESPACE, false))
        {
            Serial.println("[Settings] ERROR: Failed to open NVS namespace!");
            return false;
        }

        bool success = preferences.putBool(key, enabled);
        preferences.end();

        if (success)
        {
            cachedAdhanEnabled[idx(prayer)] = enabled ? 1 : 0;
            Serial.printf("[Settings] Adhan %s: %s\n",
                          getPrayerName(prayer).data(),
                          enabled ? "enabled" : "disabled");
        }

        return success;
    }

    uint8_t getVolume()
    {
        if (cachedVolume < 0)
        {
            preferences.begin(NAMESPACE, true);
            cachedVolume = preferences.getUChar(KEY_VOLUME, 80);
            preferences.end();
        }
        return static_cast<uint8_t>(cachedVolume);
    }

    uint8_t getHardwareVolume()
    {
        return (getVolume() * 21) / 100;
    }

    bool setVolume(uint8_t volume)
    {
        if (volume > 100)
            volume = 100;

        if (!preferences.begin(NAMESPACE, false))
        {
            Serial.println("[Settings] ERROR: Failed to open NVS namespace!");
            return false;
        }

        bool success = preferences.putUChar(KEY_VOLUME, volume) > 0;
        preferences.end();

        if (success)
        {
            cachedVolume = volume;
            Serial.printf("[Settings] Volume saved: %d%%\n", volume);
        }

        return success;
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

    // --- Location Implementation ---

    static void loadLocationIfNeeded()
    {
        if (locationLoaded)
            return;

        preferences.begin(NAMESPACE, true);
        cachedLatitude = preferences.getDouble(KEY_LATITUDE, NAN);
        cachedLongitude = preferences.getDouble(KEY_LONGITUDE, NAN);
        cachedDiyanetId = preferences.getInt(KEY_DIYANET_ID, -1);

        // Read city name into static buffer
        size_t len = preferences.getString(KEY_CITY_NAME, cachedCityName, sizeof(cachedCityName));
        if (len == 0)
            cachedCityName[0] = '\0';

        preferences.end();
        locationLoaded = true;

        Serial.printf("[Settings] Location loaded: %.4f, %.4f (%s) DiyanetID=%d\n",
                      cachedLatitude, cachedLongitude,
                      cachedCityName[0] ? cachedCityName : "unnamed",
                      cachedDiyanetId);
    }

    double getLatitude()
    {
        loadLocationIfNeeded();
        return cachedLatitude;
    }

    double getLongitude()
    {
        loadLocationIfNeeded();
        return cachedLongitude;
    }

    bool setLocation(double latitude, double longitude)
    {
        loadLocationIfNeeded();

        // Skip if unchanged
        if (cachedLatitude == latitude && cachedLongitude == longitude)
            return true;

        if (!preferences.begin(NAMESPACE, false))
        {
            Serial.println("[Settings] ERROR: Failed to open NVS!");
            return false;
        }

        bool success = preferences.putDouble(KEY_LATITUDE, latitude) &&
                       preferences.putDouble(KEY_LONGITUDE, longitude);
        preferences.end();

        if (success)
        {
            cachedLatitude = latitude;
            cachedLongitude = longitude;
            flagRecalculation = true;
            Serial.printf("[Settings] Location saved: %.4f, %.4f\n", latitude, longitude);
        }

        return success;
    }

    const char *getCityName()
    {
        loadLocationIfNeeded();
        return cachedCityName;
    }

    bool setCityName(const char *name)
    {
        if (!name)
            name = "";

        // Copy to cache (strlcpy always null-terminates)
        strlcpy(cachedCityName, name, sizeof(cachedCityName));

        if (!preferences.begin(NAMESPACE, false))
        {
            Serial.println("[Settings] ERROR: Failed to open NVS!");
            return false;
        }

        bool success = preferences.putString(KEY_CITY_NAME, cachedCityName) > 0;
        preferences.end();

        if (success)
            Serial.printf("[Settings] City name saved: %s\n", cachedCityName);

        return success;
    }

    int32_t getDiyanetId()
    {
        loadLocationIfNeeded();
        return cachedDiyanetId;
    }

    bool setDiyanetId(int32_t id)
    {
        loadLocationIfNeeded();

        if (cachedDiyanetId == id)
            return true;

        if (!preferences.begin(NAMESPACE, false))
        {
            Serial.println("[Settings] ERROR: Failed to open NVS!");
            return false;
        }

        bool success = preferences.putInt(KEY_DIYANET_ID, id) > 0;
        preferences.end();

        if (success)
        {
            cachedDiyanetId = id;
            flagRecalculation = true;
            Serial.printf("[Settings] Diyanet ID saved: %d\n", id);
        }

        return success;
    }
}
