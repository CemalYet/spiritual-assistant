/**
 * @file app_state.h
 * @brief Shared Application State
 *
 * Single source of truth for all app data.
 * main.cpp writes, UI reads via dirty flags.
 * Uses ETL strings for safe, zero-heap string handling.
 */

#ifndef APP_STATE_H
#define APP_STATE_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <etl/string.h>
#include <etl/string_view.h>

// Dirty flags bitfield
namespace DirtyFlag
{
    constexpr uint16_t NONE = 0x0000;
    constexpr uint16_t TIME = 0x0001;
    constexpr uint16_t DATE = 0x0002;
    constexpr uint16_t NEXT_PRAYER = 0x0004;
    constexpr uint16_t PRAYER_TIMES = 0x0008;
    constexpr uint16_t WIFI_STATUS = 0x0010;
    constexpr uint16_t VOLUME = 0x0020;
    constexpr uint16_t MUTED = 0x0040;
    constexpr uint16_t NTP_SYNCED = 0x0080;
    constexpr uint16_t ADHAN_AVAILABLE = 0x0100;
    constexpr uint16_t STATUS_SCREEN = 0x0200;
    constexpr uint16_t LOCATION = 0x0400;
    constexpr uint16_t COUNTDOWN = 0x0800;
    constexpr uint16_t HIJRI = 0x1000;
    constexpr uint16_t PROGRESS = 0x2000;
    constexpr uint16_t QR_INFO = 0x4000;
    constexpr uint16_t SIGNAL_BATTERY = 0x8000;
    constexpr uint16_t ALL = 0xFFFF;
}

// WiFi connection states
enum class WifiState : uint8_t
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED,
    PORTAL // AP Portal mode active
};

// Status screen types
enum class StatusScreenType : uint8_t
{
    NONE,
    CONNECTING,
    PORTAL,
    MESSAGE,
    ERROR
};

/**
 * @brief Global application state
 *
 * All UI-relevant data lives here.
 * main.cpp updates fields + sets dirty flags.
 * UI polls dirty flags and updates widgets.
 */
struct AppState
{
    // ═══════════════════════════════════════════════════
    // TIME & DATE
    // ═══════════════════════════════════════════════════
    int8_t hour = 0;
    int8_t minute = 0;
    int8_t second = 0;
    etl::string<48> date;          // Status bar: "· 8 Mart · Cuma"
    etl::string<48> gregorianFull; // Clock page: "8 Mart 2026 · Cuma"
    etl::string<48> location;      // "Istanbul • Diyanet"

    // ═══════════════════════════════════════════════════
    // PRAYER DATA
    // ═══════════════════════════════════════════════════
    etl::string<16> nextPrayerName;
    etl::string<8> nextPrayerTime;

    // Seconds until next prayer (for countdown display)
    uint32_t secondsToNext = 0;

    // Hijri date string, e.g. "17 Ramazan 1447"
    etl::string<32> hijriDate;

    // Progress 0-100 through current prayer slot
    uint8_t activePrayerProgress = 0;

    // Ramadan mode
    bool ramadanMode = false;
    int32_t iftarDeltaSeconds = 0;        // >0=before iftar, <0=after iftar
    etl::string<40> ramadanCountdownText; // e.g. "\xC4\xB0ftara Kald\xC4\xB1 02:31"

    // Qibla
    uint16_t qiblaDegrees = 0;
    etl::string<16> qiblaDistance;  // e.g. "3.182 km"
    etl::string<16> qiblaDirection; // e.g. "Güneydoğu"

    // Device / QR screen
    etl::string<16> deviceIp;
    etl::string<32> deviceHostname;
    etl::string<32> wifiSsid;
    uint8_t wifiStrength = 0; // 0-3 bars

    // Battery
    uint8_t batteryPct = 0;
    bool charging = false;

    // Settings (mirrored for display)
    uint8_t brightness = 70;
    bool adhanEnabled = true;
    bool sleepMode = true;

    // All 6 prayer times for prayer page
    etl::string<8> fajr;
    etl::string<8> sunrise;
    etl::string<8> dhuhr;
    etl::string<8> asr;
    etl::string<8> maghrib;
    etl::string<8> isha;
    int8_t activePrayerIndex = -1; // -1 = none highlighted

    // ═══════════════════════════════════════════════════
    // CONNECTIVITY
    // ═══════════════════════════════════════════════════
    WifiState wifiState = WifiState::DISCONNECTED;
    etl::string<16> wifiIP;
    int8_t wifiRssi = 0;
    bool ntpSynced = false;

    // ═══════════════════════════════════════════════════
    // AUDIO
    // ═══════════════════════════════════════════════════
    uint8_t volume = 80; // 0-100 percentage
    bool muted = false;
    bool adhanAvailable = false;

    // ═══════════════════════════════════════════════════
    // STATUS SCREENS (Connecting, Portal, Error, Message)
    // ═══════════════════════════════════════════════════
    StatusScreenType statusScreen = StatusScreenType::NONE;
    etl::string<32> statusTitle;
    etl::string<48> statusLine1;
    etl::string<48> statusLine2;
    etl::string<48> statusLine3;

    // ═══════════════════════════════════════════════════
    // DIRTY FLAGS
    // ═══════════════════════════════════════════════════
    volatile uint16_t dirty = DirtyFlag::NONE;

    // ═══════════════════════════════════════════════════
    // HELPER METHODS
    // ═══════════════════════════════════════════════════
    void markDirty(uint16_t flag) { dirty |= flag; }
    bool isDirty(uint16_t flag) const { return (dirty & flag) != 0; }
    void clearDirty(uint16_t flag) { dirty &= ~flag; }
    void clearAllDirty() { dirty = DirtyFlag::NONE; }
};

// Global instance (defined in app_state.cpp)
extern AppState g_state;

// ═══════════════════════════════════════════════════════════════
// CONVENIENCE SETTERS (safe string copy + dirty flag)
// ═══════════════════════════════════════════════════════════════
namespace AppStateHelper
{
    // Set time (second=0 default for callers that don't have seconds)
    inline void setTime(int8_t hour, int8_t minute, int8_t second = 0)
    {
        if (g_state.hour != hour || g_state.minute != minute || g_state.second != second)
        {
            g_state.hour = hour;
            g_state.minute = minute;
            g_state.second = second;
            g_state.markDirty(DirtyFlag::TIME);
        }
    }

    // Set date string
    inline void setDate(etl::string_view date)
    {
        if (g_state.date != date)
        {
            g_state.date.assign(date.begin(), date.end());
            g_state.markDirty(DirtyFlag::DATE);
        }
    }

    // Set location string (e.g., "Istanbul • Diyanet")
    inline void setLocation(etl::string_view location)
    {
        if (g_state.location != location)
        {
            g_state.location.assign(location.begin(), location.end());
            g_state.markDirty(DirtyFlag::LOCATION);
        }
    }

    // Set next prayer card
    inline void setNextPrayer(etl::string_view name, etl::string_view time)
    {
        bool changed = false;
        if (g_state.nextPrayerName != name)
        {
            g_state.nextPrayerName.assign(name.begin(), name.end());
            changed = true;
        }
        if (g_state.nextPrayerTime != time)
        {
            g_state.nextPrayerTime.assign(time.begin(), time.end());
            changed = true;
        }
        if (changed)
            g_state.markDirty(DirtyFlag::NEXT_PRAYER);
    }

    // Set all prayer times
    inline void setPrayerTimes(etl::string_view fajr, etl::string_view sunrise,
                               etl::string_view dhuhr, etl::string_view asr,
                               etl::string_view maghrib, etl::string_view isha,
                               int8_t activeIndex)
    {
        g_state.fajr.assign(fajr.begin(), fajr.end());
        g_state.sunrise.assign(sunrise.begin(), sunrise.end());
        g_state.dhuhr.assign(dhuhr.begin(), dhuhr.end());
        g_state.asr.assign(asr.begin(), asr.end());
        g_state.maghrib.assign(maghrib.begin(), maghrib.end());
        g_state.isha.assign(isha.begin(), isha.end());
        g_state.activePrayerIndex = activeIndex;
        g_state.markDirty(DirtyFlag::PRAYER_TIMES);
    }

    // Set WiFi state
    inline void setWifiState(WifiState state, const char *ip = nullptr)
    {
        if (g_state.wifiState != state)
        {
            g_state.wifiState = state;
            g_state.markDirty(DirtyFlag::WIFI_STATUS);
        }
        if (ip != nullptr)
        {
            g_state.wifiIP = ip;
            g_state.markDirty(DirtyFlag::WIFI_STATUS);
        }
    }

    // Set volume (0-100 percentage)
    inline void setVolume(uint8_t vol)
    {
        if (vol > 100)
            vol = 100;
        if (g_state.volume != vol)
        {
            g_state.volume = vol;
            g_state.markDirty(DirtyFlag::VOLUME);
        }
    }

    // Set muted
    inline void setMuted(bool muted)
    {
        if (g_state.muted != muted)
        {
            g_state.muted = muted;
            g_state.markDirty(DirtyFlag::MUTED);
        }
    }

    // Set NTP synced status
    inline void setNtpSynced(bool synced)
    {
        if (g_state.ntpSynced != synced)
        {
            g_state.ntpSynced = synced;
            g_state.markDirty(DirtyFlag::NTP_SYNCED);
        }
    }

    // Set adhan file available
    inline void setAdhanAvailable(bool available)
    {
        if (g_state.adhanAvailable != available)
        {
            g_state.adhanAvailable = available;
            g_state.markDirty(DirtyFlag::ADHAN_AVAILABLE);
        }
    }

    // Set WiFi signal strength (0-3 bars)
    inline void setWifiSignal(uint8_t bars)
    {
        if (g_state.wifiStrength != bars)
        {
            g_state.wifiStrength = bars;
            g_state.markDirty(DirtyFlag::SIGNAL_BATTERY);
        }
    }

    // Set battery status
    inline void setBatteryStatus(uint8_t pct, bool isCharging)
    {
        if (g_state.batteryPct != pct || g_state.charging != isCharging)
        {
            g_state.batteryPct = pct;
            g_state.charging = isCharging;
            g_state.markDirty(DirtyFlag::SIGNAL_BATTERY);
        }
    }

    // Show connecting screen
    inline void showConnecting(etl::string_view ssid)
    {
        g_state.statusScreen = StatusScreenType::CONNECTING;
        g_state.statusTitle = "Baglaniyor...";
        g_state.statusLine1.assign(ssid.begin(), ssid.end());
        g_state.statusLine2.clear();
        g_state.statusLine3.clear();
        g_state.markDirty(DirtyFlag::STATUS_SCREEN);
    }

    // Show portal screen
    inline void showPortal(etl::string_view ssid, etl::string_view password, etl::string_view ip)
    {
        g_state.statusScreen = StatusScreenType::PORTAL;
        g_state.statusTitle = "WiFi Kurulum";
        g_state.statusLine1.assign(ssid.begin(), ssid.end());
        g_state.statusLine2.assign(password.begin(), password.end());
        g_state.statusLine3.assign(ip.begin(), ip.end());
        g_state.markDirty(DirtyFlag::STATUS_SCREEN);
    }

    // Show message screen
    inline void showMessage(etl::string_view title, etl::string_view message)
    {
        g_state.statusScreen = StatusScreenType::MESSAGE;
        g_state.statusTitle.assign(title.begin(), title.end());
        g_state.statusLine1.assign(message.begin(), message.end());
        g_state.statusLine2.clear();
        g_state.statusLine3.clear();
        g_state.markDirty(DirtyFlag::STATUS_SCREEN);
    }

    // Show error screen
    inline void showError(etl::string_view title, etl::string_view message)
    {
        g_state.statusScreen = StatusScreenType::ERROR;
        g_state.statusTitle.assign(title.begin(), title.end());
        g_state.statusLine1.assign(message.begin(), message.end());
        g_state.statusLine2.clear();
        g_state.statusLine3.clear();
        g_state.markDirty(DirtyFlag::STATUS_SCREEN);
    }

    // Set countdown (seconds until next prayer)
    inline void setCountdown(uint32_t seconds)
    {
        g_state.secondsToNext = seconds;

        // Compute Ramadan countdown text at data layer
        // Always count down to Iftar (Maghrib) or Sahur (Fajr), not just next prayer
        if (g_state.ramadanMode)
        {
            // Parse current time
            int nowSec = g_state.hour * 3600 + g_state.minute * 60 + g_state.second;

            // Parse Maghrib (Iftar) time from "HH:MM" string
            int maghribSec = -1;
            if (g_state.maghrib.size() >= 5)
            {
                int mh = (g_state.maghrib[0] - '0') * 10 + (g_state.maghrib[1] - '0');
                int mm = (g_state.maghrib[3] - '0') * 10 + (g_state.maghrib[4] - '0');
                maghribSec = mh * 3600 + mm * 60;
            }

            // Parse Fajr (Sahur) time from "HH:MM" string
            int fajrSec = -1;
            if (g_state.fajr.size() >= 5)
            {
                int fh = (g_state.fajr[0] - '0') * 10 + (g_state.fajr[1] - '0');
                int fm = (g_state.fajr[3] - '0') * 10 + (g_state.fajr[4] - '0');
                fajrSec = fh * 3600 + fm * 60;
            }

            // Determine: before Maghrib → İftara Kaldı, after Maghrib → Sahura Kaldı
            int targetSec = -1;
            const char *prefix = nullptr;
            if (maghribSec >= 0 && nowSec < maghribSec)
            {
                // Daytime: count to Iftar (Maghrib)
                targetSec = maghribSec - nowSec;
                prefix = "\xc4\xb0"
                         "ftara";
            }
            else if (fajrSec >= 0)
            {
                // Nighttime: count to Sahur (Fajr)
                targetSec = fajrSec - nowSec;
                if (targetSec <= 0)
                    targetSec += 86400; // wrap past midnight
                prefix = "Sahura";
            }

            if (targetSec > 0 && prefix)
            {
                int h = targetSec / 3600;
                int m = (targetSec % 3600) / 60;
                char buf[40];
                snprintf(buf, sizeof(buf), "%s %02d:%02d Kald\xc4\xb1", prefix, h, m);
                g_state.ramadanCountdownText.assign(buf);
            }
            else
            {
                g_state.ramadanCountdownText.clear();
            }
        }
        else
        {
            g_state.ramadanCountdownText.clear();
        }

        g_state.markDirty(DirtyFlag::COUNTDOWN);
    }

    // Set Hijri date string separately from Gregorian
    inline void setHijriDate(etl::string_view hijri)
    {
        if (g_state.hijriDate != hijri)
        {
            g_state.hijriDate.assign(hijri.begin(), hijri.end());
            g_state.markDirty(DirtyFlag::HIJRI);
        }
    }

    // Set prayer slot progress 0-100
    inline void setProgress(uint8_t pct)
    {
        if (g_state.activePrayerProgress != pct)
        {
            g_state.activePrayerProgress = pct;
            g_state.markDirty(DirtyFlag::PROGRESS);
        }
    }

    // Clear status screen (return to normal UI)
    inline void clearStatusScreen()
    {
        if (g_state.statusScreen != StatusScreenType::NONE)
        {
            g_state.statusScreen = StatusScreenType::NONE;
            g_state.markDirty(DirtyFlag::STATUS_SCREEN);
        }
    }

} // namespace AppStateHelper

#endif // APP_STATE_H
