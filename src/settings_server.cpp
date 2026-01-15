#include "settings_server.h"
#include "settings_manager.h"
#include "prayer_types.h"
#include "http_helpers.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

namespace SettingsServer
{
    static WebServer *server = nullptr;
    static bool active = false;
    static const char *MDNS_HOSTNAME = "spiritualassistantsettings";

    // Forward declarations
    static void handleGetSettings();
    static void handlePostSettings();
    static void serveSettingsPage();

    bool startMDNS()
    {
        if (MDNS.begin(MDNS_HOSTNAME))
        {
            Serial.printf("[mDNS] Started: http://%s.local\n", MDNS_HOSTNAME);
            MDNS.addService("http", "tcp", 80);
            return true;
        }
        Serial.println("[mDNS] Failed to start");
        return false;
    }

    void start()
    {
        if (server != nullptr)
        {
            return; // Already running
        }

        // Start mDNS first
        startMDNS();

        // Mount LittleFS if not already mounted
        if (!LittleFS.begin(false))
        {
            Serial.println("[Settings] Warning: LittleFS not mounted, attempting mount...");
            if (!LittleFS.begin(true))
            {
                Serial.println("[Settings] ERROR: Failed to mount LittleFS!");
            }
        }

        server = new WebServer(80);

        // Serve settings page
        server->on("/", HTTP_GET, serveSettingsPage);

        // Serve static files
        server->serveStatic("/style.css", LittleFS, "/style.css");
        server->serveStatic("/script.js", LittleFS, "/script.js");

        // Register common browser resource handlers (favicon, apple-touch-icon)
        HttpHelpers::registerBrowserResourceHandlers(server);

        // API: Get current settings
        server->on("/api/settings", HTTP_GET, handleGetSettings);

        // API: Update settings
        server->on("/api/settings", HTTP_POST, handlePostSettings);

        // Catch-all handler - serve settings page for unknown paths
        server->onNotFound(serveSettingsPage);

        server->begin();
        active = true;
        Serial.println("[Settings] Server started on port 80");
        Serial.printf("[Settings] Available at: http://%s.local\n", MDNS_HOSTNAME);
    }

    void stop()
    {
        if (server != nullptr)
        {
            server->stop();
            delete server;
            server = nullptr;
        }

        MDNS.end();
        active = false;
        Serial.println("[Settings] Server stopped");
    }

    void handle()
    {
        if (server != nullptr && active)
        {
            server->handleClient();
        }
    }

    bool isActive()
    {
        return active;
    }

    const char *getHostname()
    {
        return MDNS_HOSTNAME;
    }

    // ─────────────────────────────────────────────────────────────
    // Internal handlers
    // ─────────────────────────────────────────────────────────────

    static void serveSettingsPage()
    {
        if (!HttpHelpers::serveFile(server, "/settings.html", "text/html", 0))
        {
            server->send(200, "text/html",
                         "<html><body><h1>Settings</h1><p>Settings page not found. Please upload settings.html to LittleFS.</p></body></html>");
        }
    }

    static void handleGetSettings()
    {
        int method = SettingsManager::getPrayerMethod();
        const char *methodName = SettingsManager::getMethodName(method);
        uint8_t volume = SettingsManager::getVolume();

        JsonDocument doc;
        doc["prayerMethod"] = method;
        doc["methodName"] = methodName;
        doc["volume"] = volume;

        // Adhan enabled for each prayer (excluding Sunrise)
        JsonObject adhan = doc["adhanEnabled"].to<JsonObject>();
        adhan["fajr"] = SettingsManager::getAdhanEnabled(PrayerType::Fajr);
        adhan["dhuhr"] = SettingsManager::getAdhanEnabled(PrayerType::Dhuhr);
        adhan["asr"] = SettingsManager::getAdhanEnabled(PrayerType::Asr);
        adhan["maghrib"] = SettingsManager::getAdhanEnabled(PrayerType::Maghrib);
        adhan["isha"] = SettingsManager::getAdhanEnabled(PrayerType::Isha);

        String response;
        serializeJson(doc, response);
        server->send(200, "application/json", response);
        Serial.printf("[Settings API] GET - Method: %d, Volume: %d%%\n", method, volume);
    }

    static void handlePostSettings()
    {
        if (!server->hasArg("plain"))
        {
            server->send(400, "application/json", "{\"error\":\"No body\"}");
            return;
        }

        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error)
        {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        bool changed = false;

        // Prayer method
        if (doc["prayerMethod"].is<int>())
        {
            int newMethod = doc["prayerMethod"];
            if (SettingsManager::setPrayerMethod(newMethod))
            {
                changed = true;
                Serial.printf("[Settings API] Method changed to: %d\n", newMethod);
            }
        }

        // Volume
        if (doc["volume"].is<int>())
        {
            int newVolume = doc["volume"];
            if (newVolume >= 0 && newVolume <= 100)
            {
                if (SettingsManager::setVolume(static_cast<uint8_t>(newVolume)))
                {
                    changed = true;
                    Serial.printf("[Settings API] Volume changed to: %d%%\n", newVolume);
                }
            }
        }

        // Adhan enabled settings
        if (doc["adhanEnabled"].is<JsonObject>())
        {
            JsonObject adhan = doc["adhanEnabled"];
            if (adhan["fajr"].is<bool>())
            {
                SettingsManager::setAdhanEnabled(PrayerType::Fajr, adhan["fajr"]);
                changed = true;
            }
            if (adhan["dhuhr"].is<bool>())
            {
                SettingsManager::setAdhanEnabled(PrayerType::Dhuhr, adhan["dhuhr"]);
                changed = true;
            }
            if (adhan["asr"].is<bool>())
            {
                SettingsManager::setAdhanEnabled(PrayerType::Asr, adhan["asr"]);
                changed = true;
            }
            if (adhan["maghrib"].is<bool>())
            {
                SettingsManager::setAdhanEnabled(PrayerType::Maghrib, adhan["maghrib"]);
                changed = true;
            }
            if (adhan["isha"].is<bool>())
            {
                SettingsManager::setAdhanEnabled(PrayerType::Isha, adhan["isha"]);
                changed = true;
            }
        }

        if (changed)
        {
            int method = SettingsManager::getPrayerMethod();
            const char *methodName = SettingsManager::getMethodName(method);

            JsonDocument response;
            response["success"] = true;
            response["prayerMethod"] = method;
            response["methodName"] = methodName;
            response["volume"] = SettingsManager::getVolume();
            response["message"] = "Settings saved successfully.";

            String responseStr;
            serializeJson(response, responseStr);
            server->send(200, "application/json", responseStr);
        }
        else
        {
            server->send(400, "application/json", "{\"error\":\"No valid settings provided\"}");
        }
    }

} // namespace SettingsServer
