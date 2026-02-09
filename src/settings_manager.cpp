#include "settings_manager.h"
#include "config.h"
#include <Preferences.h>
#include <Arduino.h>
#include <etl/string.h>

namespace SettingsManager
{
    static Preferences preferences;
    constexpr const char *NAMESPACE = "settings";

    // RAII guard for Preferences - ensures proper cleanup (DRY)
    class PreferencesGuard
    {
    public:
        PreferencesGuard(bool readOnly = true)
            : opened(preferences.begin(NAMESPACE, readOnly)) {}
        ~PreferencesGuard()
        {
            if (opened)
                preferences.end();
        }
        explicit operator bool() const { return opened; }
        bool isOpen() const { return opened; }

    private:
        bool opened;
    };

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
    constexpr const char *KEY_CONNECTION_MODE = "connMode";

    // Cached values to avoid frequent NVS reads
    static int cachedPrayerMethod = -1;
    static int8_t cachedVolume = -1;
    static int8_t cachedAdhanEnabled[6] = {-1, -1, -1, -1, -1, -1}; // -1 = not loaded

    // ETL strings for safe string handling (no buffer overflows)
    static etl::string<16> cachedConnectionMode;
    static etl::string<96> cachedCityName; // "City, State, Country" format
    static etl::string<32> shortCityBuffer;

    // Location cache
    static double cachedLatitude = NAN;
    static double cachedLongitude = NAN;
    static int32_t cachedDiyanetId = -1;
    static bool locationLoaded = false;

    // Action flags
    static volatile bool flagRecalculation = false;
    static volatile bool flagWiFiReconnect = false;

    // Available calculation methods
    static const MethodInfo methods[] = {
        {1, "Karachi", "Karachi"},
        {2, "ISNA (North America)", "ISNA"},
        {3, "MWL (Muslim World League)", "MWL"},
        {4, "Umm al-Qura (Makkah)", "Makkah"},
        {5, "Egyptian", "Egyptian"},
        {6, "Gulf", "Gulf"},
        {7, "Tehran", "Tehran"},
        {8, "Dubai", "Dubai"},
        {9, "Kuwait", "Kuwait"},
        {10, "Qatar", "Qatar"},
        {11, "Singapore", "Singapore"},
        {12, "France (UOIF)", "UOIF"},
        {13, "Turkey (Diyanet)", "Diyanet"},
        {14, "Russia", "Russia"},
        {15, "Moonsighting Committee", "Moonsight"},
        {0, nullptr, nullptr} // Terminator
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

        // Load connection mode (heap-free)
        char modeBuffer[16];
        size_t len = preferences.getString(KEY_CONNECTION_MODE, modeBuffer, sizeof(modeBuffer));
        cachedConnectionMode.assign(len > 0 ? modeBuffer : "wifi");

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
        if (cachedPrayerMethod >= 0)
            return cachedPrayerMethod;

        // Not initialized, read from NVS
        PreferencesGuard guard(true);
        cachedPrayerMethod = preferences.getInt(KEY_PRAYER_METHOD, Config::PRAYER_METHOD);
        return cachedPrayerMethod;
    }

    static constexpr int MIN_METHOD_ID = 1;
    static constexpr int MAX_METHOD_ID = 15;

    bool setPrayerMethod(int method)
    {
        // Early return: validate method ID
        if (method < MIN_METHOD_ID || method > MAX_METHOD_ID)
        {
            Serial.printf("[Settings] Invalid prayer method: %d\n", method);
            return false;
        }

        // Early return: skip if unchanged
        if (method == cachedPrayerMethod)
            return true;

        PreferencesGuard guard(false);
        if (!guard)
        {
            Serial.println("[Settings] ERROR: Failed to open NVS namespace!");
            return false;
        }

        bool success = preferences.putInt(KEY_PRAYER_METHOD, method) > 0;
        if (!success)
            return false;

        cachedPrayerMethod = method;
        flagRecalculation = true;
        Serial.printf("[Settings] Prayer method saved: %d (%s)\n",
                      method, getMethodName(method));
        return true;
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

    const char *getMethodShortName(int method)
    {
        for (const auto &m : methods)
        {
            if (m.id == method)
            {
                return m.shortName;
            }
        }
        return "?";
    }

    const MethodInfo *getAvailableMethods()
    {
        return methods;
    }

    int getMethodCount()
    {
        return 15; // Methods 1-15
    }

    // --- Connection Mode ---
    static constexpr const char *MODE_WIFI = "wifi";
    static constexpr const char *MODE_OFFLINE = "offline";

    const char *getConnectionMode()
    {
        if (!cachedConnectionMode.empty())
            return cachedConnectionMode.c_str();

        PreferencesGuard guard(true);
        // Use stack buffer to avoid Arduino String heap allocation
        char buffer[16];
        size_t len = preferences.getString(KEY_CONNECTION_MODE, buffer, sizeof(buffer));
        cachedConnectionMode.assign(len > 0 ? buffer : MODE_WIFI);
        return cachedConnectionMode.c_str();
    }

    static bool isValidConnectionMode(const char *mode)
    {
        if (!mode)
            return false;
        return cachedConnectionMode.compare(MODE_WIFI) == 0 ||
               etl::string_view(mode) == MODE_OFFLINE ||
               etl::string_view(mode) == MODE_WIFI;
    }

    bool setConnectionMode(const char *mode)
    {
        if (!mode)
        {
            Serial.println("[Settings] Invalid connection mode: null");
            return false;
        }

        etl::string_view modeView(mode);
        if (modeView != MODE_WIFI && modeView != MODE_OFFLINE)
        {
            Serial.printf("[Settings] Invalid connection mode: %s\n", mode);
            return false;
        }

        if (cachedConnectionMode == modeView)
            return true;

        PreferencesGuard guard(false);
        if (!guard)
        {
            Serial.println("[Settings] ERROR: Failed to open NVS!");
            return false;
        }

        if (preferences.putString(KEY_CONNECTION_MODE, mode) == 0)
            return false;

        cachedConnectionMode.assign(mode);
        Serial.printf("[Settings] Connection mode set: %s\n", mode);
        return true;
    }

    bool isOfflineMode()
    {
        getConnectionMode(); // Ensure loaded
        return cachedConnectionMode == MODE_OFFLINE;
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

        // Read city name into ETL string
        char buffer[96];
        size_t len = preferences.getString(KEY_CITY_NAME, buffer, sizeof(buffer));
        cachedCityName.assign(len > 0 ? buffer : "");

        preferences.end();
        locationLoaded = true;

        Serial.printf("[Settings] Location loaded: %.4f, %.4f (%s) DiyanetID=%d\n",
                      cachedLatitude, cachedLongitude,
                      cachedCityName.empty() ? "unnamed" : cachedCityName.c_str(),
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
        return cachedCityName.c_str();
    }

    const char *getShortCityName()
    {
        loadLocationIfNeeded();

        // Find first delimiter: '(' or ','
        auto parenPos = cachedCityName.find('(');
        auto commaPos = cachedCityName.find(',');

        // Use whichever comes first
        size_t delimPos = etl::string<96>::npos;
        if (parenPos != etl::string<96>::npos && commaPos != etl::string<96>::npos)
            delimPos = (parenPos < commaPos) ? parenPos : commaPos;
        else if (parenPos != etl::string<96>::npos)
            delimPos = parenPos;
        else if (commaPos != etl::string<96>::npos)
            delimPos = commaPos;

        if (delimPos != etl::string<96>::npos && delimPos > 0)
        {
            shortCityBuffer.assign(cachedCityName.begin(), cachedCityName.begin() + delimPos);
            // Trim trailing spaces
            while (!shortCityBuffer.empty() && shortCityBuffer.back() == ' ')
                shortCityBuffer.pop_back();
        }
        else
        {
            shortCityBuffer.assign(cachedCityName.c_str());
        }

        return shortCityBuffer.c_str();
    }

    bool setCityName(const char *name)
    {
        cachedCityName.assign(name ? name : "");

        PreferencesGuard guard(false);
        if (!guard)
        {
            Serial.println("[Settings] ERROR: Failed to open NVS!");
            return false;
        }

        if (preferences.putString(KEY_CITY_NAME, cachedCityName.c_str()) == 0)
            return false;

        Serial.printf("[Settings] City name saved: %s\n", cachedCityName.c_str());
        return true;
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

        PreferencesGuard guard(false);
        if (!guard)
        {
            Serial.println("[Settings] ERROR: Failed to open NVS!");
            return false;
        }

        if (preferences.putInt(KEY_DIYANET_ID, id) == 0)
            return false;

        cachedDiyanetId = id;
        flagRecalculation = true;
        Serial.printf("[Settings] Diyanet ID saved: %d\n", id);
        return true;
    }
}
