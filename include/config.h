#pragma once
#include <string_view>

// --- TEST MODE ---
// Set to true to run prayer time comparison tests
// Device will output 30 days of calculations and restart
// Uses PRAYER_METHOD setting below for calculations
#define TEST_MODE false

// ---  AP PORTAL ---
// Set to true to clear WiFi credentials on boot and force AP portal
// Useful for testing offline mode
#define FORCE_AP_PORTAL true

// --- DEBUG MODE ---
// Set to true to see detailed cache logs
#define DEBUG_CACHE_LOGS true

// --- COMPILE-TIME CONFIGURATION ---
namespace Config
{
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
    // Uncomment the one you need or add your own:

    constexpr const char *TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3"; // Europe (Belgium/Germany/France) - Default
    // constexpr const char *TIMEZONE = "TRT-3";                      // Turkey (GMT+3, NO DST since 2016)
    // constexpr const char *TIMEZONE = "EET-2EEST,M3.5.0,M10.5.0/3"; // Eastern Europe (Greece/Romania) with DST
    // constexpr const char *TIMEZONE = "GMT0BST,M3.5.0/1,M10.5.0";   // UK (London)
    // constexpr const char *TIMEZONE = "EST5EDT,M3.2.0,M11.1.0";     // USA East Coast
    // constexpr const char *TIMEZONE = "PST8PDT,M3.2.0,M11.1.0";     // USA West Coast
    // constexpr const char *TIMEZONE = "AST-3";                      // Saudi Arabia (GMT+3, no DST)
    // constexpr const char *TIMEZONE = "GST-4";                      // UAE/Dubai (GMT+4, no DST)

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
