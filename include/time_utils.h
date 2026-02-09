#pragma once

#include <cstdint>
#include <ArduinoJson.h>

namespace TimeUtils
{
    // Time request structure for setting system time
    struct TimeRequest
    {
        int day;
        int month;
        int year;
        int hour;
        int minute;
        int second;
        float timezoneOffset;

        bool isValid() const;
    };

    // Parse time request from values
    TimeRequest createRequest(int day, int month, int year, int hour, int minute, int second, float timezone);

    // Parse time request from JSON document (DRY helper)
    TimeRequest createFromJson(const JsonDocument &doc);

    // Apply system time and timezone from request
    // Returns true on success
    bool applySystemTime(const TimeRequest &req);

    // Get formatted device time string (YYYY-MM-DD HH:MM)
    // Returns "Not set" if time not available
    const char *getFormattedTime();

} // namespace TimeUtils
