#include "settings_server.h"
#include "settings_manager.h"
#include "prayer_types.h"
#include "http_helpers.h"
#include "prayer_api.h"
#include "audio_player.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace SettingsServer
{
    static WebServer *server = nullptr;
    static bool active = false;
    static constexpr char DIYANET_API[] = "https://ezanvakti.emushaf.net";
    static constexpr int PROXY_TIMEOUT = 10000;

    // Forward declarations
    static void serveSettingsPage();
    static void handleGetSettings();
    static void handlePostSettings();
    static void handleGetStatus();
    static void handleRefresh();
    static void handleTestAdhan();
    static void handleTestAudio();
    static void handleStopAdhan();
    static void handleNotFound();
    static void checkTestAudioTimeout();

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

    void start()
    {
        if (server)
            return;

        // mDNS removed - use IP address directly

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
        server->on("/api/status", HTTP_GET, handleGetStatus);
        server->on("/api/refresh", HTTP_POST, handleRefresh);
        server->on("/api/test-adhan", HTTP_POST, handleTestAdhan);
        server->on("/api/test-audio", HTTP_GET, handleTestAudio);
        server->on("/api/stop-adhan", HTTP_POST, handleStopAdhan);
        server->onNotFound(handleNotFound);

        HttpHelpers::registerBrowserResourceHandlers(server);

        server->begin();
        active = true;
        Serial.printf("[Settings] Server started at http://%s\n", WiFi.localIP().toString().c_str());
    }

    void stop()
    {
        if (!server)
            return;

        server->stop();
        delete server;
        server = nullptr;
        active = false;
        Serial.println("[Settings] Server stopped");
    }

    void handle()
    {
        if (server && active)
        {
            server->handleClient();
            checkTestAudioTimeout();
        }
    }

    bool isActive() { return active; }

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

    static void handleGetStatus()
    {
        JsonDocument doc;

        // WiFi status
        JsonObject wifi = doc["wifi"].to<JsonObject>();
        wifi["connected"] = WiFi.isConnected();
        wifi["ssid"] = WiFi.SSID();
        wifi["rssi"] = WiFi.RSSI();
        wifi["ip"] = WiFi.localIP().toString();

        // Time status
        JsonObject timeObj = doc["time"].to<JsonObject>();
        struct tm timeinfo;
        bool timeValid = getLocalTime(&timeinfo, 100);
        timeObj["synced"] = timeValid;
        if (timeValid)
        {
            char timeStr[9];
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
            timeObj["deviceTime"] = timeStr;

            // Calculate UTC offset correctly (handles midnight boundary)
            time_t now = ::time(nullptr);
            struct tm utc;
            gmtime_r(&now, &utc);
            time_t localEpoch = mktime(&timeinfo);
            time_t utcEpoch = mktime(&utc);
            int offsetHours = (int)((localEpoch - utcEpoch) / 3600);

            timeObj["timezone"] = "Local";
            timeObj["utcOffset"] = offsetHours;
        }
        else
        {
            timeObj["deviceTime"] = "--:--:--";
            timeObj["timezone"] = "Not synced";
            timeObj["utcOffset"] = 0;
        }

        // Prayer status
        JsonObject prayer = doc["prayer"].to<JsonObject>();
        int method = SettingsManager::getPrayerMethod();
        prayer["method"] = method;
        prayer["methodName"] = SettingsManager::getMethodName(method);

        if (method == PRAYER_METHOD_DIYANET)
        {
            PrayerAPI::CacheInfo cache = PrayerAPI::getCacheInfo();
            prayer["diyanetOk"] = cache.isValid;
            prayer["daysRemaining"] = cache.daysRemaining;
            prayer["usingFallback"] = !cache.isValid;
        }
        else
        {
            prayer["diyanetOk"] = true; // Not using Diyanet
            prayer["daysRemaining"] = -1;
            prayer["usingFallback"] = false;
        }

        String response;
        serializeJson(doc, response);
        sendJson(HttpHelpers::HTTP_OK, response);
    }

    static void handleRefresh()
    {
        int method = SettingsManager::getPrayerMethod();
        if (method != PRAYER_METHOD_DIYANET)
        {
            sendJson(HttpHelpers::HTTP_OK, "{\"success\":true,\"message\":\"Not using Diyanet\"}");
            return;
        }

        int ilceId = SettingsManager::getDiyanetId();
        if (ilceId <= 0)
        {
            sendJson(HttpHelpers::HTTP_BAD_REQUEST, "{\"success\":false,\"error\":\"No location configured\"}");
            return;
        }

        bool success = PrayerAPI::fetchMonthlyPrayerTimes(ilceId);
        if (success)
        {
            sendJson(HttpHelpers::HTTP_OK, "{\"success\":true,\"message\":\"Prayer times refreshed\"}");
        }
        else
        {
            sendJson(HttpHelpers::HTTP_INTERNAL_ERROR, "{\"success\":false,\"error\":\"Failed to fetch from Diyanet\"}");
        }
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

    static unsigned long testAudioStopTime = 0;
    static constexpr unsigned long TEST_AUDIO_DURATION_MS = 5000;

    static bool startTestAudio()
    {
        if (playAudioFile("/azan.mp3"))
        {
            testAudioStopTime = millis() + TEST_AUDIO_DURATION_MS;
            return true;
        }
        return false;
    }

    static void handleTestAdhan()
    {
        if (startTestAudio())
            sendJson(200, "{\"success\":true,\"message\":\"Playing 5 sec preview\"}");
        else
            sendJsonError(500, "Failed to play adhan");
    }

    static void handleTestAudio()
    {
        if (server->hasArg("volume"))
        {
            int vol = server->arg("volume").toInt();
            if (vol >= 0 && vol <= 100)
                setVolume(vol * 21 / 100);
        }

        if (startTestAudio())
            sendJson(200, "{\"success\":true,\"message\":\"Playing 5 sec preview\"}");
        else
            sendJsonError(500, "Failed to play audio");
    }

    static void handleStopAdhan()
    {
        testAudioStopTime = 0;
        stopAudio();
        sendJson(200, "{\"success\":true,\"message\":\"Adhan stopped\"}");
    }

    void checkTestAudioTimeout()
    {
        if (testAudioStopTime > 0 && millis() >= testAudioStopTime)
        {
            stopAudio();
            testAudioStopTime = 0;
        }
    }

} // namespace SettingsServer
