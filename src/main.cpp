#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "config.h"
#include "prayer_types.h"
#include "daily_prayers.h"
#include "current_time.h"
#include "lcd_display.h"
#include "network.h"
#include "prayer_api.h"
#include "prayer_calculator.h"
#include "test_mode.h"
#include "wifi_credentials.h"
#include "audio_player.h"
#include <time.h>

// TEMPORARY: Set to true to clear stored WiFi credentials for testing portal
#define CLEAR_WIFI_CREDENTIALS false

// --- GLOBAL STATE ---
DailyPrayers currentPrayers;
LCDDisplay display;
bool prayersFetched = true;

// Cached next prayer
std::optional<PrayerType> cachedNextPrayer = std::nullopt;
int cachedNextPrayerSeconds = -1;

// --- MAIN LOGIC ---
void displayNextPrayer()
{
    if (!prayersFetched)
        return;

    const auto now = CurrentTime::now();
    cachedNextPrayer = currentPrayers.findNext(now._minutes);

    if (!cachedNextPrayer)
    {
        Serial.println("[Info] Next prayer: Tomorrow's Fajr");
        cachedNextPrayerSeconds = -1;
        display.update(now, cachedNextPrayer, currentPrayers);
        return;
    }

    // Update cache for both display and loop logic
    cachedNextPrayerSeconds = currentPrayers[*cachedNextPrayer].toSeconds();

    const auto &nextTime = currentPrayers[*cachedNextPrayer];
    Serial.printf("[Info] Next prayer: %s at %s\n",
                  getPrayerName(*cachedNextPrayer).data(),
                  nextTime.value.data());

    display.update(now, cachedNextPrayer, currentPrayers);
}

void checkAndPlayAdhan()
{
    if (!prayersFetched)
        return;

    Serial.printf("\n\n🕌 === ADHAN TIME: %s === 🕌\n\n",
                  getPrayerName(*cachedNextPrayer).data());

    // Play adhan audio and wait for it to finish
    if (playAudioFile("/azan.mp3"))
    {
        Serial.println("[Adhan] Playing...");

        // Wait for playback to complete (callback sets audioFinished)
        while (!isAudioFinished())
        {
            audioPlayerLoop();
            delay(1); // Small delay, audio.loop() does the work
        }
        Serial.println("[Adhan] Finished");
    }

    // Check if this was the last prayer of the day
    const auto now = CurrentTime::now();
    auto nextPrayer = currentPrayers.findNext(now._minutes);

    if (!nextPrayer)
    {
        // Last prayer done - fetch new times
        Serial.println("[Info] Last prayer of day - fetching new times...");
        const bool fetchTomorrow = (now._minutes >= 720); // 12:00 PM
        const int dayOffset = fetchTomorrow ? 1 : 0;

        if (Config::PRAYER_METHOD == 13)
        {
            prayersFetched = PrayerAPI::getCachedPrayerTimes(currentPrayers, fetchTomorrow);
        }

        // Fallback to Adhan if cache fails
        if (!prayersFetched)
        {
            Serial.println("[Fallback] Cache miss, using Adhan calculation");
            prayersFetched = PrayerCalculator::calculateTimes(currentPrayers, Config::PRAYER_METHOD,
                                                              Config::LATITUDE, Config::LONGITUDE, dayOffset);
        }
    }

    displayNextPrayer();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n" + String('=', 40));
    Serial.println("  ESP32-S3 SPIRITUAL ASSISTANT v2.0");
    Serial.println(String('=', 40));

    Serial.printf("Flash: %d MB | PSRAM: %s (%d MB)\n",
                  ESP.getFlashChipSize() / (1024 * 1024),
                  psramInit() ? "ACTIVE" : "NO",
                  psramInit() ? ESP.getPsramSize() / (1024 * 1024) : 0);

    Serial.println(String('=', 40) + "\n");

    display.init();

    // Initialize LittleFS for audio files (portal may have unmounted it)
    if (!LittleFS.begin(true))
    {
        Serial.println("[Error] LittleFS mount failed!");
    }

    // Initialize audio player
    audioPlayerInit();

    // Verify audio file exists
    if (!LittleFS.exists("/azan.mp3"))
    {
        Serial.println("[Audio] WARNING: azan.mp3 not found!");
    }
    else
    {
        Serial.println("[Audio] azan.mp3 found - ready for adhan");
    }

#if CLEAR_WIFI_CREDENTIALS
    // TEMPORARY: Clear WiFi credentials to test portal
    Serial.println("[DEBUG] Clearing stored WiFi credentials...");
    WiFiCredentials::clear();
    Serial.println("[DEBUG] Credentials cleared - portal will open");
#endif

    // Initialize network subsystem (loads credentials or starts portal)
    Network::init();

    // Connect once for time sync
    const bool wifiOk = Network::connectWiFi();

    // If portal mode, handle it in main loop (don't proceed with normal setup)
    if (!Network::isConnected())
    {
        Serial.println("[Setup] Portal mode - waiting for configuration");
        display.showMessage("WiFi Setup", "Connect to AP");
        return; // Continue to loop for portal handling
    }

    // Only sync time if actually connected to WiFi
    if (wifiOk && Network::isConnected())
    {
        Network::syncTime();
    }

#if TEST_MODE
    // Run tests and exit
    TestMode::runPrayerTimeTests();
    return; // Tests will restart device
#endif

    // Check if RTC is synced - without it, we can't do anything
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("\n[ERROR] ═════════════════════════════════");
        Serial.println("[ERROR] RTC NOT SYNCED - Cannot proceed");
        Serial.println("[ERROR] Please ensure:");
        Serial.println("[ERROR]   1. WiFi credentials are correct");
        Serial.println("[ERROR]   2. NTP server is reachable");
        Serial.println("[ERROR]   3. Or use external RTC module");
        Serial.println("[ERROR] ═════════════════════════════════\n");
        Serial.println("[System] Waiting for time sync...\n");

        // Display error message on screen
        display.showError("PLEASE RESET", "YOUR DEVICE");

        return; // Don't proceed without time
    }

    Serial.printf("[RTC] System time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // Check if we need tomorrow's times (after Isha)
    DailyPrayers tempPrayers;
    bool needTomorrow = false;

    constexpr bool wantDiyanet = (Config::PRAYER_METHOD == 13);

    if constexpr (wantDiyanet)
    {
        // Load today first to check if Isha passed
        if (PrayerAPI::getCachedPrayerTimes(tempPrayers, false))
        {
            const int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
            const auto ishaTime = tempPrayers[PrayerType::Isha].toMinutes();
            if (currentMinutes > ishaTime)
            {
                needTomorrow = true;
                Serial.printf("[Setup] After Isha (%d > %d), loading tomorrow\n",
                              currentMinutes, ishaTime);
            }
        }

        // Now load the correct day
        prayersFetched = PrayerAPI::getCachedPrayerTimes(currentPrayers, needTomorrow);

        // If cache miss/expired, try to fetch fresh data
        if (!prayersFetched && wifiOk)
        {
            if (PrayerAPI::fetchMonthlyPrayerTimes())
            {
                prayersFetched = PrayerAPI::getCachedPrayerTimes(currentPrayers, needTomorrow);
            }
        }
    }

    // Disconnect WiFi after boot work
    if (wifiOk)
    {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        Serial.println("[WiFi] Disconnected to save power");
    }

    // If Diyanet cache/fetch failed, fallback to offline Adhan calculation
    if (!prayersFetched)
    {
        Serial.println("[Fallback] Using offline Adhan calculation");
        const int dayOffset = needTomorrow ? 1 : 0;
        prayersFetched = PrayerCalculator::calculateTimes(currentPrayers, Config::PRAYER_METHOD,
                                                          Config::LATITUDE, Config::LONGITUDE, dayOffset);
    }

    if (prayersFetched)
    {
        displayNextPrayer();
    }
    else
    {
        Serial.println("[Error] Failed to load prayer times");
    }

    Serial.println("\n[System] Ready!\n");
}

void loop()
{
    // Keep audio playing
    audioPlayerLoop();

    // Handle WiFi portal if active
    if (Network::isPortalActive())
    {
        Network::handlePortal();
        delay(10); // Small yield for stability
        return;    // Skip the rest of the loop to keep portal responsive
    }
    const auto currentTime = CurrentTime::now();

    // Update display every loop
    // LCDDisplay::update() internally checks if the minute changed, so this is safe and efficient
    display.update(currentTime, cachedNextPrayer, currentPrayers);

    if (!prayersFetched)
    {
        delay(5000);
        return;
    }

    if (cachedNextPrayerSeconds == -1)
    {
        cachedNextPrayer = currentPrayers.findNext(currentTime._minutes);
        if (cachedNextPrayer)
        {
            cachedNextPrayerSeconds = currentPrayers[*cachedNextPrayer].toSeconds();
            Serial.printf("[Cache] Next prayer: %s at %d seconds\n",
                          getPrayerName(*cachedNextPrayer).data(), cachedNextPrayerSeconds);
        }
    }

    // Calculate seconds until the next minute starts for aligned sleeping
    const int secondsToNextMinute = 60 - (currentTime._seconds % 60);

    if (cachedNextPrayerSeconds <= 0)
    {
        // No pending prayer, sleep until next minute to update clock
        delay(secondsToNextMinute * 1000);
        return;
    }

    const int secondsUntil = cachedNextPrayerSeconds - currentTime._seconds;

    if (secondsUntil <= 0)
    {
        checkAndPlayAdhan();
        cachedNextPrayer = std::nullopt;
        cachedNextPrayerSeconds = -1;
        delay(1000);
        return;
    }

    // Sleep until the next minute starts, OR until the prayer time, whichever is sooner.
    int sleepTime = secondsToNextMinute;
    if (secondsUntil < sleepTime)
    {
        sleepTime = secondsUntil;
    }

    // Ensure we sleep at least 1 second
    if (sleepTime < 1)
        sleepTime = 1;

    Serial.printf("[Sleep] Time: %s, Sleeping %ds (Next Minute: %ds, Prayer: %ds)\n",
                  currentTime._hhMM.data(), sleepTime, secondsToNextMinute, secondsUntil);

    delay(sleepTime * 1000);
}
