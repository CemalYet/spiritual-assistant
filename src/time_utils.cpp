#include "time_utils.h"
#include <Arduino.h>
#include <sys/time.h>
#include <cmath>
#include <etl/string.h>
#include <etl/format_spec.h>
#include <etl/to_string.h>

namespace TimeUtils
{
    static constexpr int SECONDS_PER_HOUR = 3600;
    static constexpr int MIN_YEAR = 2024;
    static constexpr int MAX_YEAR = 2050;

    // Static buffer for formatted time
    static etl::string<20> formattedTimeBuffer{"Not set"};

    bool TimeRequest::isValid() const
    {
        return day >= 1 && day <= 31 &&
               month >= 1 && month <= 12 &&
               year >= MIN_YEAR && year <= MAX_YEAR &&
               hour >= 0 && hour <= 23 &&
               minute >= 0 && minute <= 59 &&
               second >= 0 && second <= 59;
    }

    TimeRequest createRequest(int day, int month, int year, int hour, int minute, int second, float timezone)
    {
        return {day, month, year, hour, minute, second, timezone};
    }

    TimeRequest createFromJson(const JsonDocument &doc)
    {
        return createRequest(
            doc["day"] | 0,
            doc["month"] | 0,
            doc["year"] | 0,
            doc["hour"] | 0,
            doc["minute"] | 0,
            doc["second"] | 0,
            doc["timezone"] | 0.0f);
    }

    // Helper to append zero-padded number to string
    static void appendPadded(etl::istring &buf, int value)
    {
        if (value < 10)
            buf += "0";
        etl::to_string(value, buf, true);
    }

    static void applyTimezone(float offset)
    {
        int tzHours = static_cast<int>(offset);
        int tzMins = static_cast<int>(std::abs((offset - tzHours) * 60));

        // POSIX timezone: opposite sign (UTC+3 = GMT-3)
        etl::string<16> tzStr{"UTC"};
        etl::to_string(-tzHours, tzStr, true); // append with sign
        tzStr += ":";
        if (tzMins < 10)
            tzStr += "0";
        etl::to_string(tzMins, tzStr, true); // append minutes

        setenv("TZ", tzStr.c_str(), 1);
        tzset();
    }

    bool applySystemTime(const TimeRequest &req)
    {
        if (!req.isValid())
            return false;

        struct tm timeinfo = {};
        timeinfo.tm_year = req.year - 1900;
        timeinfo.tm_mon = req.month - 1;
        timeinfo.tm_mday = req.day;
        timeinfo.tm_hour = req.hour;
        timeinfo.tm_min = req.minute;
        timeinfo.tm_sec = req.second;
        timeinfo.tm_isdst = 0;

        time_t localTime = mktime(&timeinfo);
        long tzOffset = static_cast<long>(req.timezoneOffset * SECONDS_PER_HOUR);

        struct timeval tv = {localTime - tzOffset, 0};
        settimeofday(&tv, nullptr);

        applyTimezone(req.timezoneOffset);

        Serial.printf("[Time] Set: %04d-%02d-%02d %02d:%02d:%02d (UTC%+.1f)\n",
                      req.year, req.month, req.day, req.hour, req.minute, req.second, req.timezoneOffset);

        return true;
    }

    const char *getFormattedTime()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo, 100))
            return "Not set";

        formattedTimeBuffer.clear();
        etl::to_string(timeinfo.tm_year + 1900, formattedTimeBuffer, true);
        formattedTimeBuffer += "-";
        appendPadded(formattedTimeBuffer, timeinfo.tm_mon + 1);
        formattedTimeBuffer += "-";
        appendPadded(formattedTimeBuffer, timeinfo.tm_mday);
        formattedTimeBuffer += " ";
        appendPadded(formattedTimeBuffer, timeinfo.tm_hour);
        formattedTimeBuffer += ":";
        appendPadded(formattedTimeBuffer, timeinfo.tm_min);

        return formattedTimeBuffer.c_str();
    }

} // namespace TimeUtils
