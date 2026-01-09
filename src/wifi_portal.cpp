#include "wifi_portal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <memory>

namespace WiFiPortal
{
    // Internal state
    static std::unique_ptr<WebServer> server;
    static std::unique_ptr<DNSServer> dnsServer;
    static bool portalActive = false;
    static bool credentialsReceived = false;
    static char savedSSID[33] = "";
    static char savedPassword[65] = "";
    static unsigned long portalStartTime = 0;
    static int lastClientCount = 0;

    // Connection test state machine
    enum class ConnectState
    {
        IDLE,       // No connection attempt in progress
        PENDING,    // Credentials received, waiting to start connection test
        CONNECTING, // Currently trying to connect
        SUCCESS,    // Connection successful - ready for restart
        FAILED      // Connection failed - user can retry
    };
    static ConnectState connectState = ConnectState::IDLE;
    static char connectError[64] = "";
    static char connectedIP[16] = ""; // Store IP when connection succeeds
    static unsigned long connectStartTime = 0;
    static int connectRetryCount = 0;                // Track retry attempts
    constexpr unsigned long CONNECT_TIMEOUT = 15000; // 15 seconds per attempt
    constexpr int MAX_CONNECT_RETRIES = 3;           // 3 attempts total (matches boot)

    // Rate limiting for /save endpoint
    static unsigned long lastSaveAttempt = 0;
    static int saveAttemptCount = 0;
    constexpr unsigned long RATE_LIMIT_WINDOW = 60000; // 60 seconds
    constexpr int MAX_SAVE_ATTEMPTS = 5;               // Max attempts per window
    constexpr unsigned long MIN_SAVE_INTERVAL = 2000;  // 2 seconds between attempts

    constexpr byte DNS_PORT = 53;

    // HTTP status codes
    constexpr int HTTP_OK = 200;
    constexpr int HTTP_NO_CONTENT = 204;
    constexpr int HTTP_FOUND = 302;
    constexpr int HTTP_BAD_REQUEST = 400;
    constexpr int HTTP_NOT_FOUND = 404;
    constexpr int HTTP_TOO_MANY_REQUESTS = 429;

    // Helper function to safely calculate elapsed time (handles millis overflow)
    inline unsigned long elapsedTime(unsigned long start, unsigned long now)
    {
        // Unsigned arithmetic handles overflow correctly due to modulo 2^32
        return now - start;
    }

    // Validation helpers
    bool isValidSSID(const String &ssid)
    {
        if (ssid.length() == 0 || ssid.length() > 32)
            return false;

        // Check for printable ASCII characters (space to ~)
        for (size_t i = 0; i < ssid.length(); i++)
        {
            char c = ssid[i];
            if (c < 32 || c > 126)
                return false;
        }
        return true;
    }

    bool isValidPassword(const String &password)
    {
        size_t len = password.length();

        // WPA2 password: 8-63 characters
        if (len < 8 || len > 63)
            return false;

        // Check for printable ASCII characters (space to ~)
        for (size_t i = 0; i < len; i++)
        {
            char c = password[i];
            if (c < 32 || c > 126)
                return false;
        }
        return true;
    }

    // Serve file from LittleFS - NO GZIP (plain files only)
    bool serveFile(const char *path, const char *contentType)
    {
        if (!server)
            return false;

        // Add security headers
        server->sendHeader("X-Content-Type-Options", "nosniff");

        // Add cache control for static files
        server->sendHeader("Cache-Control", "public, max-age=3600"); // 1 hour cache

        // Maximum file size for safety (100KB)
        constexpr size_t MAX_FILE_SIZE = 102400;

        // Open plain file directly (no gzip)
        File file = LittleFS.open(path, "r");
        if (!file)
        {
            Serial.printf("[Portal] File not found: %s\n", path);
            server->send(HTTP_NOT_FOUND, "text/plain", "File not found");
            return false;
        }

        size_t fileSize = file.size();
        if (fileSize > MAX_FILE_SIZE)
        {
            Serial.printf("[Portal] File too large: %s (%u bytes)\n", path, fileSize);
            file.close();
            server->send(HTTP_NOT_FOUND, "text/plain", "File too large");
            return false;
        }

        size_t sent = server->streamFile(file, contentType);
        file.close();
        return (sent > 0);
    }

    // Check if a string is an IP address
    bool isIp(const String &str)
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

    // Captive portal redirect logic - FIXED
    bool captivePortal()
    {
        if (!server)
            return false;

        // Get the requested host (with length protection)
        const String &host = server->hostHeader();

        // Protect against oversized Host header (DoS prevention)
        if (host.length() > 128)
        {
            server->send(HTTP_BAD_REQUEST, "text/plain", "");
            return true;
        }

        String softAP_IP = WiFi.softAPIP().toString();

        // If already on our IP, don't redirect
        if (host == softAP_IP || host == "192.168.4.1")
        {
            return false;
        }

        // If it's an IP but not ours, redirect
        if (isIp(host))
        {
            Serial.printf("[Portal] Redirecting IP %s to %s\n", host.c_str(), softAP_IP.c_str());
            server->sendHeader("Location", "http://" + softAP_IP, true);
            server->send(HTTP_FOUND, "text/html", "");
            return true;
        }

        // If it's a hostname (connectivity check), redirect
        Serial.printf("[Portal] Captive portal redirect from: %s\n", host.c_str());
        server->sendHeader("Location", "http://" + softAP_IP, true);
        server->send(HTTP_FOUND, "text/html", "");
        return true;
    }

    // Handler functions
    void handleRoot()
    {
        if (!server)
            return;

        // Check if this request needs a redirect FIRST
        if (captivePortal())
            return;

        // Serve the portal page with no-cache headers
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "-1");

        if (!serveFile("/index.html", "text/html"))
        {
            Serial.println("[Portal] ERROR: Failed to serve index.html");
            // Fallback HTML if LittleFS fails
            server->send(HTTP_OK, "text/html",
                         "<h1>WiFi Setup</h1>"
                         "<form action='/save' method='POST'>"
                         "SSID: <input name='ssid' required><br>"
                         "Password: <input type='password' name='password' required><br>"
                         "<input type='submit' value='Connect'>"
                         "</form>");
        }
    }

    void handleSuccess()
    {
        if (!server)
            return;

        if (!serveFile("/success.html", "text/html"))
        {
            Serial.println("[Portal] ERROR: Failed to serve success.html");
        }
    }

    void handleSave()
    {
        if (!server)
            return;

        unsigned long now = millis();

        // Check minimum interval between attempts (overflow-safe)
        if (elapsedTime(lastSaveAttempt, now) < MIN_SAVE_INTERVAL)
        {
            Serial.println("[Portal] Rate limit: Too many attempts too quickly");
            server->send(HTTP_TOO_MANY_REQUESTS, "text/plain", "Too many requests. Please wait.");
            return;
        }

        // Reset counter if window expired (overflow-safe)
        if (elapsedTime(lastSaveAttempt, now) > RATE_LIMIT_WINDOW)
        {
            saveAttemptCount = 0;
        }

        // Check max attempts in window
        if (saveAttemptCount >= MAX_SAVE_ATTEMPTS)
        {
            Serial.printf("[Portal] Rate limit: Max attempts (%d) reached\n", MAX_SAVE_ATTEMPTS);
            server->send(HTTP_TOO_MANY_REQUESTS, "text/plain", "Too many attempts. Please wait 60 seconds.");
            return;
        }

        lastSaveAttempt = now;
        saveAttemptCount++;

        if (!server->hasArg("ssid") || !server->hasArg("password"))
        {
            server->send(HTTP_BAD_REQUEST, "text/plain", "Missing parameters");
            return;
        }

        String ssidArg = server->arg("ssid");
        String passArg = server->arg("password");

        // Trim leading and trailing whitespace from SSID
        ssidArg.trim();

        // Validate SSID format
        if (!isValidSSID(ssidArg))
        {
            Serial.printf("[Portal] Invalid SSID format: %s\n", ssidArg.c_str());
            server->send(HTTP_BAD_REQUEST, "text/plain", "Invalid SSID format");
            return;
        }

        // Validate password format
        if (!isValidPassword(passArg))
        {
            Serial.printf("[Portal] Invalid password format (length: %d)\n", passArg.length());
            server->send(HTTP_BAD_REQUEST, "text/plain", "Invalid password format");
            return;
        }

        // Copy to char buffers with bounds checking
        size_t ssidLen = ssidArg.length();
        size_t passLen = passArg.length();

        if (ssidLen >= sizeof(savedSSID))
            ssidLen = sizeof(savedSSID) - 1;
        if (passLen >= sizeof(savedPassword))
            passLen = sizeof(savedPassword) - 1;

        memcpy(savedSSID, ssidArg.c_str(), ssidLen);
        savedSSID[ssidLen] = '\0';

        memcpy(savedPassword, passArg.c_str(), passLen);
        savedPassword[passLen] = '\0';

        credentialsReceived = true;
        connectState = ConnectState::PENDING; // Signal to start connection test

        Serial.println("[Portal] Credentials received:");
        Serial.printf("[Portal] SSID: %s\n", savedSSID);
        Serial.printf("[Portal] Password: ******** (hidden for security)\n");

        // Return JSON response - page will poll /status for result
        Serial.println("[Portal] Starting connection test (page will poll /status)");
        server->sendHeader("Cache-Control", "no-cache");
        server->send(HTTP_OK, "application/json", "{\"status\":\"connecting\"}");
    }

    bool start()
    {
        if (portalActive)
        {
            Serial.println("[Portal] Already active");
            return true;
        }

        Serial.println("[Portal] Starting WiFi configuration portal...");

        // Initialize LittleFS filesystem
        if (!LittleFS.begin(true))
        {
            Serial.println("[Portal] ERROR: Failed to mount LittleFS!");
            return false;
        }
        Serial.println("[Portal] LittleFS mounted successfully");

        // Full WiFi radio reset before starting AP
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(500); // Longer delay to ensure radio fully resets

        // Use AP_STA mode to allow WiFi scanning while AP is active
        WiFi.mode(WIFI_AP_STA);
        delay(100); // Let mode change settle

        // CRITICAL FIX FOR 2026: Define explicit IP identity
        IPAddress apIP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);

        // Configure AP with explicit IP as both local IP AND gateway
        WiFi.softAPConfig(apIP, gateway, subnet);

        bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);

        if (!apStarted)
        {
            Serial.println("[Portal] Failed to start Access Point!");
            Serial.println("[Portal] Restarting device to clear WiFi state...");
            delay(1000);
            ESP.restart();
        }

        delay(500); // Let AP stabilize before checking IP

        IPAddress IP = WiFi.softAPIP();

        // Verify we got a valid IP address
        if (IP[0] == 0)
        {
            Serial.println("[Portal] ERROR: Failed to get valid AP IP address!");
            Serial.println("[Portal] Restarting device to clear WiFi state...");
            delay(1000);
            ESP.restart();
        }

        Serial.printf("[Portal] Access Point started: %s\n", AP_SSID);
        Serial.printf("[Portal] Password: %s\n", AP_PASSWORD);
        Serial.printf("[Portal] IP Address: %s\n", IP.toString().c_str());

        if (!dnsServer)
        {
            dnsServer = std::make_unique<DNSServer>();
        }

        // Start DNS server with the same explicit IP
        if (!dnsServer->start(DNS_PORT, "*", apIP))
        {
            Serial.println("[Portal] ERROR: Failed to start DNS server!");
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            LittleFS.end();
            dnsServer.reset();
            return false;
        }
        Serial.println("[Portal] DNS server started");

        if (!server)
        {
            server = std::make_unique<WebServer>(80);
        }

        // MANDATORY for 2026: Tell the server to look at the "Host" header
        const char *headerkeys[] = {"Host", "User-Agent"};
        server->collectHeaders(headerkeys, 2);

        server->on("/", HTTP_GET, handleRoot);
        server->on("/save", HTTP_POST, handleSave);
        server->on("/success.html", HTTP_GET, handleSuccess);

        // Connection status endpoint - polled by the page after /save
        server->on("/status", HTTP_GET, []()
                   {
            if (!server) return;
            
            server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            
            String json = "{";
            switch (connectState) {
                case ConnectState::IDLE:
                    json += "\"state\":\"idle\"";
                    break;
                case ConnectState::PENDING:
                case ConnectState::CONNECTING:
                    json += "\"state\":\"connecting\",\"attempt\":";
                    json += String(connectRetryCount + 1);
                    json += ",\"maxAttempts\":";
                    json += String(MAX_CONNECT_RETRIES);
                    break;
                case ConnectState::SUCCESS:
                    json += "\"state\":\"success\",\"ip\":\"";
                    json += connectedIP;
                    json += "\"";
                    break;
                case ConnectState::FAILED:
                    json += "\"state\":\"failed\",\"error\":\"";
                    // Escape any quotes in error message
                    for (size_t i = 0; i < strlen(connectError); i++) {
                        if (connectError[i] == '"') json += "\\\"";
                        else json += connectError[i];
                    }
                    json += "\"";
                    break;
            }
            json += "}";
            
            server->send(HTTP_OK, "application/json", json); });

        // Reset endpoint - allows user to try again with new credentials
        server->on("/reset", HTTP_POST, []()
                   {
            if (!server) return;
            
            Serial.println("[Portal] Reset requested - clearing state for retry");
            connectState = ConnectState::IDLE;
            connectRetryCount = 0;
            memset(connectError, 0, sizeof(connectError));
            memset(savedSSID, 0, sizeof(savedSSID));
            memset(savedPassword, 0, sizeof(savedPassword));
            credentialsReceived = false;
            
            // Reset WiFi back to AP_STA mode (allows scanning)
            WiFi.disconnect(true);
            WiFi.mode(WIFI_AP_STA);
            
            server->sendHeader("Cache-Control", "no-cache");
            server->send(HTTP_OK, "application/json", "{\"status\":\"reset\"}"); });

        // WiFi scan endpoint - returns JSON array of networks
        server->on("/scan", HTTP_GET, []()
                   {
            if (!server) return;
            
            Serial.println("[Portal] Starting WiFi scan...");
            
            // Ensure we're in AP_STA mode for scanning (AP stays active)
            if (WiFi.getMode() != WIFI_AP_STA) {
                WiFi.mode(WIFI_AP_STA);
                delay(100);
            }
            
            // Scan for networks (this blocks for 1-3 seconds)
            // Using async=false, show_hidden=false, passive=false, max_ms_per_chan=300
            int n = WiFi.scanNetworks(false, false, false, 300);
            
            // Build JSON response
            String json = "[";
            
            if (n > 0) {
                // Create index array for sorting by RSSI
                int indices[n];
                for (int i = 0; i < n; i++) indices[i] = i;
                
                // Sort indices by signal strength (strongest first)
                for (int i = 0; i < n - 1; i++) {
                    for (int j = i + 1; j < n; j++) {
                        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
                            int temp = indices[i];
                            indices[i] = indices[j];
                            indices[j] = temp;
                        }
                    }
                }
                
                // Limit to 15 networks to save memory
                int maxNetworks = (n > 15) ? 15 : n;
                int addedCount = 0;
                
                for (int i = 0; i < n && addedCount < maxNetworks; i++) {
                    int idx = indices[i];
                    String ssid = WiFi.SSID(idx);
                    
                    // Skip empty SSIDs (hidden networks)
                    if (ssid.length() == 0) continue;
                    
                    // Skip duplicates (same SSID already added)
                    bool duplicate = false;
                    for (int j = 0; j < i; j++) {
                        if (WiFi.SSID(indices[j]) == ssid) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (duplicate) continue;
                    
                    // Escape quotes in SSID for JSON
                    ssid.replace("\\", "\\\\");
                    ssid.replace("\"", "\\\"");
                    
                    if (addedCount > 0) json += ",";
                    json += "{\"ssid\":\"" + ssid + "\",";
                    json += "\"rssi\":" + String(WiFi.RSSI(idx)) + ",";
                    json += "\"secure\":" + String(WiFi.encryptionType(idx) != WIFI_AUTH_OPEN ? 1 : 0) + "}";
                    addedCount++;
                }
            }
            
            json += "]";
            
            // Clean up scan results to free memory
            WiFi.scanDelete();
            
            Serial.printf("[Portal] Scan complete, found %d networks\n", n);
            
            server->sendHeader("Cache-Control", "no-cache");
            server->send(HTTP_OK, "application/json", json); });

        // Serve static assets explicitly
        server->on("/style.css", HTTP_GET, []()
                   {
            if (server) serveFile("/style.css", "text/css"); });
        server->on("/script.js", HTTP_GET, []()
                   {
            if (server) serveFile("/script.js", "application/javascript"); });

        // CRITICAL FIX FOR WINDOWS: Stop WPAD spinning with valid PAC
        server->on("/wpad.dat", HTTP_GET, []()
                   {
            if (server) {
                // Return valid PAC that says "no proxy" - stops retry loop
                server->send(HTTP_OK, "application/x-ns-proxy-autoconfig", 
                    "function FindProxyForURL(url,host){return\"DIRECT\";}");
            } });

        // CRITICAL FIX FOR WINDOWS 2026: Answer NCSI probe correctly
        server->on("/connecttest.txt", HTTP_GET, []()
                   {
            if (server) {
                server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
                server->send(HTTP_OK, "text/plain", "Microsoft Connect Test");
            } });

        server->on("/redirect", HTTP_GET, handleRoot);
        server->on("/ncsi.txt", HTTP_ANY, []()
                   {
            if (server) {
                server->sendHeader("Cache-Control", "no-cache");
                server->send(HTTP_OK, "text/plain", "Microsoft NCSI");
            } });

        // Android/iOS connectivity checks - redirect to trigger captive portal
        auto forceRedirect = []()
        {
            if (server)
            {
                Serial.println("[Portal] Mobile connectivity check - redirecting");
                server->sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
                server->send(HTTP_FOUND, "text/plain", "");
            }
        };

        server->on("/generate_204", HTTP_ANY, forceRedirect);        // Android
        server->on("/gen_204", HTTP_ANY, forceRedirect);             // Android
        server->on("/hotspot-detect.html", HTTP_ANY, forceRedirect); // iOS
        server->on("/canonical.html", HTTP_ANY, forceRedirect);      // Firefox
        server->on("/success.txt", HTTP_ANY, forceRedirect);         // Firefox

        // Ultra-fast onNotFound handler - prioritize speed
        server->onNotFound([]()
                           {
            if (!server) return;
            
            // Use const references to avoid String copies on every request
            const String& uri = server->uri();
            const String& host = server->hostHeader();
            
            // Quick length check - reject oversized requests immediately
            if (uri.length() > 256 || host.length() > 128) {
                server->send(HTTP_BAD_REQUEST, "text/plain", "");
                return;
            }
            
            // Prioritize speed: If it's WPAD or a generic app ping, stop immediately (HTTP 204 is fastest)
            if (host.indexOf("wpad") >= 0 || uri.indexOf("favicon") >= 0 || uri.indexOf(".map") >= 0) {
                server->send(HTTP_NO_CONTENT, "text/plain", ""); 
                return;
            }
            
            // Everything else gets processed via handleRoot/captivePortal logic
            handleRoot(); });

        server->begin();
        Serial.println("[Portal] Web server started on port 80");

        portalActive = true;
        portalStartTime = millis();
        credentialsReceived = false;
        lastClientCount = 0;

        return true;
    }

    void stop()
    {
        if (!portalActive)
            return;

        Serial.println("[Portal] Stopping portal...");

        if (dnsServer)
        {
            dnsServer->stop();
            dnsServer.reset();
        }

        if (server)
        {
            server->stop();
            server.reset();
        }

        // Add delay before mode change to prevent netstack error
        delay(100);

        // Only disconnect AP if it's actually running
        if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
        {
            WiFi.softAPdisconnect(true);
        }
        WiFi.mode(WIFI_OFF);
        delay(100);

        // Unmount LittleFS filesystem
        LittleFS.end();

        // Reset rate limiting state
        lastSaveAttempt = 0;
        saveAttemptCount = 0;

        // Clear sensitive credentials from memory
        memset(savedSSID, 0, sizeof(savedSSID));
        memset(savedPassword, 0, sizeof(savedPassword));
        memset(connectError, 0, sizeof(connectError));
        credentialsReceived = false;
        connectState = ConnectState::IDLE;
        connectRetryCount = 0;

        portalActive = false;
        Serial.println("[Portal] Portal stopped");
    }

    bool isActive()
    {
        return portalActive;
    }

    bool isConnectionSuccess()
    {
        return connectState == ConnectState::SUCCESS;
    }

    // Helper: Start a new connection attempt
    void startConnectionAttempt()
    {
        WiFi.disconnect(true);
        delay(500);
        WiFi.mode(WIFI_AP_STA);
        WiFi.setSleep(false);
        delay(100);
        WiFi.begin(savedSSID, savedPassword);
        connectStartTime = millis();
    }

    // Helper: Handle connection success
    void handleConnectionSuccess()
    {
        // Store IP before disconnecting
        String ip = WiFi.localIP().toString();
        strncpy(connectedIP, ip.c_str(), sizeof(connectedIP) - 1);
        connectedIP[sizeof(connectedIP) - 1] = '\0';

        Serial.printf("[Portal] ✓ Connection successful! IP: %s\n", connectedIP);
        connectState = ConnectState::SUCCESS;
        connectRetryCount = 0;
        WiFi.disconnect(false); // Keep AP for status polling
    }

    // Helper: Handle connection failure
    void handleConnectionFailure(const char *reason)
    {
        Serial.printf("[Portal] ✗ %s\n", reason);
        strncpy(connectError, reason, sizeof(connectError) - 1);
        connectError[sizeof(connectError) - 1] = '\0';
        connectState = ConnectState::FAILED;
        connectRetryCount = 0;
        WiFi.disconnect(true);
        WiFi.mode(WIFI_AP_STA);
    }

    // Helper: Get error message for WiFi status code
    const char *getWiFiErrorMessage(wl_status_t status)
    {
        switch (status)
        {
        case WL_NO_SSID_AVAIL:
            return "Network not found - check SSID";
        case WL_CONNECT_FAILED:
            return "Connection rejected - wrong password?";
        case WL_DISCONNECTED:
            return "Could not connect - check password";
        default:
            return nullptr; // Caller will format with status code
        }
    }

    void processConnectingState()
    {
        wl_status_t status = WiFi.status();
        unsigned long elapsed = elapsedTime(connectStartTime, millis());

        if (status == WL_CONNECTED)
        {
            handleConnectionSuccess();
            return;
        }

        // Immediate rejection - fail fast
        if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED)
        {
            const char *reason = (status == WL_NO_SSID_AVAIL) ? "Network not found" : "Wrong password";
            handleConnectionFailure(reason);
            return;
        }

        if (elapsed <= CONNECT_TIMEOUT)
            return;

        connectRetryCount++;
        Serial.printf("[Portal] Attempt %d/%d timed out (status: %d)\n",
                      connectRetryCount, MAX_CONNECT_RETRIES, status);

        if (connectRetryCount < MAX_CONNECT_RETRIES)
        {
            startConnectionAttempt();
            return;
        }

        Serial.printf("[Portal] ✗ Connection failed after %d attempts\n", MAX_CONNECT_RETRIES);

        const char *errorMsg = getWiFiErrorMessage(status);
        if (errorMsg)
        {
            handleConnectionFailure(errorMsg);
            return;
        }

        char buf[64];
        snprintf(buf, sizeof(buf), "Connection failed (code: %d)", status);
        handleConnectionFailure(buf);
    }

    void handle()
    {
        if (!portalActive || !server)
            return;

        if (dnsServer)
            dnsServer->processNextRequest();

        server->handleClient();

        // Connection test state machine
        switch (connectState)
        {
        case ConnectState::PENDING:
            connectRetryCount = 0;
            Serial.println("[Portal] Starting WiFi connection test...");
            Serial.printf("[Portal] Testing connection to: %s\n", savedSSID);
            startConnectionAttempt();
            connectState = ConnectState::CONNECTING;
            break;

        case ConnectState::CONNECTING:
            processConnectingState();
            break;

        default:
            // IDLE, SUCCESS, FAILED - check portal timeout
            if (elapsedTime(portalStartTime, millis()) > PORTAL_TIMEOUT)
            {
                Serial.printf("[Portal] Timeout after %lu seconds\n", elapsedTime(portalStartTime, millis()) / 1000);
                stop();
                return;
            }
            break;
        }

        // Log client connection changes
        int currentClientCount = WiFi.softAPgetStationNum();
        if (currentClientCount != lastClientCount)
        {
            Serial.printf("[Portal] Connected clients: %d\n", currentClientCount);
            lastClientCount = currentClientCount;
        }
    }

    bool hasNewCredentials()
    {
        return credentialsReceived;
    }

    void getNewCredentials(char *ssidBuffer, size_t ssidSize, char *passBuffer, size_t passSize)
    {
        size_t ssidLen = strlen(savedSSID);
        size_t passLen = strlen(savedPassword);

        if (ssidLen >= ssidSize)
            ssidLen = ssidSize - 1;
        if (passLen >= passSize)
            passLen = passSize - 1;

        memcpy(ssidBuffer, savedSSID, ssidLen);
        ssidBuffer[ssidLen] = '\0';

        memcpy(passBuffer, savedPassword, passLen);
        passBuffer[passLen] = '\0';
    }

    void clearCredentials()
    {
        credentialsReceived = false;
        memset(savedSSID, 0, sizeof(savedSSID));
        memset(savedPassword, 0, sizeof(savedPassword));
    }
}
