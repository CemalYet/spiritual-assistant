/**
 * @file ui_page_prayer.h
 * @brief Screen 1 — Clock + Dates
 *
 * Displays:
 *   - Current time in Cinzel SemiBold 72 with blinking colon
 *   - Seconds in DM Mono 14
 *   - Decorative gold divider with ✦ ornament
 *   - Gregorian date (Cormorant italic)
 *   - Hijri date (Amiri 18)
 */

#ifndef UI_PAGE_PRAYER_H
#define UI_PAGE_PRAYER_H

#include <lvgl.h>

namespace UiPagePrayer
{
    lv_obj_t *create();
    lv_obj_t *getScreen();

    // Update clock display (called from dirty-flag handler every second)
    void setTime(int hour, int minute, int second);

    // Update date labels
    void setGregorianDate(const char *date); // e.g. "4 Haziran 2025"
    void setHijriDate(const char *hijri);    // e.g. "17 Ramazan 1447"

    // Status bar
    void setStatusBarCity(const char *city, const char *dateAbbrev);
    void setWifi(uint8_t bars);

} // namespace UiPagePrayer

#endif // UI_PAGE_PRAYER_H
