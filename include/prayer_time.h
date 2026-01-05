#pragma once
#include <array>
#include <string_view>

struct PrayerTime
{
    std::array<char, 6> value{"--:--"}; // Invalid marker for uninitialized

    constexpr bool isEmpty() const
    {
        return value[0] == '-';
    }

    int toMinutes() const
    {
        return (value[0] - '0') * 600 + (value[1] - '0') * 60 +
               (value[3] - '0') * 10 + (value[4] - '0');
    }

    int toSeconds() const
    {
        return toMinutes() * 60;
    }

    bool operator==(std::string_view other) const
    {
        return std::string_view(value.data(), 5) == other;
    }
};
