#include "http_helpers.h"
#include <LittleFS.h>

namespace HttpHelpers
{
    bool serveFile(WebServer *server, const char *path, const char *contentType, int cacheSeconds)
    {
        if (!server)
            return false;

        // Log User-Agent to identify browser
        String userAgent = "unknown";
        if (server->hasHeader("User-Agent"))
        {
            userAgent = server->header("User-Agent");
            if (userAgent.length() > 50)
                userAgent = userAgent.substring(0, 50) + "...";
        }
        Serial.printf("[HTTP] Request: %s from %s\n", path, userAgent.c_str());

        if (!LittleFS.exists(path))
        {
            Serial.printf("[HTTP] File not found: %s\n", path);
            server->send(HTTP_NOT_FOUND, "text/plain", "File not found");
            return false;
        }

        File file = LittleFS.open(path, "r");
        if (!file)
        {
            Serial.printf("[HTTP] Failed to open file\n");
            server->send(HTTP_NOT_FOUND, "text/plain", "File not found");
            return false;
        }

        size_t fileSize = file.size();
        Serial.printf("[HTTP] Serving %s (%u bytes) Free heap: %u\n", path, fileSize, ESP.getFreeHeap());

        // Set headers
        server->sendHeader("Connection", "close");
        if (cacheSeconds > 0)
        {
            server->sendHeader("Cache-Control", String("public, max-age=") + String(cacheSeconds));
        }

        // Simple streamFile - let Arduino handle chunking
        unsigned long startTime = millis();
        size_t sent = server->streamFile(file, contentType);
        unsigned long elapsed = millis() - startTime;
        file.close();

        Serial.printf("[HTTP] Done: sent %u/%u bytes in %lu ms\n", sent, fileSize, elapsed);

        if (sent != fileSize)
        {
            Serial.println("[HTTP] WARNING: Incomplete transfer!");
        }

        return (sent == fileSize);
    }

    void sendNoCacheHeaders(WebServer *server)
    {
        if (!server)
            return;
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "-1");
    }

    void registerBrowserResourceHandlers(WebServer *server)
    {
        if (!server)
            return;

        // Favicon - browsers always request this
        server->on("/favicon.ico", HTTP_GET, [server]()
                   { server->send(HTTP_NO_CONTENT); });

        // Apple touch icons (iOS)
        server->on("/apple-touch-icon.png", HTTP_GET, [server]()
                   { server->send(HTTP_NO_CONTENT); });
        server->on("/apple-touch-icon-precomposed.png", HTTP_GET, [server]()
                   { server->send(HTTP_NO_CONTENT); });
    }

    bool isIpAddress(const String &str)
    {
        // Protect against oversized input (max valid IP is 15 chars: 255.255.255.255)
        if (str.length() > 15)
            return false;

        for (size_t i = 0; i < str.length(); i++)
        {
            int c = str.charAt(i);
            if (c != '.' && (c < '0' || c > '9'))
            {
                return false;
            }
        }
        return true;
    }

} // namespace HttpHelpers
