#include "test_mode.h"
#include "prayer_calculator.h"
#include "config.h"
#include "daily_prayers.h"
#include "prayer_types.h"
#include <Arduino.h>
#include <time.h>

namespace TestMode
{
    static int daysInMonth(int year, int month)
    {
        static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        const bool leap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
        if (month == 2 && leap)
        {
            return 29;
        }
        return days[month - 1];
    }

    void print30DaysAdhanLibrary(int method, double lat, double lon)
    {
        Serial.println("\n═══════════════════════════════════════════════");
        time_t now = time(nullptr);
        struct tm now_tm;
        localtime_r(&now, &now_tm);

        struct tm start = now_tm;
        start.tm_mday = 1;
        start.tm_hour = 12;
        start.tm_min = 0;
        start.tm_sec = 0;
        time_t start_time = mktime(&start);

        const int year = start.tm_year + 1900;
        const int month = start.tm_mon + 1;
        const int days_this_month = daysInMonth(year, month);

        Serial.printf("  30-DAY PRAYER TIME TEST - Method %d (%04d-%02d)\n", method, year, month);
        Serial.printf("  Location: %.4f, %.4f\n", lat, lon);
        Serial.println("═══════════════════════════════════════════════\n");
        Serial.println("Date       | Fajr  | Dhuhr | Asr   | Maghrib | Isha");
        Serial.println("-----------|-------|-------|-------|---------|-------");

        int baseOffset = (start_time - now) / 86400;

        for (int day = 0; day < days_this_month; day++)
        {
            DailyPrayers prayers;
            int actualOffset = baseOffset + day;

            if (PrayerCalculator::calculateTimes(prayers, method, lat, lon, actualOffset))
            {
                struct tm displayDate = start;
                displayDate.tm_mday += day;
                mktime(&displayDate);

                Serial.printf("%04d-%02d-%02d | %s | %s | %s | %s   | %s\n",
                              displayDate.tm_year + 1900,
                              displayDate.tm_mon + 1,
                              displayDate.tm_mday,
                              prayers[PrayerType::Fajr].value.data(),
                              prayers[PrayerType::Dhuhr].value.data(),
                              prayers[PrayerType::Asr].value.data(),
                              prayers[PrayerType::Maghrib].value.data(),
                              prayers[PrayerType::Isha].value.data());
            }
            else
            {
                Serial.printf("Day %d: FAILED\n", day + 1);
            }
        }

        Serial.println("-----------|-------|-------|-------|---------|-------");
        Serial.println("\n✅ Test Complete - Copy this output for comparison\n");
    }

    void compareAllMethods(double lat, double lon)
    {
        Serial.println("\n═══════════════════════════════════════════════");
        Serial.println("  COMPARISON: ALL CALCULATION METHODS");
        Serial.printf("  Location: %.4f, %.4f\n", lat, lon);
        Serial.println("═══════════════════════════════════════════════\n");

        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        Serial.printf("Date: %04d-%02d-%02d\n\n",
                      timeinfo.tm_year + 1900,
                      timeinfo.tm_mon + 1,
                      timeinfo.tm_mday);

        const char *methodNames[] = {
            "Shia Ithna-Ansari",                             // 0
            "University of Islamic Sciences, Karachi",       // 1
            "Islamic Society of North America",              // 2
            "Muslim World League (MWL)",                     // 3
            "Umm Al-Qura University",                        // 4
            "Egyptian General Authority",                    // 5
            "Institute of Geophysics, University of Tehran", // 6
            "Dubai",                                         // 7
            "Kuwait",                                        // 8
            "Qatar",                                         // 9
            "Singapore",                                     // 10
            "France (UOIF)",                                 // 11
            "France (UOIF)",                                 // 12
            "Turkey (Diyanet)"                               // 13
        };

        Serial.println("Method | Name                                  | Fajr  | Isha");
        Serial.println("-------|---------------------------------------|-------|-------");

        for (int method = 0; method <= 13; method++)
        {
            DailyPrayers prayers;
            if (PrayerCalculator::calculateTimes(prayers, method, lat, lon, 0))
            {
                Serial.printf("  %2d   | %-37s | %s | %s\n",
                              method,
                              methodNames[method],
                              prayers[PrayerType::Fajr].value.data(),
                              prayers[PrayerType::Isha].value.data());
            }
        }

        Serial.println("-------|---------------------------------------|-------|-------");
        Serial.println("\n✅ Method comparison complete\n");
    }

    void runPrayerTimeTests()
    {
        Serial.println("\n\n");
        Serial.println("╔═══════════════════════════════════════════════╗");
        Serial.println("║         🧪 PRAYER TIME TEST MODE 🧪          ║");
        Serial.println("╚═══════════════════════════════════════════════╝");
        Serial.println();

        // Wait for RTC sync
        Serial.println("[Test] Waiting for RTC sync...");
        struct tm timeinfo;
        int retries = 0;
        while (!getLocalTime(&timeinfo) && retries < 30)
        {
            delay(500);
            retries++;
        }

        if (retries >= 30)
        {
            Serial.println("[Test] ❌ ERROR: RTC not synced - cannot run tests");
            Serial.println("[Test] Please ensure WiFi is connected and NTP is reachable");
            return;
        }

        Serial.printf("[Test] ✅ RTC synced: %04d-%02d-%02d %02d:%02d:%02d\n\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        // Run 30-day test with configured method
        Serial.printf("[Test] Running: 30-day prayer times (Method %d)\n", Config::PRAYER_METHOD);
        print30DaysAdhanLibrary(Config::PRAYER_METHOD, Config::LATITUDE, Config::LONGITUDE);

        Serial.println("\n╔═══════════════════════════════════════════════╗");
        Serial.println("║          🎉 ALL TESTS COMPLETED 🎉           ║");
        Serial.println("╚═══════════════════════════════════════════════╝\n");
        Serial.println("Copy the output above and compare with API results");
        Serial.println("Test mode will stay active - upload again to exit test mode\n");

        // No restart - stay in test mode until new upload
        while (true)
        {
            delay(1000);
        }
    }
}
