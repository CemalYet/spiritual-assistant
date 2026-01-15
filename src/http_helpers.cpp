#include "http_helpers.h"
#include <LittleFS.h>

namespace HttpHelpers
{
    bool serveFile(WebServer *server, const char *path, const char *contentType, int cacheSeconds)
    {
        if (!server)
            return false;

        // Add security headers
        server->sendHeader("X-Content-Type-Options", "nosniff");

        // Add cache control
        if (cacheSeconds > 0)
        {
            server->sendHeader("Cache-Control", String("public, max-age=") + String(cacheSeconds));
        }
        else
        {
            server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            server->sendHeader("Pragma", "no-cache");
            server->sendHeader("Expires", "-1");
        }

        // Open file
        File file = LittleFS.open(path, "r");
        if (!file)
        {
            Serial.printf("[HTTP] File not found: %s\n", path);
            server->send(HTTP_NOT_FOUND, "text/plain", "File not found");
            return false;
        }

        // Check file size
        size_t fileSize = file.size();
        if (fileSize > MAX_FILE_SIZE)
        {
            Serial.printf("[HTTP] File too large: %s (%u bytes)\n", path, fileSize);
            file.close();
            server->send(HTTP_NOT_FOUND, "text/plain", "File too large");
            return false;
        }

        size_t sent = server->streamFile(file, contentType);
        file.close();
        return (sent > 0);
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
