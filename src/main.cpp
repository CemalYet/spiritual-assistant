#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <LittleFS.h>
#include "config.h"
#include "tft_config.h"
#include "network.h"
#include "settings_server.h"
#include "settings_manager.h"
#include "wifi_credentials.h"
#include "audio_player.h"
#include "lvgl_display.h"
#include "app_state.h"
#include "ui_page_settings.h"
#include "prayer_types.h"

#include "boot_manager.h"
#include "prayer_engine.h"
#include "wifi_manager.h"
#include "portal_handler.h"
#include "display_ticker.h"
#include "tca_expander.h"
#include "rtc_manager.h"
#include "pmu_manager.h"
#include "power_manager.h"
#include "imu_manager.h"

#if TEST_MODE || TEST_ADHAN_AUDIO
#include "test_mode.h"
#endif

// ── Hardware Init ────────────────────────────────────────

static void initHardware()
{
    Serial.begin(115200);
    // USB CDC: short wait for serial monitor (non-blocking if not connected)
    unsigned long t0 = millis();
    while (!Serial && (millis() - t0 < 1000))
    {
        delay(10);
    }
    delay(200);

    Serial.println("\n========================================");
    Serial.println("  ESP32-S3 SPIRITUAL ASSISTANT v3.0");
    Serial.println("========================================");

    Serial.printf("Flash: %d MB | PSRAM: %s (%d MB)\n",
                  ESP.getFlashChipSize() / (1024 * 1024),
                  psramInit() ? "ACTIVE" : "NO",
                  psramInit() ? ESP.getPsramSize() / (1024 * 1024) : 0);

    if (!LittleFS.begin(true))
        Serial.println("[Error] LittleFS mount failed!");

    Wire.begin(I2C_SDA, I2C_SCL);

    TcaExpander::init();
    PmuManager::init(); // ALDO1 powers ES8311 codec — must precede audioPlayerInit
    audioPlayerInit();  // ES8311 codec + I2S + audio task
    RtcManager::init();
    ImuManager::init();
    SettingsManager::init();

    // Apply timezone from NVS before setting system clock
    const char *tzCold = SettingsManager::getTimezone();
    setenv("TZ", tzCold, 1);
    tzset();

    if (RtcManager::hasValidTime())
        RtcManager::setSystemClockFromRTC();

    // Verify per-prayer adhan files exist
    bool adhanFound = false;
    for (uint8_t i = 0; i < static_cast<uint8_t>(PrayerType::COUNT); i++)
    {
        const auto file = getAdhanFile(static_cast<PrayerType>(i));
        if (!file.empty())
        {
            bool exists = LittleFS.exists(file.data());
            Serial.printf("[Audio] %s: %s\n", file.data(),
                          exists ? "found" : "NOT FOUND");
            if (exists)
                adhanFound = true;
        }
    }
    AppStateHelper::setAdhanAvailable(adhanFound);
}

// ── Settings Button ───────────────────────────────────

static void onSettingsPressed()
{
    Serial.println("[Settings] Button pressed");

    if (Network::isConnected())
    {
        if (!SettingsServer::isActive())
        {
            SettingsServer::start();
            WifiManager::resetTimeout();
        }
        AppStateHelper::setWifiState(WifiState::CONNECTED,
                                     WiFi.localIP().toString().c_str());
        return;
    }

    const char *mode = SettingsManager::getConnectionMode();
    bool isOffline = (strcmp(mode, "offline") == 0);

    if (!isOffline && WiFiCredentials::hasCredentials())
    {
        WifiManager::reconnect();
        return;
    }

    PortalHandler::open();
}

// ── Setup ────────────────────────────────────────────────

void setup()
{
    initHardware();

#if FORCE_AP_PORTAL
    Serial.println("[DEBUG] FORCE_AP_PORTAL: Full reset for testing");
    WiFiCredentials::clear();
    SettingsManager::setConnectionMode("wifi");
    Network::init(true);
#else
    Network::init(false);
#endif

    if (!LvglDisplay::begin())
    {
        Serial.println("[Display] FATAL: LVGL init failed!");
        return;
    }
    delay(100);

#if TEST_MODE
    // Boot manager still needed for WiFi/NTP in test mode
    BootManager::run();
    TestMode::runPrayerTimeTests();
    return;
#endif

#if TEST_ADHAN_AUDIO
    TestMode::testAllAdhan();
#endif

    // ── Boot (blocking) ──
    bool bootOk = BootManager::run();

    // ── Init modules ──
    LvglDisplay::showPrayerScreen();

    AppStateHelper::setNextPrayer("YUKLENIYOR", "--:--"); // placeholder until PrayerEngine loads
    DisplayTicker::forceUpdate();                         // populate initial time / date / hijri / ntp in AppState

    if (bootOk)
        PrayerEngine::init();

    WifiManager::init(BootManager::didConnectWiFi());

    UiPageSettings::setAdvancedCallback(onSettingsPressed);

    // Volume — all 0-100 everywhere
    uint8_t vol = SettingsManager::getVolume();
    AppStateHelper::setVolume(vol);
    setVolume(vol); // Apply NVS volume to ES8311 codec

    // Mute state — persisted in NVS
    g_state.muted = SettingsManager::getMuted();

    PowerManager::init();

    Serial.println("\n[System] Ready!\n");
}

// ── Loop ─────────────────────────────────────────────────

void loop()
{
    LvglDisplay::loop();

    PortalHandler::tick();
    WifiManager::tick();
    PrayerEngine::tick();
    DisplayTicker::tick();
    PowerManager::tick();
    RtcManager::correctDriftFromRTC();

    if (Network::isConnected())
    {
        SettingsServer::handle();
        if (RtcManager::periodicSyncTick())
            Network::syncTime();
    }
    else if (!SettingsManager::isOfflineMode() && RtcManager::periodicSyncTick())
    {
        // WiFi is off but NTP sync is due — reconnect, sync, WiFi auto-disconnects later
        Serial.println("[NTP] Sync due — reconnecting WiFi...");
        WifiManager::reconnect();
    }

    static unsigned long lastLog = 0;
    if (millis() - lastLog > 300000)
    {
        lastLog = millis();
        Serial.printf("[Status] Heap: %d | Min: %d | WiFi: %s\n",
                      ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                      Network::isConnected() ? "ON" : "OFF");
    }

    delay(5);
}
