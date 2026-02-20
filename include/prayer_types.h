#pragma once
#include <array>
#include <string_view>
#include <cstdint>

enum class PrayerType : uint8_t
{
    Fajr = 0,
    Sunrise = 1,
    Dhuhr = 2,
    Asr = 3,
    Maghrib = 4,
    Isha = 5,
    COUNT = 6
};

// Helper to convert enum to index
constexpr uint8_t idx(PrayerType type)
{
    return static_cast<uint8_t>(type);
}

// Prayer name lookups
constexpr std::array<std::string_view, 6> PRAYER_NAMES_EN = {
    "Fajr", "Sunrise", "Dhuhr", "Asr", "Maghrib", "Isha"};

constexpr std::array<std::string_view, 6> PRAYER_NAMES_TR = {
    "Sabah", "Gunes", "Ogle", "Ikindi", "Aksam", "Yatsi"};

constexpr std::string_view getPrayerName(PrayerType type, bool turkish = true)
{
    return turkish ? PRAYER_NAMES_TR[idx(type)] : PRAYER_NAMES_EN[idx(type)];
}

constexpr std::string_view getJsonKey(PrayerType type)
{
    return PRAYER_NAMES_EN[idx(type)];
}

constexpr std::array<std::string_view, 6> ADHAN_FILES = {
    "/sabah.mp3",  // Fajr
    "",            // Sunrise (no adhan)
    "/ogle.mp3",   // Dhuhr
    "/ikindi.mp3", // Asr
    "/aksam.mp3",  // Maghrib
    "/yatsi.mp3"   // Isha
};

constexpr std::string_view getAdhanFile(PrayerType type)
{
    return ADHAN_FILES[idx(type)];
}

// Prayer calculation methods
constexpr int PRAYER_METHOD_DIYANET = 13;
