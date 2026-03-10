#include "display_ticker.h"
#include "app_state.h"
#include "hijri_date.h"
#include "locale_tr.h"
#include "pmu_manager.h"
#include "network.h"
#include <WiFi.h>
#include <Arduino.h>

namespace DisplayTicker
{
    static int s_lastMinute = -1;
    static unsigned long s_lastCheck = 0;
    static unsigned long s_lastSignalCheck = 0;

    // Push Gregorian + Hijri date strings into AppState.
    // Pure data transformation — no LVGL or display knowledge.
    static void pushDateTime(const struct tm &t)
    {
        // Status bar date: "· 8 Mart · Cuma"
        char buf[48];
        snprintf(buf, sizeof(buf), "\xC2\xB7 %d %s \xC2\xB7 %s",
                 t.tm_mday, LocaleTR::MONTHS[t.tm_mon], LocaleTR::DAYS[t.tm_wday]);
        AppStateHelper::setDate(buf);

        // Full date for clock page: "8 Mart 2026 · Cuma"
        char gbuf[48];
        snprintf(gbuf, sizeof(gbuf), "%d %s %d \xC2\xB7 %s",
                 t.tm_mday, LocaleTR::MONTHS[t.tm_mon], 1900 + t.tm_year, LocaleTR::DAYS[t.tm_wday]);
        g_state.gregorianFull = gbuf;

        HijriDate h = gregorianToHijri(1900 + t.tm_year, t.tm_mon + 1, t.tm_mday);
        char hbuf[32];
        snprintf(hbuf, sizeof(hbuf), "%d %s %d", h.day, getHijriMonth(h.month), h.year);
        AppStateHelper::setHijriDate(hbuf);
    }

    static void pushNtpStatus()
    {
        struct tm tmp;
        AppStateHelper::setNtpSynced(getLocalTime(&tmp, 0));
    }

    void forceUpdate()
    {
        struct tm t;
        if (!getLocalTime(&t))
            return;
        AppStateHelper::setTime(t.tm_hour, t.tm_min, t.tm_sec);
        pushDateTime(t);
        pushNtpStatus();
        s_lastMinute = t.tm_hour * 60 + t.tm_min;
    }

    void tick()
    {
        unsigned long now = millis();
        if (now - s_lastCheck < 1000)
            return;
        s_lastCheck = now;

        struct tm t;
        if (!getLocalTime(&t))
            return;

        // Update time every second (Clock screen needs seconds)
        AppStateHelper::setTime(t.tm_hour, t.tm_min, t.tm_sec);

        // Update date/Hijri/NTP only on minute boundary to keep load low
        int currentMinute = t.tm_hour * 60 + t.tm_min;
        if (currentMinute != s_lastMinute)
        {
            s_lastMinute = currentMinute;
            pushDateTime(t);
            pushNtpStatus();
        }

        // Update WiFi signal + battery every 5 seconds
        if (now - s_lastSignalCheck >= 5000)
        {
            s_lastSignalCheck = now;

            // WiFi signal: RSSI → 0-3 bars
            uint8_t bars = 0;
            if (WiFi.isConnected())
            {
                int rssi = WiFi.RSSI();
                if (rssi > -50)
                    bars = 3;
                else if (rssi > -70)
                    bars = 2;
                else if (rssi > -85)
                    bars = 1;
            }
            AppStateHelper::setWifiSignal(bars);

            // Battery
            int pct = PmuManager::getBatteryPercent();
            if (pct < 0)
                pct = 0;
            if (pct > 100)
                pct = 100;
            AppStateHelper::setBatteryStatus(static_cast<uint8_t>(pct), PmuManager::isCharging());
        }
    }

} // namespace DisplayTicker
