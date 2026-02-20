#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "config.h"
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

#if TEST_MODE || TEST_ADHAN_AUDIO
#include "test_mode.h"
#endif

// ── Hardware Init ────────────────────────────────────────

static void initHardware()
{
    delay(2000);
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================");
    Serial.println("  ESP32-S3 SPIRITUAL ASSISTANT v3.0");
    Serial.println("========================================");

    Serial.printf("Flash: %d MB | PSRAM: %s (%d MB)\n",
                  ESP.getFlashChipSize() / (1024 * 1024),
                  psramInit() ? "ACTIVE" : "NO",
                  psramInit() ? ESP.getPsramSize() / (1024 * 1024) : 0);

    if (!LittleFS.begin(true))
        Serial.println("[Error] LittleFS mount failed!");

    audioPlayerInit();

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

    SettingsManager::init();

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

    if (bootOk)
        PrayerEngine::init();

    WifiManager::init(BootManager::didConnectWiFi());

    UiPageSettings::setAdvancedCallback(onSettingsPressed);

    // Volume
    uint8_t vol = SettingsManager::getVolume();
    int level = (vol + 10) / 20;
    level = constrain(level, 0, 5);
    AppStateHelper::setVolume(level);

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

    if (Network::isConnected())
        SettingsServer::handle();

    static unsigned long lastLog = 0;
    if (millis() - lastLog > 30000)
    {
        lastLog = millis();
        Serial.printf("[Status] Heap: %d | Min: %d | WiFi: %s\n",
                      ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                      Network::isConnected() ? "ON" : "OFF");
    }

    delay(5);
}
