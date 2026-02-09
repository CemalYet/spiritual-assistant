#include "settings_server.h"
#include "settings_manager.h"
#include "prayer_types.h"
#include "http_helpers.h"
#include "prayer_api.h"
#include "audio_player.h"
#include "time_utils.h"
#include "wifi_credentials.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <memory>

namespace SettingsServer
{
    static std::unique_ptr<WebServer> server;
    static bool active = false;
    static constexpr char DIYANET_API[] = "https://ezanvakti.emushaf.net";
    static constexpr int PROXY_TIMEOUT = 10000;

    // Forward declarations
    static void serveSettingsPage();
    static void serveStyleCss();
    static void serveScriptJs();
    static void serveGzippedFile(const char *path, const char *contentType);
    static void handleApiMode();
    static void handleGetSettings();
    static void handlePostSettings();
    static void handleGetStatus();
    static void handleRefresh();
    static void handleTestAdhan();
    static void handleTestAudio();
    static void handleStopAdhan();
    static void handleSetTime();
    static void handleRestart();
    static void handleGetWifi();
    static void handleSaveWifi();
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

        server = std::make_unique<WebServer>(80);

        const char *headerkeys[] = {"User-Agent", "Accept-Encoding"};
        server->collectHeaders(headerkeys, 2);

        // Routes
        server->on("/", HTTP_GET, serveSettingsPage);
        server->on("/style.css", HTTP_GET, serveStyleCss);
        server->on("/script.js", HTTP_GET, serveScriptJs);
        server->on("/api/mode", HTTP_GET, handleApiMode);
        server->on("/api/settings", HTTP_GET, handleGetSettings);
        server->on("/api/settings", HTTP_POST, handlePostSettings);
        server->on("/api/status", HTTP_GET, handleGetStatus);
        server->on("/api/refresh", HTTP_POST, handleRefresh);
        server->on("/api/test-adhan", HTTP_POST, handleTestAdhan);
        server->on("/api/test-audio", HTTP_GET, handleTestAudio);
        server->on("/api/stop-adhan", HTTP_POST, handleStopAdhan);
        server->on("/api/time", HTTP_POST, handleSetTime);
        server->on("/api/restart", HTTP_POST, handleRestart);
        server->on("/api/wifi", HTTP_GET, handleGetWifi);
        server->on("/api/wifi", HTTP_POST, handleSaveWifi);
        server->onNotFound(handleNotFound);

        HttpHelpers::registerBrowserResourceHandlers(server.get());

        server->begin();
        active = true;
        Serial.printf("[Settings] Server started at http://%s\n", WiFi.localIP().toString().c_str());
    }

    void stop()
    {
        if (!server)
            return;

        server->stop();
        server.reset(); // Automatically deletes and sets to nullptr
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

    // Serve gzipped file if available, otherwise serve original
    static void serveGzippedFile(const char *path, const char *contentType)
    {
        String gzPath = String(path) + ".gz";

        // Check if browser accepts gzip
        bool acceptsGzip = server->hasHeader("Accept-Encoding") &&
                           server->header("Accept-Encoding").indexOf("gzip") >= 0;

        // Try gzipped version first
        if (acceptsGzip && LittleFS.exists(gzPath.c_str()))
        {
            File file = LittleFS.open(gzPath.c_str(), "r");
            if (file)
            {
                Serial.printf("[Server] Streaming %s (%u bytes, gzip)\n", gzPath.c_str(), file.size());
                server->sendHeader("Content-Encoding", "gzip");
                server->sendHeader("Cache-Control", "max-age=86400"); // Cache 1 day
                server->streamFile(file, contentType);
                file.close();
                return;
            }
        }

        // Fall back to original file
        if (LittleFS.exists(path))
        {
            File file = LittleFS.open(path, "r");
            if (file)
            {
                Serial.printf("[Server] Streaming %s (%u bytes)\n", path, file.size());
                server->sendHeader("Cache-Control", "max-age=86400");
                server->streamFile(file, contentType);
                file.close();
                return;
            }
        }

        server->send(404, "text/plain", "File not found");
    }

    // Stream settings.html directly from flash - zero RAM usage
    static void serveSettingsPage()
    {
        serveGzippedFile("/settings.html", "text/html");
    }

    static void serveStyleCss()
    {
        serveGzippedFile("/style.css", "text/css");
    }

    static void serveScriptJs()
    {
        serveGzippedFile("/script.js", "application/javascript");
    }

    // API endpoint to return current mode
    static void handleApiMode()
    {
        server->send(HttpHelpers::HTTP_OK, "application/json", "{\"mode\":\"connected\"}");
    }

    static void handleGetSettings()
    {
        JsonDocument doc;
        doc["prayerMethod"] = SettingsManager::getPrayerMethod();
        doc["methodName"] = SettingsManager::getMethodName(SettingsManager::getPrayerMethod());
        doc["volume"] = SettingsManager::getVolume();
        doc["connectionMode"] = SettingsManager::getConnectionMode();

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

        // Connection mode
        if (doc["connectionMode"].is<const char *>())
            changed |= SettingsManager::setConnectionMode(doc["connectionMode"].as<const char *>());

        // Location data
        if (doc["latitude"].is<double>() && doc["longitude"].is<double>())
        {
            changed |= SettingsManager::setLocation(
                doc["latitude"].as<double>(),
                doc["longitude"].as<double>());
        }

        if (doc["cityName"].is<const char *>())
            changed |= SettingsManager::setCityName(doc["cityName"].as<const char *>());

        // Handle diyanetId - can be int or null (null means manual coordinates)
        if (doc["diyanetId"].is<int>())
            changed |= SettingsManager::setDiyanetId(doc["diyanetId"].as<int32_t>());
        else if (doc["diyanetId"].isNull())
            changed |= SettingsManager::setDiyanetId(0); // Clear diyanetId for manual coords

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
    static constexpr int MAX_VOLUME_HW = 21;   // Hardware max volume level
    static constexpr int MAX_VOLUME_PCT = 100; // Percentage scale

    static bool startTestAudio()
    {
        if (!playAudioFile("/azan.mp3"))
            return false;

        testAudioStopTime = millis() + TEST_AUDIO_DURATION_MS;
        return true;
    }

    static void applyVolumeFromRequest()
    {
        if (!server->hasArg("volume"))
            return;

        int vol = server->arg("volume").toInt();
        if (vol < 0 || vol > MAX_VOLUME_PCT)
            return;

        setVolume(vol * MAX_VOLUME_HW / MAX_VOLUME_PCT);
    }

    static void sendTestAudioResponse()
    {
        if (startTestAudio())
            sendJson(HttpHelpers::HTTP_OK, "{\"success\":true,\"message\":\"Playing 5 sec preview\"}");
        else
            sendJsonError(HttpHelpers::HTTP_INTERNAL_ERROR, "Failed to play audio");
    }

    static void handleTestAdhan()
    {
        sendTestAudioResponse();
    }

    static void handleTestAudio()
    {
        applyVolumeFromRequest();
        sendTestAudioResponse();
    }

    static void handleStopAdhan()
    {
        testAudioStopTime = 0;
        stopAudio();
        sendJson(200, "{\"success\":true,\"message\":\"Adhan stopped\"}");
    }

    static void handleSetTime()
    {
        if (!server->hasArg("plain"))
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "No body");

        JsonDocument doc;
        if (deserializeJson(doc, server->arg("plain")))
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "Invalid JSON");

        auto req = TimeUtils::createFromJson(doc);

        if (!TimeUtils::applySystemTime(req))
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "Invalid date/time values");

        JsonDocument response;
        response["success"] = true;
        response["deviceTime"] = TimeUtils::getFormattedTime();

        String responseStr;
        serializeJson(response, responseStr);
        sendJson(HttpHelpers::HTTP_OK, responseStr);
    }

    static void handleRestart()
    {
        sendJson(HttpHelpers::HTTP_OK, "{\"success\":true,\"message\":\"Restarting...\"}");
        delay(500);
        ESP.restart();
    }

    static void handleGetWifi()
    {
        JsonDocument doc;

        // Get saved WiFi credentials (SSID only for security, not password)
        char ssidBuffer[33] = "";
        char passBuffer[65] = "";

        bool hasCredentials = WiFiCredentials::load(ssidBuffer, sizeof(ssidBuffer), passBuffer, sizeof(passBuffer));

        doc["hasSavedCredentials"] = hasCredentials;
        if (hasCredentials && strlen(ssidBuffer) > 0)
        {
            doc["savedSsid"] = ssidBuffer;
        }

        // Current connection status
        doc["connected"] = WiFi.isConnected();
        if (WiFi.isConnected())
        {
            doc["currentSsid"] = WiFi.SSID();
            doc["ip"] = WiFi.localIP().toString();
            doc["rssi"] = WiFi.RSSI();
        }

        String response;
        serializeJson(doc, response);
        sendJson(HttpHelpers::HTTP_OK, response);
    }

    static void handleSaveWifi()
    {
        if (!server->hasArg("plain"))
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "No body");

        JsonDocument doc;
        if (deserializeJson(doc, server->arg("plain")))
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "Invalid JSON");

        const char *ssid = doc["ssid"] | "";
        const char *password = doc["password"] | "";

        if (strlen(ssid) == 0 || strlen(ssid) > 32)
        {
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "Invalid SSID");
        }
        if (strlen(password) < 8 || strlen(password) > 64)
        {
            return sendJsonError(HttpHelpers::HTTP_BAD_REQUEST, "Password must be 8-64 characters");
        }

        // Save credentials
        if (!WiFiCredentials::save(ssid, password))
        {
            sendJsonError(HttpHelpers::HTTP_INTERNAL_ERROR, "Failed to save credentials");
            return;
        }

        Serial.printf("[Settings] WiFi credentials saved: %s\n", ssid);

        // Return success - device needs restart to apply
        JsonDocument response;
        response["success"] = true;
        response["message"] = "WiFi credentials saved. Restart to apply.";
        response["ssid"] = ssid;

        String responseStr;
        serializeJson(response, responseStr);
        sendJson(HttpHelpers::HTTP_OK, responseStr);
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
