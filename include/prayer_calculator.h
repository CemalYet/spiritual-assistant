#pragma once
#include "daily_prayers.h"

namespace PrayerCalculator
{
    // Calculate prayer times using Adhan library (offline calculation)
    bool calculateTimes(DailyPrayers &prayers, int method, double latitude, double longitude, int day = 0, bool verbose = true);

    // Helper: Get method name for display
    const char *getMethodName(int method);
}
