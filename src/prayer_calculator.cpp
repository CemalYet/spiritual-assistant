#include "prayer_calculator.h"
#include "config.h"
#include "settings_manager.h"
#include "prayer_types.h"
#include <Arduino.h>
#include <time.h>
#include <algorithm>
#include <iterator>

extern "C"
{
#include "prayer_times.h"
#include "data_components.h"
#include "calculation_parameters.h"
#include "calculation_method.h"
#include "coordinates.h"
#include "madhab.h"
#include "high_latitude_rule.h"
}

namespace
{
    struct NetAdjustments
    {
        int fajr = 0;
        int sunrise = 0;
        int dhuhr = 0;
        int asr = 0;
        int maghrib = 0;
        int isha = 0;
    };

    struct MethodSpec
    {
        int id;
        const char *name;
        calculation_method calcMethod;
        bool overrideAngles;
        double fajrAngle;
        double ishaAngle;
        int ishaInterval;
        bool forceHanafi;
        bool warnTehranMaghribAngle;
        NetAdjustments dartNet;
    };

    static constexpr MethodSpec kMethodSpecs[] = {
        // id, name, calcMethod, overrideAngles, fajr, isha, interval, forceHanafi, warnTehran, dartNet
        {1, "Karachi", KARACHI, false, 0, 0, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {2, "ISNA", NORTH_AMERICA, false, 0, 0, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {3, "MWL", MUSLIM_WORLD_LEAGUE, false, 0, 0, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {4, "Umm al-Qura", UMM_AL_QURA, false, 0, 0, 0, false, false, NetAdjustments{}},
        // Egyptian: override to Dart (19.5/17.5)
        {5, "Egyptian", EGYPTIAN, true, 19.5, 17.5, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        // Gulf: Fajr 19.5°, Isha 90 minutes after Maghrib
        {6, "Gulf", GULF, false, 0, 0, 0, false, false, NetAdjustments{}},
        // Tehran: not supported fully (maghribAngle missing)
        {7, "Tehran", OTHER, false, 17.7, 14.0, 0, false, true, NetAdjustments{}},
        {8, "Dubai", OTHER, false, 18.2, 18.2, 0, false, false, NetAdjustments{0, -3, 3, 3, 3, 0}},
        {9, "Kuwait", KUWAIT, false, 0, 0, 0, false, false, NetAdjustments{}},
        {10, "Qatar", QATAR, false, 0, 0, 0, false, false, NetAdjustments{}},
        {11, "Singapore", OTHER, false, 20.0, 18.0, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {12, "France UOIF", OTHER, false, 12.0, 12.0, 0, false, false, NetAdjustments{}},
        {13, "Turkey Diyanet", OTHER, false, 18.0, 17.0, 0, false, false, NetAdjustments{0, -7, 5, 4, 7, 0}},
        {14, "Russia", OTHER, false, 16.0, 15.0, 0, true, false, NetAdjustments{}},
        {15, "Moonsighting", MOON_SIGHTING_COMMITTEE, false, 0, 0, 0, false, false, NetAdjustments{0, 0, 5, 0, 3, 0}},
    };

    static const MethodSpec *findMethodSpec(int method)
    {
        auto it = std::find_if(std::begin(kMethodSpecs), std::end(kMethodSpecs),
                               [method](const MethodSpec &spec)
                               { return spec.id == method; });
        return (it != std::end(kMethodSpecs)) ? &(*it) : nullptr;
    }

    static const MethodSpec &defaultMethodSpec()
    {
        // Default to MWL (id=3)
        return kMethodSpecs[2];
    }

    static calculation_parameters_t buildParameters(const MethodSpec &spec)
    {
        calculation_parameters_t params;

        if (spec.calcMethod != OTHER)
        {
            params = getParameters(spec.calcMethod);
        }
        else if (spec.ishaInterval > 0)
        {
            params = new_calculation_parameters4(spec.fajrAngle, spec.ishaInterval, OTHER);
        }
        else
        {
            params = new_calculation_parameters3(spec.fajrAngle, spec.ishaAngle, OTHER);
        }

        if (spec.overrideAngles)
        {
            params.fajrAngle = spec.fajrAngle;
            params.ishaAngle = spec.ishaAngle;
        }

        if (spec.forceHanafi)
        {
            params.madhab = madhab_t::HANAFI;
        }

        return params;
    }

    static int internalDhuhrOffsetMinutes(calculation_method calcMethod)
    {
        if (calcMethod == MOON_SIGHTING_COMMITTEE)
            return 5;

        if (calcMethod == UMM_AL_QURA || calcMethod == GULF || calcMethod == QATAR)
            return 0;

        return 1;
    }

    static int internalMaghribOffsetMinutes(calculation_method calcMethod)
    {
        return (calcMethod == MOON_SIGHTING_COMMITTEE) ? 3 : 0;
    }

    static void applyDartNetAdjustments(calculation_parameters_t &params, const MethodSpec &spec)
    {
        const auto &net = spec.dartNet;

        params.adjustments.fajr = net.fajr;
        params.adjustments.sunrise = net.sunrise;
        params.adjustments.asr = net.asr;
        params.adjustments.isha = net.isha;

        // C library already applies internal offsets for Dhuhr/Maghrib; compensate so net output matches Dart.
        params.adjustments.dhuhr = net.dhuhr - internalDhuhrOffsetMinutes(spec.calcMethod);
        params.adjustments.maghrib = net.maghrib - internalMaghribOffsetMinutes(spec.calcMethod);
    }

    // Diyanet high-latitude rules (applies for latitude >= 45°)
    // Based on official Diyanet KARAR (ruling) for prayer times at high latitudes
    //
    // The KARAR solves the problem of "perpetual twilight" in SUMMER when
    // astronomical calculations fail or produce extreme times.
    // In WINTER, astronomical calculations work fine and should be used.
    //
    // Official rules from Diyanet document:
    // a) For lat >= 45°: Isha = Maghrib + 1/3 of Islamic night (= 1/6 of full night)
    // b) For lat >= 52°: Cap Isha offset at 80 minutes (1 hour 20 min)
    // c) For March-September: Fajr = Sunrise - (Isha offset + 10 min)
    // d) For lat >= 62°: Use 62° latitude for calculations
    //
    // Implementation: Use whichever gives EARLIER Isha time:
    // - Winter: Astronomical is earlier → use it (karar not needed)
    // - Summer: Night/6 is earlier or astro fails → apply karar
    static constexpr double DIYANET_HIGH_LAT_THRESHOLD = 45.0;
    static constexpr double DIYANET_ISHA_CAP_THRESHOLD = 52.0; // Cap only applies above this
    static constexpr double DIYANET_MAX_LAT_CLAMP = 62.0;
    static constexpr int DIYANET_ISHA_CAP_MINUTES = 80; // 1 hour 20 minutes
    static constexpr int DIYANET_FAJR_EXTRA_MINUTES = 10;

    static void applyDiyanetHighLatitudeRules(prayer_times_t &times, double latitude, int month)
    {
        if (latitude < DIYANET_HIGH_LAT_THRESHOLD)
        {
            return; // No adjustments needed for lower latitudes
        }

        // Get time components
        const time_t maghrib = times.maghrib;
        const time_t sunrise = times.sunrise;
        const time_t fajr_astro = times.fajr;
        const time_t isha_astro = times.isha;

        struct tm maghribTm, sunriseTm;
        localtime_r(&maghrib, &maghribTm);
        localtime_r(&sunrise, &sunriseTm);

        // Maghrib includes +7 min adjustment, so sunset is maghrib - 7 min
        int sunsetSeconds = maghribTm.tm_hour * 3600 + maghribTm.tm_min * 60 + maghribTm.tm_sec - (7 * 60);
        int sunriseSeconds = sunriseTm.tm_hour * 3600 + sunriseTm.tm_min * 60 + sunriseTm.tm_sec;

        // Full night = (24:00 - sunset) + sunrise
        int nightDuration = (24 * 3600 - sunsetSeconds) + sunriseSeconds;

        // Diyanet KARAR: 1/6 of full night (= 1/3 of Islamic half-night)
        int ishaOffsetSeconds = nightDuration / 6;

        // Rule b) Cap Isha at 80 minutes for latitudes >= 52°
        if (latitude >= DIYANET_ISHA_CAP_THRESHOLD)
        {
            const int maxIshaOffset = DIYANET_ISHA_CAP_MINUTES * 60;
            if (ishaOffsetSeconds > maxIshaOffset)
            {
                ishaOffsetSeconds = maxIshaOffset;
            }
        }

        // Calculate astronomical Isha offset from sunset
        struct tm ishaTm;
        localtime_r(&isha_astro, &ishaTm);
        int ishaAstroSeconds = ishaTm.tm_hour * 3600 + ishaTm.tm_min * 60 + ishaTm.tm_sec;
        int astroIshaOffset = ishaAstroSeconds - sunsetSeconds;

        // Handle case where Isha crosses midnight
        if (astroIshaOffset < 0)
        {
            astroIshaOffset += 24 * 3600;
        }

        // The KARAR applies when astronomical twilight fails or produces extreme times (summer)
        // In winter, when astronomical offset is smaller, use it instead
        // Logic: Use whichever gives the EARLIER Isha time

        if (astroIshaOffset <= ishaOffsetSeconds && astroIshaOffset > 0 && astroIshaOffset < 12 * 3600)
        {
            // Astronomical Isha is earlier (winter) - KARAR not needed
            // Keep the astronomical times from the library
            // (times.isha and times.fajr already set)
        }
        else
        {
            // Night/6 is earlier OR astronomical failed (summer) - apply KARAR
            times.isha = maghrib + ishaOffsetSeconds;

            // Rule c) Fajr = Sunrise - (Isha offset + 10 minutes) for March-September
            if (month >= 3 && month <= 9)
            {
                int fajrOffsetSeconds = ishaOffsetSeconds + (DIYANET_FAJR_EXTRA_MINUTES * 60);
                times.fajr = sunrise - fajrOffsetSeconds;
            }
            // Outside March-September, keep astronomical Fajr (already set by library)
        }
    }

    static double clampLatitudeForDiyanet(double latitude)
    {
        // Rule d: For latitudes >= 62°, use 62° for calculation
        if (latitude >= DIYANET_MAX_LAT_CLAMP)
        {
            return DIYANET_MAX_LAT_CLAMP;
        }
        if (latitude <= -DIYANET_MAX_LAT_CLAMP)
        {
            return -DIYANET_MAX_LAT_CLAMP;
        }
        return latitude;
    }
}

const char *PrayerCalculator::getMethodName(int method)
{
    // Delegate to SettingsManager (single source of truth)
    return SettingsManager::getMethodName(method);
}

bool PrayerCalculator::calculateTimes(DailyPrayers &prayers, int method, double latitude, double longitude, int day, bool verbose)
{
    if (verbose)
    {
        Serial.printf("[Calc] Calculating prayer times (Method: %s)\n", getMethodName(method));
    }

    // Get current date + offset
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // `getLocalTime()` can fail transiently even when system time is set.
        // Fall back to `time()` + `localtime_r()`.
        time_t now = time(nullptr);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year + 1900 < 2020)
        {
            if (verbose)
            {
                Serial.println("[Calc] Failed to get local time");
            }
            return false;
        }
    }

    // Apply day offset (0 = today, 1 = tomorrow, etc.)
    if (day != 0)
    {
        timeinfo.tm_mday += day;
        mktime(&timeinfo); // Normalize
    }

    // Compute effective UTC offset for the TARGET date (handles DST correctly for future months)
    long offset_seconds = 0;
    {
        struct tm local_copy = timeinfo;
        time_t local_epoch = mktime(&local_copy); // interprets tm as local time for that date

        struct tm utc_tm;
        gmtime_r(&local_epoch, &utc_tm);    // convert to UTC representation
        time_t utc_epoch = mktime(&utc_tm); // interprets as local, so difference yields offset

        offset_seconds = (long)(local_epoch - utc_epoch);
    }

    // Setup Adhan calculation parameters
    coordinates_t coordinates;
    coordinates.latitude = latitude;
    coordinates.longitude = longitude;

    // Check if using Diyanet method - apply latitude clamping for 62°+ rule
    const bool isDiyanetMethod = (method == PRAYER_METHOD_DIYANET);
    if (isDiyanetMethod)
    {
        coordinates.latitude = clampLatitudeForDiyanet(latitude);
    }

    date_components_t date;
    date.year = timeinfo.tm_year + 1900;
    date.month = timeinfo.tm_mon + 1;
    date.day = timeinfo.tm_mday;

    const auto *spec = findMethodSpec(method);
    if (!spec)
    {
        if (verbose)
        {
            Serial.printf("[Calc] Unknown method %d, defaulting to MWL\n", method);
        }
        spec = &defaultMethodSpec();
    }

    auto params = buildParameters(*spec);
    applyDartNetAdjustments(params, *spec);
    if (spec->warnTehranMaghribAngle)
    {
        if (verbose)
        {
            Serial.println("[Calc] Tehran: this Adhan C version doesn't support maghribAngle=4.5°, using standard Maghrib (sunset)");
        }
    }

    const time_t dateEpoch = resolve_time(&date);

    // Adhan C library provides a TZ-hours API. Use it when offset is whole hours.
    // For half/quarter-hour timezones, fall back to applying the exact seconds offset.
    prayer_times_t times;
    if ((offset_seconds % 3600) == 0)
    {
        const int tzHours = (int)(offset_seconds / 3600);
        times = new_prayer_times_with_tz(&coordinates, dateEpoch, &params, tzHours);
    }
    else
    {
        times = new_prayer_times2(&coordinates, dateEpoch, &params);
        times.fajr += offset_seconds;
        times.sunrise += offset_seconds;
        times.dhuhr += offset_seconds;
        times.asr += offset_seconds;
        times.maghrib += offset_seconds;
        times.isha += offset_seconds;
    }

    // Apply Diyanet high-latitude rules (for method 13 at latitudes >= 45°)
    if (isDiyanetMethod)
    {
        applyDiyanetHighLatitudeRules(times, latitude, date.month);
    }

    // Format using local time (this is what you want to display on the device).
    auto formatTime = [](time_t timestamp) -> std::array<char, 6>
    {
        struct tm t_local;
        localtime_r(&timestamp, &t_local);
        std::array<char, 6> result;
        snprintf(result.data(), result.size(), "%02d:%02d", t_local.tm_hour, t_local.tm_min);
        result[5] = '\0';
        return result;
    };

    prayers[PrayerType::Fajr].value = formatTime(times.fajr);
    prayers[PrayerType::Sunrise].value = formatTime(times.sunrise);
    prayers[PrayerType::Dhuhr].value = formatTime(times.dhuhr);
    prayers[PrayerType::Asr].value = formatTime(times.asr);
    prayers[PrayerType::Maghrib].value = formatTime(times.maghrib);
    prayers[PrayerType::Isha].value = formatTime(times.isha);

    if (verbose)
    {
        Serial.println("\n=== PRAYER TIMES CALCULATED (Adhan Library) ===");
        Serial.printf("Date    : %04d-%02d-%02d\n", date.year, date.month, date.day);
        Serial.printf("Method  : %s\n", getMethodName(method));
        for (uint8_t i = 0; i < 6; ++i)
        {
            const auto type = PrayerType(i);
            Serial.printf("%-8s: %s\n", getPrayerName(type).data(),
                          prayers[type].value.data());
        }
        Serial.println("============================\n");
    }

    return true;
}
