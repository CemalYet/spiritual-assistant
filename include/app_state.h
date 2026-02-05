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
    etl::string<32> date;
    etl::string<48> location; // "Istanbul • Diyanet"

    // ═══════════════════════════════════════════════════
    // PRAYER DATA
    // ═══════════════════════════════════════════════════
    etl::string<16> nextPrayerName;
    etl::string<8> nextPrayerTime;

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
    int8_t volume = 3; // 0-5
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
    // Set time
    inline void setTime(int8_t hour, int8_t minute)
    {
        if (g_state.hour != hour || g_state.minute != minute)
        {
            g_state.hour = hour;
            g_state.minute = minute;
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

    // Set volume (0-5)
    inline void setVolume(int8_t level)
    {
        if (level < 0)
            level = 0;
        if (level > 5)
            level = 5;
        if (g_state.volume != level)
        {
            g_state.volume = level;
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
