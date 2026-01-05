#pragma once
#include <Arduino.h>
#include <array>
#include <string_view>
#include "time.h"

struct CurrentTime
{
    std::array<char, 6> _hhMM{"00:00"};
    int _minutes{0};
    int _seconds{0};

    static CurrentTime now()
    {
        CurrentTime ct;
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            const uint8_t h = (timeinfo.tm_hour < 24) ? timeinfo.tm_hour : 0;
            const uint8_t m = (timeinfo.tm_min < 60) ? timeinfo.tm_min : 0;
            const uint8_t s = (timeinfo.tm_sec < 60) ? timeinfo.tm_sec : 0;

            ct._hhMM[0] = '0' + (h / 10);
            ct._hhMM[1] = '0' + (h % 10);
            ct._hhMM[2] = ':';
            ct._hhMM[3] = '0' + (m / 10);
            ct._hhMM[4] = '0' + (m % 10);
            ct._hhMM[5] = '\0';

            ct._minutes = h * 60 + m;
            ct._seconds = h * 3600 + m * 60 + s;
        }
        return ct;
    }

    static std::array<char, 20> getTomorrowDate()
    {
        std::array<char, 20> buf{}; // Zero-initialize
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            timeinfo.tm_mday += 1;
            mktime(&timeinfo); // Normalize date
            strftime(buf.data(), buf.size(), "&date=%d-%m-%Y", &timeinfo);
        }
        return buf;
    }

    static std::array<char, 20> getTodayDate()
    {
        std::array<char, 20> buf{}; // Zero-initialize
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            strftime(buf.data(), buf.size(), "&date=%d-%m-%Y", &timeinfo);
        }
        return buf;
    }

    static std::array<char, 12> getCurrentDate()
    {
        std::array<char, 12> buf{};
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            strftime(buf.data(), buf.size(), "%d %b", &timeinfo);
        }
        return buf;
    }

    std::string_view view() const
    {
        return {_hhMM.data(), 5};
    }
};
