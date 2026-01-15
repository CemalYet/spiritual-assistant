#ifndef SETTINGS_SERVER_H
#define SETTINGS_SERVER_H

namespace SettingsServer
{
    /// Start mDNS and HTTP settings server (call after WiFi connected)
    void start();

    /// Stop the settings server and mDNS
    void stop();

    /// Handle incoming HTTP requests (call from loop)
    void handle();

    /// Check if settings server is currently running
    bool isActive();

    /// Get the mDNS hostname (without .local suffix)
    const char *getHostname();

} // namespace SettingsServer

#endif // SETTINGS_SERVER_H
