#include "prayer_api.h"
#include "config.h"
#include "current_time.h"
#include "diyanet_parser.h"
#include "settings_manager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>

namespace
{
    constexpr uint32_t CACHE_VALID_DAYS = 25; // Refresh before 30 days expire
    constexpr size_t HTTP_TIMEOUT_MS = 8000;

    struct DiyanetCache
    {
        int ilceId;
        time_t fetchedAt; // Unix timestamp of first day
        uint8_t totalDays;
        DailyPrayers days[30];
    };

    static DiyanetCache s_cache = {};
    static Preferences s_prefs;
    static bool s_nvsInitialized = false;

    static bool loadCache()
    {
        if (!s_prefs.begin("prayers", true)) // read-only
        {
            if (!s_nvsInitialized)
            {
                Serial.println("[Cache] First boot - initializing NVS");
                s_nvsInitialized = true;
            }
            return false;
        }

        size_t len = s_prefs.getBytesLength("diyanet");
        if (len != sizeof(DiyanetCache))
        {
            s_prefs.end();
            Serial.println("[Cache] No valid cache found");
            return false;
        }

        s_prefs.getBytes("diyanet", &s_cache, sizeof(s_cache));
        s_prefs.end();

        Serial.printf("[Cache] Loaded: ilceId=%d, days=%d, fetchedAt=%ld\n",
                      s_cache.ilceId, s_cache.totalDays, s_cache.fetchedAt);

#if DEBUG_CACHE_LOGS
        // Show first day details
        if (s_cache.totalDays > 0)
        {
            struct tm t = {};
            t.tm_year = 70;
            t.tm_mon = 0;
            t.tm_mday = 1;
            time_t temp = s_cache.fetchedAt;
            gmtime_r(&temp, &t);

            Serial.printf("[Cache] First day: %04d-%02d-%02d\n",
                          t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
            Serial.printf("[Cache]   Fajr: %s, Dhuhr: %s, Asr: %s\n",
                          s_cache.days[0][PrayerType::Fajr].value.data(),
                          s_cache.days[0][PrayerType::Dhuhr].value.data(),
                          s_cache.days[0][PrayerType::Asr].value.data());
            Serial.printf("[Cache]   Maghrib: %s, Isha: %s\n",
                          s_cache.days[0][PrayerType::Maghrib].value.data(),
                          s_cache.days[0][PrayerType::Isha].value.data());

            if (s_cache.totalDays > 1)
            {
                Serial.printf("[Cache] Last day stored: day %d/%d\n",
                              s_cache.totalDays, s_cache.totalDays);
            }
        }
#endif

        return true;
    }

    static void saveCache()
    {
        if (!s_prefs.begin("prayers", false)) // read-write
        {
            Serial.println("[Cache] Failed to open NVS for write");
            return;
        }

        size_t written = s_prefs.putBytes("diyanet", &s_cache, sizeof(s_cache));
        s_prefs.end();

        if (written != sizeof(s_cache))
        {
            Serial.println("[Cache] ERROR: NVS write failed or incomplete");
            return;
        }

#if DEBUG_CACHE_LOGS
        struct tm t = {};
        time_t temp = s_cache.fetchedAt;
        gmtime_r(&temp, &t);

        Serial.printf("[Cache] Saved: %d days starting from %04d-%02d-%02d\n",
                      s_cache.totalDays, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        Serial.printf("[Cache] First day Fajr: %s, Last day: day %d\n",
                      s_cache.days[0][PrayerType::Fajr].value.data(),
                      s_cache.totalDays);
#else
        Serial.printf("[Cache] Saved: %d days\n", s_cache.totalDays);
#endif
    }

    static bool isCacheValid(int ilceId)
    {
        if (s_cache.ilceId != ilceId || s_cache.totalDays == 0)
            return false;

        const time_t now = time(nullptr);
        if (DiyanetParser::isCacheExpired(s_cache.fetchedAt, now, CACHE_VALID_DAYS))
        {
            Serial.println("[Cache] Expired");
            return false;
        }

        return true;
    }
}

bool PrayerAPI::fetchMonthlyPrayerTimes(int ilceId)
{
    if (ilceId <= 0)
        ilceId = SettingsManager::getDiyanetId();

    // Still no valid ID? Can't fetch
    if (ilceId <= 0)
    {
        Serial.println("[Diyanet] ERROR: No diyanetId configured!");
        return false;
    }

    // Check cache first
    if (s_cache.totalDays == 0)
        loadCache();

    if (isCacheValid(ilceId))
    {
        Serial.println("[Diyanet] Using cached prayer times");
        return true;
    }

    Serial.printf("[Diyanet] Fetching 30 days for ilceId=%d\n", ilceId);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = String(Config::DIYANET_API_BASE.data()) + "/vakitler/" + String(ilceId);
    http.begin(client, url);
    http.useHTTP10(true);
    http.setTimeout(HTTP_TIMEOUT_MS);

    const int httpCode = http.GET();
    if (httpCode != 200)
    {
        Serial.printf("[Diyanet] HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }

    // Filter only needed fields
    JsonDocument filter;
    filter[0]["MiladiTarihKisaIso8601"] = true;
    filter[0]["Imsak"] = true;
    filter[0]["Gunes"] = true;
    filter[0]["Ogle"] = true;
    filter[0]["Ikindi"] = true;
    filter[0]["Aksam"] = true;
    filter[0]["Yatsi"] = true;

    JsonDocument doc;
    const auto err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();

    if (err)
    {
        Serial.printf("[Diyanet] JSON error: %s\n", err.c_str());
        return false;
    }

    JsonArray timesArray = doc.as<JsonArray>();
    if (timesArray.size() == 0)
    {
        Serial.println("[Diyanet] Empty response");
        return false;
    }

    // Parse and store
    memset(&s_cache, 0, sizeof(s_cache));
    s_cache.ilceId = ilceId;
    s_cache.totalDays = 0;

    for (JsonObject day : timesArray)
    {
        if (s_cache.totalDays >= 30)
            break;

        const char *dateStr = day["MiladiTarihKisaIso8601"]; // "31.12.2025"

        // Parse DD.MM.YYYY -> Unix timestamp
        int d, m, y;
        if (!DiyanetParser::parseDate(dateStr, d, m, y))
            continue;

        struct tm t = {};
        t.tm_year = y - 1900;
        t.tm_mon = m - 1;
        t.tm_mday = d;
        t.tm_hour = 0;
        t.tm_min = 0;
        t.tm_sec = 0;
        time_t dayTimestamp = mktime(&t);

        if (s_cache.totalDays == 0)
            s_cache.fetchedAt = dayTimestamp;

        DailyPrayers &prayers = s_cache.days[s_cache.totalDays];

        DiyanetParser::parseTime(day["Imsak"], prayers[PrayerType::Fajr]);
        DiyanetParser::parseTime(day["Gunes"], prayers[PrayerType::Sunrise]);
        DiyanetParser::parseTime(day["Ogle"], prayers[PrayerType::Dhuhr]);
        DiyanetParser::parseTime(day["Ikindi"], prayers[PrayerType::Asr]);
        DiyanetParser::parseTime(day["Aksam"], prayers[PrayerType::Maghrib]);
        DiyanetParser::parseTime(day["Yatsi"], prayers[PrayerType::Isha]);

        s_cache.totalDays++;
    }

    if (s_cache.totalDays == 0)
    {
        Serial.println("[Diyanet] No valid times parsed");
        return false;
    }

    saveCache();

    Serial.printf("[Diyanet] Cached %d days\n", s_cache.totalDays);
    return true;
}

bool PrayerAPI::getCachedPrayerTimes(DailyPrayers &prayers, bool forTomorrow)
{
    if (s_cache.totalDays == 0)
    {
        if (!loadCache())
            return false;
    }

    int32_t currentIlceId = SettingsManager::getDiyanetId();
    if (s_cache.ilceId != currentIlceId)
    {
        return false; // Location changed, need fresh data
    }

    int dayOffset = 0; // Default: today (first day in cache)

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        // RTC available - calculate exact day offset
        if (forTomorrow)
        {
            timeinfo.tm_mday += 1;
            mktime(&timeinfo);
        }

        timeinfo.tm_hour = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
        time_t targetDate = mktime(&timeinfo);

        dayOffset = DiyanetParser::calculateDayOffset(s_cache.fetchedAt, targetDate);
    }
    else
    {
        // RTC not available - use default (today = 0, tomorrow = 1)
        dayOffset = forTomorrow ? 1 : 0;
        Serial.println("[Cache] RTC not synced, using relative offset");
    }

    if (!DiyanetParser::isDayOffsetValid(dayOffset, s_cache.totalDays))
    {
        Serial.printf("[Cache] Date out of range: offset=%d, total=%d\n",
                      dayOffset, s_cache.totalDays);
        return false;
    }

    prayers = s_cache.days[dayOffset];

    Serial.printf("[Cache] Retrieved day %d/%d\n", dayOffset + 1, s_cache.totalDays);
    return true;
}

PrayerAPI::CacheInfo PrayerAPI::getCacheInfo()
{
    CacheInfo info = {0, 0, false};

    // Load cache if not in memory
    if (s_cache.totalDays == 0)
        loadCache();

    if (s_cache.totalDays == 0)
        return info;

    info.ilceId = s_cache.ilceId;

    // Calculate days remaining from today
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        timeinfo.tm_hour = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
        time_t today = mktime(&timeinfo);

        int dayOffset = DiyanetParser::calculateDayOffset(s_cache.fetchedAt, today);
        info.daysRemaining = s_cache.totalDays - dayOffset;

        if (info.daysRemaining < 0)
            info.daysRemaining = 0;
    }
    else
    {
        info.daysRemaining = s_cache.totalDays;
    }

    // Check if ilceId matches current settings
    int32_t currentIlceId = SettingsManager::getDiyanetId();
    info.isValid = (s_cache.ilceId == currentIlceId) && (info.daysRemaining > 0);

    return info;
}
