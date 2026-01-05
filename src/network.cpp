#include "network.h"
#include "config.h"
#include "wifi_portal.h"
#include "wifi_credentials.h"
#include <WiFi.h>
#include <stdlib.h>
#include <time.h>

namespace Network
{
    static bool portalMode = false;
    static char currentSSID[33] = "";     // Max SSID: 32 chars + null terminator
    static char currentPassword[65] = ""; // Max WPA2: 64 chars + null terminator

    void init()
    {
        Serial.println("[Network] Initializing...");

        WiFiCredentials::init();

        // Try to load stored credentials directly into stack buffers
        if (WiFiCredentials::load(currentSSID, sizeof(currentSSID), currentPassword, sizeof(currentPassword)))
        {
            Serial.println("[Network] Found stored WiFi credentials");
            return;
        }

        // Check if hardcoded credentials exist (from config.h)
        size_t hardcodedSSIDLen = strlen(Config::WIFI_SSID.data());
        if (hardcodedSSIDLen == 0)
        {
            Serial.println("[Network] No WiFi credentials found - portal required");
            return;
        }

        Serial.println("[Network] Using hardcoded credentials from config.h");

        // Direct copy with bounds checking
        size_t copyLen = (hardcodedSSIDLen < sizeof(currentSSID) - 1) ? hardcodedSSIDLen : sizeof(currentSSID) - 1;
        memcpy(currentSSID, Config::WIFI_SSID.data(), copyLen);
        currentSSID[copyLen] = '\0';

        size_t hardcodedPassLen = strlen(Config::WIFI_PASS.data());
        copyLen = (hardcodedPassLen < sizeof(currentPassword) - 1) ? hardcodedPassLen : sizeof(currentPassword) - 1;
        memcpy(currentPassword, Config::WIFI_PASS.data(), copyLen);
        currentPassword[copyLen] = '\0';

        WiFiCredentials::save(currentSSID, currentPassword);
    }

    bool connectWiFi()
    {
        // If no credentials available, start portal
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
        delay(500); // Let WiFi chip fully reset

        WiFi.mode(WIFI_STA);
        WiFi.persistent(false);      // Don't write to flash (faster, less wear)
        WiFi.setAutoReconnect(true); // Auto-reconnect if connection drops
        WiFi.setSleep(false);        // Disable sleep mode for stable connection

        // Try up to 3 times with full reset between attempts
        for (int retry = 0; retry < 3; retry++)
        {
            if (retry > 0)
            {
                Serial.println("\n[WiFi] Retrying...");
                WiFi.disconnect(true); // true = full reset (wifioff)
                delay(500);            // Increased delay for retry
                WiFi.mode(WIFI_STA);
                WiFi.setSleep(false);
            }

            WiFi.begin(currentSSID, currentPassword);

            // Wait up to 15 seconds, checking every 100ms
            for (int i = 0; i < 150; ++i)
            {
                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
                    portalMode = false;
                    return true;
                }
                delay(100);
                if (i % 5 == 0)
                    Serial.print(".");
            }

            Serial.printf("\n[WiFi] Attempt %d failed (status: %d)\n", retry + 1, WiFi.status());
        }

        Serial.println("[WiFi] Connection failed after 3 attempts");

        // If connection failed, start portal for reconfiguration
        Serial.println("[Network] Starting portal for reconfiguration...");
        portalMode = true;
        WiFiPortal::start();

        return false;
    }

    void handlePortal()
    {
        if (!portalMode || !WiFiPortal::isActive())
            return;

        WiFiPortal::handle();

        if (!WiFiPortal::hasNewCredentials())
            return;

        char newSSID[33];
        char newPassword[65];

        WiFiPortal::getNewCredentials(newSSID, sizeof(newSSID), newPassword, sizeof(newPassword));

        Serial.println("[Network] New credentials received from portal");

        if (!WiFiCredentials::save(newSSID, newPassword))
        {
            Serial.println("[Network] Failed to save credentials");
            WiFiPortal::clearCredentials();
            return;
        }

        // Direct copy to current buffers
        size_t ssidLen = strlen(newSSID);
        size_t passLen = strlen(newPassword);

        memcpy(currentSSID, newSSID, ssidLen + 1);
        memcpy(currentPassword, newPassword, passLen + 1);

        // Keep portal running for 3 seconds so browser can load success page
        Serial.println("[Network] Allowing success page to load...");
        unsigned long startWait = millis();
        while (millis() - startWait < 3000)
        {
            WiFiPortal::handle(); // Keep serving requests
            delay(10);
        }

        WiFiPortal::stop();
        portalMode = false;
        WiFiPortal::clearCredentials();

        Serial.println("[Network] Restarting in 2 seconds...");
        delay(2000);
        ESP.restart();
    }

    bool isConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    bool isPortalActive()
    {
        return portalMode;
    }

    void syncTime()
    {

        Serial.println("[NTP] Syncing time with automatic DST...");

        // Ensure C library timezone is set immediately (Adhan C library relies on localtime/mktime).
        setenv("TZ", Config::TIMEZONE, 1);
        tzset();

        // Also configure SNTP with the same TZ string.
        configTzTime(Config::TIMEZONE, Config::NTP_SERVER);

        // Wait up to 30 seconds for NTP sync
        struct tm timeinfo;
        for (int i = 0; i < 60; ++i)
        {
            if (getLocalTime(&timeinfo, 500))
            {
                Serial.printf("[NTP] Time synced: %02d:%02d:%02d\n",
                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                Serial.printf("[NTP] Timezone: %s (DST: %s)\n",
                              Config::TIMEZONE, timeinfo.tm_isdst ? "Active" : "Inactive");
                return;
            }
            delay(500);
        }

        Serial.println("[NTP] Failed to sync time");
    }

} // namespace Network
