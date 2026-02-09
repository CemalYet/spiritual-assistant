#include "boot_manager.h"
#include "network.h"
#include "wifi_credentials.h"
#include "wifi_portal.h"
#include "settings_manager.h"
#include "settings_server.h"
#include "lvgl_display.h"
#include "app_state.h"
#include <WiFi.h>
#include <cmath>

namespace BootManager
{
    static bool wifiConnected = false;

    static bool hasLocation()
    {
        double lat = SettingsManager::getLatitude();
        double lng = SettingsManager::getLongitude();
        constexpr double MIN_VALID = 0.0001;
        return (std::abs(lat) > MIN_VALID || std::abs(lng) > MIN_VALID);
    }

    static bool hasClock()
    {
        struct tm timeinfo;
        return getLocalTime(&timeinfo) && timeinfo.tm_year >= 120;
    }

    static void showStatus(const char *title, const char *line)
    {
        AppStateHelper::showMessage(title, line);
        LvglDisplay::loop();
    }

    static void showPortalStatus()
    {
        AppStateHelper::showPortal(
            WiFiPortal::AP_SSID,
            WiFiPortal::AP_PASSWORD,
            "192.168.4.1");
        LvglDisplay::loop();
    }

    // ── Portal blocking loop ────────────────────────────

    static bool runPortalBlocking()
    {
        Network::startPortal();
        showPortalStatus();

        while (true)
        {
            Network::handlePortal();
            LvglDisplay::loop();

            if (Network::didPortalConnectWiFi())
            {
                Network::clearPortalConnectFlag();
                return true;
            }

            if (!Network::isPortalActive())
                return Network::isConnected();

            if (SettingsManager::needsRecalculation())
            {
                SettingsManager::clearRecalculationFlag();
                Network::stopPortal();
                return Network::isConnected();
            }

            if (WiFiPortal::isOfflineModeRequested())
            {
                WiFiPortal::clearOfflineModeFlag();
                Network::stopPortal();
                return false;
            }

            delay(10);
        }
    }

    // ── WiFi ────────────────────────────────────────────

    static bool tryConnectWiFi()
    {
        if (!WiFiCredentials::hasCredentials())
            return false;

        if (Network::isConnected())
            return true;

        char ssid[33] = "";
        char pass[65] = "";
        WiFiCredentials::load(ssid, sizeof(ssid), pass, sizeof(pass));
        AppStateHelper::showConnecting(ssid);
        LvglDisplay::loop();

        bool ok = Network::connectWiFi();

        if (ok)
        {
            Serial.printf("[Boot] WiFi connected: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }

        Serial.println("[Boot] WiFi failed — starting portal");
        showStatus("Baglanti Basarisiz", "WiFi agina baglanilamadi");
        delay(2000);
        return runPortalBlocking();
    }

    // ── Boot ────────────────────────────────────────────

    bool run()
    {
        Serial.println("[Boot] Starting boot sequence...");

        if (!hasLocation())
        {
            Serial.println("[Boot] No location — first-time setup portal");
            showStatus("Ilk Kurulum", "Cihazi ayarlamak icin telefonunuzu baglayin");
            delay(3000);
            wifiConnected = runPortalBlocking();

            if (wifiConnected && !hasLocation())
            {
                SettingsServer::start();
                etl::string<40> url{"http://"};
                url += WiFi.localIP().toString().c_str();
                showStatus("Ayarlari Yapin", url.c_str());

                while (!hasLocation())
                {
                    SettingsServer::handle();
                    LvglDisplay::loop();

                    if (SettingsManager::needsRecalculation())
                    {
                        SettingsManager::clearRecalculationFlag();
                        if (hasLocation())
                            break;
                    }
                    delay(10);
                }
            }
        }

        // Step 2: WiFi
        bool offlineMode = SettingsManager::isOfflineMode();
        if (!offlineMode && !wifiConnected)
        {
            wifiConnected = tryConnectWiFi();
        }

        // Step 3: NTP
        if (wifiConnected)
        {
            showStatus("Saat Senkronize", "NTP sunucusuna baglaniliyor...");
            Network::syncTime();
        }

        // Step 4: No clock + no WiFi → portal for time sync
        if (!hasClock() && !wifiConnected && !offlineMode)
        {
            Serial.println("[Boot] No clock, no WiFi — starting portal");
            showStatus("Saat Alinamadi", "Saat icin WiFi baglantisi gerekli");
            delay(2000);
            wifiConnected = runPortalBlocking();

            if (wifiConnected && !hasClock())
            {
                Network::syncTime();
            }
        }

        // Step 5: Validate
        if (!hasClock())
        {
            if (offlineMode)
                Serial.println("[Boot] Offline mode, no clock — limited functionality");
            else
                Serial.println("[Boot] WARNING: No clock available");
        }

        AppStateHelper::clearStatusScreen();

        Serial.println("[Boot] Boot sequence complete");
        return hasClock() || offlineMode;
    }

    bool didConnectWiFi()
    {
        return wifiConnected;
    }

} // namespace BootManager
