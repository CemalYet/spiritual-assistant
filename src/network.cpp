#include "network.h"
#include "config.h"
#include "wifi_portal.h"
#include "wifi_credentials.h"
#include "settings_server.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <stdlib.h>
#include <time.h>

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
    static bool isRetryPortal = false; // True if portal opened after failed connection

    void init()
    {
        Serial.println("[Network] Initializing...");

        WiFiCredentials::init();

        if (WiFiCredentials::load(currentSSID, sizeof(currentSSID), currentPassword, sizeof(currentPassword)))
        {
            Serial.println("[Network] Found stored WiFi credentials");
            return;
        }

        size_t hardcodedSSIDLen = strlen(Config::WIFI_SSID.data());
        if (hardcodedSSIDLen == 0)
        {
            Serial.println("[Network] No WiFi credentials found - portal required");
            return;
        }

        Serial.println("[Network] Using hardcoded credentials from config.h");
        Serial.println("[Network] Note: These will only be saved after successful connection");

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
            return WiFiPortal::start();
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
        WiFi.persistent(false);      // Don't write to flash (faster, less wear)
        WiFi.setAutoReconnect(true); // Auto-reconnect if connection drops
        WiFi.setSleep(false);        // Disable sleep for stable connection

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

                    if (!WiFiCredentials::hasCredentials())
                    {
                        Serial.println("[WiFi] Saving verified credentials to NVS");
                        WiFiCredentials::save(currentSSID, currentPassword);
                    }

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

        Serial.println("[Network] Opening portal for reconfiguration...");
        Serial.println("[Network] Please reconnect to the AP and enter credentials again\n");

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

        Serial.println("[Network] ✓ Connection test passed!");

        if (!WiFiCredentials::save(newSSID, newPassword))
        {
            Serial.println("[Network] Failed to save credentials");
            WiFiPortal::clearCredentials();
            return;
        }

        size_t ssidLen = strlen(newSSID);
        size_t passLen = strlen(newPassword);
        memcpy(currentSSID, newSSID, ssidLen + 1);
        memcpy(currentPassword, newPassword, passLen + 1);

        isRetryPortal = false;
        connectionAttempts = 0;

        // Keep portal running so browser can see success status
        Serial.println("[Network] Allowing browser to show success...");
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
            Serial.println("[Network] Reconnecting to WiFi after portal...");
            WiFi.mode(WIFI_STA);
            WiFi.begin(currentSSID, currentPassword);

            unsigned long startConnect = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startConnect < 10000)
            {
                delay(100);
            }

            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.printf("[Network] Reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
            }
            else
            {
                Serial.println("[Network] Failed to reconnect WiFi");
            }
        }

        // Remount LittleFS (portal may have unmounted it)
        if (!LittleFS.begin(true))
        {
            Serial.println("[Network] Warning: LittleFS remount failed");
        }

        Serial.println("[Network] Portal complete - syncing time...");
        syncTime();

        Serial.println("[Network] Starting settings server...");
        SettingsServer::start();

        Serial.println("[Network] ✓ Setup complete! Device is ready.");
        Serial.printf("[Network] Settings available at: http://%s.local\n", SettingsServer::getHostname());
    }

    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    bool isPortalActive()
    {
        return portalMode;
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
        Serial.println("[NTP] Syncing time with automatic DST...");

        // Wait for network stack to stabilize after connection
        delay(1000);

        // Ensure C library timezone is set immediately (Adhan C library relies on localtime/mktime).
        setenv("TZ", Config::TIMEZONE, 1);
        tzset();

        // Also configure SNTP with the same TZ string.
        configTzTime(Config::TIMEZONE, Config::NTP_SERVER);

        // Wait up to 30 seconds for NTP sync
        constexpr int ntpPollIntervalMs = 500;
        struct tm timeinfo;
        for (int i = 0; i < NTP_SYNC_TIMEOUT_ATTEMPTS; ++i)
        {
            if (getLocalTime(&timeinfo, ntpPollIntervalMs))
            {
                Serial.printf("[NTP] Time synced: %02d:%02d:%02d\n",
                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                Serial.printf("[NTP] Timezone: %s (DST: %s)\n",
                              Config::TIMEZONE, timeinfo.tm_isdst ? "Active" : "Inactive");
                return;
            }
            delay(ntpPollIntervalMs);
        }

        Serial.println("[NTP] Failed to sync time");
    }

} // namespace Network
