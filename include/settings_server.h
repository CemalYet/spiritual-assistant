#ifndef SETTINGS_SERVER_H
#define SETTINGS_SERVER_H

namespace SettingsServer
{
    /// Start HTTP settings server (call after WiFi connected)
    void start();

    /// Stop the settings server
    void stop();

    /// Handle incoming HTTP requests (call from loop)
    void handle();

    /// Check if settings server is currently running
    bool isActive();

} // namespace SettingsServer

#endif // SETTINGS_SERVER_H
