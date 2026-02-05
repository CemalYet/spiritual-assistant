/**
 * @file ui_page_home.cpp
 * @brief Home Page - Clock, Date, Prayer Card, Status Icons
 */

#include "ui_page_home.h"
#include "ui_theme.h"
#include "ui_icons.h"
#include "ui_components.h"
#include "app_state.h"
#include <etl/string.h>
#include <cctype>
#include <cstdio>

namespace UiPageHome
{
    using namespace UiTheme;

    // UI Elements
    static lv_obj_t *scr = nullptr;
    static lv_obj_t *lbl_clock = nullptr;
    static lv_obj_t *lbl_location = nullptr; // City name
    static lv_obj_t *lbl_date = nullptr;     // Date
    static lv_obj_t *prayer_card = nullptr;
    static lv_obj_t *lbl_prayer_name = nullptr;
    static lv_obj_t *lbl_prayer_time = nullptr;
    static lv_obj_t *icon_sync = nullptr;
    static lv_obj_t *btn_mute = nullptr;
    static lv_obj_t *icon_mute = nullptr;

    // State
    static bool muted = false;
    static bool synced = false;
    static bool adhanAvailable = false;

    // Cache for partial updates
    static int lastHour = -1;
    static int lastMinute = -1;
    static etl::string<32> lastPrayerName;
    static etl::string<16> lastPrayerTime;
    static etl::string<48> cachedLocation;
    static etl::string<64> cachedDate;

    static uint32_t lastMuteClick = 0;

    static void onMuteClick(lv_event_t *e)
    {
        uint32_t now = lv_tick_get();
        if (now - lastMuteClick < DEBOUNCE_MS)
            return;
        lastMuteClick = now;

        muted = !muted;
        AppStateHelper::setMuted(muted); // Update global state for adhan
        UiIcons::drawSpeakerIcon(icon_mute, muted, muted ? COLOR_DIM : COLOR_ACCENT);
    }

    static void createHeroClock()
    {
        lbl_clock = lv_label_create(scr);
        lv_obj_add_style(lbl_clock, getStyleTitle(), 0);
        lv_label_set_text(lbl_clock, "00:00");
        lv_obj_align(lbl_clock, LV_ALIGN_TOP_MID, 0, CLOCK_Y);
    }

    static void createHeaderLabels()
    {
        // Location label (top, smaller)
        lbl_location = lv_label_create(scr);
        lv_obj_set_style_text_font(lbl_location, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_location, COLOR_HEADER, 0);
        lv_label_set_text(lbl_location, "");
        lv_obj_set_width(lbl_location, 220);                     // Max width with margin
        lv_label_set_long_mode(lbl_location, LV_LABEL_LONG_DOT); // Truncate if too long
        lv_obj_set_style_text_align(lbl_location, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl_location, LV_ALIGN_TOP_MID, 0, HEADER_Y);

        // Date label (below location)
        lbl_date = lv_label_create(scr);
        lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_date, COLOR_HEADER, 0);
        lv_label_set_text(lbl_date, "");
        lv_obj_set_width(lbl_date, 220);
        lv_label_set_long_mode(lbl_date, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(lbl_date, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl_date, LV_ALIGN_TOP_MID, 0, HEADER_Y + 18);
    }

    static void createPrayerCard()
    {
        prayer_card = lv_obj_create(scr);
        lv_obj_remove_style_all(prayer_card);
        lv_obj_add_style(prayer_card, getStyleCard(), 0);
        lv_obj_set_size(prayer_card, CARD_W, CARD_H);
        lv_obj_align(prayer_card, LV_ALIGN_TOP_MID, 0, CARD_Y);
        lv_obj_clear_flag(prayer_card, LV_OBJ_FLAG_SCROLLABLE);

        lbl_prayer_name = lv_label_create(prayer_card);
        lv_obj_add_style(lbl_prayer_name, getStyleCardLabel(), 0);
        lv_label_set_text(lbl_prayer_name, "SABAH");
        lv_obj_align(lbl_prayer_name, LV_ALIGN_LEFT_MID, 0, 0);

        lbl_prayer_time = lv_label_create(prayer_card);
        lv_obj_add_style(lbl_prayer_time, getStyleCardTime(), 0);
        lv_label_set_text(lbl_prayer_time, "21:45");
        lv_obj_align(lbl_prayer_time, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    static void createStatusRow()
    {
        lv_obj_t *status_row = lv_obj_create(scr);
        lv_obj_remove_style_all(status_row);
        lv_obj_add_style(status_row, getStyleTransparent(), 0);
        lv_obj_set_size(status_row, 200, 48);
        lv_obj_align(status_row, LV_ALIGN_TOP_MID, 0, STATUS_Y);
        lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(status_row, SPACING_MD, 0);
        lv_obj_clear_flag(status_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        int16_t icon_size = 32;
        int16_t cont_size = 48;

        // Sync icon
        lv_obj_t *sync_outer = lv_obj_create(status_row);
        lv_obj_remove_style_all(sync_outer);
        lv_obj_add_style(sync_outer, getStyleTransparent(), 0);
        lv_obj_set_size(sync_outer, cont_size, cont_size);
        lv_obj_clear_flag(sync_outer, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *sync_icon = lv_obj_create(sync_outer);
        lv_obj_remove_style_all(sync_icon);
        lv_obj_add_style(sync_icon, getStyleTransparent(), 0);
        lv_obj_set_size(sync_icon, icon_size, icon_size);
        lv_obj_center(sync_icon);
        lv_obj_clear_flag(sync_icon, LV_OBJ_FLAG_CLICKABLE);
        icon_sync = sync_icon;
        UiIcons::drawSyncIcon(icon_sync, synced ? COLOR_ACCENT : COLOR_DIM, synced);

        // Mute button
        btn_mute = lv_btn_create(status_row);
        lv_obj_remove_style_all(btn_mute);
        lv_obj_add_style(btn_mute, getStyleIconBtn(), 0);
        lv_obj_set_size(btn_mute, cont_size, cont_size);
        lv_obj_set_ext_click_area(btn_mute, 8);
        lv_obj_add_flag(btn_mute, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_mute, onMuteClick, LV_EVENT_PRESSED, nullptr);

        lv_obj_t *mute_cont = lv_obj_create(btn_mute);
        lv_obj_remove_style_all(mute_cont);
        lv_obj_add_style(mute_cont, getStyleTransparent(), 0);
        lv_obj_set_size(mute_cont, icon_size, icon_size);
        lv_obj_center(mute_cont);
        lv_obj_clear_flag(mute_cont, LV_OBJ_FLAG_CLICKABLE);
        icon_mute = mute_cont;
        UiIcons::drawSpeakerIcon(icon_mute, muted, adhanAvailable ? COLOR_ACCENT : COLOR_DIM, adhanAvailable);
    }

    lv_obj_t *create()
    {
        UiTheme::initStyles();

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_add_style(scr, getStyleScreen(), 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        createHeroClock();
        createHeaderLabels();
        createPrayerCard();
        createStatusRow();
        UiComponents::createNavBar(scr, 0);

        return scr;
    }

    lv_obj_t *getScreen() { return scr; }

    void setTime(int hour, int minute)
    {
        if (hour == lastHour && minute == lastMinute)
            return;
        lastHour = hour;
        lastMinute = minute;
        if (!lbl_clock)
            return;

        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
        lv_label_set_text(lbl_clock, buf);
    }

    void setDate(const char *date)
    {
        if (!date || !lbl_date)
            return;
        if (cachedDate == date)
            return;
        cachedDate = date;
        lv_label_set_text(lbl_date, date);
    }

    void setLocation(const char *location)
    {
        if (!location || !lbl_location)
            return;
        if (cachedLocation == location)
            return;
        cachedLocation = location;
        lv_label_set_text(lbl_location, location);
    }

    void setNextPrayer(const char *name, const char *time)
    {
        if (!lbl_prayer_name || !lbl_prayer_time)
            return;

        if (name)
        {
            etl::string<32> buf(name);
            for (auto &c : buf)
                c = toupper(c);

            if (lastPrayerName != buf)
            {
                lastPrayerName = buf;
                lv_label_set_text(lbl_prayer_name, buf.c_str());
            }
        }

        if (time && lastPrayerTime != time)
        {
            lastPrayerTime = time;
            lv_label_set_text(lbl_prayer_time, time);
        }
    }

    void setNtpSynced(bool s)
    {
        if (!icon_sync || synced == s)
            return;
        synced = s;
        UiIcons::drawSyncIcon(icon_sync, s ? COLOR_ACCENT : COLOR_DIM, s);
    }

    void setAdhanAvailable(bool available)
    {
        if (!icon_mute || adhanAvailable == available)
            return;
        adhanAvailable = available;
        lv_color_t col = available ? (muted ? COLOR_DIM : COLOR_ACCENT) : COLOR_DIM;
        UiIcons::drawSpeakerIcon(icon_mute, muted, col, available);
    }

    void setMuted(bool m)
    {
        if (!icon_mute || muted == m)
            return;
        muted = m;
        if (adhanAvailable)
        {
            UiIcons::drawSpeakerIcon(icon_mute, muted, muted ? COLOR_DIM : COLOR_ACCENT, true);
        }
    }

    bool isMuted() { return muted; }

} // namespace UiPageHome
