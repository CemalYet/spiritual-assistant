#pragma once
#include <string_view>

// --- TEST MODE ---
// Set to true to run prayer time comparison tests
// Device will output 30 days of calculations and restart
// Uses PRAYER_METHOD setting below for calculations
#define TEST_MODE false

// --- TEST ADHAN AUDIO ---
// Set to true to play 5s of each adhan file on boot (then normal operation)
#define TEST_ADHAN_AUDIO false

// ---  AP PORTAL ---
// Set to true to clear WiFi credentials on boot and force AP portal
// Useful for testing offline mode
#define FORCE_AP_PORTAL false

// --- DEBUG MODE ---
// Set to true to see detailed cache logs
#define DEBUG_CACHE_LOGS true

// --- COMPILE-TIME CONFIGURATION ---
namespace Config
{
    // Keep sleep wake-early and clock-jump catch-up window aligned across modules.
    constexpr int PRAYER_WAKE_EARLY_SEC = 5;

    constexpr std::string_view WIFI_SSID = "";
    constexpr std::string_view WIFI_PASS = "";

    // --- TEST MODE DEFAULTS (only used when TEST_MODE = true) ---
    constexpr double TEST_LATITUDE = 50.8798; // Leuven
    constexpr double TEST_LONGITUDE = 4.7005;
    constexpr int TEST_DIYANET_ILCE_ID = 11706; // Leuven

    constexpr int PRAYER_METHOD = 13; // Diyanet (13 = use abdus.dev API or Adhan library)

    // Prayer times are computed offline using Adhan.
    // For Turkey only, we try abdus.dev (Diyanet) first and fall back to offline.

    // Custom Calculation Parameters (optional - set to -1 to use method defaults)
    constexpr double CUSTOM_FAJR_ANGLE = -1; // e.g., 18.0 for Diyanet
    constexpr double CUSTOM_ISHA_ANGLE = -1; // e.g., 17.0 for Diyanet
    constexpr int CUSTOM_ISHA_INTERVAL = -1; // Minutes after Maghrib (0 = use angle, -1 = use method default)
    constexpr bool USE_HANAFI_ASR = false;   // false = Shafi/Maliki/Hanbali (standard), true = Hanafi (later time)

    // High Latitude Rule (optional override - set to -1 to use method default)
    // -1=Use method default, 0=None, 1=MiddleOfTheNight, 2=SeventhOfTheNight, 3=TwilightAngle
    // Only override for high latitude locations (>48°N) if method doesn't handle it well
    constexpr int HIGH_LATITUDE_RULE = -1; // -1 = use method default (recommended)

    // Timezone with automatic DST handling (POSIX format)
    // Default timezone now stored in SettingsManager (NVS-persisted).
    // Phone portal or NTP path saves the POSIX TZ string to NVS.

    // Manual Timezone Offset (in seconds)
    // Use this if automatic DST fails.
    // CET Winter: 3600 (UTC+1)
    // CEST Summer: 7200 (UTC+2)
    // Turkey: 10800 (UTC+3)
    constexpr int GMT_OFFSET_SEC = 3600;      // Base offset
    constexpr int DAYLIGHT_OFFSET_SEC = 3600; // DST offset (set to 0 if not using DST or included in GMT_OFFSET)

    // NTP servers (multiple for redundancy)
    constexpr const char *NTP_SERVER1 = "pool.ntp.org";
    constexpr const char *NTP_SERVER2 = "time.google.com";
    constexpr const char *NTP_SERVER3 = "time.cloudflare.com";

    // Diyanet API (ezanvakti.emushaf.net) - 30 days cached
    constexpr std::string_view DIYANET_API_BASE = "https://ezanvakti.emushaf.net";
    // DiyanetId is now stored in NVS via SettingsManager
}
