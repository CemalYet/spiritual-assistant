#ifndef HTTP_HELPERS_H
#define HTTP_HELPERS_H

#include <WebServer.h>

namespace HttpHelpers
{
    // HTTP status codes
    constexpr int HTTP_OK = 200;
    constexpr int HTTP_NO_CONTENT = 204;
    constexpr int HTTP_FOUND = 302;
    constexpr int HTTP_BAD_REQUEST = 400;
    constexpr int HTTP_NOT_FOUND = 404;
    constexpr int HTTP_TOO_MANY_REQUESTS = 429;
    constexpr int HTTP_BAD_GATEWAY = 502;

    // Maximum file size for safety (100KB)
    constexpr size_t MAX_FILE_SIZE = 102400;

    /// Serve a file from LittleFS with appropriate headers
    /// @param server WebServer instance to send response
    /// @param path File path in LittleFS (e.g., "/index.html")
    /// @param contentType MIME type (e.g., "text/html")
    /// @param cacheSeconds Cache-Control max-age (0 = no-cache)
    /// @return true if file was served successfully
    bool serveFile(WebServer *server, const char *path, const char *contentType, int cacheSeconds = 3600);

    /// Send no-cache headers for dynamic content
    void sendNoCacheHeaders(WebServer *server);

    /// Register common browser resource handlers (favicon, apple-touch-icon)
    /// These return 204 No Content to prevent unnecessary requests
    void registerBrowserResourceHandlers(WebServer *server);

    /// Check if a string looks like an IP address
    bool isIpAddress(const String &str);

} // namespace HttpHelpers

#endif // HTTP_HELPERS_H
