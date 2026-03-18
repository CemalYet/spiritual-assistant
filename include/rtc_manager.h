#pragma once

#include <cstdint>
#include <ctime>

namespace RtcManager
{
    constexpr unsigned long NTP_SYNC_INTERVAL_RTC = 6UL * 60 * 60 * 1000;
    constexpr unsigned long NTP_SYNC_INTERVAL_NO_RTC = 12UL * 60 * 60 * 1000;
    constexpr unsigned long RTC_DRIFT_CHECK_INTERVAL = 5UL * 60 * 1000; // 5 minutes
    constexpr unsigned long NTP_TRUST_WINDOW_MS = 10UL * 60 * 1000;     // 10 minutes

    bool init();
    bool setSystemClockFromRTC();
    void writeSystemClockToRTC(time_t exactEpoch = 0);
    bool isAvailable();
    bool hasValidTime();
    void markNtpSync();
    bool periodicSyncTick();
    void correctDriftFromRTC();
    void resetSyncTimer();
    void postponeSync(unsigned long ms);

    void setAlarm(uint8_t hour, uint8_t minute, uint8_t second);
    void clearAlarm();
    bool isAlarmActive();
}
