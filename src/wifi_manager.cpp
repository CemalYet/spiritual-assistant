#include "wifi_manager.h"
#include "network.h"
#include "settings_server.h"
#include "settings_manager.h"
#include "wifi_credentials.h"
#include "app_state.h"
#include <Arduino.h>
#include <WiFi.h>

namespace WifiManager
{
    constexpr unsigned long WIFI_TIMEOUT_MS = 5 * 60 * 1000;
    constexpr unsigned long WIFI_RECONNECT_TIMEOUT_MS = 15000;
    constexpr unsigned long WIFI_FAILED_DISPLAY_MS = 3000;

    enum class State
    {
        IDLE,
        CONNECTED,
        RECONNECTING,
        SHOW_FAILED,
        DISCONNECTED_BY_TIMEOUT
    };

    static State s_state = State::IDLE;
    static unsigned long s_connectedAt = 0;
    static unsigned long s_stateEnteredAt = 0;
    static bool s_wasConnected = false;

    static void enterState(State newState)
    {
        s_state = newState;
        s_stateEnteredAt = millis();
    }

    void init(bool connected)
    {
        if (connected)
        {
            enterState(State::CONNECTED);
            s_connectedAt = millis();
            SettingsServer::start();
            Serial.println("[WiFi] Connected (auto-disconnect in 5 min)");
            AppStateHelper::setWifiState(WifiState::CONNECTED,
                                         WiFi.localIP().toString().c_str());
        }
        else
        {
            enterState(State::IDLE);
            AppStateHelper::setWifiState(WifiState::DISCONNECTED);
        }
        s_wasConnected = connected;
    }

    void tick()
    {
        switch (s_state)
        {
        case State::RECONNECTING:
        {
            if (Network::isConnected())
            {
                enterState(State::CONNECTED);
                s_connectedAt = millis();
                String ip = WiFi.localIP().toString();
                Serial.printf("[WiFi] Reconnected: %s\n", ip.c_str());
                SettingsServer::start();
                AppStateHelper::setWifiState(WifiState::CONNECTED, ip.c_str());
            }
            else if (millis() - s_stateEnteredAt > WIFI_RECONNECT_TIMEOUT_MS)
            {
                enterState(State::SHOW_FAILED);
                Serial.println("[WiFi] Reconnect timeout");
                Network::disconnect();
                AppStateHelper::setWifiState(WifiState::FAILED);
            }
            return;
        }

        case State::SHOW_FAILED:
        {
            if (millis() - s_stateEnteredAt > WIFI_FAILED_DISPLAY_MS)
            {
                enterState(State::IDLE);
                AppStateHelper::setWifiState(WifiState::DISCONNECTED);
            }
            return;
        }

        case State::CONNECTED:
        {
            if (millis() - s_connectedAt > WIFI_TIMEOUT_MS)
            {
                disconnect();
                return;
            }
            break;
        }

        default:
            break;
        }

        // Drop detection (all states except RECONNECTING/SHOW_FAILED)
        bool isConnectedNow = Network::isConnected();
        if (s_wasConnected && !isConnectedNow)
        {
            Serial.println("[WiFi] Connection lost");
            if (SettingsServer::isActive())
                SettingsServer::stop();
            enterState(State::DISCONNECTED_BY_TIMEOUT);
            AppStateHelper::setWifiState(WifiState::DISCONNECTED);
        }
        s_wasConnected = isConnectedNow;
    }

    void reconnect()
    {
        if (Network::isConnected())
        {
            enterState(State::CONNECTED);
            s_connectedAt = millis();

            if (!SettingsServer::isActive())
                SettingsServer::start();

            AppStateHelper::setWifiState(WifiState::CONNECTED,
                                         WiFi.localIP().toString().c_str());
            return;
        }

        Serial.println("[WiFi] Reconnecting...");
        AppStateHelper::setWifiState(WifiState::CONNECTING);
        WiFi.mode(WIFI_STA);

        char ssid[33] = "";
        char pass[65] = "";
        if (WiFiCredentials::load(ssid, sizeof(ssid), pass, sizeof(pass)))
        {
            WiFi.begin(ssid, pass);
            enterState(State::RECONNECTING);
        }
        else
        {
            enterState(State::IDLE);
            AppStateHelper::setWifiState(WifiState::DISCONNECTED);
        }
    }

    void disconnect()
    {
        if (!Network::isConnected())
            return;

        Serial.println("[WiFi] Auto-disconnect: timeout");
        SettingsServer::stop();
        Network::disconnect();
        enterState(State::DISCONNECTED_BY_TIMEOUT);
        AppStateHelper::setWifiState(WifiState::DISCONNECTED);
    }

    void resetTimeout()
    {
        s_connectedAt = millis();
    }

} // namespace WifiManager
