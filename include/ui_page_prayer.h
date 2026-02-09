/**
 * @file ui_page_prayer.h
 * @brief Prayer Times Page - 6 Prayer Times List
 */

#ifndef UI_PAGE_PRAYER_H
#define UI_PAGE_PRAYER_H

#include <lvgl.h>

namespace UiPagePrayer
{
    struct PrayerTimesData
    {
        const char *fajr;
        const char *sunrise;
        const char *dhuhr;
        const char *asr;
        const char *maghrib;
        const char *isha;
        int nextPrayerIndex; // 0-5, -1 if none
    };

    // Create/recreate prayer times screen
    lv_obj_t *create();

    // Get the screen object
    lv_obj_t *getScreen();

    // Set prayer times data (caches for next create)
    void setPrayerTimes(const PrayerTimesData &data);

} // namespace UiPagePrayer

#endif // UI_PAGE_PRAYER_H
