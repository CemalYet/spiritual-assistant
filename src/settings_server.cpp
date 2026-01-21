#include "settings_server.h"
#include "settings_manager.h"
#include "prayer_types.h"
#include "http_helpers.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace SettingsServer
{
    static WebServer *server = nullptr;
    static bool active = false;
    static constexpr char MDNS_HOSTNAME[] = "spiritualassistantsettings";
    static constexpr char DIYANET_API[] = "https://ezanvakti.emushaf.net";
    static constexpr int PROXY_TIMEOUT = 10000;

    // Forward declarations
    static void serveSettingsPage();
    static void handleGetSettings();
    static void handlePostSettings();
    static void handleNotFound();

    static void sendJson(int code, const String &json)
    {
        server->send(code, "application/json", json);
    }

    static void sendJsonError(int code, const char *message)
    {
        sendJson(code, "{\"error\":\"" + String(message) + "\"}");
    }

    static String extractLastPathSegment(const String &uri)
    {
        int lastSlash = uri.lastIndexOf('/');
        return (lastSlash >= 0) ? uri.substring(lastSlash + 1) : "";
    }

    bool startMDNS()
    {
        if (!MDNS.begin(MDNS_HOSTNAME))
        {
            Serial.println("[mDNS] Failed to start");
            return false;
        }
        Serial.printf("[mDNS] Started: http://%s.local\n", MDNS_HOSTNAME);
        MDNS.addService("http", "tcp", 80);
        return true;
    }

    void start()
    {
        if (server)
            return;

        startMDNS();

        if (!LittleFS.begin(false) && !LittleFS.begin(true))
            Serial.println("[Settings] ERROR: Failed to mount LittleFS!");

        server = new WebServer(80);

        const char *headerkeys[] = {"User-Agent"};
        server->collectHeaders(headerkeys, 1);

        // Routes
        server->on("/", HTTP_GET, serveSettingsPage);
        server->serveStatic("/style.css", LittleFS, "/style.css");
        server->serveStatic("/script.js", LittleFS, "/script.js");
        server->on("/api/settings", HTTP_GET, handleGetSettings);
        server->on("/api/settings", HTTP_POST, handlePostSettings);
        server->onNotFound(handleNotFound);

        HttpHelpers::registerBrowserResourceHandlers(server);

        server->begin();
        active = true;
        Serial.printf("[Settings] Server started at http://%s.local\n", MDNS_HOSTNAME);
    }

    void stop()
    {
        if (!server)
            return;

        server->stop();
        delete server;
        server = nullptr;
        MDNS.end();
        active = false;
        Serial.println("[Settings] Server stopped");
    }

    void handle()
    {
        if (server && active)
            server->handleClient();
    }

    bool isActive() { return active; }
    const char *getHostname() { return MDNS_HOSTNAME; }

    static void serveSettingsPage()
    {
        if (HttpHelpers::serveFile(server, "/settings.html", "text/html", 300)) // 5 min cache
            return;

        server->send(HttpHelpers::HTTP_OK, "text/html",
                     "<html><body><h1>Settings</h1><p>Upload settings.html to LittleFS.</p></body></html>");
    }

    static void handleGetSettings()
    {
        JsonDocument doc;
        doc["prayerMethod"] = SettingsManager::getPrayerMethod();
        doc["methodName"] = SettingsManager::getMethodName(SettingsManager::getPrayerMethod());
        doc["volume"] = SettingsManager::getVolume();

        // Location data
        doc["latitude"] = SettingsManager::getLatitude();
        doc["longitude"] = SettingsManager::getLongitude();
        doc["cityName"] = SettingsManager::getCityName();
        int32_t diyanetId = SettingsManager::getDiyanetId();
        if (diyanetId > 0)
            doc["diyanetId"] = diyanetId;

        JsonObject adhan = doc["adhanEnabled"].to<JsonObject>();
        adhan["fajr"] = SettingsManager::getAdhanEnabled(PrayerType::Fajr);
        adhan["dhuhr"] = SettingsManager::getAdhanEnabled(PrayerType::Dhuhr);
        adhan["asr"] = SettingsManager::getAdhanEnabled(PrayerType::Asr);
        adhan["maghrib"] = SettingsManager::getAdhanEnabled(PrayerType::Maghrib);
        adhan["isha"] = SettingsManager::getAdhanEnabled(PrayerType::Isha);

        String response;
        serializeJson(doc, response);
        sendJson(HttpHelpers::HTTP_OK, response);
    }

    static void handlePostSettings()
    {
        if (!server->hasArg("plain"))
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "No body");

        JsonDocument doc;
        if (deserializeJson(doc, server->arg("plain")))
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "Invalid JSON");

        bool changed = false;

        // Prayer method
        if (doc["prayerMethod"].is<int>())
            changed |= SettingsManager::setPrayerMethod(doc["prayerMethod"].as<int>());

        // Volume (0-100)
        if (doc["volume"].is<int>())
        {
            int vol = doc["volume"].as<int>();
            if (vol >= 0 && vol <= 100)
                changed |= SettingsManager::setVolume(static_cast<uint8_t>(vol));
        }

        // Adhan toggles
        if (doc["adhanEnabled"].is<JsonObject>())
        {
            JsonObject adhan = doc["adhanEnabled"];
            static constexpr struct
            {
                const char *key;
                PrayerType type;
            } prayers[] = {
                {"fajr", PrayerType::Fajr}, {"dhuhr", PrayerType::Dhuhr}, {"asr", PrayerType::Asr}, {"maghrib", PrayerType::Maghrib}, {"isha", PrayerType::Isha}};

            for (const auto &p : prayers)
            {
                if (adhan[p.key].is<bool>())
                {
                    SettingsManager::setAdhanEnabled(p.type, adhan[p.key].as<bool>());
                    changed = true;
                }
            }
        }

        // Location data
        if (doc["latitude"].is<double>() && doc["longitude"].is<double>())
        {
            changed |= SettingsManager::setLocation(
                doc["latitude"].as<double>(),
                doc["longitude"].as<double>());
        }

        if (doc["cityName"].is<const char *>())
            changed |= SettingsManager::setCityName(doc["cityName"].as<const char *>());

        if (doc["diyanetId"].is<int>())
            changed |= SettingsManager::setDiyanetId(doc["diyanetId"].as<int32_t>());

        if (!changed)
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "No valid settings");

        // Success response
        JsonDocument response;
        response["success"] = true;
        response["prayerMethod"] = SettingsManager::getPrayerMethod();
        response["methodName"] = SettingsManager::getMethodName(SettingsManager::getPrayerMethod());
        response["volume"] = SettingsManager::getVolume();

        String responseStr;
        serializeJson(response, responseStr);
        sendJson(HttpHelpers::HTTP_OK, responseStr);
    }

    static void proxyDiyanet(const String &endpoint)
    {
        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;
        http.useHTTP10(true);
        http.begin(client, String(DIYANET_API) + endpoint);
        http.setTimeout(PROXY_TIMEOUT);

        int httpCode = http.GET();
        Serial.printf("[Diyanet] %s -> %d\n", endpoint.c_str(), httpCode);

        if (httpCode != HTTP_CODE_OK)
        {
            http.end();
            return sendJsonError(HttpHelpers::HTTP_BAD_GATEWAY, "API request failed");
        }

        server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        server->sendHeader("Content-Type", "application/json");
        server->sendHeader("Cache-Control", "public, max-age=86400");
        server->send(HttpHelpers::HTTP_OK);

        WiFiClient *stream = http.getStreamPtr();
        uint8_t buffer[512];
        while (http.connected() && stream->available())
        {
            size_t size = stream->readBytes(buffer, sizeof(buffer));
            server->sendContent((const char *)buffer, size);
        }
        server->sendContent("");
        http.end();
    }

    static void handleNotFound()
    {
        const String &uri = server->uri();

        // Diyanet API routes
        if (uri.startsWith("/api/diyanet/"))
        {
            if (uri.startsWith("/api/diyanet/ulkeler"))
                return proxyDiyanet("/ulkeler");

            String id = extractLastPathSegment(uri);
            if (id.isEmpty())
                return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "Missing ID");

            if (uri.startsWith("/api/diyanet/sehirler/"))
                return proxyDiyanet("/sehirler/" + id);

            if (uri.startsWith("/api/diyanet/ilceler/"))
                return proxyDiyanet("/ilceler/" + id);
        }

        // Default: serve settings page
        serveSettingsPage();
    }

} // namespace SettingsServer
