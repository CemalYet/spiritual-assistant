#include "prayer_engine.h"
#include "config.h"
#include "prayer_types.h"
#include "daily_prayers.h"
#include "current_time.h"
#include "network.h"
#include "prayer_api.h"
#include "prayer_calculator.h"
#include "settings_manager.h"
#include "settings_server.h"
#include "audio_player.h"
#include "lvgl_display.h"
#include "app_state.h"
#include <Arduino.h>
#include <cmath>
#include <optional>

namespace PrayerEngine
{
    static DailyPrayers s_prayers;
    static std::optional<PrayerType> s_nextPrayer;
    static int s_nextPrayerSeconds = -1;
    static bool s_prayersFetched = false;
    static bool s_showingTomorrow = false;
    static int s_lastDay = -1;

    static uint8_t s_currentVolume = 0;
    static int s_lastAdhanMinute = -1;

    static int getDayOffset()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
            return 0;

        const int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        int method = SettingsManager::getPrayerMethod();

        DailyPrayers tempPrayers;
        bool loaded = false;

        if (method == PRAYER_METHOD_DIYANET)
        {
            loaded = PrayerAPI::getCachedPrayerTimes(tempPrayers, false);
        }
        else
        {
            double lat = SettingsManager::getLatitude();
            double lng = SettingsManager::getLongitude();
            if (!std::isnan(lat) && !std::isnan(lng))
                loaded = PrayerCalculator::calculateTimes(tempPrayers, method, lat, lng, 0, false);
        }

        if (loaded)
        {
            const int ishaMinutes = tempPrayers[PrayerType::Isha].toMinutes();
            if (currentMinutes > ishaMinutes)
                return 1;
        }

        return 0;
    }

    static bool loadPrayerTimes(int method, int dayOffset = 0)
    {
        const bool wantDiyanet = (method == PRAYER_METHOD_DIYANET);
        const bool fetchTomorrow = (dayOffset > 0);
        s_showingTomorrow = fetchTomorrow;

        if (wantDiyanet)
        {
            if (PrayerAPI::getCachedPrayerTimes(s_prayers, fetchTomorrow))
                return true;

            if (Network::isConnected())
            {
                Serial.println("[Prayer] Cache miss, fetching from API...");
                if (PrayerAPI::fetchMonthlyPrayerTimes())
                {
                    if (PrayerAPI::getCachedPrayerTimes(s_prayers, fetchTomorrow))
                        return true;
                }
            }
            Serial.println("[Fallback] Diyanet unavailable, using Adhan calculation");
        }

        double lat = SettingsManager::getLatitude();
        double lng = SettingsManager::getLongitude();

        if (std::isnan(lat) || std::isnan(lng))
        {
            Serial.println("[Prayer] ERROR: Location not configured!");
            return false;
        }

        return PrayerCalculator::calculateTimes(s_prayers, method, lat, lng, dayOffset);
    }

    static void displayNextPrayer()
    {
        if (!s_prayersFetched)
            return;

        if (s_showingTomorrow)
            s_nextPrayer = PrayerType::Fajr;
        else
            s_nextPrayer = s_prayers.findNext(CurrentTime::now()._minutes);

        if (!s_nextPrayer)
        {
            s_nextPrayerSeconds = -1;
            AppStateHelper::setNextPrayer("SABAH", "Yarin");
            return;
        }

        const PrayerType prayer = *s_nextPrayer;
        const auto &prayerTime = s_prayers[prayer];
        s_nextPrayerSeconds = prayerTime.toSeconds();

        Serial.printf("[Prayer] Next: %s at %s%s\n",
                      getPrayerName(prayer).data(),
                      prayerTime.value.data(),
                      s_showingTomorrow ? " (tomorrow)" : "");

        AppStateHelper::setNextPrayer(
            getPrayerName(prayer, true).data(),
            prayerTime.value.data());
        AppStateHelper::setPrayerTimes(
            s_prayers[PrayerType::Fajr].value.data(),
            s_prayers[PrayerType::Sunrise].value.data(),
            s_prayers[PrayerType::Dhuhr].value.data(),
            s_prayers[PrayerType::Asr].value.data(),
            s_prayers[PrayerType::Maghrib].value.data(),
            s_prayers[PrayerType::Isha].value.data(),
            s_showingTomorrow ? -1 : static_cast<int>(prayer));
    }

    static void onAdhanLoop()
    {
        LvglDisplay::loop();
        SettingsServer::handle();

        const auto now = CurrentTime::now();
        if (now._minutes != s_lastAdhanMinute)
        {
            s_lastAdhanMinute = now._minutes;
            LvglDisplay::updateTime();
        }

        if (g_state.muted)
        {
            if (s_currentVolume != 0)
            {
                s_currentVolume = 0;
                setVolume(0);
            }
            return;
        }

        uint8_t newVolume = SettingsManager::getVolume();
        if (newVolume != s_currentVolume)
        {
            s_currentVolume = newVolume;
            setVolume(SettingsManager::getHardwareVolume());
        }
    }

    static void checkAndPlayAdhan()
    {
        if (!s_prayersFetched || !s_nextPrayer)
            return;

        PrayerType currentPrayer = *s_nextPrayer;
        Serial.printf("\n\n🕌 === PRAYER TIME: %s === 🕌\n\n",
                      getPrayerName(currentPrayer).data());

        const auto adhanFile = getAdhanFile(currentPrayer);
        if (adhanFile.empty())
            return;
        bool shouldPlay = SettingsManager::getAdhanEnabled(currentPrayer) && !g_state.muted;

        if (shouldPlay)
        {
            s_currentVolume = SettingsManager::getVolume();
            setVolume(SettingsManager::getHardwareVolume());

            Serial.printf("[Adhan] Playing %s\n", adhanFile.data());
            playAudioFileBlocking(adhanFile.data(), onAdhanLoop);
            Serial.println("[Adhan] Finished");
        }
        else
        {
            Serial.printf("[Adhan] Skipped for %s (%s)\n",
                          getPrayerName(currentPrayer).data(),
                          g_state.muted ? "muted" : "disabled");
        }

        // Load tomorrow if last prayer passed
        const auto now = CurrentTime::now();
        auto next = s_prayers.findNext(now._minutes);

        if (!next)
        {
            Serial.println("[Prayer] Last prayer — loading tomorrow");
            int method = SettingsManager::getPrayerMethod();
            s_prayersFetched = loadPrayerTimes(method, 1);
        }

        displayNextPrayer();
    }

    bool init()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("[Prayer] No clock — cannot calculate times");
            AppStateHelper::setNextPrayer("BEKLE", "--:--");
            return false;
        }

        int method = SettingsManager::getPrayerMethod();
        s_prayersFetched = loadPrayerTimes(method, getDayOffset());

        if (s_prayersFetched)
        {
            AppStateHelper::setLocation(SettingsManager::getShortCityName());
            displayNextPrayer();
            return true;
        }

        Serial.println("[Prayer] Failed to load prayer times");
        AppStateHelper::showError("Hata", "Namaz vakitleri yuklenemedi");
        return false;
    }

    void tick()
    {
        if (!s_prayersFetched)
            return;

        const auto now = CurrentTime::now();

        if (SettingsManager::needsRecalculation())
        {
            SettingsManager::clearRecalculationFlag();
            recalculate();
            return;
        }

        // Day transition
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            if (s_lastDay != -1 && s_lastDay != timeinfo.tm_mday)
            {
                Serial.println("[Prayer] New day — reloading");
                int method = SettingsManager::getPrayerMethod();
                s_prayersFetched = loadPrayerTimes(method, 0);
                displayNextPrayer();
            }
            s_lastDay = timeinfo.tm_mday;
        }

        // Cache refresh — also push to UI so display stays current
        if (s_nextPrayerSeconds == -1)
        {
            displayNextPrayer();
        }

        // Adhan check
        if (s_nextPrayerSeconds > 0)
        {
            if (s_showingTomorrow && now._seconds > s_nextPrayerSeconds)
                return;

            const int secondsUntil = s_nextPrayerSeconds - now._seconds;
            if (secondsUntil <= 0)
            {
                checkAndPlayAdhan();
                s_nextPrayer = std::nullopt;
                s_nextPrayerSeconds = -1;
            }
        }
    }

    void recalculate()
    {
        int method = SettingsManager::getPrayerMethod();
        s_prayersFetched = loadPrayerTimes(method, getDayOffset());

        if (s_prayersFetched)
        {
            s_nextPrayer = std::nullopt;
            s_nextPrayerSeconds = -1;
            AppStateHelper::setLocation(SettingsManager::getShortCityName());
            displayNextPrayer();
        }
    }

    bool isReady()
    {
        return s_prayersFetched;
    }

} // namespace PrayerEngine
