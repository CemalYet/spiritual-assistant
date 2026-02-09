#include "portal_handler.h"
#include "network.h"
#include "wifi_portal.h"
#include "settings_manager.h"
#include "prayer_engine.h"
#include "wifi_manager.h"
#include "app_state.h"
#include <Arduino.h>

namespace PortalHandler
{
    static bool s_active = false;

    void open()
    {
        Serial.println("[Portal] Opening runtime portal");
        SettingsManager::clearRecalculationFlag();
        WiFiPortal::clearOfflineModeFlag();

        Network::startPortal();
        s_active = true;
        AppStateHelper::setWifiState(WifiState::PORTAL);
    }

    void tick()
    {
        if (!s_active)
            return;

        if (Network::didPortalConnectWiFi())
        {
            Network::clearPortalConnectFlag();
            s_active = false;

            SettingsManager::clearRecalculationFlag(); // Prevent duplicate recalc in PrayerEngine::tick()
            Network::syncTime();
            WifiManager::init(true);
            PrayerEngine::recalculate();
            Serial.println("[Portal] WiFi connected, returning to normal");
            return;
        }

        if (!Network::isPortalActive())
        {
            s_active = false;
            AppStateHelper::setWifiState(WifiState::DISCONNECTED);
            return;
        }

        Network::handlePortal();

        if (SettingsManager::needsRecalculation())
        {
            SettingsManager::clearRecalculationFlag();
            Network::stopPortal();
            s_active = false;

            if (Network::isConnected())
            {
                Network::syncTime();
                WifiManager::init(true);
            }
            else
            {
                AppStateHelper::setWifiState(WifiState::DISCONNECTED);
            }

            PrayerEngine::recalculate();
            Serial.println("[Portal] Settings saved, recalculated");
            return;
        }

        if (WiFiPortal::isOfflineModeRequested())
        {
            WiFiPortal::clearOfflineModeFlag();
            Network::stopPortal();
            s_active = false;

            AppStateHelper::setWifiState(WifiState::DISCONNECTED);
            PrayerEngine::recalculate();
            Serial.println("[Portal] Offline mode selected");
            return;
        }
    }

    bool isActive()
    {
        return s_active;
    }

} // namespace PortalHandler
