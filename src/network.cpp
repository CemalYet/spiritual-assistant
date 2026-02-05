#include "network.h"
#include "config.h"
#include "wifi_portal.h"
#include "wifi_credentials.h"
#include "settings_server.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <LittleFS.h>
#include <stdlib.h>
#include <time.h>

// WiFi event handler - only log important events
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.printf("[WiFi] Disconnected (reason: %d)\n", info.wifi_sta_disconnected.reason);
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        Serial.println("[WiFi] Lost IP!");
        break;
    default:
        break;
    }
}

namespace Network
{
    constexpr int WIFI_CONNECT_TIMEOUT_MS = 15000;
    constexpr int WIFI_MAX_RETRIES = 3;
    constexpr int WIFI_RESET_DELAY_MS = 500;
    constexpr int NTP_SYNC_TIMEOUT_ATTEMPTS = 60; // 60 * 500ms = 30 seconds

    static bool portalMode = false;
    static char currentSSID[33] = "";     // Max SSID: 32 + null
    static char currentPassword[65] = ""; // Max WPA2: 64 + null
    static int connectionAttempts = 0;
    static bool isRetryPortal = false;       // True if portal opened after failed connection
    static bool portalConnectedWiFi = false; // True if portal just closed with WiFi success

    void init(bool skipHardcodedCredentials)
    {
        WiFi.onEvent(onWiFiEvent);
        WiFiCredentials::init();

        if (WiFiCredentials::load(currentSSID, sizeof(currentSSID), currentPassword, sizeof(currentPassword)))
            return;

        if (skipHardcodedCredentials)
            return;

        size_t hardcodedSSIDLen = strlen(Config::WIFI_SSID.data());
        if (hardcodedSSIDLen == 0)
            return;

        // Copy but DON'T save to NVS - only save after verified connection
        size_t copyLen = (hardcodedSSIDLen < sizeof(currentSSID) - 1) ? hardcodedSSIDLen : sizeof(currentSSID) - 1;
        memcpy(currentSSID, Config::WIFI_SSID.data(), copyLen);
        currentSSID[copyLen] = '\0';

        size_t hardcodedPassLen = strlen(Config::WIFI_PASS.data());
        copyLen = (hardcodedPassLen < sizeof(currentPassword) - 1) ? hardcodedPassLen : sizeof(currentPassword) - 1;
        memcpy(currentPassword, Config::WIFI_PASS.data(), copyLen);
        currentPassword[copyLen] = '\0';
    }

    bool connectWiFi()
    {
        if (strlen(currentSSID) == 0)
        {
            Serial.println("[Network] No credentials - starting configuration portal");
            portalMode = true;
            WiFiPortal::start();
            return false; // Portal started, but WiFi not connected
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("[WiFi] Already connected!");
            return true;
        }

        Serial.println("\n[WiFi] Connecting...");
        Serial.printf("[WiFi] SSID: %s\n", currentSSID);

        // Full WiFi reset before first attempt
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(WIFI_RESET_DELAY_MS);

        WiFi.mode(WIFI_STA);
        WiFi.persistent(false);
        WiFi.setAutoReconnect(true);
        WiFi.setSleep(false);
        WiFi.setTxPower(WIFI_POWER_17dBm);

        for (int retry = 0; retry < WIFI_MAX_RETRIES; retry++)
        {
            if (retry > 0)
            {
                Serial.println("\n[WiFi] Retrying...");
                WiFi.disconnect(true);
                delay(WIFI_RESET_DELAY_MS);
                WiFi.mode(WIFI_STA);
                WiFi.setSleep(false);
            }

            WiFi.begin(currentSSID, currentPassword);

            constexpr int pollIntervalMs = 100;
            constexpr int maxPolls = WIFI_CONNECT_TIMEOUT_MS / pollIntervalMs;
            for (int i = 0; i < maxPolls; ++i)
            {
                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

                    // Disable WiFi power saving for stable transfers
                    esp_wifi_set_ps(WIFI_PS_NONE);

                    if (!WiFiCredentials::hasCredentials())
                        WiFiCredentials::save(currentSSID, currentPassword);

                    portalMode = false;
                    isRetryPortal = false;
                    connectionAttempts = 0;
                    return true;
                }
                delay(pollIntervalMs);
                if (i % 5 == 0)
                    Serial.print(".");
            }

            Serial.printf("\n[WiFi] Attempt %d failed (status: %d)\n", retry + 1, WiFi.status());
        }

        // Log detailed failure information
        wl_status_t status = WiFi.status();
        Serial.println("\n═════════════════════════════════════════");
        Serial.println("[WiFi] ❌ CONNECTION FAILED");
        Serial.printf("[WiFi] Attempts: %d\n", connectionAttempts + 1);
        Serial.printf("[WiFi] Status Code: %d - ", status);

        switch (status)
        {
        case WL_NO_SSID_AVAIL:
            Serial.println("Network not found (out of range or wrong SSID)");
            break;
        case WL_CONNECT_FAILED:
            Serial.println("Connection failed (wrong password or router blocked)");
            break;
        case WL_CONNECTION_LOST:
            Serial.println("Connection lost");
            break;
        case WL_DISCONNECTED:
            Serial.println("Disconnected (general failure)");
            break;
        default:
            Serial.println("Unknown error");
            break;
        }
        Serial.println("═════════════════════════════════════════\n");

        connectionAttempts++;
        isRetryPortal = true;

        portalMode = true;
        WiFiPortal::start();

        return false;
    }

    void handlePortal()
    {
        if (!portalMode)
        {
            return;
        }
        if (!WiFiPortal::isActive())
        {
            return;
        }

        WiFiPortal::handle();

        if (!WiFiPortal::isConnectionSuccess())
            return;

        char newSSID[33];
        char newPassword[65];
        WiFiPortal::getNewCredentials(newSSID, sizeof(newSSID), newPassword, sizeof(newPassword));

        if (!WiFiCredentials::save(newSSID, newPassword))
        {
            WiFiPortal::clearCredentials();
            return;
        }

        size_t ssidLen = strlen(newSSID);
        size_t passLen = strlen(newPassword);
        memcpy(currentSSID, newSSID, ssidLen + 1);
        memcpy(currentPassword, newPassword, passLen + 1);

        isRetryPortal = false;
        connectionAttempts = 0;

        // Keep portal running so browser can see success
        unsigned long startWait = millis();
        while (millis() - startWait < 5000)
        {
            WiFiPortal::handle();
            delay(10);
        }

        WiFiPortal::stop();
        portalMode = false;
        WiFiPortal::clearCredentials();

        // Ensure WiFi STA is still connected after portal stops
        if (WiFi.status() != WL_CONNECTED)
        {
            WiFi.mode(WIFI_STA);
            WiFi.begin(currentSSID, currentPassword);

            unsigned long startConnect = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startConnect < 10000)
                delay(100);
        }

        // Remount LittleFS
        LittleFS.begin(true);

        syncTime();
        SettingsServer::start();

        // Signal to main.cpp that portal closed with WiFi success
        portalConnectedWiFi = true;
    }

    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    void startPortal()
    {
        portalMode = true;
        WiFiPortal::start();
    }

    void disconnect()
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
        }
    }

    bool isPortalActive()
    {
        return portalMode;
    }

    void stopPortal()
    {
        if (!portalMode)
            return;

        WiFiPortal::stop();
        portalMode = false;
        Serial.println("[Network] Portal stopped");
    }

    bool isRetryConnection()
    {
        return isRetryPortal;
    }

    int getConnectionAttempts()
    {
        return connectionAttempts;
    }

    void syncTime()
    {
        Serial.println("[NTP] Syncing time...");

        // Wait for network stack to stabilize after connection
        delay(1000);

        // Ensure C library timezone is set immediately (Adhan C library relies on localtime/mktime).
        setenv("TZ", Config::TIMEZONE, 1);
        tzset();

        // Configure SNTP with multiple servers for redundancy
        configTzTime(Config::TIMEZONE, Config::NTP_SERVER1, Config::NTP_SERVER2, Config::NTP_SERVER3);

        // Wait up to 30 seconds for NTP sync
        constexpr int ntpPollIntervalMs = 500;
        struct tm timeinfo;
        for (int i = 0; i < NTP_SYNC_TIMEOUT_ATTEMPTS; ++i)
        {
            if (getLocalTime(&timeinfo, ntpPollIntervalMs))
            {
                Serial.printf("[NTP] Time synced: %02d:%02d:%02d\n",
                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                return;
            }
            delay(ntpPollIntervalMs);
        }

        Serial.println("[NTP] Failed to sync time");
    }

    bool didPortalConnectWiFi()
    {
        return portalConnectedWiFi;
    }

    void clearPortalConnectFlag()
    {
        portalConnectedWiFi = false;
    }

} // namespace Network
