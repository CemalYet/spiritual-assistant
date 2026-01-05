#include "wifi_credentials.h"
#include <Preferences.h>
#include <Arduino.h>
#include <cstring>

namespace WiFiCredentials
{
    static Preferences preferences;
    constexpr const char *NAMESPACE = "wifi";
    constexpr const char *KEY_SSID = "ssid";
    constexpr const char *KEY_PASSWORD = "pass";
    constexpr const char *KEY_CONFIGURED = "configured";

    bool init()
    {
        Serial.println("[WiFiCreds] Storage initialized");
        return true;
    }

    bool hasCredentials()
    {
        preferences.begin(NAMESPACE, true);
        bool configured = preferences.getBool(KEY_CONFIGURED, false);
        preferences.end();
        return configured;
    }

    bool save(const char *ssid, const char *password)
    {
        size_t ssidLen = strlen(ssid);
        size_t passLen = strlen(password);

        if (ssidLen == 0 || ssidLen > 32)
        {
            Serial.println("[WiFiCreds] Invalid SSID length");
            return false;
        }

        if (passLen < 8 || passLen > 64)
        {
            Serial.println("[WiFiCreds] Invalid password length");
            return false;
        }

        Serial.printf("[WiFiCreds] Attempting to save: SSID='%s' (%d chars), Pass length=%d\n",
                      ssid, ssidLen, passLen);

        if (!preferences.begin(NAMESPACE, false))
        {
            Serial.println("[WiFiCreds] ERROR: Failed to open NVS namespace!");
            return false;
        }

        size_t ssidWritten = preferences.putString(KEY_SSID, ssid);
        Serial.printf("[WiFiCreds] SSID write: %d bytes\n", ssidWritten);

        size_t passWritten = preferences.putString(KEY_PASSWORD, password);
        Serial.printf("[WiFiCreds] Password write: %d bytes\n", passWritten);

        bool configWritten = preferences.putBool(KEY_CONFIGURED, true);
        Serial.printf("[WiFiCreds] Config flag write: %s\n", configWritten ? "OK" : "FAILED");

        preferences.end();

        bool success = (ssidWritten > 0) && (passWritten > 0) && configWritten;

        if (success)
        {
            Serial.println("[WiFiCreds] ✓ Credentials saved successfully");
            Serial.printf("[WiFiCreds] SSID: %s\n", ssid);
        }
        else
        {
            Serial.println("[WiFiCreds] ✗ Failed to save credentials!");
        }

        return success;
    }

    bool load(char *ssidBuffer, size_t ssidSize, char *passBuffer, size_t passSize)
    {
        if (!hasCredentials())
        {
            Serial.println("[WiFiCreds] No credentials stored");
            return false;
        }

        preferences.begin(NAMESPACE, true);

        size_t ssidLen = preferences.getString(KEY_SSID, ssidBuffer, ssidSize);
        size_t passLen = preferences.getString(KEY_PASSWORD, passBuffer, passSize);

        preferences.end();

        if (ssidLen == 0)
        {
            Serial.println("[WiFiCreds] Failed to load credentials");
            return false;
        }

        Serial.println("[WiFiCreds] Credentials loaded successfully");
        Serial.printf("[WiFiCreds] SSID: %s\n", ssidBuffer);
        return true;
    }

    void clear()
    {
        preferences.begin(NAMESPACE, false);
        preferences.clear();
        preferences.end();
        Serial.println("[WiFiCreds] Credentials cleared");
    }
}
