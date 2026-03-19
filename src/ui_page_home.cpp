/**
 * @file ui_page_home.cpp
 * @brief Screen 0 — Main Dashboard
 *
 * Visual spec matches the HTML mockup exactly:
 *   - Status bar: city + date | wifi bars + battery
 *   - Separator: transparent→gold→transparent gradient
 *   - Greeting: Cormorant italic — left dim + right gold
 *   - Hero: NEXT PRAYER label / prayer name / countdown 60px / EZANA KALAN / iftar pill
 *   - Prayer strip: 6 cols, done/active/future states, glow bar on active
 *   - Nav dots: 5 pills bottom-center
 */

#include "ui_page_home.h"
#include "ui_components.h"
#include "ui_theme.h"
#include "locale_tr.h"
#include "app_state.h"
#include <lvgl.h>
#include <cstdio>
#include <cstring>

namespace UiPageHome
{
    using UiComponents::noScrollNoBorder;

    // ── Widget handles ────────────────────────────────────────────────
    static lv_obj_t *scr = nullptr;
    static UiComponents::StatusBarHandles sb_handles = {};
    static lv_obj_t *lbl_next_label = nullptr;
    static lv_obj_t *lbl_prayer_nm = nullptr;
    static lv_obj_t *lbl_countdown = nullptr;
    static lv_obj_t *iftar_pill = nullptr;
    static lv_obj_t *lbl_iftar_val = nullptr;

    // Strip: 6 columns
    static const char *STRIP_NAMES[6] = {
        "\xC4\xB0MSAK", "G\xC3\x9CNE\xC5\x9E", "\xC3\x96\xC4\x9ELE",
        "\xC4\xB0K\xC4\xB0ND\xC4\xB0", "AK\xC5\x9E"
                                       "AM",
        "YATSI"};
    static lv_obj_t *strip_col[6] = {};
    static lv_obj_t *strip_dot[6] = {};
    static lv_obj_t *strip_name[6] = {};
    static lv_obj_t *strip_time_lbl[6] = {};
    static lv_obj_t *strip_glow_bar = nullptr; // lives inside active column
    static int8_t cur_active_idx = -1;

    // Nav dot pills — MOVED TO UiComponents::createNavDots()

    // ── Helpers ────────────────────────────────────────────────────
    // ── Status bar ── MOVED TO UiComponents::createStatusBar() ────────

    // ── Separator ── MOVED TO UiComponents::createSeparator() ──────────

    // ── Hero area (clean layout: no greeting row) ────────────────────
    static void buildHero(lv_obj_t *parent)
    {
        lv_obj_t *hero = lv_obj_create(parent);
        lv_obj_remove_style_all(hero);
        noScrollNoBorder(hero);
        lv_obj_set_size(hero, 480, 144);
        lv_obj_set_pos(hero, 0, 50);
        lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(hero, 10, 0);
        lv_obj_set_style_pad_left(hero, 20, 0);
        lv_obj_set_style_pad_right(hero, 20, 0);

        // 1. "{PrayerName} vaktine" label (~24px)
        lbl_next_label = lv_label_create(hero);
        lv_obj_set_style_text_font(lbl_next_label, FONT_PRAYER_24, 0);
        lv_obj_set_style_text_color(lbl_next_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_opa(lbl_next_label, 245, 0);
        lv_obj_set_style_text_letter_space(lbl_next_label, 1, 0);
        lv_label_set_recolor(lbl_next_label, true);
        lv_label_set_text(lbl_next_label, "--- vaktine");

        // 2. Prayer name (hidden — merged into vakite label above)
        lbl_prayer_nm = nullptr;

        // 3. Countdown (60px font)
        lbl_countdown = lv_label_create(hero);
        lv_obj_set_style_text_font(lbl_countdown, FONT_MONO_60, 0);
        lv_obj_set_style_text_color(lbl_countdown, COLOR_GOLD_LIGHT, 0);
        lv_obj_set_style_text_letter_space(lbl_countdown, 4, 0);
        lv_obj_set_style_shadow_color(lbl_countdown, COLOR_GOLD, 0);
        lv_obj_set_style_shadow_spread(lbl_countdown, 10, 0);
        lv_obj_set_style_shadow_opa(lbl_countdown, 58, 0);
        lv_label_set_text(lbl_countdown, "00:00:00");

        // 4. Iftar pill — HTML-like compact card with left accent line
        iftar_pill = lv_obj_create(parent);
        lv_obj_remove_style_all(iftar_pill);
        lv_obj_set_size(iftar_pill, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(iftar_pill, LV_ALIGN_TOP_MID, 0, 178);
        lv_obj_set_style_radius(iftar_pill, 4, 0);
        lv_obj_set_style_bg_color(iftar_pill, COLOR_BG2, 0);
        lv_obj_set_style_bg_opa(iftar_pill, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(iftar_pill, COLOR_DIM, 0);
        lv_obj_set_style_border_opa(iftar_pill, 130, 0);
        lv_obj_set_style_border_width(iftar_pill, 3, 0);
        lv_obj_set_style_border_side(iftar_pill, LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_pad_top(iftar_pill, 6, 0);
        lv_obj_set_style_pad_bottom(iftar_pill, 6, 0);
        lv_obj_set_style_pad_left(iftar_pill, 10, 0);
        lv_obj_set_style_pad_right(iftar_pill, 14, 0);
        lv_obj_clear_flag(iftar_pill, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(iftar_pill, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(iftar_pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(iftar_pill, 8, 0);
        lv_obj_add_flag(iftar_pill, LV_OBJ_FLAG_HIDDEN);

        // dot
        lv_obj_t *idot = lv_obj_create(iftar_pill);
        lv_obj_remove_style_all(idot);
        lv_obj_set_size(idot, 7, 7);
        lv_obj_set_style_radius(idot, 3, 0);
        lv_obj_set_style_bg_color(idot, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(idot, 166, 0);
        lv_obj_set_style_border_width(idot, 0, 0);

        // Full text label: "İftara Kaldı 02:31" or "Sahura Kaldı 05:12"
        lbl_iftar_val = lv_label_create(iftar_pill);
        lv_obj_set_style_text_font(lbl_iftar_val, FONT_BODY_12, 0);
        lv_obj_set_style_text_color(lbl_iftar_val, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(lbl_iftar_val, 1, 0);
        lv_label_set_text(lbl_iftar_val, "");
    }

    // ── Prayer strip (HTML-like: single top border + 1px vertical dividers) ──
    static void buildPrayerStrip(lv_obj_t *parent)
    {
        lv_obj_t *strip = lv_obj_create(parent);
        lv_obj_remove_style_all(strip);
        lv_obj_set_size(strip, 480, 64);
        lv_obj_set_pos(strip, 0, 238);
        lv_obj_set_style_bg_color(strip, COLOR_STRIP_BG, 0);
        lv_obj_set_style_bg_opa(strip, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(strip, 0, 0);
        lv_obj_clear_flag(strip, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(strip, 0, 0);

        lv_obj_t *top_line = lv_obj_create(strip);
        lv_obj_remove_style_all(top_line);
        lv_obj_set_size(top_line, 480, 1);
        lv_obj_set_pos(top_line, 0, 0);
        lv_obj_set_style_bg_color(top_line, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(top_line, 166, 0);
        lv_obj_set_style_border_width(top_line, 0, 0);
        lv_obj_clear_flag(top_line, LV_OBJ_FLAG_SCROLLABLE);

        // 6 columns (w=80 each)
        for (int i = 0; i < 6; i++)
        {
            lv_obj_t *col = lv_obj_create(strip);
            lv_obj_remove_style_all(col);
            lv_obj_set_size(col, 80, 63);
            lv_obj_set_pos(col, i * 80, 1);
            lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(col, 3, 0);
            lv_obj_set_style_pad_all(col, 2, 0);
            lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(col, 0, 0);
            lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);
            strip_col[i] = col;

            // Divider (except first column)
            if (i > 0)
            {
                lv_obj_t *div = lv_obj_create(strip);
                lv_obj_remove_style_all(div);
                lv_obj_set_size(div, 1, 63);
                lv_obj_set_pos(div, i * 80, 1);
                lv_obj_set_style_bg_color(div, COLOR_BORDER, 0);
                lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
                lv_obj_set_style_border_width(div, 0, 0);
                lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
            }

            // marker (rectangle for cleaner readability on low DPI)
            lv_obj_t *dot = lv_obj_create(col);
            lv_obj_remove_style_all(dot);
            lv_obj_set_size(dot, 0, 0);
            lv_obj_set_style_radius(dot, 1, 0);
            lv_obj_set_style_bg_color(dot, COLOR_DIM, 0);
            lv_obj_set_style_bg_opa(dot, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            strip_dot[i] = dot;

            // name
            lv_obj_t *nm = lv_label_create(col);
            lv_obj_set_style_text_font(nm, FONT_HEADING_12, 0);
            lv_obj_set_style_text_color(nm, COLOR_TEXT, 0);
            lv_obj_set_style_text_opa(nm, LV_OPA_COVER, 0);
            lv_obj_set_style_text_letter_space(nm, 0, 0);
            lv_label_set_text(nm, STRIP_NAMES[i]);
            strip_name[i] = nm;

            // time
            lv_obj_t *t = lv_label_create(col);
            lv_obj_set_style_text_font(t, FONT_MONO_14, 0);
            lv_obj_set_style_text_color(t, COLOR_TEXT, 0);
            lv_obj_set_style_text_opa(t, LV_OPA_COVER, 0);
            lv_obj_set_style_text_letter_space(t, 0, 0);
            lv_label_set_text(t, "--:--");
            strip_time_lbl[i] = t;
        }
    }

    // ── Nav dots (bottom-center, y=306) ──────────────────────────────

    // ────────────────────────────────────────────────────────────────
    // PUBLIC API
    // ────────────────────────────────────────────────────────────────

    lv_obj_t *create()
    {
        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_set_size(scr, 480, 320);
        lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_border_width(scr, 0, 0);
        lv_obj_set_style_pad_all(scr, 0, 0);

        // Phase D: keep home background clean (no motif/glow overlays)
        sb_handles = UiComponents::createStatusBar(scr);
        UiComponents::createSeparator(scr);
        buildHero(scr);
        buildPrayerStrip(scr);
        UiComponents::createNavDots(scr, 0);

        return scr;
    }

    lv_obj_t *getScreen() { return scr; }

    // ────────────────────────
    // SETTERS
    // ────────────────────────

    void setCountdown(uint32_t s)
    {
        if (!lbl_countdown)
            return;
        char buf[12];
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
                 (unsigned long)(s / 3600),
                 (unsigned long)((s % 3600) / 60),
                 (unsigned long)(s % 60));
        lv_label_set_text(lbl_countdown, buf);
        lv_color_t col = (s < 600) ? COLOR_AMBER : COLOR_GOLD_LIGHT;
        lv_obj_set_style_text_color(lbl_countdown, col, 0);
    }

    void setNextPrayerName(const char *name)
    {
        if (!lbl_next_label || !name)
            return;
        // Two-tone recolor: prayer name in GOLD, "VAKTİNE" inherits DIM
        char buf[80];
        snprintf(buf, sizeof(buf), "#EAC96A %s# VAKT\xC4\xB0NE", LocaleTR::toUpperTR(name));
        lv_label_set_text(lbl_next_label, buf);
    }

    void setPrayerTimes(const char *fajr, const char *sunrise,
                        const char *dhuhr, const char *asr,
                        const char *maghrib, const char *isha)
    {
        const char *times[6] = {fajr, sunrise, dhuhr, asr, maghrib, isha};
        for (int i = 0; i < 6; i++)
        {
            if (strip_time_lbl[i] && times[i])
                lv_label_set_text(strip_time_lbl[i], times[i]);
        }
    }

    void setActivePrayerIndex(int8_t idx)
    {
        if (idx == cur_active_idx)
            return;
        int8_t old = cur_active_idx;
        cur_active_idx = idx;

        // When there is no active prayer slot (e.g., showing tomorrow times),
        // remove the previous highlight bar so focus does not stick on Isha.
        if (idx < 0 && strip_glow_bar)
        {
            lv_obj_del(strip_glow_bar);
            strip_glow_bar = nullptr;
        }

        // Reset old active column
        if (old >= 0 && old < 6)
        {
            lv_obj_set_style_bg_opa(strip_col[old], LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_grad_dir(strip_col[old], LV_GRAD_DIR_NONE, 0);
            lv_obj_set_style_text_color(strip_name[old], COLOR_DIM, 0);
            lv_obj_set_style_text_opa(strip_name[old], 220, 0);
            lv_obj_set_style_text_color(strip_time_lbl[old], COLOR_DIM, 0);
            lv_obj_set_style_text_opa(strip_time_lbl[old], 220, 0);
            lv_obj_set_style_bg_color(strip_dot[old], COLOR_DIM, 0);
            lv_obj_set_style_bg_opa(strip_dot[old], 130, 0);
            lv_obj_set_style_shadow_opa(strip_dot[old], 0, 0);
        }

        for (int i = 0; i < 6; i++)
        {
            bool done = (i < idx);
            bool active = (i == idx);

            if (done)
            {
                lv_obj_set_style_opa(strip_col[i], 205, 0);
                lv_obj_set_style_bg_opa(strip_col[i], LV_OPA_TRANSP, 0);
                lv_obj_set_style_bg_grad_dir(strip_col[i], LV_GRAD_DIR_NONE, 0);
                lv_obj_set_style_text_color(strip_name[i], COLOR_TEXT, 0);
                lv_obj_set_style_text_opa(strip_name[i], 235, 0);
                lv_obj_set_style_text_color(strip_time_lbl[i], COLOR_TEXT, 0);
                lv_obj_set_style_text_opa(strip_time_lbl[i], 235, 0);
                lv_obj_set_style_bg_color(strip_dot[i], COLOR_GREEN, 0);
                lv_obj_set_style_bg_opa(strip_dot[i], 220, 0);
                lv_obj_set_style_shadow_opa(strip_dot[i], 0, 0);
            }
            else if (active)
            {
                lv_obj_set_style_opa(strip_col[i], LV_OPA_COVER, 0);
                // Active tile: darker block like HTML reference (#252840)
                lv_obj_set_style_bg_color(strip_col[i], COLOR_BG2, 0);
                lv_obj_set_style_bg_grad_dir(strip_col[i], LV_GRAD_DIR_NONE, 0);
                lv_obj_set_style_bg_opa(strip_col[i], LV_OPA_COVER, 0);
                lv_obj_set_style_text_color(strip_name[i], COLOR_GOLD_LIGHT, 0);
                lv_obj_set_style_text_opa(strip_name[i], LV_OPA_COVER, 0);
                lv_obj_set_style_text_color(strip_time_lbl[i], COLOR_GOLD, 0);
                lv_obj_set_style_text_opa(strip_time_lbl[i], LV_OPA_COVER, 0);
                lv_obj_set_style_bg_color(strip_dot[i], COLOR_GOLD, 0);
                lv_obj_set_style_bg_opa(strip_dot[i], LV_OPA_COVER, 0);
                lv_obj_set_style_shadow_color(strip_dot[i], COLOR_GOLD, 0);
                lv_obj_set_style_shadow_spread(strip_dot[i], 4, 0);
                lv_obj_set_style_shadow_opa(strip_dot[i], 200, 0);

                // Create a constant yellow indicator at the bottom of active column.
                if (strip_glow_bar)
                    lv_obj_del(strip_glow_bar);
                strip_glow_bar = lv_obj_create(strip_col[i]);
                lv_obj_remove_style_all(strip_glow_bar);
                lv_obj_set_size(strip_glow_bar, 80, 2);
                lv_obj_align(strip_glow_bar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
                lv_obj_set_style_bg_color(strip_glow_bar, COLOR_GOLD_LIGHT, 0);
                lv_obj_set_style_bg_grad_color(strip_glow_bar, COLOR_GOLD, 0);
                lv_obj_set_style_bg_grad_dir(strip_glow_bar, LV_GRAD_DIR_HOR, 0);
                lv_obj_set_style_bg_opa(strip_glow_bar, LV_OPA_COVER, 0);
                lv_obj_set_style_shadow_color(strip_glow_bar, COLOR_GOLD, 0);
                lv_obj_set_style_shadow_spread(strip_glow_bar, 4, 0);
                lv_obj_set_style_shadow_opa(strip_glow_bar, 180, 0);
                lv_obj_set_style_radius(strip_glow_bar, 0, 0);
                lv_obj_set_style_border_width(strip_glow_bar, 0, 0);
                lv_obj_set_pos(strip_glow_bar, 0, 60);
            }
            else
            {
                // future
                lv_obj_set_style_opa(strip_col[i], LV_OPA_COVER, 0);
                lv_obj_set_style_bg_opa(strip_col[i], LV_OPA_TRANSP, 0);
                lv_obj_set_style_bg_grad_dir(strip_col[i], LV_GRAD_DIR_NONE, 0);
                lv_obj_set_style_text_color(strip_name[i], COLOR_TEXT, 0);
                lv_obj_set_style_text_opa(strip_name[i], 230, 0);
                lv_obj_set_style_text_color(strip_time_lbl[i], COLOR_TEXT, 0);
                lv_obj_set_style_text_opa(strip_time_lbl[i], 230, 0);
                lv_obj_set_style_bg_color(strip_dot[i], COLOR_DIM, 0);
                lv_obj_set_style_bg_opa(strip_dot[i], 120, 0);
                lv_obj_set_style_shadow_opa(strip_dot[i], 0, 0);
            }
        }
    }

    void setActivePrayerProgress(uint8_t pct)
    {
        (void)pct;
        if (!strip_glow_bar || cur_active_idx < 0)
            return;

        // Keep active indicator constant width (no progress animation).
        lv_obj_set_width(strip_glow_bar, 80);
    }

    void setGreeting(const char *left, const char *right)
    {
        (void)left;
        (void)right;
    }

    void setIftarDelta(bool visible, const char *text)
    {
        if (!iftar_pill)
            return;
        if (!visible || !text || text[0] == '\0')
        {
            lv_obj_add_flag(iftar_pill, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        lv_obj_clear_flag(iftar_pill, LV_OBJ_FLAG_HIDDEN);
        if (lbl_iftar_val)
            lv_label_set_text(lbl_iftar_val, LocaleTR::toUpperTR(text));
    }

    void setStatusBarCity(const char *city, const char *dateAbbrev)
    {
        UiComponents::updateStatusBarCity(sb_handles, city, dateAbbrev);
    }

    void setMuted(bool muted)
    {
        UiComponents::updateStatusBarMute(sb_handles, muted);
    }

} // namespace UiPageHome
