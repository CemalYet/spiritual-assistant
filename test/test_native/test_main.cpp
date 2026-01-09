/**
 * Unit Tests for SpiritualAssistant
 *
 * Tests pure logic modules that don't require hardware:
 * - PrayerTime: time parsing and conversion
 * - DailyPrayers: prayer lookup and countdown
 * - PrayerTypes: enum helpers and name lookups
 * - CalculationMethods: method ID and name lookups
 * - DiyanetParser: API response parsing utilities
 */

#include <unity.h>
#include <string>

// Include actual project headers (they're Arduino-independent!)
#include "prayer_types.h"
#include "prayer_time.h"
#include "daily_prayers.h"
#include "calculation_methods.h"
#include "diyanet_parser.h"

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// PrayerTime Tests
// ============================================================================

void test_PrayerTime_isEmpty_default(void)
{
    PrayerTime pt;
    TEST_ASSERT_TRUE(pt.isEmpty());
}

void test_PrayerTime_isEmpty_when_set(void)
{
    PrayerTime pt;
    pt.value = {'0', '5', ':', '3', '0', '\0'};
    TEST_ASSERT_FALSE(pt.isEmpty());
}

void test_PrayerTime_toMinutes_0530(void)
{
    PrayerTime pt;
    pt.value = {'0', '5', ':', '3', '0', '\0'};
    TEST_ASSERT_EQUAL_INT(330, pt.toMinutes()); // 5*60 + 30 = 330
}

void test_PrayerTime_toMinutes_1215(void)
{
    PrayerTime pt;
    pt.value = {'1', '2', ':', '1', '5', '\0'};
    TEST_ASSERT_EQUAL_INT(735, pt.toMinutes()); // 12*60 + 15 = 735
}

void test_PrayerTime_toMinutes_2359(void)
{
    PrayerTime pt;
    pt.value = {'2', '3', ':', '5', '9', '\0'};
    TEST_ASSERT_EQUAL_INT(1439, pt.toMinutes()); // 23*60 + 59 = 1439
}

void test_PrayerTime_toSeconds(void)
{
    PrayerTime pt;
    pt.value = {'0', '1', ':', '0', '0', '\0'};
    TEST_ASSERT_EQUAL_INT(3600, pt.toSeconds()); // 1 hour = 3600 seconds
}

void test_PrayerTime_equality(void)
{
    PrayerTime pt;
    pt.value = {'0', '5', ':', '3', '0', '\0'};
    TEST_ASSERT_TRUE(pt == "05:30");
    TEST_ASSERT_FALSE(pt == "05:31");
}

// ============================================================================
// DailyPrayers Tests
// ============================================================================

DailyPrayers createTestPrayers()
{
    DailyPrayers dp;
    dp[PrayerType::Fajr].value = {'0', '5', ':', '3', '0', '\0'};    // 05:30 = 330
    dp[PrayerType::Sunrise].value = {'0', '7', ':', '0', '0', '\0'}; // 07:00 = 420
    dp[PrayerType::Dhuhr].value = {'1', '2', ':', '1', '5', '\0'};   // 12:15 = 735
    dp[PrayerType::Asr].value = {'1', '5', ':', '3', '0', '\0'};     // 15:30 = 930
    dp[PrayerType::Maghrib].value = {'1', '8', ':', '0', '0', '\0'}; // 18:00 = 1080
    dp[PrayerType::Isha].value = {'1', '9', ':', '3', '0', '\0'};    // 19:30 = 1170
    return dp;
}

void test_DailyPrayers_findNext_before_fajr(void)
{
    DailyPrayers dp = createTestPrayers();
    auto next = dp.findNext(300); // 05:00
    TEST_ASSERT_TRUE(next.has_value());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PrayerType::Fajr), static_cast<int>(*next));
}

void test_DailyPrayers_findNext_after_fajr(void)
{
    DailyPrayers dp = createTestPrayers();
    auto next = dp.findNext(400); // 06:40 (after Fajr, before Sunrise)
    TEST_ASSERT_TRUE(next.has_value());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PrayerType::Sunrise), static_cast<int>(*next));
}

void test_DailyPrayers_findNext_midday(void)
{
    DailyPrayers dp = createTestPrayers();
    auto next = dp.findNext(800); // 13:20 (after Dhuhr)
    TEST_ASSERT_TRUE(next.has_value());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(PrayerType::Asr), static_cast<int>(*next));
}

void test_DailyPrayers_findNext_after_isha(void)
{
    DailyPrayers dp = createTestPrayers();
    auto next = dp.findNext(1200); // 20:00 (after Isha)
    TEST_ASSERT_FALSE(next.has_value());
}

void test_DailyPrayers_minutesUntilNext(void)
{
    DailyPrayers dp = createTestPrayers();
    int mins = dp.minutesUntilNext(700); // 11:40, next is Dhuhr at 12:15
    TEST_ASSERT_EQUAL_INT(35, mins);     // 735 - 700 = 35
}

void test_DailyPrayers_minutesUntilNext_none(void)
{
    DailyPrayers dp = createTestPrayers();
    int mins = dp.minutesUntilNext(1200); // After all prayers
    TEST_ASSERT_EQUAL_INT(-1, mins);
}

// ============================================================================
// PrayerType Tests
// ============================================================================

void test_idx_conversion(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, idx(PrayerType::Fajr));
    TEST_ASSERT_EQUAL_UINT8(1, idx(PrayerType::Sunrise));
    TEST_ASSERT_EQUAL_UINT8(5, idx(PrayerType::Isha));
}

void test_getPrayerName_english(void)
{
    TEST_ASSERT_EQUAL_STRING("Fajr", std::string(getPrayerName(PrayerType::Fajr, false)).c_str());
    TEST_ASSERT_EQUAL_STRING("Maghrib", std::string(getPrayerName(PrayerType::Maghrib, false)).c_str());
}

void test_getPrayerName_turkish(void)
{
    TEST_ASSERT_EQUAL_STRING("Sabah", std::string(getPrayerName(PrayerType::Fajr, true)).c_str());
    TEST_ASSERT_EQUAL_STRING("Aksam", std::string(getPrayerName(PrayerType::Maghrib, true)).c_str());
}

void test_getJsonKey(void)
{
    TEST_ASSERT_EQUAL_STRING("Fajr", std::string(getJsonKey(PrayerType::Fajr)).c_str());
    TEST_ASSERT_EQUAL_STRING("Isha", std::string(getJsonKey(PrayerType::Isha)).c_str());
}

// ============================================================================
// CalculationMethods Tests
// ============================================================================

void test_findCalculationMethod_valid(void)
{
    const CalculationMethodInfo *method = findCalculationMethod(1);
    TEST_ASSERT_NOT_NULL(method);
    TEST_ASSERT_EQUAL_STRING("Karachi", method->name);
}

void test_findCalculationMethod_turkey(void)
{
    const CalculationMethodInfo *method = findCalculationMethod(13);
    TEST_ASSERT_NOT_NULL(method);
    TEST_ASSERT_EQUAL_STRING("Turkey Diyanet", method->name);
}

void test_findCalculationMethod_invalid(void)
{
    const CalculationMethodInfo *method = findCalculationMethod(999);
    TEST_ASSERT_NULL(method);
}

void test_findCalculationMethod_zero(void)
{
    const CalculationMethodInfo *method = findCalculationMethod(0);
    TEST_ASSERT_NULL(method);
}

void test_getCalculationMethodName(void)
{
    TEST_ASSERT_EQUAL_STRING("MWL", getCalculationMethodName(3));
    TEST_ASSERT_NULL(getCalculationMethodName(100));
}

void test_calculationMethodCount(void)
{
    TEST_ASSERT_EQUAL_INT(15, CALCULATION_METHOD_COUNT);
}

// ============================================================================
// DiyanetParser Tests
// ============================================================================

void test_parseTime_valid(void)
{
    PrayerTime pt;
    TEST_ASSERT_TRUE(DiyanetParser::parseTime("05:30", pt));
    TEST_ASSERT_EQUAL_STRING("05:30", pt.value.data());
}

void test_parseTime_midnight(void)
{
    PrayerTime pt;
    TEST_ASSERT_TRUE(DiyanetParser::parseTime("00:00", pt));
    TEST_ASSERT_EQUAL_STRING("00:00", pt.value.data());
}

void test_parseTime_endOfDay(void)
{
    PrayerTime pt;
    TEST_ASSERT_TRUE(DiyanetParser::parseTime("23:59", pt));
    TEST_ASSERT_EQUAL_STRING("23:59", pt.value.data());
}

void test_parseTime_null(void)
{
    PrayerTime pt;
    TEST_ASSERT_FALSE(DiyanetParser::parseTime(nullptr, pt));
}

void test_parseTime_tooShort(void)
{
    PrayerTime pt;
    TEST_ASSERT_FALSE(DiyanetParser::parseTime("5:30", pt));
}

void test_parseTime_invalidHour(void)
{
    PrayerTime pt;
    TEST_ASSERT_FALSE(DiyanetParser::parseTime("35:00", pt));
}

void test_parseTime_invalidMinute(void)
{
    PrayerTime pt;
    TEST_ASSERT_FALSE(DiyanetParser::parseTime("12:60", pt));
}

void test_parseDate_valid(void)
{
    int d, m, y;
    TEST_ASSERT_TRUE(DiyanetParser::parseDate("31.12.2025", d, m, y));
    TEST_ASSERT_EQUAL_INT(31, d);
    TEST_ASSERT_EQUAL_INT(12, m);
    TEST_ASSERT_EQUAL_INT(2025, y);
}

void test_parseDate_singleDigits(void)
{
    int d, m, y;
    TEST_ASSERT_TRUE(DiyanetParser::parseDate("01.01.2026", d, m, y));
    TEST_ASSERT_EQUAL_INT(1, d);
    TEST_ASSERT_EQUAL_INT(1, m);
    TEST_ASSERT_EQUAL_INT(2026, y);
}

void test_parseDate_null(void)
{
    int d, m, y;
    TEST_ASSERT_FALSE(DiyanetParser::parseDate(nullptr, d, m, y));
}

void test_parseDate_invalidFormat(void)
{
    int d, m, y;
    TEST_ASSERT_FALSE(DiyanetParser::parseDate("2025-12-31", d, m, y));
}

void test_parseDate_invalidDay(void)
{
    int d, m, y;
    TEST_ASSERT_FALSE(DiyanetParser::parseDate("32.12.2025", d, m, y));
}

void test_calculateDayOffset(void)
{
    time_t start = 1000000;
    time_t target = start + (5 * 86400); // 5 days later
    TEST_ASSERT_EQUAL_INT(5, DiyanetParser::calculateDayOffset(start, target));
}

void test_calculateDayOffset_sameDay(void)
{
    time_t start = 1000000;
    TEST_ASSERT_EQUAL_INT(0, DiyanetParser::calculateDayOffset(start, start));
}

void test_calculateDayOffset_negative(void)
{
    time_t start = 1000000;
    time_t target = start - 86400; // 1 day before
    TEST_ASSERT_EQUAL_INT(-1, DiyanetParser::calculateDayOffset(start, target));
}

void test_isDayOffsetValid_valid(void)
{
    TEST_ASSERT_TRUE(DiyanetParser::isDayOffsetValid(0, 30));
    TEST_ASSERT_TRUE(DiyanetParser::isDayOffsetValid(15, 30));
    TEST_ASSERT_TRUE(DiyanetParser::isDayOffsetValid(29, 30));
}

void test_isDayOffsetValid_invalid(void)
{
    TEST_ASSERT_FALSE(DiyanetParser::isDayOffsetValid(-1, 30));
    TEST_ASSERT_FALSE(DiyanetParser::isDayOffsetValid(30, 30));
    TEST_ASSERT_FALSE(DiyanetParser::isDayOffsetValid(100, 30));
}

void test_isCacheExpired_fresh(void)
{
    time_t now = 1000000;
    time_t fetchedAt = now - (10 * 86400); // 10 days ago
    TEST_ASSERT_FALSE(DiyanetParser::isCacheExpired(fetchedAt, now, 25));
}

void test_isCacheExpired_expired(void)
{
    time_t now = 1000000;
    time_t fetchedAt = now - (30 * 86400); // 30 days ago
    TEST_ASSERT_TRUE(DiyanetParser::isCacheExpired(fetchedAt, now, 25));
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    // PrayerTime tests (7)
    RUN_TEST(test_PrayerTime_isEmpty_default);
    RUN_TEST(test_PrayerTime_isEmpty_when_set);
    RUN_TEST(test_PrayerTime_toMinutes_0530);
    RUN_TEST(test_PrayerTime_toMinutes_1215);
    RUN_TEST(test_PrayerTime_toMinutes_2359);
    RUN_TEST(test_PrayerTime_toSeconds);
    RUN_TEST(test_PrayerTime_equality);

    // DailyPrayers tests (6)
    RUN_TEST(test_DailyPrayers_findNext_before_fajr);
    RUN_TEST(test_DailyPrayers_findNext_after_fajr);
    RUN_TEST(test_DailyPrayers_findNext_midday);
    RUN_TEST(test_DailyPrayers_findNext_after_isha);
    RUN_TEST(test_DailyPrayers_minutesUntilNext);
    RUN_TEST(test_DailyPrayers_minutesUntilNext_none);

    // PrayerType tests (4)
    RUN_TEST(test_idx_conversion);
    RUN_TEST(test_getPrayerName_english);
    RUN_TEST(test_getPrayerName_turkish);
    RUN_TEST(test_getJsonKey);

    // CalculationMethods tests (6)
    RUN_TEST(test_findCalculationMethod_valid);
    RUN_TEST(test_findCalculationMethod_turkey);
    RUN_TEST(test_findCalculationMethod_invalid);
    RUN_TEST(test_findCalculationMethod_zero);
    RUN_TEST(test_getCalculationMethodName);
    RUN_TEST(test_calculationMethodCount);

    // DiyanetParser tests (18)
    RUN_TEST(test_parseTime_valid);
    RUN_TEST(test_parseTime_midnight);
    RUN_TEST(test_parseTime_endOfDay);
    RUN_TEST(test_parseTime_null);
    RUN_TEST(test_parseTime_tooShort);
    RUN_TEST(test_parseTime_invalidHour);
    RUN_TEST(test_parseTime_invalidMinute);
    RUN_TEST(test_parseDate_valid);
    RUN_TEST(test_parseDate_singleDigits);
    RUN_TEST(test_parseDate_null);
    RUN_TEST(test_parseDate_invalidFormat);
    RUN_TEST(test_parseDate_invalidDay);
    RUN_TEST(test_calculateDayOffset);
    RUN_TEST(test_calculateDayOffset_sameDay);
    RUN_TEST(test_calculateDayOffset_negative);
    RUN_TEST(test_isDayOffsetValid_valid);
    RUN_TEST(test_isDayOffsetValid_invalid);
    RUN_TEST(test_isCacheExpired_fresh);
    RUN_TEST(test_isCacheExpired_expired);

    return UNITY_END();
}
