#include "rtc_manager.h"
#include "tft_config.h"
#include <SensorPCF85063.hpp>
#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

namespace
{
    SensorPCF85063 rtc;
    SemaphoreHandle_t s_i2cMutex = nullptr;

    bool s_available = false;
    bool s_validTime = false;
    unsigned long s_lastSyncMs = 0;
    unsigned long s_lastDriftCheckMs = 0;
    unsigned long s_lastNtpSyncMs = 0;

    constexpr uint16_t MIN_VALID_YEAR = 2024;
    constexpr uint8_t NO_ALARM = 0xFF;
    constexpr int TM_YEAR_BASE = 1900;
    constexpr long SECONDS_PER_DAY = 86400L;
    constexpr int SECONDS_PER_HOUR = 3600;
    constexpr int SECONDS_PER_MINUTE = 60;

    /// UTC struct tm → time_t (Rata Die algorithm, no TZ dependency).
    /// Reference: http://howardhinnant.github.io/date_algorithms.html
    time_t utcToEpoch(const struct tm &t)
    {
        int year = t.tm_year + TM_YEAR_BASE;
        int mon = t.tm_mon + 1; // 0-based → 1-based

        // Rata Die: Jan/Feb as months 13/14 of previous year
        if (mon <= 2)
        {
            year--;
            mon += 12;
        }

        long days = 365L * year + year / 4 - year / 100 + year / 400 + (153L * (mon - 3) + 2) / 5 + t.tm_mday - 719469L;

        return static_cast<time_t>(days) * SECONDS_PER_DAY + t.tm_hour * SECONDS_PER_HOUR + t.tm_min * SECONDS_PER_MINUTE + t.tm_sec;
    }
}

namespace RtcManager
{
    void markNtpSync()
    {
        s_lastNtpSyncMs = millis();
    }

    bool init()
    {
        if (s_available)
            return true;

        if (!s_i2cMutex)
            s_i2cMutex = xSemaphoreCreateMutex();

        // I2C bus is initialized by PMU init earlier in boot sequence.
        if (!rtc.begin(Wire))
        {
            Serial.println("[RTC] PCF85063 not found at 0x51");
            return false;
        }

        s_available = true;

        if (!rtc.isClockIntegrityGuaranteed())
        {
            Serial.println("[RTC] Clock integrity lost — need NTP");

            s_validTime = false;
            return true;
        }

        // Quick sanity: year must be >= 2024
        RTC_DateTime dt = rtc.getDateTime();
        if (dt.getYear() < MIN_VALID_YEAR)
        {
            Serial.printf("[RTC] Year %u too old — need NTP\n", dt.getYear());
            s_validTime = false;
            return true;
        }

        s_validTime = true;
        Serial.printf("[RTC] OK — %04u-%02u-%02u %02u:%02u:%02u UTC\n",
                      dt.getYear(), dt.getMonth(), dt.getDay(),
                      dt.getHour(), dt.getMinute(), dt.getSecond());
        return true;
    }

    bool setSystemClockFromRTC()
    {
        if (!s_available || !s_validTime)
            return false;

        if (xSemaphoreTake(s_i2cMutex, pdMS_TO_TICKS(100)) != pdTRUE)
            return false;
        RTC_DateTime dt = rtc.getDateTime();
        xSemaphoreGive(s_i2cMutex);

        struct tm timeinfo = {};
        timeinfo.tm_year = dt.getYear() - TM_YEAR_BASE;
        timeinfo.tm_mon = dt.getMonth() - 1; // 0-based
        timeinfo.tm_mday = dt.getDay();
        timeinfo.tm_hour = dt.getHour();
        timeinfo.tm_min = dt.getMinute();
        timeinfo.tm_sec = dt.getSecond();

        // timegm unavailable on ESP32 newlib
        time_t epoch = utcToEpoch(timeinfo);
        if (epoch < 0)
            return false;

        struct timeval tv = {.tv_sec = epoch, .tv_usec = 0};
        settimeofday(&tv, nullptr);

        Serial.printf("[RTC] System clock set from RTC: %04u-%02u-%02u %02u:%02u:%02u UTC\n",
                      dt.getYear(), dt.getMonth(), dt.getDay(),
                      dt.getHour(), dt.getMinute(), dt.getSecond());
        return true;
    }

    void writeSystemClockToRTC(time_t exactEpoch)
    {
        if (!s_available)
            return;

        // Use exact epoch if provided (from SNTP callback), otherwise read system clock
        time_t now = (exactEpoch > 0) ? exactEpoch : time(nullptr);
        struct tm utc;
        gmtime_r(&now, &utc);

        Serial.printf("[RTC] Writing to RTC: %04d-%02d-%02d %02d:%02d:%02d UTC (epoch=%ld)\n",
                      utc.tm_year + TM_YEAR_BASE, utc.tm_mon + 1, utc.tm_mday,
                      utc.tm_hour, utc.tm_min, utc.tm_sec, (long)now);

        if (xSemaphoreTake(s_i2cMutex, pdMS_TO_TICKS(100)) != pdTRUE)
            return;
        rtc.setDateTime(
            utc.tm_year + TM_YEAR_BASE,
            utc.tm_mon + 1, // 0-based → 1-based
            utc.tm_mday,
            utc.tm_hour,
            utc.tm_min,
            utc.tm_sec);
        xSemaphoreGive(s_i2cMutex);

        s_validTime = true;
        s_lastSyncMs = millis();
    }

    bool isAvailable()
    {
        return s_available;
    }

    bool hasValidTime()
    {
        return s_available && s_validTime;
    }

    bool periodicSyncTick()
    {
        unsigned long interval = (s_available && s_validTime)
                                     ? NTP_SYNC_INTERVAL_RTC
                                     : NTP_SYNC_INTERVAL_NO_RTC;

        if (millis() - s_lastSyncMs < interval)
            return false;

        // Don't advance timer here — syncTime() calls resetSyncTimer() on success
        return true;
    }

    void resetSyncTimer()
    {
        s_lastSyncMs = millis();
        s_lastDriftCheckMs = millis();
    }

    void postponeSync(unsigned long ms)
    {
        s_lastSyncMs = millis() - (s_validTime ? NTP_SYNC_INTERVAL_RTC : NTP_SYNC_INTERVAL_NO_RTC) + ms;
    }

    void correctDriftFromRTC()
    {
        if (!s_available || !s_validTime)
            return;

        unsigned long now = millis();
        if (now - s_lastDriftCheckMs < RTC_DRIFT_CHECK_INTERVAL)
            return;
        s_lastDriftCheckMs = now;

        if (xSemaphoreTake(s_i2cMutex, pdMS_TO_TICKS(100)) != pdTRUE)
            return;
        RTC_DateTime dt = rtc.getDateTime();
        xSemaphoreGive(s_i2cMutex);
        struct tm rtcTm = {};
        rtcTm.tm_year = dt.getYear() - TM_YEAR_BASE;
        rtcTm.tm_mon = dt.getMonth() - 1;
        rtcTm.tm_mday = dt.getDay();
        rtcTm.tm_hour = dt.getHour();
        rtcTm.tm_min = dt.getMinute();
        rtcTm.tm_sec = dt.getSecond();
        time_t rtcEpoch = utcToEpoch(rtcTm);

        time_t sysEpoch = time(nullptr);
        long drift = static_cast<long>(sysEpoch - rtcEpoch);
        bool ntpFresh = (s_lastNtpSyncMs != 0) && ((now - s_lastNtpSyncMs) < NTP_TRUST_WINDOW_MS);

        struct tm sysTm;
        gmtime_r(&sysEpoch, &sysTm);
        Serial.printf("[RTC] Drift check: sys=%02d:%02d:%02d rtc=%02u:%02u:%02u drift=%+ld sec\n",
                      sysTm.tm_hour, sysTm.tm_min, sysTm.tm_sec,
                      dt.getHour(), dt.getMinute(), dt.getSecond(), drift);

        if (drift >= -1 && drift <= 1)
            return;

        auto setSystemFromRtc = [&]()
        {
            struct timeval tv = {.tv_sec = rtcEpoch, .tv_usec = 0};
            settimeofday(&tv, nullptr);
        };

        if (drift < -1)
        {
            // RTC ahead of system → system drifted, update system from RTC
            setSystemFromRtc();
            Serial.printf("[RTC] System updated from RTC: drift was %+ld sec\n", drift);
            return;
        }

        if (!ntpFresh)
        {
            // Protect external RTC from sleep-induced system drift.
            setSystemFromRtc();
            Serial.printf("[RTC] System corrected from RTC (NTP stale): drift was %+ld sec\n", drift);
            return;
        }

        // Fresh NTP means system time is trusted for a short window.
        writeSystemClockToRTC(0);
        Serial.printf("[RTC] RTC updated from trusted system (fresh NTP): drift was %+ld sec\n", drift);
    }

    // Alarm functions

    void setAlarm(uint8_t hour, uint8_t minute, uint8_t second)
    {
        if (!s_available)
            return;

        rtc.setAlarm(hour, minute, second, NO_ALARM, NO_ALARM);
        rtc.resetAlarm();  // Clear any pending alarm flag
        rtc.enableAlarm(); // Enable alarm interrupt on INT pin

        Serial.printf("[RTC] Alarm set: %02u:%02u:%02u UTC\n",
                      hour, minute, second);
    }

    void clearAlarm()
    {
        if (!s_available)
            return;

        rtc.disableAlarm();
        rtc.resetAlarm();
    }

    bool isAlarmActive()
    {
        if (!s_available)
            return false;

        return rtc.isAlarmActive();
    }
}
