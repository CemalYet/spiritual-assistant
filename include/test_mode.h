#pragma once

namespace TestMode
{
    // Run prayer time comparison tests
    void runPrayerTimeTests();

    // Print 30 days of prayer times for manual comparison
    void print30DaysAdhanLibrary(int method, double lat, double lon);

    // Print today's prayer times in multiple methods
    void compareAllMethods(double lat, double lon);
}
