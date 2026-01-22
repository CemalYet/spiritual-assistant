#pragma once
#include "daily_prayers.h"

namespace PrayerAPI
{
    // Cache info for status display
    struct CacheInfo
    {
        int ilceId;
        int daysRemaining;
        bool isValid;
    };

    // Fetch 30 days of prayer times from Diyanet API and cache them
    // Returns true if successful (either from cache or fresh fetch)
    bool fetchMonthlyPrayerTimes(int ilceId = 0);

    // Get cached prayer times for today or tomorrow
    // Returns false if cache is invalid/expired
    bool getCachedPrayerTimes(DailyPrayers &prayers, bool forTomorrow = false);

    // Get cache status info for display
    CacheInfo getCacheInfo();
}
