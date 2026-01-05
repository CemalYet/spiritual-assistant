#pragma once
#include <cstddef>

namespace WiFiCredentials
{
    // Initialize storage
    bool init();

    // Check if credentials exist
    bool hasCredentials();

    // Save WiFi credentials (const char* inputs)
    bool save(const char *ssid, const char *password);

    // Load WiFi credentials (writes to provided buffers)
    // ssidBuffer must be at least 33 bytes, passBuffer at least 65 bytes
    bool load(char *ssidBuffer, size_t ssidSize, char *passBuffer, size_t passSize);

    // Clear stored credentials
    void clear();
}
