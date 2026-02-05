/**
 * @file ui_page_prayer.cpp
 * @brief Prayer Times Page - 6 Prayer Times List
 */

#include "ui_page_prayer.h"
#include "ui_theme.h"
#include "ui_components.h"

namespace UiPagePrayer
{
    using namespace UiTheme;

    static lv_obj_t *scr = nullptr;
    static lv_obj_t *prayer_time_labels[6] = {nullptr};
    static const char *PRAYER_NAMES[6] = {"SABAH", "GUNES", "OGLE", "IKINDI", "AKSAM", "YATSI"};
    static PrayerTimesData cachedData = {"--:--", "--:--", "--:--", "--:--", "--:--", "--:--", -1};

    lv_obj_t *create()
    {
        UiTheme::initStyles();

        if (scr)
        {
            lv_obj_del(scr);
            scr = nullptr;
            for (int i = 0; i < 6; i++)
                prayer_time_labels[i] = nullptr;
        }

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_add_style(scr, getStyleScreen(), 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        // Title
        lv_obj_t *title = lv_label_create(scr);
        lv_label_set_text(title, "NAMAZ VAKITLERI");
        lv_obj_set_style_text_color(title, COLOR_ACCENT_BRIGHT, 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_letter_space(title, 2, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

        int16_t startY = 36;
        int16_t rowH = 36;
        int16_t padX = 16;

        const char *times[6] = {
            cachedData.fajr, cachedData.sunrise, cachedData.dhuhr,
            cachedData.asr, cachedData.maghrib, cachedData.isha};

        for (int i = 0; i < 6; i++)
        {
            int16_t y = startY + i * rowH;
            bool isNext = (i == cachedData.nextPrayerIndex);
            bool isPast = (cachedData.nextPrayerIndex >= 0 && i < cachedData.nextPrayerIndex);
            bool isFuture = (cachedData.nextPrayerIndex >= 0 && i > cachedData.nextPrayerIndex);

            // Determine row color
            lv_color_t textColor;
            if (isNext)
                textColor = COLOR_TEXT; // White - most important
            else if (isPast)
                textColor = COLOR_SUBTITLE; // Dark gray - already passed
            else if (isFuture)
                textColor = COLOR_DIM; // Light gray - upcoming
            else
                textColor = COLOR_DIM; // Fallback (no next prayer today)

            // Highlight row for next prayer
            if (isNext)
            {
                lv_obj_t *row_bg = lv_obj_create(scr);
                lv_obj_remove_style_all(row_bg);
                lv_obj_set_size(row_bg, 224, rowH - 4);
                lv_obj_set_pos(row_bg, 8, y);
                lv_obj_set_style_bg_color(row_bg, COLOR_ACCENT, 0);
                lv_obj_set_style_bg_opa(row_bg, LV_OPA_30, 0); // 30% opacity - visible!
                lv_obj_set_style_radius(row_bg, 8, 0);
                lv_obj_set_style_border_width(row_bg, 2, 0);            // Thicker border
                lv_obj_set_style_border_color(row_bg, COLOR_ACCENT, 0); // Bright cyan border
                lv_obj_clear_flag(row_bg, LV_OBJ_FLAG_CLICKABLE);
            }

            // Prayer name (left)
            lv_obj_t *name_lbl = lv_label_create(scr);
            lv_label_set_text(name_lbl, PRAYER_NAMES[i]);
            lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(name_lbl, textColor, 0);
            if (isPast)
                lv_obj_set_style_text_opa(name_lbl, LV_OPA_70, 0);
            lv_obj_set_pos(name_lbl, padX, y + 8);

            // Prayer time (right)
            lv_obj_t *time_lbl = lv_label_create(scr);
            lv_label_set_text(time_lbl, times[i]);
            lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(time_lbl, textColor, 0);
            if (isPast)
                lv_obj_set_style_text_opa(time_lbl, LV_OPA_70, 0);
            lv_obj_align(time_lbl, LV_ALIGN_TOP_RIGHT, -padX, y + 6);
            prayer_time_labels[i] = time_lbl;
        }

        UiComponents::createNavBar(scr, 1);

        return scr;
    }

    lv_obj_t *getScreen() { return scr; }

    void setPrayerTimes(const PrayerTimesData &data)
    {
        // If next prayer changed, recreate the page to update all styling
        if (cachedData.nextPrayerIndex != data.nextPrayerIndex)
        {
            cachedData = data;
            if (scr)
            {
                lv_obj_del(scr);
                scr = nullptr;
                for (int i = 0; i < 6; i++)
                    prayer_time_labels[i] = nullptr;
                create();
            }
            return;
        }

        cachedData = data;

        // Update labels if page exists (just times, no styling change needed)
        if (!scr)
            return;

        const char *times[6] = {data.fajr, data.sunrise, data.dhuhr, data.asr, data.maghrib, data.isha};
        for (int i = 0; i < 6; i++)
        {
            if (prayer_time_labels[i])
            {
                lv_label_set_text(prayer_time_labels[i], times[i]);
            }
        }
    }

} // namespace UiPagePrayer
