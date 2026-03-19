#include "prayer_engine.h"
#include "config.h"
#include "prayer_types.h"
#include "daily_prayers.h"
#include "current_time.h"
#include "network.h"
#include "prayer_api.h"
#include "prayer_calculator.h"
#include "settings_manager.h"
#include "audio_player.h"
#include "power_manager.h"
#include "app_state.h"
#include <Arduino.h>
#include <climits>
#include <cmath>
#include <optional>

namespace PrayerEngine
{
    static constexpr uint8_t INVALID_WAKE_LOCK = 0xFF;
    static constexpr int CLOCK_JUMP_THRESHOLD_SEC = 3;
    static constexpr int JUMP_CATCHUP_WINDOW_SEC = Config::PRAYER_WAKE_EARLY_SEC;
    static constexpr int SECONDS_PER_DAY = 86400;
    static constexpr int HALF_DAY_SECONDS = SECONDS_PER_DAY / 2;
    static constexpr int PROGRESS_MAX_PERCENT = 100;

    static DailyPrayers s_prayers;
    static std::optional<PrayerType> s_nextPrayer;
    static int s_nextPrayerSeconds = -1;
    static int s_slotStartSeconds = -1; // start of current prayer slot (seconds from midnight)
    static bool s_prayersFetched = false;
    static bool s_showingTomorrow = false;
    static int s_lastDay = -1;

    static bool s_adhanPlaying = false;
    static uint8_t s_adhanWakeLockId = INVALID_WAKE_LOCK;
    static int s_prevNowSeconds = -1;
    static int s_prevSecondsUntil = INT_MAX;

    static int computeSecondsUntil(int nowSeconds)
    {
        if (s_nextPrayerSeconds < 0)
            return -1;

        return s_showingTomorrow
                   ? (SECONDS_PER_DAY - nowSeconds) + s_nextPrayerSeconds
                   : s_nextPrayerSeconds - nowSeconds;
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
            AppStateHelper::setNextPrayer("İMSAK", "Yarın");
            return;
        }

        const PrayerType prayer = *s_nextPrayer;
        const auto &prayerTime = s_prayers[prayer];
        s_nextPrayerSeconds = prayerTime.toSeconds();

        Serial.printf("[Prayer] Next: %s at %s%s\n",
                      getPrayerName(prayer).data(),
                      prayerTime.value.data(),
                      s_showingTomorrow ? " (tomorrow)" : "");

        const char *nextPrayerLabel = (prayer == PrayerType::Fajr)
                          ? "İMSAK"
                                          : getPrayerName(prayer, true).data();
        AppStateHelper::setNextPrayer(nextPrayerLabel, prayerTime.value.data());
        AppStateHelper::setPrayerTimes(
            s_prayers[PrayerType::Fajr].value.data(),
            s_prayers[PrayerType::Sunrise].value.data(),
            s_prayers[PrayerType::Dhuhr].value.data(),
            s_prayers[PrayerType::Asr].value.data(),
            s_prayers[PrayerType::Maghrib].value.data(),
            s_prayers[PrayerType::Isha].value.data(),
            s_showingTomorrow ? static_cast<int>(PrayerType::Fajr) : static_cast<int>(prayer));

        // Track start of current slot for progress calculation
        if (s_showingTomorrow)
        {
            s_slotStartSeconds = -1;
            return;
        }

        const int nextIdx = static_cast<int>(prayer);
        s_slotStartSeconds = (nextIdx > 0)
                                 ? s_prayers[static_cast<PrayerType>(nextIdx - 1)].toSeconds()
                                 : 0; // before Fajr — slot started at midnight
    }

    static void advanceNextPrayerAfterTrigger()
    {
        const auto freshNow = CurrentTime::now();
        auto next = s_prayers.findNext(freshNow._minutes);
        if (!next)
        {
            int method = SettingsManager::getPrayerMethod();
            s_prayersFetched = loadPrayerTimes(method, 1);
        }
        displayNextPrayer();
        s_prevSecondsUntil = INT_MAX;
    }

    static void finishAdhan(bool ok)
    {
        s_adhanPlaying = false;
        if (s_adhanWakeLockId != INVALID_WAKE_LOCK)
        {
            PowerManager::releaseWakeLock(s_adhanWakeLockId);
            s_adhanWakeLockId = INVALID_WAKE_LOCK;
        }
        Serial.printf("[Adhan] %s\n", ok ? "Finished OK" : "PLAYBACK FAILED");
    }

    static void startAdhan(PrayerType currentPrayer)
    {
        if (!s_prayersFetched || s_adhanPlaying)
            return;

        Serial.printf("\n\n🕌 === PRAYER TIME: %s === 🕌\n\n",
                      getPrayerName(currentPrayer).data());

        const auto adhanFile = getAdhanFile(currentPrayer);
        if (adhanFile.empty())
            return;
        bool shouldPlay = SettingsManager::getAdhanEnabled(currentPrayer);

        if (shouldPlay)
        {
            s_adhanWakeLockId = PowerManager::acquireWakeLock("adhan");
            PowerManager::wakeScreen();

            const uint8_t startVolume = g_state.muted ? 0 : g_state.volume;
            Serial.printf("[Adhan] Volume: %d%%%s, File: %s\n",
                          startVolume, g_state.muted ? " (muted)" : "", adhanFile.data());
            setVolume(startVolume);
            setTargetVolume(startVolume);

            s_adhanPlaying = true;
            if (!playAudioFile(adhanFile.data()))
                finishAdhan(false);
        }
        else
        {
            Serial.printf("[Adhan] Skipped for %s (disabled)\n",
                          getPrayerName(currentPrayer).data());
        }
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
        s_prayersFetched = loadPrayerTimes(method, 0);

        if (s_prayersFetched && !s_prayers.findNext(CurrentTime::now()._minutes))
            s_prayersFetched = loadPrayerTimes(method, 1);

        if (s_prayersFetched)
        {
            AppStateHelper::setLocation(SettingsManager::getShortCityName());
            displayNextPrayer();
            // Seed crossing guard so a past prayer does not trigger on first tick
            s_prevSecondsUntil = computeSecondsUntil(CurrentTime::now()._seconds);
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

        if (s_adhanPlaying)
        {
            setTargetVolume(g_state.muted ? 0 : g_state.volume);
            if (isAudioFinished())
                finishAdhan(true);
        }

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
                s_prevSecondsUntil = INT_MAX;
            }
            s_lastDay = timeinfo.tm_mday;
        }

        // Cache refresh — also push to UI so display stays current
        if (s_nextPrayerSeconds == -1)
        {
            displayNextPrayer();
            s_prevSecondsUntil = INT_MAX;
        }

        if (s_prevNowSeconds >= 0)
        {
            int delta = now._seconds - s_prevNowSeconds;
            if (delta < -HALF_DAY_SECONDS)
                delta += SECONDS_PER_DAY;
            else if (delta > HALF_DAY_SECONDS)
                delta -= SECONDS_PER_DAY;

            if (delta < -CLOCK_JUMP_THRESHOLD_SEC || delta > CLOCK_JUMP_THRESHOLD_SEC)
            {
                const int jumpSecondsUntil = computeSecondsUntil(now._seconds);
                const bool jumpCatchup = !s_adhanPlaying &&
                                         s_nextPrayer.has_value() &&
                                         s_prevSecondsUntil > 0 &&
                                         s_prevSecondsUntil <= JUMP_CATCHUP_WINDOW_SEC &&
                                         jumpSecondsUntil <= 0 &&
                                         jumpSecondsUntil >= -JUMP_CATCHUP_WINDOW_SEC;

                Serial.printf("[Prayer] Clock jump detected (%+d s) — revalidating\n", delta);

                if (jumpCatchup)
                {
                    const PrayerType triggeredPrayer = *s_nextPrayer;
                    advanceNextPrayerAfterTrigger();
                    startAdhan(triggeredPrayer);
                    s_prevNowSeconds = now._seconds;
                    return;
                }

                displayNextPrayer();
                s_prevSecondsUntil = INT_MAX;
            }
        }
        s_prevNowSeconds = now._seconds;

        // Adhan check
        if (s_nextPrayerSeconds > 0)
        {
            const int secondsUntil = computeSecondsUntil(now._seconds);

            if (!s_adhanPlaying && s_nextPrayer.has_value() && s_prevSecondsUntil > 0 && secondsUntil <= 0)
            {
                AppStateHelper::setCountdown(0);

                // Remember triggered prayer, then advance BEFORE adhan
                // so hero/iftar update during playback
                PrayerType triggeredPrayer = *s_nextPrayer;

                advanceNextPrayerAfterTrigger();
                startAdhan(triggeredPrayer);
                return;
            }

            s_prevSecondsUntil = secondsUntil;

            AppStateHelper::setCountdown(secondsUntil > 0 ? static_cast<uint32_t>(secondsUntil) : 0);

            if (s_slotStartSeconds < 0)
                return;

            const int slotDuration = s_nextPrayerSeconds - s_slotStartSeconds;
            if (slotDuration <= 0)
                return;

            int pct = ((now._seconds - s_slotStartSeconds) * 100) / slotDuration;
            if (pct < 0)
                pct = 0;
            if (pct > PROGRESS_MAX_PERCENT)
                pct = PROGRESS_MAX_PERCENT;
            AppStateHelper::setProgress(static_cast<uint8_t>(pct));
        }
    }

    void recalculate()
    {
        int method = SettingsManager::getPrayerMethod();
        s_prayersFetched = loadPrayerTimes(method, 0);

        if (s_prayersFetched && !s_prayers.findNext(CurrentTime::now()._minutes))
            s_prayersFetched = loadPrayerTimes(method, 1);

        if (s_prayersFetched)
        {
            s_nextPrayer = std::nullopt;
            s_nextPrayerSeconds = -1;
            s_prevSecondsUntil = INT_MAX;
            AppStateHelper::setLocation(SettingsManager::getShortCityName());
            displayNextPrayer();
        }
    }

    bool isReady()
    {
        return s_prayersFetched;
    }

    int getSecondsUntilNextPrayer()
    {
        return computeSecondsUntil(CurrentTime::now()._seconds);
    }

    bool isAdhanPlaying()
    {
        return s_adhanPlaying;
    }

} // namespace PrayerEngine
