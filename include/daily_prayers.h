#pragma once
#include "prayer_types.h"
#include "prayer_time.h"
#include <array>
#include <optional>

class DailyPrayers
{
private:
    std::array<PrayerTime, 6> _times;

public:
    const PrayerTime &operator[](PrayerType type) const
    {
        return _times[idx(type)];
    }

    PrayerTime &operator[](PrayerType type)
    {
        return _times[idx(type)];
    }

    std::optional<PrayerType> findNext(int currentMinutes) const
    {
        for (uint8_t i = 0; i < 6; ++i)
        {
            if (!_times[i].isEmpty() && _times[i].toMinutes() > currentMinutes)
            {
                return PrayerType(i);
            }
        }
        return std::nullopt;
    }

    int minutesUntilNext(int currentMinutes) const
    {
        for (uint8_t i = 0; i < 6; ++i)
        {
            if (!_times[i].isEmpty() && _times[i].toMinutes() > currentMinutes)
            {
                return _times[i].toMinutes() - currentMinutes;
            }
        }
        return -1;
    }
};
