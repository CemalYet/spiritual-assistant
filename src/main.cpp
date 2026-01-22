#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <algorithm>
#include "config.h"
#include "prayer_types.h"
#include "daily_prayers.h"
#include "current_time.h"
#include "lcd_display.h"
#include "network.h"
#include "settings_server.h"
#include "prayer_api.h"
#include "prayer_calculator.h"
#include "test_mode.h"
#include "wifi_credentials.h"
#include "audio_player.h"
#include "settings_manager.h"
#include <time.h>

// TEMPORARY: Set to true to clear stored WiFi credentials for testing portal
#define CLEAR_WIFI_CREDENTIALS false

// --- APPLICATION STATE ---
struct AppState
{
    DailyPrayers prayers;                 // Current day's prayer times
    std::optional<PrayerType> nextPrayer; // Next upcoming prayer (nullopt if none today)
    int nextPrayerSeconds = -1;           // Seconds since midnight for next prayer
    bool prayersFetched = false;          // Whether prayer times are loaded
    bool setupComplete = false;           // Whether setup finished successfully
};

AppState app;       // Global application state
LCDDisplay display; // LCD display (separate - has its own state)

// --- PRAYER LOADING (Single source of truth) ---
// Loads prayer times based on method. Handles Diyanet cache/API and Adhan fallback.
// Returns true if prayer times were loaded successfully.
bool loadPrayerTimes(int method, int dayOffset = 0)
{
    const bool wantDiyanet = (method == PRAYER_METHOD_DIYANET);
    const bool fetchTomorrow = (dayOffset > 0);

    // For Diyanet, try cache first
    if (wantDiyanet)
    {
        if (PrayerAPI::getCachedPrayerTimes(app.prayers, fetchTomorrow))
        {
            return true;
        }

        // Cache miss - try API fetch if connected
        if (Network::isConnected())
        {
            Serial.println("[Prayer] Cache miss, fetching from API...");
            if (PrayerAPI::fetchMonthlyPrayerTimes())
            {
                if (PrayerAPI::getCachedPrayerTimes(app.prayers, fetchTomorrow))
                {
                    return true;
                }
            }
        }

        // Diyanet failed - fall through to Adhan
        Serial.println("[Fallback] Diyanet unavailable, using Adhan calculation");
    }

    // Use Adhan library calculation (logging happens inside calculateTimes)
    double lat = SettingsManager::getLatitude();
    double lng = SettingsManager::getLongitude();

    if (std::isnan(lat) || std::isnan(lng))
    {
        Serial.println("[Prayer] ERROR: Location not configured!");
        return false;
    }

    return PrayerCalculator::calculateTimes(app.prayers, method, lat, lng, dayOffset);
}

// Checks if we need tomorrow's prayer times (current time is past Isha)
// Returns day offset: 0 for today, 1 for tomorrow
int getDayOffset()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return 0; // Default to today if time unavailable
    }

    const int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    int method = SettingsManager::getPrayerMethod();

    // For Diyanet, check cached Isha time
    if (method == PRAYER_METHOD_DIYANET)
    {
        DailyPrayers tempPrayers;
        if (PrayerAPI::getCachedPrayerTimes(tempPrayers, false))
        {
            const int ishaMinutes = tempPrayers[PrayerType::Isha].toMinutes();
            if (currentMinutes > ishaMinutes)
            {
                Serial.printf("[Time] After Isha (%d > %d), using tomorrow\n",
                              currentMinutes, ishaMinutes);
                return 1;
            }
        }
    }
    else
    {
        // For Adhan methods, estimate: after 8pm likely after Isha
        if (currentMinutes > 1200) // 20:00
        {
            return 1;
        }
    }

    return 0;
}

// Non-blocking delay that keeps settings server responsive
// Returns true if settings changed (caller should handle recalculation)
bool nonBlockingDelay(unsigned long ms)
{
    unsigned long start = millis();
    while (millis() - start < ms)
    {
        // Handle settings server during wait
        SettingsServer::handle();
        audioPlayerLoop();

        // Break early if settings changed - needs immediate recalculation
        if (SettingsManager::needsRecalculation())
        {
            return true; // Signal that recalculation is needed
        }

        delay(1); // Minimal yield for responsive web server
    }
    return false; // Normal completion
}

// --- MAIN LOGIC ---
void displayNextPrayer()
{
    if (!app.prayersFetched)
        return;

    const auto now = CurrentTime::now();
    app.nextPrayer = app.prayers.findNext(now._minutes);

    if (!app.nextPrayer)
    {
        Serial.println("[Info] Next prayer: Tomorrow's Fajr");
        app.nextPrayerSeconds = -1;
        display.update(now, app.nextPrayer, app.prayers);
        return;
    }

    // Update cache for both display and loop logic
    app.nextPrayerSeconds = app.prayers[*app.nextPrayer].toSeconds();

    const auto &nextTime = app.prayers[*app.nextPrayer];
    Serial.printf("[Info] Next prayer: %s at %s\n",
                  getPrayerName(*app.nextPrayer).data(),
                  nextTime.value.data());

    display.update(now, app.nextPrayer, app.prayers);
}

// Callback for handling volume updates during adhan playback
static uint8_t s_currentVolume = 0;

void onAdhanLoop()
{
    // Handle settings server for real-time volume changes
    SettingsServer::handle();

    // Apply volume changes in real-time
    uint8_t newVolume = SettingsManager::getVolume();
    if (newVolume != s_currentVolume)
    {
        s_currentVolume = newVolume;
        setVolume(SettingsManager::getHardwareVolume());
        Serial.printf("[Adhan] Volume changed to %d%%\n", s_currentVolume);
    }
}

void checkAndPlayAdhan()
{
    if (!app.prayersFetched || !app.nextPrayer)
        return;

    PrayerType currentPrayer = *app.nextPrayer;

    Serial.printf("\n\n🕌 === PRAYER TIME: %s === 🕌\n\n",
                  getPrayerName(currentPrayer).data());

    // Check if adhan should play for this prayer
    // Sunrise never plays adhan, and user can disable individual prayers
    bool shouldPlayAdhan = SettingsManager::getAdhanEnabled(currentPrayer);

    if (shouldPlayAdhan)
    {
        // Set initial volume
        s_currentVolume = SettingsManager::getVolume();
        setVolume(SettingsManager::getHardwareVolume());

        Serial.println("[Adhan] Playing...");
        playAudioFileBlocking("/azan.mp3", onAdhanLoop);
        Serial.println("[Adhan] Finished");
    }
    else
    {
        Serial.printf("[Adhan] Skipped for %s (disabled or sunrise)\n",
                      getPrayerName(currentPrayer).data());
    }

    // Check if this was the last prayer of the day
    const auto now = CurrentTime::now();
    auto nextPrayer = app.prayers.findNext(now._minutes);

    if (!nextPrayer)
    {
        // Last prayer done - fetch tomorrow's times
        Serial.println("[Info] Last prayer of day - fetching new times...");
        int method = SettingsManager::getPrayerMethod();
        app.prayersFetched = loadPrayerTimes(method, 1); // 1 = tomorrow
    }

    displayNextPrayer();
}

// --- HELPER FUNCTIONS ---

// Initialize hardware and filesystems
void initHardware()
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

    if (!LittleFS.begin(true))
    {
        Serial.println("[Error] LittleFS mount failed!");
    }

    audioPlayerInit();

    if (LittleFS.exists("/azan.mp3"))
    {
        Serial.println("[Audio] azan.mp3 found - ready for adhan");
    }
    else
    {
        Serial.println("[Audio] WARNING: azan.mp3 not found!");
    }
}

// Complete the normal boot process (after WiFi + NTP are ready)
// Returns true if successful
bool finishSetup()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("[Error] RTC not synced - cannot proceed");
        display.showError("TIME ERROR", "Please Reset");
        return false;
    }

    Serial.printf("[RTC] System time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // Load prayer times
    int method = SettingsManager::getPrayerMethod();
    app.prayersFetched = loadPrayerTimes(method, getDayOffset());

    if (app.prayersFetched)
    {
        displayNextPrayer();
    }
    else
    {
        Serial.println("[Error] Failed to load prayer times");
    }

    app.setupComplete = true;
    Serial.println("\n[System] Ready!\n");
    return true;
}

// Handle settings recalculation when method changes
void handleSettingsChange()
{
    if (!SettingsManager::needsRecalculation() || !app.setupComplete)
        return;

    Serial.println("[Settings] Prayer method changed - recalculating...");
    SettingsManager::clearRecalculationFlag();

    int method = SettingsManager::getPrayerMethod();
    app.prayersFetched = loadPrayerTimes(method, getDayOffset());

    if (app.prayersFetched)
    {
        app.nextPrayer = std::nullopt;
        app.nextPrayerSeconds = -1;
        display.forceRefresh();
        displayNextPrayer();
        Serial.printf("[Settings] Recalculation complete - Method: %s\n",
                      SettingsManager::getMethodName(method));
    }
}

// Handle prayer time checking and adhan playback
void handlePrayerTime(const CurrentTime &now)
{
    // Update cached next prayer if needed
    if (app.nextPrayerSeconds == -1)
    {
        app.nextPrayer = app.prayers.findNext(now._minutes);
        if (app.nextPrayer)
        {
            app.nextPrayerSeconds = app.prayers[*app.nextPrayer].toSeconds();
            Serial.printf("[Cache] Next prayer: %s at %d seconds\n",
                          getPrayerName(*app.nextPrayer).data(), app.nextPrayerSeconds);
        }
    }

    // Check if it's time for adhan
    if (app.nextPrayerSeconds > 0)
    {
        const int secondsUntil = app.nextPrayerSeconds - now._seconds;
        if (secondsUntil <= 0)
        {
            checkAndPlayAdhan();
            app.nextPrayer = std::nullopt;
            app.nextPrayerSeconds = -1;
        }
    }
}

// Calculate optimal sleep time
int calculateSleepTime(const CurrentTime &now)
{
    const int secondsToNextMinute = 60 - (now._seconds % 60);

    if (app.nextPrayerSeconds <= 0)
    {
        return secondsToNextMinute;
    }

    const int secondsUntilPrayer = app.nextPrayerSeconds - now._seconds;
    int sleepTime = std::min(secondsToNextMinute, secondsUntilPrayer);

    return std::max(1, sleepTime);
}

// --- MAIN FUNCTIONS ---

void setup()
{
    initHardware();

#if CLEAR_WIFI_CREDENTIALS
    Serial.println("[DEBUG] Clearing stored WiFi credentials...");
    WiFiCredentials::clear();
#endif

    Network::init();
    SettingsManager::init();

    const bool wifiOk = Network::connectWiFi();

    // Portal mode - handle in loop
    if (!Network::isConnected())
    {
        const char *msg = Network::isRetryConnection() ? "Reconnect to AP" : "Connect to AP";
        if (Network::isRetryConnection())
        {
            display.showError("Connection Failed", "Check Credentials");
            delay(5000);
        }
        display.showMessage("WiFi Setup", msg);
        return;
    }

    Network::syncTime();

#if TEST_MODE
    TestMode::runPrayerTimeTests();
    return;
#endif

    if (!finishSetup())
        return;

    // Start settings server
    if (wifiOk)
    {
        SettingsServer::start();
        Serial.println("[WiFi] Staying connected for settings server");
    }
}

// Complete setup after portal succeeds (called from loop when settings mode activates)
static bool portalSetupAttempted = false;

void completeSetupAfterPortal()
{
    if (portalSetupAttempted)
        return;
    portalSetupAttempted = true;

    Serial.println("[Setup] Portal completed - finishing setup...");
    finishSetup();
}

void loop()
{
    audioPlayerLoop();

    // Handle WiFi portal if active
    if (Network::isPortalActive())
    {
        Network::handlePortal();
        delay(10);
        return;
    }

    SettingsServer::handle();
    handleSettingsChange();

    // Complete setup after portal
    if (SettingsServer::isActive() && !app.setupComplete)
    {
        completeSetupAfterPortal();
    }

    // Wait if setup incomplete - but keep handling settings server
    if (!app.setupComplete || !app.prayersFetched)
    {
        nonBlockingDelay(1000);
        return;
    }

    const auto now = CurrentTime::now();
    display.update(now, app.nextPrayer, app.prayers);

    handlePrayerTime(now);

    // Cap sleep time to 5 seconds max for responsive settings server
    const int sleepTime = min(calculateSleepTime(now), 5);
    nonBlockingDelay(sleepTime * 1000);
}
