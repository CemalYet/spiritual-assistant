/**
 * test_prayer_calc.cpp — Native prayer time calculation test harness
 *
 * Compiles Adhan C library + prayer_calculator.cpp logic on PC (no Arduino).
 * Outputs prayer times for all 15 methods × 6 cities × 5 dates to cpp_output.csv.
 *
 * Self-contained: includes Adhan C sources directly, copies calculation logic
 * from prayer_calculator.cpp. Does NOT depend on Arduino headers.
 *
 * TIMEZONE APPROACH: We set TZ=UTC so that mktime()/localtime() inside the
 * Adhan C library produce correct UTC epochs. Then new_prayer_times_with_tz()
 * shifts results by the target city's UTC offset, and gmtime_r() formats them
 * to show the correct local time.
 */

#include <unity.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <array>
#include <iterator>

// ============================================================================
// Platform compatibility
// ============================================================================

// Provide localtime_r/gmtime_r on Windows (MinGW doesn't have them).
// Single-threaded test — using non-reentrant wrappers is safe.
#ifdef _WIN32
static struct tm *portable_localtime_r(const time_t *timep, struct tm *result)
{
    struct tm *p = localtime(timep);
    if (p)
    {
        *result = *p;
        return result;
    }
    return nullptr;
}
static struct tm *portable_gmtime_r(const time_t *timep, struct tm *result)
{
    struct tm *p = gmtime(timep);
    if (p)
    {
        *result = *p;
        return result;
    }
    return nullptr;
}
#define localtime_r portable_localtime_r
#define gmtime_r portable_gmtime_r
#endif

// Force UTC timezone — critical for deterministic Adhan C calculations.
// Without this, mktime()/localtime() inside the library use the Windows
// system TZ, causing the exact 1-2 hour shift bug seen on ESP32.
static void force_utc_timezone()
{
#ifdef _WIN32
    _putenv("TZ=UTC");
#else
    setenv("TZ", "UTC", 1);
#endif
    tzset();
}

// ============================================================================
// Include Adhan C library sources directly (compiled as C++ with C linkage)
// Order matters: dependencies first
// ============================================================================

extern "C"
{
#include "astronomical.c"
#include "calendrical_helper.c"
#include "solar_coordinates.c"
#include "data_components.c"
#include "solar_time.c"
#include "calculation_parameters.c"
#include "prayer_times.c"
}

// ============================================================================
// Prayer calculation logic — copied from prayer_calculator.cpp
// This is an exact copy so we test the SAME code that runs on the ESP32.
// ============================================================================

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

    // *** Exact copy of kMethodSpecs from prayer_calculator.cpp ***
    static constexpr MethodSpec kMethodSpecs[] = {
        {1, "Karachi", KARACHI, false, 0, 0, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {2, "ISNA", NORTH_AMERICA, false, 0, 0, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {3, "MWL", MUSLIM_WORLD_LEAGUE, false, 0, 0, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {4, "Umm al-Qura", UMM_AL_QURA, false, 0, 0, 0, false, false, NetAdjustments{}},
        {5, "Egyptian", EGYPTIAN, true, 19.5, 17.5, 0, false, false, NetAdjustments{0, 0, 1, 0, 0, 0}},
        {6, "Gulf", GULF, false, 0, 0, 0, false, false, NetAdjustments{}},
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

    static constexpr int NUM_METHODS = sizeof(kMethodSpecs) / sizeof(kMethodSpecs[0]);

    static const MethodSpec *findMethodSpec(int method)
    {
        auto it = std::find_if(std::begin(kMethodSpecs), std::end(kMethodSpecs),
                               [method](const MethodSpec &spec)
                               { return spec.id == method; });
        return (it != std::end(kMethodSpecs)) ? &(*it) : nullptr;
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
            params.madhab = (madhab_t)HANAFI;
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
        params.adjustments.dhuhr = net.dhuhr - internalDhuhrOffsetMinutes(spec.calcMethod);
        params.adjustments.maghrib = net.maghrib - internalMaghribOffsetMinutes(spec.calcMethod);
    }

    // Diyanet high-latitude rules — KARAR 2006 + 2009 revision
    // Mirrors prayer_calculator.cpp exactly
    static constexpr double DIYANET_HIGH_LAT_THRESHOLD = 45.0;
    static constexpr double DIYANET_MAX_LAT_CLAMP = 62.0;
    static constexpr int DIYANET_ISHA_CAP_MINUTES = 80;
    static constexpr int DIYANET_FAJR_EXTRA_MINUTES = 10;

    static void applyDiyanetHighLatitudeRules(prayer_times_t &times, double latitude, int month)
    {
        if (latitude < DIYANET_HIGH_LAT_THRESHOLD)
            return;

        const time_t maghrib = times.maghrib;
        const time_t sunrise = times.sunrise;
        const time_t fajr_astro = times.fajr;
        const time_t isha_astro = times.isha;

        // Şer'î gece = Maghrib → Fajr (astronomical)
        int seriGeceSeconds = (int)difftime(fajr_astro, maghrib);
        if (seriGeceSeconds <= 0)
            seriGeceSeconds += 24 * 3600;

        // KARAR: 1/3 of şer'î gece (uncapped, for comparison)
        int nightThirdSeconds = seriGeceSeconds / 3;

        // Astronomical Isha offset from Maghrib
        int astroIshaOffset = (int)difftime(isha_astro, maghrib);
        if (astroIshaOffset <= 0)
            astroIshaOffset += 24 * 3600;

        // Compare with UNCAPPED 1/3: winter astro wins, summer KARAR wins
        if (astroIshaOffset <= nightThirdSeconds && astroIshaOffset > 0 && astroIshaOffset < 12 * 3600)
        {
            // Astronomical is earlier (winter) — keep
        }
        else
        {
            // KARAR applies — now cap at 80 dk
            int ishaOffsetSeconds = nightThirdSeconds;
            const int maxIshaOffset = DIYANET_ISHA_CAP_MINUTES * 60;
            if (ishaOffsetSeconds > maxIshaOffset)
            {
                ishaOffsetSeconds = maxIshaOffset;
            }

            times.isha = maghrib + ishaOffsetSeconds;

            if (month >= 3 && month <= 9)
            {
                int fajrOffsetSeconds = ishaOffsetSeconds + (DIYANET_FAJR_EXTRA_MINUTES * 60);
                times.fajr = sunrise - fajrOffsetSeconds;
            }
        }
    }

    static double clampLatitudeForDiyanet(double latitude)
    {
        if (latitude >= DIYANET_MAX_LAT_CLAMP)
            return DIYANET_MAX_LAT_CLAMP;
        if (latitude <= -DIYANET_MAX_LAT_CLAMP)
            return -DIYANET_MAX_LAT_CLAMP;
        return latitude;
    }

} // anonymous namespace

// ============================================================================
// Test configuration
// ============================================================================

struct TestCity
{
    const char *name;
    double lat;
    double lon;
    int tz_winter; // UTC offset hours (Nov-Mar for EU)
    int tz_summer; // UTC offset hours (Apr-Oct for EU)
    bool has_dst;
};

static const TestCity TEST_CITIES[] = {
    // Low latitude (control)
    {"Makkah", 21.4225, 39.8262, 3, 3, false},
    {"Cairo", 30.0444, 31.2357, 2, 2, false}, // EET, no DST since 2014
    // Below 45° threshold
    {"Istanbul", 41.0082, 28.9784, 3, 3, false}, // TRT, no DST since 2016
    // 45-52° band (Diyanet high-lat active, no cap)
    {"Paris", 48.8566, 2.3522, 1, 2, true},    // CET/CEST — largest Muslim pop in W. Europe
    {"Vienna", 48.2082, 16.3738, 1, 2, true},  // CET/CEST
    {"Brussels", 50.8798, 4.7005, 1, 2, true}, // CET/CEST — primary test location
    {"London", 51.5074, -0.1278, 0, 1, true},  // GMT/BST
    {"Cologne", 50.9375, 6.9603, 1, 2, true},  // CET/CEST — large Turkish community
    // At/above 52° cap threshold
    {"Berlin", 52.5200, 13.4050, 1, 2, true},     // CET/CEST
    {"Amsterdam", 52.3676, 4.9041, 1, 2, true},   // CET/CEST
    {"Birmingham", 52.4862, -1.8904, 0, 1, true}, // GMT/BST — largest UK Muslim city
    {"Copenhagen", 55.6761, 12.5683, 1, 2, true}, // CET/CEST
    {"Moscow", 55.7558, 37.6173, 3, 3, false},    // MSK, no DST
    // Near 62° clamp
    {"Oslo", 59.9139, 10.7522, 1, 2, true},      // CET/CEST
    {"Stockholm", 59.3293, 18.0686, 1, 2, true}, // CET/CEST
    {"Helsinki", 60.1699, 24.9384, 2, 3, true},  // EET/EEST
    // Above 62° clamp (clamped for Diyanet)
    {"Tromso", 69.6492, 18.9553, 1, 2, true}, // CET/CEST
};
static constexpr int NUM_CITIES = sizeof(TEST_CITIES) / sizeof(TEST_CITIES[0]);

struct TestDate
{
    int year, month, day;
    bool is_summer; // For EU DST determination
    const char *label;
};

static const TestDate TEST_DATES[] = {
    {2026, 1, 15, false, "2026-01-15"},  // Deep winter
    {2026, 3, 15, false, "2026-03-15"},  // Late winter (DST starts Mar 29)
    {2026, 6, 21, true, "2026-06-21"},   // Summer solstice
    {2026, 9, 22, true, "2026-09-22"},   // Autumn equinox (DST ends Oct 25)
    {2026, 12, 21, false, "2026-12-21"}, // Winter solstice
};
static constexpr int NUM_DATES = sizeof(TEST_DATES) / sizeof(TEST_DATES[0]);

constexpr int PRAYER_METHOD_DIYANET = 13;

// ============================================================================
// Helper: format time_t (after TZ shift) as HH:MM using gmtime
// ============================================================================

static std::array<char, 6> formatTime(time_t t)
{
    struct tm tm;
    gmtime_r(&t, &tm);
    std::array<char, 6> buf;
    snprintf(buf.data(), buf.size(), "%02d:%02d", tm.tm_hour, tm.tm_min);
    buf[5] = '\0';
    return buf;
}

static int timeToMinutes(time_t t)
{
    struct tm tm;
    gmtime_r(&t, &tm);
    return tm.tm_hour * 60 + tm.tm_min;
}

// ============================================================================
// Core: calculate prayer times for one (method, city, date) combination
// Returns prayer_times_t with TZ-shifted epochs (use gmtime to format)
// ============================================================================

static prayer_times_t calculateForTest(const MethodSpec &spec,
                                       double lat, double lon,
                                       int year, int month, int day,
                                       int tzHours)
{
    coordinates_t coords;
    coords.latitude = lat;
    coords.longitude = lon;

    const bool isDiyanet = (spec.id == PRAYER_METHOD_DIYANET);
    double originalLat = lat;

    if (isDiyanet)
    {
        coords.latitude = clampLatitudeForDiyanet(lat);
    }

    date_components_t date = {day, month, year};
    time_t dateEpoch = resolve_time(&date);

    auto params = buildParameters(spec);
    applyDartNetAdjustments(params, spec);

    prayer_times_t times = new_prayer_times_with_tz(&coords, dateEpoch, &params, tzHours);

    // Tehran method: compute Maghrib as sun at -4.5° below horizon
    // The Adhan C library has no maghribAngle field, so we manually compute
    // using hourAngle() from the solar_time module.
    if (spec.warnTehranMaghribAngle)
    {
        solar_time_t solarTime = new_solar_time(dateEpoch, &coords);
        time_components_t tc = from_double(hourAngle(&solarTime, -4.5, true));
        if (is_valid_time(tc))
        {
            time_t tehranMaghrib = get_date_components(dateEpoch, &tc);
            tehranMaghrib += tzHours * 3600;            // Apply timezone
            tehranMaghrib += spec.dartNet.maghrib * 60; // Net adjustment (0 for Tehran)
            times.maghrib = tehranMaghrib;
        }
    }

    // Diyanet high-latitude KARAR rules (must run AFTER adjustments)
    if (isDiyanet)
    {
        applyDiyanetHighLatitudeRules(times, originalLat, month);
    }

    return times;
}

// ============================================================================
// Tests
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

// --- Sanity: verify TZ=UTC makes resolve_time produce correct epoch ---
void test_epoch_sanity()
{
    date_components_t date = {15, 1, 2026};
    time_t epoch = resolve_time(&date);

    struct tm tm;
    gmtime_r(&epoch, &tm);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2026, tm.tm_year + 1900, "Year should be 2026");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, tm.tm_mon, "Month should be January (0)");
    TEST_ASSERT_EQUAL_INT_MESSAGE(15, tm.tm_mday, "Day should be 15");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, tm.tm_hour, "Hour should be 0 (midnight UTC)");
}

// --- Sanity: MWL Brussels Jan 15 against prayer-time-api known values ---
// Reference from prayer-time-api: Fajr 06:40, Sunrise 08:39, Dhuhr 12:52,
//   Asr 14:45, Maghrib 17:06, Isha 18:58
void test_brussels_mwl_jan15()
{
    const auto *spec = findMethodSpec(3); // MWL
    TEST_ASSERT_NOT_NULL(spec);

    prayer_times_t times = calculateForTest(*spec,
                                            50.8798, 4.7005, // Brussels
                                            2026, 1, 15,     // Jan 15
                                            1);              // CET = UTC+1

    int fajr = timeToMinutes(times.fajr);
    int sunrise = timeToMinutes(times.sunrise);
    int dhuhr = timeToMinutes(times.dhuhr);
    int asr = timeToMinutes(times.asr);
    int maghrib = timeToMinutes(times.maghrib);
    int isha = timeToMinutes(times.isha);

    // Allow ±3 min tolerance (API rounding + slight computation differences)
    TEST_ASSERT_INT_WITHIN_MESSAGE(3, 400, fajr, "Fajr ~06:40");        // 6*60+40=400
    TEST_ASSERT_INT_WITHIN_MESSAGE(3, 519, sunrise, "Sunrise ~08:39");  // 8*60+39=519
    TEST_ASSERT_INT_WITHIN_MESSAGE(3, 772, dhuhr, "Dhuhr ~12:52");      // 12*60+52=772
    TEST_ASSERT_INT_WITHIN_MESSAGE(3, 885, asr, "Asr ~14:45");          // 14*60+45=885
    TEST_ASSERT_INT_WITHIN_MESSAGE(3, 1026, maghrib, "Maghrib ~17:06"); // 17*60+6=1026
    TEST_ASSERT_INT_WITHIN_MESSAGE(3, 1138, isha, "Isha ~18:58");       // 18*60+58=1138
}

// --- Sanity: Turkey method has the expected minute adjustments ---
// MWL and Turkey share the same angles. Differences should be:
// Sunrise: -7, Dhuhr: +5, Asr: +4, Maghrib: +7
void test_turkey_adjustments_vs_mwl()
{
    const auto *mwl = findMethodSpec(3);
    const auto *turkey = findMethodSpec(13);
    TEST_ASSERT_NOT_NULL(mwl);
    TEST_ASSERT_NOT_NULL(turkey);

    // Istanbul (below 45°, no high-lat rules) Jan 15 TRT=UTC+3
    auto mwl_times = calculateForTest(*mwl, 41.0082, 28.9784, 2026, 1, 15, 3);
    auto turkey_times = calculateForTest(*turkey, 41.0082, 28.9784, 2026, 1, 15, 3);

    int sr_diff = timeToMinutes(turkey_times.sunrise) - timeToMinutes(mwl_times.sunrise);
    int dh_diff = timeToMinutes(turkey_times.dhuhr) - timeToMinutes(mwl_times.dhuhr);
    int as_diff = timeToMinutes(turkey_times.asr) - timeToMinutes(mwl_times.asr);
    int mg_diff = timeToMinutes(turkey_times.maghrib) - timeToMinutes(mwl_times.maghrib);

    // Sunrise: Turkey -7 vs MWL 0 → diff = -7
    TEST_ASSERT_INT_WITHIN_MESSAGE(1, -7, sr_diff, "Sunrise diff should be ~-7 min");
    // Dhuhr: Turkey +5 vs MWL +1 → net +4 display
    TEST_ASSERT_INT_WITHIN_MESSAGE(1, 4, dh_diff, "Dhuhr diff should be ~+4 min");
    // Asr: Turkey +4 vs MWL 0 → diff = +4
    TEST_ASSERT_INT_WITHIN_MESSAGE(1, 4, as_diff, "Asr diff should be ~+4 min");
    // Maghrib: Turkey +7 vs MWL 0 → diff = +7
    TEST_ASSERT_INT_WITHIN_MESSAGE(1, 7, mg_diff, "Maghrib diff should be ~+7 min");
}

// --- Sanity: Hanafi Asr for Russia should be later than Shafi ---
void test_russia_hanafi_asr()
{
    const auto *russia = findMethodSpec(14);
    const auto *mwl = findMethodSpec(3);
    TEST_ASSERT_NOT_NULL(russia);
    TEST_ASSERT_NOT_NULL(mwl);

    auto russia_times = calculateForTest(*russia, 50.8798, 4.7005, 2026, 1, 15, 1);
    auto mwl_times = calculateForTest(*mwl, 50.8798, 4.7005, 2026, 1, 15, 1);

    int russia_asr = timeToMinutes(russia_times.asr);
    int mwl_asr = timeToMinutes(mwl_times.asr);

    // Hanafi Asr should be ~30-50 min later than Shafi in winter
    TEST_ASSERT_GREATER_THAN_MESSAGE(mwl_asr + 20, russia_asr, "Hanafi Asr should be >20 min later");
}

// --- Sanity: 90-min interval methods (Umm al-Qura, Gulf, Qatar) ---
void test_90min_interval_isha()
{
    for (int methodId : {4, 6, 10})
    {
        const auto *spec = findMethodSpec(methodId);
        TEST_ASSERT_NOT_NULL(spec);

        auto times = calculateForTest(*spec, 50.8798, 4.7005, 2026, 1, 15, 1);
        int maghrib = timeToMinutes(times.maghrib);
        int isha = timeToMinutes(times.isha);

        char msg[64];
        snprintf(msg, sizeof(msg), "Method %d: Isha should be Maghrib + 90 min", methodId);
        TEST_ASSERT_INT_WITHIN_MESSAGE(2, 90, isha - maghrib, msg);
    }
}

// --- Sanity: Diyanet latitude clamping at 62° ---
void test_diyanet_latitude_clamping()
{
    // Compare as integers (×100) since Unity double is disabled
    TEST_ASSERT_EQUAL_INT(6200, (int)(clampLatitudeForDiyanet(69.65) * 100));
    TEST_ASSERT_EQUAL_INT(6200, (int)(clampLatitudeForDiyanet(62.0) * 100));
    TEST_ASSERT_EQUAL_INT(5088, (int)(clampLatitudeForDiyanet(50.88) * 100));
    TEST_ASSERT_EQUAL_INT(-6200, (int)(clampLatitudeForDiyanet(-70.0) * 100));
}

// --- Main: generate the full CSV file ---
void test_generate_csv()
{
    FILE *fp = fopen("cpp_output.csv", "w");
    TEST_ASSERT_NOT_NULL_MESSAGE(fp, "Failed to create cpp_output.csv");

    fprintf(fp, "method_id,method_name,lat,lon,date,tz_hours,fajr,sunrise,dhuhr,asr,maghrib,isha\n");

    int rows = 0;

    for (int m = 0; m < NUM_METHODS; m++)
    {
        const auto &spec = kMethodSpecs[m];

        for (int c = 0; c < NUM_CITIES; c++)
        {
            const auto &city = TEST_CITIES[c];

            for (int d = 0; d < NUM_DATES; d++)
            {
                const auto &td = TEST_DATES[d];

                int tz = (city.has_dst && td.is_summer) ? city.tz_summer : city.tz_winter;

                prayer_times_t times = calculateForTest(spec,
                                                        city.lat, city.lon,
                                                        td.year, td.month, td.day,
                                                        tz);

                auto fajr = formatTime(times.fajr);
                auto sunrise = formatTime(times.sunrise);
                auto dhuhr = formatTime(times.dhuhr);
                auto asr = formatTime(times.asr);
                auto maghrib = formatTime(times.maghrib);
                auto isha = formatTime(times.isha);

                fprintf(fp, "%d,%s,%.4f,%.4f,%s,%d,%s,%s,%s,%s,%s,%s\n",
                        spec.id, spec.name, city.lat, city.lon,
                        td.label, tz,
                        fajr.data(), sunrise.data(), dhuhr.data(),
                        asr.data(), maghrib.data(), isha.data());

                rows++;
            }
        }
    }

    fclose(fp);
    printf("\n>>> Written %d rows to cpp_output.csv\n", rows);
    TEST_ASSERT_EQUAL_INT_MESSAGE(NUM_METHODS * NUM_CITIES * NUM_DATES, rows,
                                  "Expected rows = methods × cities × dates");
}

// ============================================================================
// Entry point
// ============================================================================

int main(int argc, char **argv)
{
    force_utc_timezone();

    UNITY_BEGIN();

    // Sanity checks first (fast, catch TZ issues early)
    RUN_TEST(test_epoch_sanity);
    RUN_TEST(test_brussels_mwl_jan15);
    RUN_TEST(test_turkey_adjustments_vs_mwl);
    RUN_TEST(test_russia_hanafi_asr);
    RUN_TEST(test_90min_interval_isha);
    RUN_TEST(test_diyanet_latitude_clamping);

    // Full matrix (writes cpp_output.csv)
    RUN_TEST(test_generate_csv);

    return UNITY_END();
}
