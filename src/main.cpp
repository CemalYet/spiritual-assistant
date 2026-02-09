#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <algorithm>
#include <cmath>
#include <etl/string.h>
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
#include "wifi_portal.h"
#include "audio_player.h"
#include "settings_manager.h"
#include "lvgl_display.h"
#include "app_state.h"
#include "ui_state_reader.h"
#include "ui_page_settings.h"
#include "ui_page_status.h"
#include <time.h>

struct AppLogic
{
    DailyPrayers prayers;
    std::optional<PrayerType> nextPrayer;
    int nextPrayerSeconds = -1;
    bool prayersFetched = false;
    bool showingTomorrow = false;
    bool setupComplete = false;
};

static AppLogic app;

constexpr unsigned long WIFI_TIMEOUT_MS = 5 * 60 * 1000;
constexpr unsigned long WIFI_RECONNECT_TIMEOUT_MS = 15000;
static unsigned long wifiConnectedAt = 0;
static bool wifiAutoDisconnected = false;
static bool wifiReconnectPending = false;
static unsigned long wifiReconnectStarted = 0;

bool loadPrayerTimes(int method, int dayOffset = 0)
{
    const bool wantDiyanet = (method == PRAYER_METHOD_DIYANET);
    const bool fetchTomorrow = (dayOffset > 0);
    app.showingTomorrow = fetchTomorrow;

    if (wantDiyanet)
    {
        if (PrayerAPI::getCachedPrayerTimes(app.prayers, fetchTomorrow))
            return true;

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
        Serial.println("[Fallback] Diyanet unavailable, using Adhan calculation");
    }

    double lat = SettingsManager::getLatitude();
    double lng = SettingsManager::getLongitude();

    if (std::isnan(lat) || std::isnan(lng))
    {
        Serial.println("[Prayer] ERROR: Location not configured!");
        return false;
    }

    return PrayerCalculator::calculateTimes(app.prayers, method, lat, lng, dayOffset);
}

int getDayOffset()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
        return 0;

    const int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    int method = SettingsManager::getPrayerMethod();

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
    else if (currentMinutes > 1200)
    {
        return 1;
    }

    return 0;
}

bool nonBlockingDelay(unsigned long ms)
{
    unsigned long start = millis();

    while (millis() - start < ms)
    {
        LvglDisplay::loop();
        SettingsServer::handle();
        if (SettingsManager::needsRecalculation())
            return true;
        delay(5);
    }
    return false;
}

void displayNextPrayer()
{
    if (!app.prayersFetched)
        return;

    if (app.showingTomorrow)
        app.nextPrayer = PrayerType::Fajr;
    else
        app.nextPrayer = app.prayers.findNext(CurrentTime::now()._minutes);

    if (!app.nextPrayer)
    {
        app.nextPrayerSeconds = -1;
        AppStateHelper::setNextPrayer("SABAH", "Yarin");
        return;
    }

    const PrayerType prayer = *app.nextPrayer;
    const auto &prayerTime = app.prayers[prayer];
    app.nextPrayerSeconds = prayerTime.toSeconds();

    Serial.printf("[Info] Next prayer: %s at %s%s\n",
                  getPrayerName(prayer).data(),
                  prayerTime.value.data(),
                  app.showingTomorrow ? " (tomorrow)" : "");

    AppStateHelper::setNextPrayer(getPrayerName(prayer, true).data(), prayerTime.value.data());
    AppStateHelper::setPrayerTimes(
        app.prayers[PrayerType::Fajr].value.data(),
        app.prayers[PrayerType::Sunrise].value.data(),
        app.prayers[PrayerType::Dhuhr].value.data(),
        app.prayers[PrayerType::Asr].value.data(),
        app.prayers[PrayerType::Maghrib].value.data(),
        app.prayers[PrayerType::Isha].value.data(),
        app.showingTomorrow ? -1 : static_cast<int>(prayer));
}

static uint8_t s_currentVolume = 0;
static int s_lastAdhanMinute = -1;

void onAdhanLoop()
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

void checkAndPlayAdhan()
{
    if (!app.prayersFetched || !app.nextPrayer)
        return;

    PrayerType currentPrayer = *app.nextPrayer;
    Serial.printf("\n\n🕌 === PRAYER TIME: %s === 🕌\n\n", getPrayerName(currentPrayer).data());

    bool shouldPlayAdhan = SettingsManager::getAdhanEnabled(currentPrayer) && !g_state.muted;

    if (shouldPlayAdhan)
    {
        s_currentVolume = SettingsManager::getVolume();
        setVolume(SettingsManager::getHardwareVolume());

        Serial.println("[Adhan] Playing...");
        playAudioFileBlocking("/azan.mp3", onAdhanLoop);
        Serial.println("[Adhan] Finished");
    }
    else
    {
        Serial.printf("[Adhan] Skipped for %s (%s)\n",
                      getPrayerName(currentPrayer).data(),
                      g_state.muted ? "muted" : "disabled");
    }

    const auto now = CurrentTime::now();
    auto nextPrayer = app.prayers.findNext(now._minutes);

    if (!nextPrayer)
    {
        Serial.println("[Info] Last prayer - loading tomorrow");
        int method = SettingsManager::getPrayerMethod();
        app.prayersFetched = loadPrayerTimes(method, 1);
    }

    displayNextPrayer();
}

void initHardware()
{
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
        Serial.println("[Error] LittleFS mount failed!");

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

bool finishSetup()
{
    if (!LittleFS.begin(false))
    {
        Serial.println("[Warning] LittleFS remount needed");
        if (!LittleFS.begin(true))
            Serial.println("[Error] LittleFS mount failed!");
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("[Error] RTC not synced - cannot proceed");
        AppStateHelper::showError("Hata", "Saat esitlenmedi");
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
        AppStateHelper::setLocation(SettingsManager::getShortCityName());
        AppStateHelper::clearStatusScreen();
        LvglDisplay::showPrayerScreen();
        displayNextPrayer();
    }
    else
    {
        Serial.println("[Error] Failed to load prayer times");
        AppStateHelper::showError("Hata", "Namaz vakitleri yuklenemedi");
    }

    app.setupComplete = true;
    Serial.println("\n[System] Ready!\n");
    return true;
}

void disconnectWiFi()
{
    if (!Network::isConnected())
        return;

    Serial.println("[WiFi] Auto-disconnect: 5 min timeout reached");
    SettingsServer::stop();
    Network::disconnect();
    wifiAutoDisconnected = true;
    LvglDisplay::updateStatus();
    AppStateHelper::setWifiState(WifiState::DISCONNECTED);
}

void reconnectWiFi()
{
    if (Network::isConnected())
    {
        wifiConnectedAt = millis();
        wifiAutoDisconnected = false;

        String ipStr = WiFi.localIP().toString();
        AppStateHelper::setWifiState(WifiState::CONNECTED, ipStr.c_str());
        Serial.printf("[WiFi] Already connected: %s\n", ipStr.c_str());
        return;
    }

    Serial.println("[WiFi] User requested reconnect...");
    AppStateHelper::setWifiState(WifiState::CONNECTING);
    WiFi.mode(WIFI_STA);

    char ssidBuffer[33] = "";
    char passBuffer[65] = "";
    if (WiFiCredentials::load(ssidBuffer, sizeof(ssidBuffer), passBuffer, sizeof(passBuffer)))
    {
        WiFi.begin(ssidBuffer, passBuffer);
        wifiReconnectPending = true;
        wifiReconnectStarted = millis();
        wifiAutoDisconnected = false;
        Serial.printf("[WiFi] Connecting to %s...\n", ssidBuffer);
    }
    else
    {
        Serial.println("[WiFi] No saved credentials");
        AppStateHelper::setWifiState(WifiState::DISCONNECTED);
    }
}

void onSettingsPressed()
{
    Serial.println("[Settings] Button pressed");

    SettingsManager::clearRecalculationFlag();
    WiFiPortal::clearOfflineModeFlag();

    const char *mode = SettingsManager::getConnectionMode();
    bool isOfflineMode = (strcmp(mode, "offline") == 0);

    if (isOfflineMode || !WiFiCredentials::hasCredentials())
    {
        Serial.println("[Settings] Starting AP portal...");
        AppStateHelper::setWifiState(WifiState::CONNECTING);
        Network::startPortal();
        AppStateHelper::setWifiState(WifiState::PORTAL);
        return;
    }

    reconnectWiFi();
}

void handleSettingsChange()
{
    if (!SettingsManager::needsRecalculation() || !app.setupComplete)
        return;

    SettingsManager::clearRecalculationFlag();

    int method = SettingsManager::getPrayerMethod();
    app.prayersFetched = loadPrayerTimes(method, getDayOffset());

    if (app.prayersFetched)
    {
        app.nextPrayer = std::nullopt;
        app.nextPrayerSeconds = -1;
        AppStateHelper::setLocation(SettingsManager::getShortCityName());

        displayNextPrayer();
    }
}

void handlePrayerTime(const CurrentTime &now)
{
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

void setup()
{
    initHardware();

#if FORCE_AP_PORTAL
    Serial.println("[DEBUG] FORCE_AP_PORTAL: Clearing stored WiFi credentials...");
    WiFiCredentials::clear();
#endif

    Serial.println("[Display] Initializing LVGL...");
    if (!LvglDisplay::begin())
    {
        Serial.println("[Display] FATAL: LVGL init failed!");
        return;
    }
    delay(100);

#if FORCE_AP_PORTAL
    Network::init(true); // Skip hardcoded credentials
#else
    Network::init(false);
#endif
    SettingsManager::init();

    bool offlineMode = SettingsManager::isOfflineMode();
    bool wifiOk = false;

    if (offlineMode)
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo) || timeinfo.tm_year < 120)
        {
            Serial.println("[Mode] Offline but RTC not set - forcing WiFi for NTP");
            offlineMode = false;
        }
    }

    if (offlineMode)
    {
        Serial.println("[Mode] Offline - skipping WiFi");
        UiPageStatus::showMessage("Çevrimdışı Mod", "Manuel saat kullanılıyor...");
        LvglDisplay::loop();
        delay(1000);
    }
    else
    {
        char ssidBuffer[33] = "";
        char passBuffer[65] = "";
        if (WiFiCredentials::load(ssidBuffer, sizeof(ssidBuffer), passBuffer, sizeof(passBuffer)))
        {
            UiPageStatus::showConnecting(ssidBuffer);
            LvglDisplay::loop();
        }
        else
        {
            UiPageStatus::showMessage("WiFi kaydedilmedi", "Portal baslatiliyor...");
            LvglDisplay::loop();
        }

        wifiOk = Network::connectWiFi();

        if (wifiOk)
        {
            Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        }
        else
        {
            UiPageStatus::showPortal("AdhanSettings", "12345678", "192.168.4.1");
            LvglDisplay::loop();
        }

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

        UiPageStatus::showMessage("Saat Senkronize", "NTP sunucusuna baglaniliyor...");
        LvglDisplay::loop();

        Network::syncTime();
    }

#if TEST_MODE
    TestMode::runPrayerTimeTests();
    return;
#endif

    if (!finishSetup())
    {
        // Still set callback so button works even if setup failed
        UiPageSettings::setAdvancedCallback(onSettingsPressed);
        AppStateHelper::setWifiState(WifiState::DISCONNECTED);
        return;
    }

    if (wifiOk)
    {
        SettingsServer::start();
        wifiConnectedAt = millis();
        Serial.println("[WiFi] Connected (auto-disconnect in 5 min)");
        AppStateHelper::setWifiState(WifiState::CONNECTED, WiFi.localIP().toString().c_str());
    }

    UiPageSettings::setAdvancedCallback(onSettingsPressed);

    if (!wifiOk)
    {
        AppStateHelper::setWifiState(WifiState::DISCONNECTED);
    }

    uint8_t savedVolume = SettingsManager::getVolume();
    int level = (savedVolume + 10) / 20;
    if (level < 0)
        level = 0;
    if (level > 5)
        level = 5;
    AppStateHelper::setVolume(level);
}

static bool portalSetupAttempted = false;
static bool waitingForUserConfig = false;

void completeSetupAfterPortal()
{
    if (portalSetupAttempted)
        return;
    portalSetupAttempted = true;

    double lat = SettingsManager::getLatitude();
    double lng = SettingsManager::getLongitude();
    constexpr double MIN_VALID_COORD = 0.0001;
    if (std::abs(lat) > MIN_VALID_COORD || std::abs(lng) > MIN_VALID_COORD)
    {
        finishSetup();
        UiPageSettings::setAdvancedCallback(onSettingsPressed);
        wifiConnectedAt = millis();
        AppStateHelper::setWifiState(WifiState::CONNECTED, WiFi.localIP().toString().c_str());
        return;
    }

    etl::string<40> url{"http://"};
    url += WiFi.localIP().toString().c_str();
    UiPageStatus::showMessage("Ayarlari Yapin", url.c_str());
    waitingForUserConfig = true;
}

void loop()
{
    LvglDisplay::loop();

    if (Network::didPortalConnectWiFi())
    {
        Network::clearPortalConnectFlag();
        wifiConnectedAt = millis();
        wifiAutoDisconnected = false;
        AppStateHelper::setWifiState(WifiState::CONNECTED, WiFi.localIP().toString().c_str());
    }

    if (Network::isPortalActive())
    {
        Network::handlePortal();
        LvglDisplay::loop();

        if (SettingsManager::needsRecalculation())
        {
            SettingsManager::clearRecalculationFlag();
            Network::stopPortal();

            if (Network::isConnected())
            {
                Network::syncTime();
                SettingsServer::start();
                wifiConnectedAt = millis();
                wifiAutoDisconnected = false;
                AppStateHelper::setWifiState(WifiState::CONNECTED, WiFi.localIP().toString().c_str());
            }
            else
            {
                AppStateHelper::setWifiState(WifiState::DISCONNECTED);
            }

            if (!app.setupComplete)
            {
                finishSetup();
                UiPageSettings::setAdvancedCallback(onSettingsPressed);
            }
            else
            {
                int method = SettingsManager::getPrayerMethod();
                app.prayersFetched = loadPrayerTimes(method, getDayOffset());
                AppStateHelper::setLocation(SettingsManager::getShortCityName());
                displayNextPrayer();
            }
            return;
        }

        if (WiFiPortal::isOfflineModeRequested())
        {
            WiFiPortal::clearOfflineModeFlag();
            Network::stopPortal();

            if (!app.setupComplete)
            {
                finishSetup();
                UiPageSettings::setAdvancedCallback(onSettingsPressed);
            }

            AppStateHelper::setWifiState(WifiState::DISCONNECTED);
            return;
        }

        delay(10);
        return;
    }

    static unsigned long wifiFailedShownAt = 0;
    static bool wifiShowingFailed = false;

    if (wifiReconnectPending)
    {
        if (Network::isConnected())
        {
            wifiReconnectPending = false;
            wifiConnectedAt = millis();
            String ipStr = WiFi.localIP().toString();
            Serial.printf("[WiFi] Reconnected! IP: %s\n", ipStr.c_str());
            SettingsServer::start();
            AppStateHelper::setWifiState(WifiState::CONNECTED, ipStr.c_str());
        }
        else if (millis() - wifiReconnectStarted > WIFI_RECONNECT_TIMEOUT_MS)
        {
            wifiReconnectPending = false;
            wifiShowingFailed = true;
            wifiFailedShownAt = millis();
            Serial.println("[WiFi] Reconnect timeout - failed");
            Network::disconnect();
            AppStateHelper::setWifiState(WifiState::FAILED);
        }
    }

    if (wifiShowingFailed && millis() - wifiFailedShownAt > 3000)
    {
        wifiShowingFailed = false;
        AppStateHelper::setWifiState(WifiState::DISCONNECTED);
    }

    if (Network::isConnected() && !wifiAutoDisconnected && wifiConnectedAt > 0)
    {
        if (millis() - wifiConnectedAt > WIFI_TIMEOUT_MS)
        {
            disconnectWiFi();
        }
    }

    static unsigned long lastStatusLog = 0;
    if (millis() - lastStatusLog > 30000)
    {
        lastStatusLog = millis();
        Serial.printf("[Status] Free heap: %d bytes, Min free: %d bytes, WiFi: %s\n",
                      ESP.getFreeHeap(),
                      ESP.getMinFreeHeap(),
                      WiFi.status() == WL_CONNECTED ? "OK" : "OFF");
    }

    if (Network::isConnected())
    {
        SettingsServer::handle();
    }

    if (SettingsServer::isActive() && !app.setupComplete && !portalSetupAttempted)
    {
        completeSetupAfterPortal();
    }

    if (waitingForUserConfig && SettingsManager::needsRecalculation())
    {
        SettingsManager::clearRecalculationFlag();
        waitingForUserConfig = false;
        finishSetup();
        UiPageSettings::setAdvancedCallback(onSettingsPressed);
        wifiConnectedAt = millis();
        AppStateHelper::setWifiState(WifiState::CONNECTED, WiFi.localIP().toString().c_str());
    }

    handleSettingsChange();

    if (!app.setupComplete || !app.prayersFetched)
    {
        nonBlockingDelay(1000);
        return;
    }

    const auto now = CurrentTime::now();
    static int lastDisplayMinute = -1;
    static int lastDay = -1;
    if (now._minutes != lastDisplayMinute)
    {
        lastDisplayMinute = now._minutes;
        LvglDisplay::updateTime();
        LvglDisplay::updateStatus();

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

    const int sleepTime = min(calculateSleepTime(now), 5);
    nonBlockingDelay(sleepTime * 1000);
}
