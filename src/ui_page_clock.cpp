/**
 * @file ui_page_clock.cpp
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

#include "ui_page_clock.h"
#include "ui_theme.h"
#include "ui_components.h"
#include "locale_tr.h"
#include <lvgl.h>
#include <cstdio>
#include <cstring>

namespace UiPageClock
{
    using UiComponents::noScrollNoBorder;

    // ── Widget handles ────────────────────────────────────────────────────
    static lv_obj_t *scr = nullptr;
    static UiComponents::StatusBarHandles sb_handles = {};
    // Clock
    static lv_obj_t *lbl_h = nullptr;
    static lv_obj_t *lbl_colon = nullptr;
    static lv_obj_t *lbl_m = nullptr;
    // Dates
    static lv_obj_t *lbl_date = nullptr;
    static lv_obj_t *lbl_hijri = nullptr;

    // ── Colon blink animation ──────────────────────────────────────
    // Colon pulse: driven by setTime() every second — no separate timer
    // Alternates opacity each second: visible → dim → visible → dim
    static bool colonVisible = true;

    static void startColonAnim()
    {
        // No-op — colon blink is now handled in setTime()
        colonVisible = true;
    }

    // ── Helpers ─────────────────────────────────────────────────────
    // ── Status bar (h=22) ──────────────────────────────────────────
    // ── Separator gradient ─────────────────────────────────────────
    // ── Content (vertically centered, flex column) ─────────────────
    static void buildContent(lv_obj_t *parent)
    {
        lv_obj_t *cont = lv_obj_create(parent);
        lv_obj_remove_style_all(cont);
        lv_obj_set_size(cont, 480, 286);
        lv_obj_set_pos(cont, 0, 29);
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
        lv_label_set_text(lbl_h, "--");

        lbl_colon = lv_label_create(clock_row);
        lv_obj_set_style_text_font(lbl_colon, FONT_CLOCK_72, 0);
        lv_obj_set_style_text_color(lbl_colon, COLOR_GOLD, 0);
        lv_label_set_text(lbl_colon, ":");

        lbl_m = lv_label_create(clock_row);
        lv_obj_set_style_text_font(lbl_m, FONT_CLOCK_72, 0);
        lv_obj_set_style_text_color(lbl_m, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(lbl_m, 4, 0);
        lv_label_set_text(lbl_m, "--");

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
        lv_obj_set_size(dl, 60, 1);
        lv_obj_set_style_bg_color(dl, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(dl, 115, 0);
        lv_obj_set_style_border_width(dl, 0, 0);

        lv_obj_t *star = lv_label_create(divider);
        lv_obj_set_style_text_font(star, FONT_HEADING_10, 0);
        lv_obj_set_style_text_color(star, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(star, 90, 0);
        lv_label_set_text(star, "\xC2\xB7"); // · middle dot U+00B7 (in font range)

        lv_obj_t *dr = lv_obj_create(divider);
        lv_obj_remove_style_all(dr);
        lv_obj_set_size(dr, 60, 1);
        lv_obj_set_style_bg_color(dr, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(dr, 115, 0);
        lv_obj_set_style_border_width(dr, 0, 0);

        // Gregorian date
        lbl_date = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl_date, FONT_HIJRI_18, 0);
        lv_obj_set_style_text_color(lbl_date, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(lbl_date, 209, 0);
        lv_label_set_text(lbl_date, "");

        // Hijri date
        lbl_hijri = lv_label_create(cont);
        lv_obj_set_style_text_font(lbl_hijri, FONT_PRAYER_24, 0);
        lv_obj_set_style_text_color(lbl_hijri, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(lbl_hijri, 209, 0);
        lv_label_set_text(lbl_hijri, "");
    }

    // ── Nav dots (5 pills, active=1) ──────────────────────────────

    // ── Public API ─────────────────────────────────────────────────
    lv_obj_t *create()
    {
        UiTheme::initStyles();

        if (scr)
        {
            lv_obj_del(scr);
            scr = nullptr;
            sb_handles = {};
            lbl_h = lbl_colon = lbl_m = nullptr;
            lbl_date = lbl_hijri = nullptr;
        }

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(scr, 0, 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        UiComponents::applyMotif(scr);
        UiComponents::createAmbientGlow(scr);

        sb_handles = UiComponents::createStatusBar(scr);
        UiComponents::createSeparator(scr);
        buildContent(scr);
        UiComponents::createNavDots(scr, 1);

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
        if (lbl_colon)
        {
            colonVisible = !colonVisible;
            lv_obj_set_style_text_opa(lbl_colon, colonVisible ? LV_OPA_COVER : 26, 0);
        }
        if (lbl_m)
        {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02d", minute);
            lv_label_set_text(lbl_m, buf);
        }
    }

    void setGregorianDate(const char *date)
    {
        if (lbl_date && date)
            lv_label_set_text(lbl_date, LocaleTR::toUpperTR(date));
    }

    void setHijriDate(const char *hijri)
    {
        if (lbl_hijri && hijri)
            lv_label_set_text(lbl_hijri, LocaleTR::toUpperTR(hijri));
    }

    void setRamadanCountdown(bool visible, const char *text)
    {
        // Ramadan/iftar pill now lives only on home page
        (void)visible;
        (void)text;
    }

    void setStatusBarCity(const char *city, const char *dateAbbrev)
    {
        UiComponents::updateStatusBarCity(sb_handles, city, dateAbbrev);
    }

    void setWifi(uint8_t bars)
    {
        UiComponents::updateStatusBarWifi(sb_handles, bars);
    }

    void setBattery(uint8_t pct, bool charging)
    {
        UiComponents::updateStatusBarBattery(sb_handles, pct, charging);
    }

} // namespace UiPageClock
