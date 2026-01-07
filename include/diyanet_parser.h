#pragma once
#include "prayer_time.h"
#include "prayer_types.h"
#include "daily_prayers.h"
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <ctime>

namespace DiyanetParser
{

    constexpr size_t TIME_STRING_LENGTH = 5; // "HH:MM" format
    constexpr time_t SECONDS_PER_DAY = 86400;

    /**
     * Parse Diyanet time format "HH:MM" into PrayerTime
     * Returns true if parsing succeeded
     */
    inline bool parseTime(const char *hhMM, PrayerTime &out)
    {
        if (!hhMM || std::strlen(hhMM) < TIME_STRING_LENGTH)
        {
            return false;
        }

        // Validate format: digits, colon, digits
        if (hhMM[0] < '0' || hhMM[0] > '2')
            return false;
        if (hhMM[1] < '0' || hhMM[1] > '9')
            return false;
        if (hhMM[2] != ':')
            return false;
        if (hhMM[3] < '0' || hhMM[3] > '5')
            return false;
        if (hhMM[4] < '0' || hhMM[4] > '9')
            return false;

        std::memcpy(out.value.data(), hhMM, TIME_STRING_LENGTH);
        out.value[TIME_STRING_LENGTH] = '\0';
        return true;
    }

    /**
     * Parse Diyanet date format "DD.MM.YYYY" into components
     * Returns true if parsing succeeded
     */
    inline bool parseDate(const char *dateStr, int &day, int &month, int &year)
    {
        if (!dateStr || std::strlen(dateStr) < 10)
        {
            return false;
        }

        // Format: "31.12.2025"
        if (sscanf(dateStr, "%d.%d.%d", &day, &month, &year) != 3)
        {
            return false;
        }

        // Basic validation
        if (day < 1 || day > 31)
            return false;
        if (month < 1 || month > 12)
            return false;
        if (year < 2020 || year > 2100)
            return false;

        return true;
    }

    /**
     * Calculate day offset from cache start to target date
     */
    inline int calculateDayOffset(time_t cacheStart, time_t targetDate)
    {
        return static_cast<int>((targetDate - cacheStart) / SECONDS_PER_DAY);
    }

    /**
     * Check if day offset is valid for given cache size
     */
    inline bool isDayOffsetValid(int offset, int totalDays)
    {
        return offset >= 0 && offset < totalDays;
    }

    /**
     * Check if cache needs refresh based on age
     */
    inline bool isCacheExpired(time_t fetchedAt, time_t now, int maxAgeDays)
    {
        const time_t age = now - fetchedAt;
        const int daysOld = static_cast<int>(age / SECONDS_PER_DAY);
        return daysOld >= maxAgeDays;
    }

} // namespace DiyanetParser
