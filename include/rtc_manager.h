#pragma once

#include <cstdint>

namespace RtcManager
{
    constexpr unsigned long NTP_SYNC_INTERVAL_RTC = 24UL * 60 * 60 * 1000;
    constexpr unsigned long NTP_SYNC_INTERVAL_NO_RTC = 12UL * 60 * 60 * 1000;
    constexpr unsigned long RTC_DRIFT_CHECK_INTERVAL = 5UL * 60 * 1000; // 5 minutes

    bool init();
    bool setSystemClockFromRTC();
    void writeSystemClockToRTC();
    bool isAvailable();
    bool hasValidTime();
    bool periodicSyncTick();
    void correctDriftFromRTC();
    void resetSyncTimer();
    void postponeSync(unsigned long ms);

    void setAlarm(uint8_t hour, uint8_t minute, uint8_t second);
    void clearAlarm();
    bool isAlarmActive();
}
