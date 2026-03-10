/**
 * @file ui_page_prayer.cpp
 * @brief Screen 1 — Clock + Dates
 *
 * Visual spec:
 *   - Status bar: city + date | wifi bars + battery
 *   - Separator gradient
 *   - "ŞU AN" label
 *   - Clock: HH + blinking : + MM  (Cinzel SemiBold 72)
 *   - Seconds: DM Mono 14
 *   - Gold ✦ divider
 *   - Gregorian date (Cormorant italic 14)
 *   - Hijri date (Amiri 18)
 *   - Nav dots (active = 1)
 */

#include "ui_page_prayer.h"
#include "ui_theme.h"
#include "ui_components.h"
#include <lvgl.h>
#include <cstdio>
#include <cstring>

namespace UiPagePrayer
{
    // ── Widget handles ─────────────────────────────────────────────
    static lv_obj_t *scr = nullptr;
    // Status bar
    static lv_obj_t *lbl_city = nullptr;
    static lv_obj_t *lbl_dateabbr = nullptr;
    static lv_obj_t *wifi_bars[3] = {};
    static lv_obj_t *bat_fill = nullptr;
    static lv_obj_t *lbl_charge = nullptr;
    // Clock
    static lv_obj_t *lbl_h = nullptr;
    static lv_obj_t *lbl_colon = nullptr;
    static lv_obj_t *lbl_m = nullptr;
    static lv_obj_t *lbl_sec = nullptr;
    // Dates
    static lv_obj_t *lbl_date = nullptr;
    static lv_obj_t *lbl_hijri = nullptr;
    // Nav dots
    static lv_obj_t *nav_dot[4] = {};

    // ── Colon blink animation ──────────────────────────────────────
    static void colon_anim_cb(void *obj, int32_t val)
    {
        lv_obj_set_style_text_opa((lv_obj_t *)obj, (lv_opa_t)val, 0);
    }

    static void startColonAnim()
    {
        if (!lbl_colon)
            return;
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_exec_cb(&a, colon_anim_cb);
        lv_anim_set_var(&a, lbl_colon);
        lv_anim_set_values(&a, LV_OPA_COVER, 26);
        lv_anim_set_time(&a, 500);
        lv_anim_set_playback_time(&a, 500);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
    }

    // ── Helpers ─────────────────────────────────────────────────────
    static void noScrollNoBorder(lv_obj_t *o)
    {
        lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_border_width(o, 0, 0);
        lv_obj_set_style_pad_all(o, 0, 0);
        lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    }

    // ── Status bar (h=22) ──────────────────────────────────────────
    static void buildStatusBar(lv_obj_t *parent)
    {
        lv_obj_t *bar = lv_obj_create(parent);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, 480, 22);
        lv_obj_set_pos(bar, 0, 0);
        noScrollNoBorder(bar);
        lv_obj_set_style_bg_color(bar, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);

        // Left: flex row gap=6
        lv_obj_t *left = lv_obj_create(bar);
        lv_obj_remove_style_all(left);
        lv_obj_set_size(left, LV_SIZE_CONTENT, 22);
        lv_obj_set_pos(left, 8, 0);
        noScrollNoBorder(left);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(left, 6, 0);

        lv_obj_t *pin = lv_obj_create(left);
        lv_obj_remove_style_all(pin);
        lv_obj_set_size(pin, 3, 4);
        lv_obj_set_style_radius(pin, 1, 0);
        lv_obj_set_style_bg_color(pin, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(pin, 80, 0);
        lv_obj_set_style_border_width(pin, 0, 0);

        lbl_city = lv_label_create(left);
        lv_obj_set_style_text_font(lbl_city, FONT_BODY_12, 0);
        lv_obj_set_style_text_color(lbl_city, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(lbl_city, 2, 0);
        lv_label_set_text(lbl_city, "");

        lbl_dateabbr = lv_label_create(left);
        lv_obj_set_style_text_font(lbl_dateabbr, FONT_HEADING_10, 0);
        lv_obj_set_style_text_color(lbl_dateabbr, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(lbl_dateabbr, 209, 0);
        lv_label_set_text(lbl_dateabbr, "");

        // Right: flex row gap=6
        lv_obj_t *right = lv_obj_create(bar);
        lv_obj_remove_style_all(right);
        lv_obj_set_size(right, LV_SIZE_CONTENT, 22);
        noScrollNoBorder(right);
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, -8, 0);
        lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(right, 6, 0);

        // Wifi bars
        static const lv_coord_t BAR_H[3] = {4, 7, 10};
        lv_obj_t *wifi_cont = lv_obj_create(right);
        lv_obj_remove_style_all(wifi_cont);
        noScrollNoBorder(wifi_cont);
        lv_obj_set_size(wifi_cont, 14, 10);
        for (int i = 0; i < 3; i++)
        {
            wifi_bars[i] = lv_obj_create(wifi_cont);
            lv_obj_remove_style_all(wifi_bars[i]);
            lv_obj_set_size(wifi_bars[i], 3, BAR_H[i]);
            lv_obj_set_pos(wifi_bars[i], i * 4, 10 - BAR_H[i]);
            lv_obj_set_style_radius(wifi_bars[i], 1, 0);
            lv_obj_set_style_bg_color(wifi_bars[i], COLOR_DIM, 0);
            lv_obj_set_style_bg_opa(wifi_bars[i], 128, 0);
            lv_obj_set_style_border_width(wifi_bars[i], 0, 0);
        }

        // Battery wrapper (w=21 h=9)
        lv_obj_t *bat_wrap = lv_obj_create(right);
        lv_obj_remove_style_all(bat_wrap);
        lv_obj_set_size(bat_wrap, 21, 9);
        lv_obj_set_style_bg_opa(bat_wrap, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(bat_wrap, 0, 0);
        lv_obj_clear_flag(bat_wrap, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *bat_outer = lv_obj_create(bat_wrap);
        lv_obj_remove_style_all(bat_outer);
        lv_obj_set_size(bat_outer, 18, 9);
        lv_obj_set_pos(bat_outer, 0, 0);
        lv_obj_set_style_radius(bat_outer, 2, 0);
        lv_obj_set_style_border_width(bat_outer, 1, 0);
        lv_obj_set_style_border_color(bat_outer, COLOR_GOLD, 0);
        lv_obj_set_style_border_opa(bat_outer, 102, 0);
        lv_obj_set_style_bg_opa(bat_outer, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(bat_outer, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *bat_nub = lv_obj_create(bat_wrap);
        lv_obj_remove_style_all(bat_nub);
        lv_obj_set_size(bat_nub, 2, 5);
        lv_obj_set_pos(bat_nub, 19, 2);
        lv_obj_set_style_radius(bat_nub, 0, 0);
        lv_obj_set_style_bg_color(bat_nub, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(bat_nub, 130, 0);
        lv_obj_set_style_border_width(bat_nub, 0, 0);

        bat_fill = lv_obj_create(bat_outer);
        lv_obj_remove_style_all(bat_fill);
        lv_obj_set_size(bat_fill, 10, 5);
        lv_obj_set_pos(bat_fill, 2, 2);
        lv_obj_set_style_radius(bat_fill, 1, 0);
        lv_obj_set_style_bg_color(bat_fill, COLOR_GREEN, 0);
        lv_obj_set_style_bg_opa(bat_fill, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bat_fill, 0, 0);
        lv_obj_clear_flag(bat_fill, LV_OBJ_FLAG_SCROLLABLE);

        lbl_charge = lv_label_create(bat_outer);
        lv_obj_set_style_text_font(lbl_charge, FONT_MONO_10, 0);
        lv_obj_set_style_text_color(lbl_charge, COLOR_GOLD, 0);
        lv_label_set_text(lbl_charge, "\xE2\x9A\xA1");
        lv_obj_center(lbl_charge);
        lv_obj_add_flag(lbl_charge, LV_OBJ_FLAG_HIDDEN);
    }

    // ── Separator gradient ─────────────────────────────────────────
    static void buildSeparator(lv_obj_t *parent)
    {
        lv_obj_t *sep_l = lv_obj_create(parent);
        lv_obj_remove_style_all(sep_l);
        lv_obj_set_size(sep_l, 240, 2);
        lv_obj_set_pos(sep_l, 0, 22);
        lv_obj_set_style_bg_color(sep_l, COLOR_BG, 0);
        lv_obj_set_style_bg_grad_color(sep_l, COLOR_GOLD, 0);
        lv_obj_set_style_bg_grad_dir(sep_l, LV_GRAD_DIR_HOR, 0);
        lv_obj_set_style_bg_opa(sep_l, 230, 0);
        lv_obj_set_style_shadow_color(sep_l, COLOR_GOLD, 0);
        lv_obj_set_style_shadow_width(sep_l, 8, 0);
        lv_obj_set_style_shadow_opa(sep_l, 90, 0);
        lv_obj_set_style_shadow_spread(sep_l, 0, 0);
        lv_obj_set_style_border_width(sep_l, 0, 0);

        lv_obj_t *sep_r = lv_obj_create(parent);
        lv_obj_remove_style_all(sep_r);
        lv_obj_set_size(sep_r, 240, 2);
        lv_obj_set_pos(sep_r, 240, 22);
        lv_obj_set_style_bg_color(sep_r, COLOR_GOLD, 0);
        lv_obj_set_style_bg_grad_color(sep_r, COLOR_BG, 0);
        lv_obj_set_style_bg_grad_dir(sep_r, LV_GRAD_DIR_HOR, 0);
        lv_obj_set_style_bg_opa(sep_r, 230, 0);
        lv_obj_set_style_shadow_color(sep_r, COLOR_GOLD, 0);
        lv_obj_set_style_shadow_width(sep_r, 8, 0);
        lv_obj_set_style_shadow_opa(sep_r, 90, 0);
        lv_obj_set_style_shadow_spread(sep_r, 0, 0);
        lv_obj_set_style_border_width(sep_r, 0, 0);
    }

    // ── Content (vertically centered, flex column) ─────────────────
    static void buildContent(lv_obj_t *parent)
    {
        lv_obj_t *cont = lv_obj_create(parent);
        lv_obj_remove_style_all(cont);
        lv_obj_set_size(cont, 480, LV_SIZE_CONTENT);
        lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cont, 0, 0);
        lv_obj_set_style_pad_all(cont, 0, 0);
        lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(cont, 4, 0);

        // Clock row: HH + : + MM
        lv_obj_t *clock_row = lv_obj_create(cont);
        lv_obj_remove_style_all(clock_row);
        lv_obj_set_size(clock_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(clock_row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(clock_row, 0, 0);
        lv_obj_set_style_pad_all(clock_row, 0, 0);
        lv_obj_clear_flag(clock_row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(clock_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(clock_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(clock_row, 0, 0);

        lbl_h = lv_label_create(clock_row);
        lv_obj_set_style_text_font(lbl_h, FONT_CLOCK_72, 0);
        lv_obj_set_style_text_color(lbl_h, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(lbl_h, 4, 0);
        lv_label_set_text(lbl_h, "20");

        lbl_colon = lv_label_create(clock_row);
        lv_obj_set_style_text_font(lbl_colon, FONT_CLOCK_72, 0);
        lv_obj_set_style_text_color(lbl_colon, COLOR_GOLD, 0);
        lv_label_set_text(lbl_colon, ":");

        lbl_m = lv_label_create(clock_row);
        lv_obj_set_style_text_font(lbl_m, FONT_CLOCK_72, 0);
        lv_obj_set_style_text_color(lbl_m, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(lbl_m, 4, 0);
        lv_label_set_text(lbl_m, "36");

        // Seconds
        lbl_sec = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl_sec, FONT_MONO_14, 0);
        lv_obj_set_style_text_color(lbl_sec, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(lbl_sec, 204, 0);
        lv_obj_set_style_text_letter_space(lbl_sec, 4, 0);
        lv_label_set_text(lbl_sec, ":00");

        // Decorative divider: line + ✦ + line
        lv_obj_t *divider = lv_obj_create(cont);
        lv_obj_remove_style_all(divider);
        lv_obj_set_size(divider, LV_SIZE_CONTENT, 16);
        lv_obj_set_style_bg_opa(divider, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(divider, 0, 0);
        lv_obj_set_style_pad_all(divider, 0, 0);
        lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(divider, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(divider, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(divider, 10, 0);

        lv_obj_t *dl = lv_obj_create(divider);
        lv_obj_remove_style_all(dl);
        lv_obj_set_size(dl, 55, 1);
        lv_obj_set_style_bg_color(dl, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(dl, 115, 0);
        lv_obj_set_style_border_width(dl, 0, 0);

        lv_obj_t *star = lv_label_create(divider);
        lv_obj_set_style_text_font(star, FONT_HEADING_10, 0);
        lv_obj_set_style_text_color(star, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(star, 140, 0);
        lv_label_set_text(star, "\xC2\xB7"); // · U+00B7 (middle dot — closest available)

        lv_obj_t *dr = lv_obj_create(divider);
        lv_obj_remove_style_all(dr);
        lv_obj_set_size(dr, 55, 1);
        lv_obj_set_style_bg_color(dr, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(dr, 115, 0);
        lv_obj_set_style_border_width(dr, 0, 0);

        // Gregorian date
        lbl_date = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl_date, FONT_BODY_14, 0);
        lv_obj_set_style_text_color(lbl_date, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(lbl_date, 209, 0);
        lv_obj_set_style_text_letter_space(lbl_date, 2, 0);
        lv_label_set_text(lbl_date, "");

        // Hijri date
        lbl_hijri = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl_hijri, FONT_MONO_14, 0);
        lv_obj_set_style_text_color(lbl_hijri, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(lbl_hijri, 217, 0);
        lv_obj_set_style_text_letter_space(lbl_hijri, 2, 0);
        lv_label_set_text(lbl_hijri, "");
    }

    // ── Nav dots (5 pills, active=1) ──────────────────────────────
    static void buildNavDots(lv_obj_t *parent)
    {
        lv_obj_t *nav = lv_obj_create(parent);
        lv_obj_remove_style_all(nav);
        lv_obj_set_size(nav, LV_SIZE_CONTENT, 8);
        lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, -8);
        lv_obj_set_style_bg_opa(nav, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(nav, 0, 0);
        lv_obj_set_style_pad_all(nav, 0, 0);
        lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(nav, 6, 0);

        for (int i = 0; i < 4; i++)
        {
            nav_dot[i] = lv_obj_create(nav);
            lv_obj_remove_style_all(nav_dot[i]);
            bool active = (i == 1);
            lv_obj_set_size(nav_dot[i], active ? 16 : 5, 5);
            lv_obj_set_style_radius(nav_dot[i], 3, 0);
            lv_obj_set_style_bg_color(nav_dot[i], COLOR_GOLD, 0);
            lv_obj_set_style_bg_opa(nav_dot[i], active ? 230 : 82, 0);
            lv_obj_set_style_border_width(nav_dot[i], 0, 0);
            if (active)
            {
                lv_obj_set_style_shadow_color(nav_dot[i], COLOR_GOLD, 0);
                lv_obj_set_style_shadow_spread(nav_dot[i], 3, 0);
                lv_obj_set_style_shadow_opa(nav_dot[i], 115, 0);
            }
        }
    }

    // ── Public API ─────────────────────────────────────────────────
    lv_obj_t *create()
    {
        UiTheme::initStyles();

        if (scr)
        {
            lv_anim_del(lbl_colon, colon_anim_cb);
            lv_obj_del(scr);
            scr = nullptr;
            lbl_city = lbl_dateabbr = nullptr;
            for (int i = 0; i < 3; i++)
                wifi_bars[i] = nullptr;
            bat_fill = lbl_charge = nullptr;
            lbl_h = lbl_colon = lbl_m = lbl_sec = nullptr;
            lbl_date = lbl_hijri = nullptr;
            for (int i = 0; i < 4; i++)
                nav_dot[i] = nullptr;
        }

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(scr, 0, 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        UiComponents::applyMotif(scr);
        buildStatusBar(scr);
        buildSeparator(scr);
        buildContent(scr);
        buildNavDots(scr);

        startColonAnim();
        return scr;
    }

    lv_obj_t *getScreen() { return scr; }

    void setTime(int hour, int minute, int second)
    {
        if (lbl_h)
        {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02d", hour);
            lv_label_set_text(lbl_h, buf);
        }
        if (lbl_m)
        {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02d", minute);
            lv_label_set_text(lbl_m, buf);
        }
        if (lbl_sec)
        {
            char buf[4];
            snprintf(buf, sizeof(buf), ":%02d", second);
            lv_label_set_text(lbl_sec, buf);
        }
    }

    void setGregorianDate(const char *date)
    {
        if (lbl_date && date)
            lv_label_set_text(lbl_date, date);
    }

    void setHijriDate(const char *hijri)
    {
        if (lbl_hijri && hijri)
            lv_label_set_text(lbl_hijri, hijri);
    }

    void setStatusBarCity(const char *city, const char *dateAbbrev)
    {
        if (lbl_city && city)
            lv_label_set_text(lbl_city, city);
        if (lbl_dateabbr && dateAbbrev)
            lv_label_set_text(lbl_dateabbr, dateAbbrev);
    }

    void setWifi(uint8_t bars)
    {
        for (int i = 0; i < 3; i++)
        {
            if (!wifi_bars[i])
                continue;
            bool lit = (i < bars);
            lv_obj_set_style_bg_color(wifi_bars[i], lit ? COLOR_GREEN : COLOR_DIM, 0);
            lv_obj_set_style_bg_opa(wifi_bars[i], lit ? LV_OPA_COVER : 50, 0);
        }
    }

    void setBattery(uint8_t pct, bool charging)
    {
        if (!bat_fill)
            return;
        int32_t fill_w = ((int32_t)pct * 14) / 100;
        if (fill_w < 0)
            fill_w = 0;
        if (fill_w > 14)
            fill_w = 14;
        lv_obj_set_width(bat_fill, (lv_coord_t)fill_w);
        lv_color_t col = (pct > 20) ? COLOR_GREEN : lv_color_hex(0xFF4444);
        lv_obj_set_style_bg_color(bat_fill, col, 0);
        if (lbl_charge)
        {
            if (charging)
                lv_obj_clear_flag(lbl_charge, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(lbl_charge, LV_OBJ_FLAG_HIDDEN);
        }
    }

} // namespace UiPagePrayer
