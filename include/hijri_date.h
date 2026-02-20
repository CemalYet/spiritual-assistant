#pragma once
#include <cstdint>

/**
 * @brief Hijri (Islamic) date from Gregorian using the Kuwaiti algorithm.
 *
 * Pure arithmetic — no heap, no tables, no network.
 * Accuracy: matches tabular Hijri calendar (30-year cycle).
 * Real-world ±1 day possible at month boundaries due to moon sighting.
 */
struct HijriDate
{
    int year;
    uint8_t month; // 1-12
    uint8_t day;   // 1-30
};

/**
 * @brief Convert Gregorian date to Hijri using Kuwaiti algorithm.
 *
 * @param gYear  Gregorian year (e.g. 2026)
 * @param gMonth Gregorian month (1-12)
 * @param gDay   Gregorian day (1-31)
 * @return HijriDate
 */
inline HijriDate gregorianToHijri(int gYear, int gMonth, int gDay)
{
    // Tabular Islamic calendar — 30-year arithmetic cycle.
    // JD formula handles Julian/Gregorian cutover (15 Oct 1582).
    // Reference: https://en.wikipedia.org/wiki/Tabular_Islamic_calendar

    int jd;
    if ((gYear > 1582) ||
        (gYear == 1582 && gMonth > 10) ||
        (gYear == 1582 && gMonth == 10 && gDay > 14))
    {
        // Gregorian calendar
        jd = (1461 * (gYear + 4800 + (gMonth - 14) / 12)) / 4 +
             (367 * (gMonth - 2 - 12 * ((gMonth - 14) / 12))) / 12 -
             (3 * ((gYear + 4900 + (gMonth - 14) / 12) / 100)) / 4 +
             gDay - 32075;
    }
    else
    {
        // Julian calendar
        jd = 367 * gYear -
             (7 * (gYear + 5001 + (gMonth - 9) / 7)) / 4 +
             (275 * gMonth) / 9 +
             gDay + 1729777;
    }

    // Convert Julian Day to Hijri (Kuwaiti / tabular method)
    int l = jd - 1948440 + 10632;
    int n = (l - 1) / 10631;
    l = l - 10631 * n + 354;

    int j = ((10985 - l) / 5316) * ((50 * l) / 17719) +
            (l / 5670) * ((43 * l) / 15238);
    l = l - ((30 - j) / 15) * ((17719 * j) / 50) -
        (j / 16) * ((15238 * j) / 43) + 29;

    int hMonth = (24 * l) / 709;
    int hDay = l - (709 * hMonth) / 24;
    int hYear = 30 * n + j - 30;

    return {hYear, static_cast<uint8_t>(hMonth), static_cast<uint8_t>(hDay)};
}

// Hijri month names in Turkish (abbreviated, 3 chars)
constexpr const char *HIJRI_MONTHS_TR_SHORT[] = {
    "Mhr", // 1  Muharrem
    "Sfr", // 2  Safer
    "REl", // 3  Rebiülevvel
    "RAh", // 4  Rebiülahir
    "CEl", // 5  Cemaziyelevvel
    "CAh", // 6  Cemaziyelahir
    "Rec", // 7  Recep
    "Sab", // 8  Şaban
    "Ram", // 9  Ramazan
    "Svl", // 10 Şevval
    "ZKa", // 11 Zilkade
    "ZHc"  // 12 Zilhicce
};

// Hijri month names in Turkish (display-friendly lengths)
// Long months use standard Turkish abbreviations: R.Evvel, R.Ahir, C.Evvel, C.Ahir
constexpr const char *HIJRI_MONTHS_TR[] = {
    "Muharrem", // 1
    "Safer",    // 2
    "R.Evvel",  // 3  Rebiulevvel
    "R.Ahir",   // 4  Rebiulahir
    "C.Evvel",  // 5  Cemaziyelevvel
    "C.Ahir",   // 6  Cemaziyelahir
    "Recep",    // 7
    "Saban",    // 8
    "Ramazan",  // 9
    "Sevval",   // 10
    "Zilkade",  // 11
    "Zilhicce"  // 12
};

/**
 * @brief Get Hijri month name (Turkish, display-friendly).
 * @param month 1-12
 */
inline const char *getHijriMonth(int month)
{
    if (month < 1 || month > 12)
        return "???";
    return HIJRI_MONTHS_TR[month - 1];
}
