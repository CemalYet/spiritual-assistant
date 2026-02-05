#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <algorithm>
#include "config.h"
#include "prayer_types.h"
#include "daily_prayers.h"
#include "current_time.h"
#include "network.h"
#include "settings_server.h"
#include "prayer_api.h"
#include "prayer_calculator.h"
#include "test_mode.h"
#include "wifi_credentials.h"
#include "audio_player.h"
#include "settings_manager.h"
#include "lvgl_display.h"
#include "ui_home.h"
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
    bool showingTomorrow = false;         // True if displaying tomorrow's prayers (after Isha)
    bool setupComplete = false;           // Whether setup finished successfully
};

AppState app; // Global application state

// --- PRAYER LOADING (Single source of truth) ---
// Loads prayer times based on method. Handles Diyanet cache/API and Adhan fallback.
// Also sets app.showingTomorrow flag based on dayOffset.
// Returns true if prayer times were loaded successfully.
bool loadPrayerTimes(int method, int dayOffset = 0)
{
    const bool wantDiyanet = (method == PRAYER_METHOD_DIYANET);
    const bool fetchTomorrow = (dayOffset > 0);

    // Set flag: are we showing tomorrow's prayers?
    app.showingTomorrow = fetchTomorrow;

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

// Non-blocking delay that keeps settings server and touch responsive
// Returns true if settings changed (caller should handle recalculation)
bool nonBlockingDelay(unsigned long ms)
{
    unsigned long start = millis();

    while (millis() - start < ms)
    {
        LvglDisplay::loop(); // Keep touch responsive
        SettingsServer::handle();

        if (SettingsManager::needsRecalculation())
            return true;

        delay(5); // Small delay, but loop runs often for touch
    }
    return false; // Normal completion
}

// --- MAIN LOGIC ---
void displayNextPrayer()
{
    if (!app.prayersFetched)
        return;

    // Determine which prayer to show
    if (app.showingTomorrow)
    {
        // After Isha: show tomorrow's Fajr
        app.nextPrayer = PrayerType::Fajr;
    }
    else
    {
        // Normal: find next prayer today
        const auto now = CurrentTime::now();
        app.nextPrayer = app.prayers.findNext(now._minutes);
    }

    // No prayer found (shouldn't happen if showingTomorrow logic is correct)
    if (!app.nextPrayer)
    {
        app.nextPrayerSeconds = -1;
        UiHome::setNextPrayer("SABAH", "Yarin");
        return;
    }

    // Update display with next prayer
    const PrayerType prayer = *app.nextPrayer;
    const auto &prayerTime = app.prayers[prayer];
    app.nextPrayerSeconds = prayerTime.toSeconds();

    Serial.printf("[Info] Next prayer: %s at %s%s\n",
                  getPrayerName(prayer).data(),
                  prayerTime.value.data(),
                  app.showingTomorrow ? " (tomorrow)" : "");

    UiHome::setNextPrayer(getPrayerName(prayer, true).data(), prayerTime.value.data());

    // Update prayer times page data
    UiHome::PrayerTimesData ptData = {
        app.prayers[PrayerType::Fajr].value.data(),
        app.prayers[PrayerType::Sunrise].value.data(),
        app.prayers[PrayerType::Dhuhr].value.data(),
        app.prayers[PrayerType::Asr].value.data(),
        app.prayers[PrayerType::Maghrib].value.data(),
        app.prayers[PrayerType::Isha].value.data(),
        app.showingTomorrow ? -1 : static_cast<int>(prayer)};
    UiHome::setPrayerTimes(ptData);
}

// Callback for handling volume updates during adhan playback
static uint8_t s_currentVolume = 0;

void onAdhanLoop()
{
    // Keep LVGL running during adhan playback
    LvglDisplay::loop();

    // Handle settings server for real-time volume changes
    SettingsServer::handle();

    // Stop adhan if user mutes
    if (UiHome::isMuted())
    {
        stopAudio();
        Serial.println("[Adhan] Stopped by user mute");
        return;
    }

    // Apply volume changes in real-time
    uint8_t newVolume = SettingsManager::getVolume();
    if (newVolume != s_currentVolume)
    {
        s_currentVolume = newVolume;
        setVolume(SettingsManager::getHardwareVolume());
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
    // Also check if user has muted from UI
    bool shouldPlayAdhan = SettingsManager::getAdhanEnabled(currentPrayer) && !UiHome::isMuted();

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
        if (UiHome::isMuted())
            Serial.printf("[Adhan] Skipped for %s (muted by user)\n",
                          getPrayerName(currentPrayer).data());
        else
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
    // Wait for PSRAM to stabilize
    delay(2000);

    Serial.begin(115200);
    delay(500);

    Serial.println("\n" + String('=', 40));
    Serial.println("  ESP32-S3 SPIRITUAL ASSISTANT v2.0");
    Serial.println(String('=', 40));

    Serial.printf("Flash: %d MB | PSRAM: %s (%d MB)\n",
                  ESP.getFlashChipSize() / (1024 * 1024),
                  psramInit() ? "ACTIVE" : "NO",
                  psramInit() ? ESP.getPsramSize() / (1024 * 1024) : 0);

    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

    Serial.println(String('=', 40) + "\n");

    if (!LittleFS.begin(true))
    {
        Serial.println("[Error] LittleFS mount failed!");
    }

    audioPlayerInit();

    // TFT will be initialized AFTER WiFi connects
    // to avoid PSRAM conflicts during WiFi setup

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
        UiHome::showError("Hata", "Saat esitlenmedi");
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
        // First create the screen, then update with prayer data
        LvglDisplay::showPrayerScreen();
        displayNextPrayer();
    }
    else
    {
        Serial.println("[Error] Failed to load prayer times");
        UiHome::showError("Hata", "Namaz vakitleri yuklenemedi");
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

    // Initialize LVGL display BEFORE WiFi
    Serial.println("[Display] Initializing LVGL...");
    if (!LvglDisplay::begin())
    {
        Serial.println("[Display] FATAL: LVGL init failed!");
        return;
    }
    delay(100);

    Network::init();
    SettingsManager::init();

    // Show connecting screen with SSID
    char ssidBuffer[33] = "";
    char passBuffer[65] = "";
    if (WiFiCredentials::load(ssidBuffer, sizeof(ssidBuffer), passBuffer, sizeof(passBuffer)))
    {
        UiHome::showConnecting(ssidBuffer);
        LvglDisplay::loop(); // Flush screen
    }
    else
    {
        UiHome::showMessage("WiFi kaydedilmedi", "Portal baslatiliyor...");
        LvglDisplay::loop();
    }

    const bool wifiOk = Network::connectWiFi();

    // Update screen with result
    if (wifiOk)
    {
        Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        // Go directly to prayer times
    }
    else
    {
        UiHome::showPortal("SpiritualAssistant-Setup", "12345678", "192.168.4.1");
        LvglDisplay::loop();
    }

    // Portal mode - handle in loop
    if (!Network::isConnected())
    {
        const char *msg = Network::isRetryConnection() ? "Reconnect to AP" : "Connect to AP";
        if (Network::isRetryConnection())
        {
            Serial.println("[WiFi] Connection Failed - Check Credentials");
            delay(5000);
        }
        Serial.printf("[WiFi] %s\n", msg);
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
    LvglDisplay::loop();

    // Handle WiFi portal if active
    if (Network::isPortalActive())
    {
        Network::handlePortal();
        LvglDisplay::loop();
        delay(10);
        return;
    }

    // Memory and WiFi status logging every 30 seconds
    static unsigned long lastStatusLog = 0;
    if (millis() - lastStatusLog > 30000)
    {
        lastStatusLog = millis();
        Serial.printf("[Status] Free heap: %d bytes, Min free: %d bytes, WiFi: %s, RSSI: %d dBm\n",
                      ESP.getFreeHeap(),
                      ESP.getMinFreeHeap(),
                      WiFi.status() == WL_CONNECTED ? "OK" : "DISCONNECTED",
                      WiFi.RSSI());
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

    // Update time display every minute
    static int lastDisplayMinute = -1;
    static int lastDay = -1;
    if (now._minutes != lastDisplayMinute)
    {
        lastDisplayMinute = now._minutes;
        LvglDisplay::updateTime();
        LvglDisplay::updateStatus();

        // Check for day change at midnight - reload today's prayers
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            if (lastDay != -1 && lastDay != timeinfo.tm_mday)
            {
                Serial.println("[Time] New day detected - loading today's prayers");
                int method = SettingsManager::getPrayerMethod();
                app.prayersFetched = loadPrayerTimes(method, 0);
                displayNextPrayer();
            }
            lastDay = timeinfo.tm_mday;
        }
    }

    handlePrayerTime(now);

    // Cap sleep time to 5 seconds max for responsive settings server
    const int sleepTime = min(calculateSleepTime(now), 5);
    nonBlockingDelay(sleepTime * 1000);
}
