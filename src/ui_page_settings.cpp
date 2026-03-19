/**
 * @file ui_page_settings.cpp
 * @brief Settings Page — Ekran, Ses, Güç & Mod, Bağlantı
 *
 * 4-section scrollable layout matching HTML mockup.
 * Slider rows, toggle rows, and QR-nav button.
 * Portal overlay preserved for AP mode.
 */

#include "ui_page_settings.h"
#include "ui_theme.h"
#include "ui_components.h"
#include "ui_icons.h"
#include "locale_tr.h"
#include "settings_manager.h"
#include "audio_player.h"
#include "app_state.h"
#include "volume_control.h"
#include "network.h"
#include "wifi_portal.h"
#include "lvgl_display.h"
#include <WiFi.h>
#include <lvgl.h>
#include <extra/libs/qrcode/lv_qrcode.h>
#include <cstdio>
#include <cstring>

namespace UiPageSettings
{
    using namespace UiTheme;

    // ═══════════════════════════════════════════════════════════════
    // WIDGET HANDLES
    // ═══════════════════════════════════════════════════════════════
    static lv_obj_t *scr = nullptr;
    static UiComponents::StatusBarHandles sb_handles{};

    // Sliders
    static lv_obj_t *bright_fill = nullptr;
    static lv_obj_t *bright_thumb = nullptr;
    static lv_obj_t *vol_fill = nullptr;
    static lv_obj_t *vol_thumb = nullptr;
    static lv_obj_t *vol_track = nullptr;
    static lv_obj_t *vol_track_icon = nullptr;

    // Toggles
    static lv_obj_t *tog_sleep = nullptr;

    // WiFi / connect button
    static lv_obj_t *wifi_btn = nullptr;
    static lv_obj_t *wifi_btn_name = nullptr;


    // Portal overlay & QR
    static lv_obj_t *portal_overlay = nullptr;
    static lv_obj_t *qr_code_obj = nullptr;
    static char lastQrData[128] = "";

    // State
    static int currentVolumeLevel = 80; // 0-100
    static int currentBrightness = 70;  // 0-100
    static AdvancedCallback advancedCallback = nullptr;

    struct SliderDragState
    {
        bool active;
        bool locked;
        bool vertical;
        lv_point_t start;
    };
    static SliderDragState s_sliderDrag[2] = {};

    // ═══════════════════════════════════════════════════════════════
    // LAYOUT CONSTANTS (from HTML mockup, scaled to 480×320)
    // ═══════════════════════════════════════════════════════════════
    static constexpr int16_t BODY_PAD_X = 14;
    static constexpr int16_t ROW_H = LV_SIZE_CONTENT;
    static constexpr int16_t ROW_PAD_X = 12;
    static constexpr int16_t ROW_PAD_Y = 9;
    static constexpr int16_t ROW_RADIUS = 8;
    static constexpr int16_t SETTINGS_COL_GAP = 10;
    static constexpr int16_t RIGHT_COL_W = 164;
    static constexpr int16_t LEFT_COL_W = SCREEN_W - (BODY_PAD_X * 2) - RIGHT_COL_W - SETTINGS_COL_GAP;
    static constexpr int16_t SLIDER_CARD_GAP = 8;
    static constexpr int16_t SLIDER_CARD_W = (LEFT_COL_W - SLIDER_CARD_GAP) / 2;
    static constexpr int16_t SLIDER_TRACK_W = 58;
    static constexpr int16_t SLIDER_TRACK_H = 166;
    static constexpr int16_t THUMB_SIZE = 0;
    static constexpr uint8_t MIN_VISIBLE_BACKLIGHT = 20;
    static constexpr int16_t TOG_W = 51;
    static constexpr int16_t TOG_H = 31;
    static constexpr int16_t TOG_KNOB = 27;
    static constexpr int16_t TOG_INSET = 2;
    static constexpr lv_opa_t SECTION_OPA = 199;
    static constexpr lv_opa_t ROW_BG_OPA = 26;
    static constexpr lv_opa_t ROW_BORDER_OPA = 56;
    static constexpr const char *TXT_DEVICE_SETTINGS = "Cihaz Ayarları";
    static constexpr const char *TXT_LOADING = "Hazırlanıyor...";
    static constexpr const char *TXT_RETRY = "Tekrar dene";
    static constexpr const char *TXT_PORTAL_ON = "Portal Açık";

    // ═══════════════════════════════════════════════════════════════
    // FORWARD DECLARATIONS
    // ═══════════════════════════════════════════════════════════════
    static void onSliderTouch(lv_event_t *e);
    static void onSliderRelease(lv_event_t *e);
    static void onToggle(lv_event_t *e);
    static void onWiFiBtn(lv_event_t *e);
    static void onPortalOverlayClick(lv_event_t *e);
    static void showPortalOverlay();
    static void hidePortalOverlay();
    static void showConnectedQRPage(const char *ip);
    static void setWiFiButtonEnabled(bool enabled);
    static void setVolumeSliderEnabled(bool enabled);
    static void clearPortalScreenIfPresent(bool returnToSettingsIfActive);

    // ═══════════════════════════════════════════════════════════════
    // SLIDER UPDATE HELPERS
    // ═══════════════════════════════════════════════════════════════
    static void updateSliderVisual(lv_obj_t *fill, lv_obj_t *thumb, int pct)
    {
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;

        const int16_t fill_h = (pct * SLIDER_TRACK_H) / 100;
        const int16_t clamped_fill_h = fill_h > 0 ? fill_h : 1;
        lv_obj_set_size(fill, SLIDER_TRACK_W, clamped_fill_h);
        lv_obj_set_pos(fill, 0, SLIDER_TRACK_H - clamped_fill_h);

        if (thumb)
        {
            int16_t thumb_y = SLIDER_TRACK_H - clamped_fill_h - (THUMB_SIZE / 2);
            const int16_t min_y = 0;
            const int16_t max_y = SLIDER_TRACK_H - THUMB_SIZE;
            if (thumb_y < min_y)
                thumb_y = min_y;
            if (thumb_y > max_y)
                thumb_y = max_y;
            lv_obj_set_pos(thumb, (SLIDER_TRACK_W - THUMB_SIZE) / 2, thumb_y);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // TOGGLE HELPERS
    // ═══════════════════════════════════════════════════════════════
    static constexpr uint16_t TOG_ANIM_MS = 300;

    static void _togKnobAnimCb(void *obj, int32_t v)
    {
        lv_obj_set_x(static_cast<lv_obj_t *>(obj), static_cast<lv_coord_t>(v));
    }

    static void setToggleVisual(lv_obj_t *tog, bool on, bool animate = false)
    {
        if (!tog)
            return;
        lv_obj_t *knob = lv_obj_get_child(tog, 0);

        const int16_t target_x = on ? (TOG_W - TOG_KNOB - TOG_INSET) : TOG_INSET;

        // Colors & opacity snap instantly (no semi-transparent mid-frames)
        lv_obj_set_style_bg_color(tog, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(tog, on ? LV_OPA_COVER : 26, 0);


        if (animate && knob)
        {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, knob);
            lv_anim_set_values(&a, lv_obj_get_x(knob), target_x);
            lv_anim_set_time(&a, TOG_ANIM_MS);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
            lv_anim_set_exec_cb(&a, _togKnobAnimCb);
            lv_anim_start(&a);
        }
        else if (knob)
        {
            lv_obj_set_x(knob, target_x);
        }
    }

    static bool getToggleState(lv_obj_t *tog)
    {
        if (!tog)
            return false;
        lv_obj_t *knob = lv_obj_get_child(tog, 0);
        if (!knob)
            return false;

        const int16_t off_x = TOG_INSET;
        const int16_t on_x = TOG_W - TOG_KNOB - TOG_INSET;
        const int16_t mid_x = (off_x + on_x) / 2;
        return lv_obj_get_x(knob) >= mid_x;
    }

    // ═══════════════════════════════════════════════════════════════
    // WIDGET BUILDERS
    // ═══════════════════════════════════════════════════════════════

    // Section header: "EKRAN", "SES", etc.
    static lv_obj_t *createSectionLabel(lv_obj_t *parent, const char *text)
    {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, LocaleTR::toUpperTR(text));
        lv_obj_set_style_text_font(lbl, FONT_HEADING_12, 0);
        lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
        lv_obj_set_style_text_opa(lbl, 245, 0);
        lv_obj_set_style_text_letter_space(lbl, 2, 0);
        lv_obj_set_style_pad_left(lbl, ROW_PAD_X, 0);
        return lbl;
    }

    // A row card container (horizontal, space-between)
    static lv_obj_t *createRow(lv_obj_t *parent)
    {
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, lv_pct(100), ROW_H);
        lv_obj_set_style_bg_color(row, COLOR_BG2, 0);
        lv_obj_set_style_bg_opa(row, ROW_BG_OPA, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, COLOR_BORDER, 0);
        lv_obj_set_style_border_opa(row, ROW_BORDER_OPA, 0);
        lv_obj_set_style_radius(row, ROW_RADIUS, 0);
        lv_obj_set_style_pad_left(row, ROW_PAD_X, 0);
        lv_obj_set_style_pad_right(row, ROW_PAD_X, 0);
        lv_obj_set_style_pad_top(row, ROW_PAD_Y, 0);
        lv_obj_set_style_pad_bottom(row, ROW_PAD_Y, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 8, 0);
        return row;
    }

    // Left side of a row: name + description
    static void createRowLeft(lv_obj_t *row, const char *name, const char *desc)
    {
        lv_obj_t *left = lv_obj_create(row);
        lv_obj_remove_style_all(left);
        lv_obj_set_size(left, 0, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(left, 1);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(left, 0, 0);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *nm = lv_label_create(left);
        lv_label_set_text(nm, LocaleTR::toUpperTR(name));
        lv_obj_set_width(nm, lv_pct(100));
        lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(nm, FONT_BODY_12, 0);
        lv_obj_set_style_text_color(nm, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(nm, LV_OPA_COVER, 0);
        lv_obj_set_style_text_letter_space(nm, 1, 0);

        if (desc && desc[0] != '\0')
        {
            lv_obj_t *ds = lv_label_create(left);
            lv_label_set_text(ds, LocaleTR::toUpperTR(desc));
            lv_obj_set_width(ds, lv_pct(100));
            lv_label_set_long_mode(ds, LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_font(ds, FONT_HEADING_10, 0);
            lv_obj_set_style_text_color(ds, COLOR_TEXT, 0);
            lv_obj_set_style_text_opa(ds, 210, 0);
        }
    }

    // Slider control: track + fill + thumb
    struct SliderWidgets
    {
        lv_obj_t *track;
        lv_obj_t *fill;
        lv_obj_t *thumb;
        lv_obj_t *icon;
    };

    static SliderWidgets createSliderCtrl(lv_obj_t *parent, const char *title, const char *icon, int initialPct, int tag)
    {
        lv_obj_t *card = lv_obj_create(parent);
        lv_obj_remove_style_all(card);
        lv_obj_set_size(card, SLIDER_CARD_W, lv_pct(100));
        lv_obj_set_style_bg_color(card, COLOR_BG2, 0);
        lv_obj_set_style_bg_opa(card, 8, 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, COLOR_BORDER, 0);
        lv_obj_set_style_border_opa(card, 56, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_top(card, 12, 0);
        lv_obj_set_style_pad_bottom(card, 10, 0);
        lv_obj_set_style_pad_left(card, 8, 0);
        lv_obj_set_style_pad_right(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(card, 7, 0);

        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_text(lbl, LocaleTR::toUpperTR(title));
        lv_obj_set_style_text_font(lbl, FONT_HEADING_12, 0);
        lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
        lv_obj_set_style_text_opa(lbl, 245, 0);
        lv_obj_set_style_text_letter_space(lbl, 2, 0);

        lv_obj_t *track = lv_obj_create(card);
        lv_obj_remove_style_all(track);
        lv_obj_set_size(track, SLIDER_TRACK_W, SLIDER_TRACK_H);
        lv_obj_set_style_bg_color(track, COLOR_BORDER, 0);
        lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(track, 12, 0);
        lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(track, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_clear_flag(track, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_set_ext_click_area(track, 24);
        lv_obj_add_event_cb(track, onSliderTouch, LV_EVENT_PRESSING,
                            reinterpret_cast<void *>(static_cast<intptr_t>(tag)));
        lv_obj_add_event_cb(track, onSliderRelease, LV_EVENT_RELEASED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(tag)));

        lv_obj_t *fill = lv_obj_create(track);
        lv_obj_remove_style_all(fill);
        lv_obj_set_size(fill, SLIDER_TRACK_W, 1);
        lv_obj_set_pos(fill, 0, SLIDER_TRACK_H - 1);
        lv_obj_set_style_bg_color(fill, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(fill, 12, 0);
        lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *track_icon = lv_obj_create(track);
        lv_obj_remove_style_all(track_icon);
        lv_obj_set_size(track_icon, ICON_SIZE_CONTROL, ICON_SIZE_CONTROL);
        lv_obj_set_style_bg_opa(track_icon, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(track_icon, 0, 0);
        lv_obj_clear_flag(track_icon, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_opa(track_icon, ICON_OPA_ACTIVE, 0);
        lv_obj_align(track_icon, LV_ALIGN_BOTTOM_MID, 0, -6);

        if (tag == 1)
        {
            UiIcons::drawSpeakerIcon(track_icon, false, COLOR_TEXT, true);
        }
        else
        {
            UiIcons::drawBrightnessIcon(track_icon, COLOR_TEXT);
        }

        lv_obj_move_foreground(track_icon);

        SliderWidgets sw{track, fill, nullptr, track_icon};
        updateSliderVisual(fill, nullptr, initialPct);
        return sw;
    }

    static void setVolumeSliderEnabled(bool enabled)
    {
        if (!vol_track)
            return;

        if (enabled)
        {
            if (vol_fill)
                lv_obj_set_style_bg_opa(vol_fill, LV_OPA_COVER, 0);
            if (vol_track_icon)
                lv_obj_set_style_opa(vol_track_icon, ICON_OPA_DEFAULT, 0);
        }
        else
        {
            // Keep slider interactive; dragging while muted should auto-unmute.
            if (vol_fill)
                lv_obj_set_style_bg_opa(vol_fill, 120, 0);
            if (vol_track_icon)
                lv_obj_set_style_opa(vol_track_icon, ICON_OPA_DISABLED, 0);
        }
    }

    static void setWiFiButtonEnabled(bool enabled)
    {
        if (!wifi_btn)
            return;

        if (enabled)
        {
            lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_opa(wifi_btn, LV_OPA_COVER, 0);
        }
        else
        {
            lv_obj_clear_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_opa(wifi_btn, 180, 0);
        }
    }

    static void setWiFiButtonLabel(const char *text, lv_color_t color)
    {
        if (wifi_btn_name)
        {
            lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR(text));
            lv_obj_set_style_text_color(wifi_btn_name, color, 0);
        }
    }

    // Toggle switch
    static lv_obj_t *createToggle(lv_obj_t *row, bool initialOn, int tag)
    {
        lv_obj_t *tog = lv_obj_create(row);
        lv_obj_remove_style_all(tog);
        lv_obj_set_size(tog, TOG_W, TOG_H);
        lv_obj_set_style_radius(tog, TOG_H / 2, 0);
        lv_obj_set_style_border_width(tog, 0, 0);
        lv_obj_set_style_bg_opa(tog, LV_OPA_COVER, 0);
        lv_obj_clear_flag(tog, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(tog, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(tog, 18);
        lv_obj_add_event_cb(tog, onToggle, LV_EVENT_PRESSED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(tag)));

        lv_obj_t *knob = lv_obj_create(tog);
        lv_obj_remove_style_all(knob);
        lv_obj_set_size(knob, TOG_KNOB, TOG_KNOB);
        lv_obj_set_style_bg_color(knob, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(knob, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(knob, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_shadow_color(knob, lv_color_black(), 0);
        lv_obj_set_style_shadow_width(knob, 4, 0);
        lv_obj_set_style_shadow_opa(knob, 100, 0);
        lv_obj_set_y(knob, (TOG_H - TOG_KNOB) / 2);
        lv_obj_clear_flag(knob, LV_OBJ_FLAG_CLICKABLE);

        setToggleVisual(tog, initialOn);
        return tog;
    }

    // QR-nav button (primary action style, "Cihaz Ayarları")
    static void createQRButton(lv_obj_t *parent)
    {
        wifi_btn = lv_obj_create(parent);
        lv_obj_remove_style_all(wifi_btn);
        lv_obj_set_size(wifi_btn, lv_pct(100), ROW_H);

        // Raised-button: gradient bg (lighter top → darker bottom = convex surface)
        lv_obj_set_style_bg_color(wifi_btn, COLOR_BTN_HI, 0);
        lv_obj_set_style_bg_opa(wifi_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_grad_color(wifi_btn, COLOR_BTN_LO, 0);
        lv_obj_set_style_bg_grad_dir(wifi_btn, LV_GRAD_DIR_VER, 0);

        lv_obj_set_style_border_width(wifi_btn, 0, 0);
        lv_obj_set_style_radius(wifi_btn, ROW_RADIUS, 0);

        // Drop shadow below → elevated / floating feel
        lv_obj_set_style_shadow_color(wifi_btn, lv_color_black(), 0);
        lv_obj_set_style_shadow_width(wifi_btn, 12, 0);
        lv_obj_set_style_shadow_opa(wifi_btn, 90, 0);
        lv_obj_set_style_shadow_ofs_y(wifi_btn, 4, 0);

        // Pressed: shadow shrinks + shifts down = "pushed in" feel
        lv_obj_set_style_bg_color(wifi_btn, COLOR_BTN_LO, LV_STATE_PRESSED);
        lv_obj_set_style_bg_grad_color(wifi_btn, COLOR_BTN_LO, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(wifi_btn, 4, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(wifi_btn, 30, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_ofs_y(wifi_btn, 1, LV_STATE_PRESSED);
        lv_obj_set_style_translate_y(wifi_btn, 2, LV_STATE_PRESSED);
        lv_obj_set_style_pad_left(wifi_btn, ROW_PAD_X, 0);
        lv_obj_set_style_pad_right(wifi_btn, ROW_PAD_X, 0);
        lv_obj_set_style_pad_top(wifi_btn, 10, 0);
        lv_obj_set_style_pad_bottom(wifi_btn, 10, 0);
        lv_obj_set_size(wifi_btn, lv_pct(100), 52);
        lv_obj_clear_flag(wifi_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(wifi_btn, 16);
        lv_obj_add_event_cb(wifi_btn, onWiFiBtn, LV_EVENT_CLICKED, nullptr);
        lv_obj_set_flex_flow(wifi_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(wifi_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(wifi_btn, 8, 0);

        lv_obj_t *icon = lv_label_create(wifi_btn);
        lv_label_set_text(icon, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(icon, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(icon, 200, 0);
        lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);

        wifi_btn_name = lv_label_create(wifi_btn);
        lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR(TXT_DEVICE_SETTINGS));
        lv_obj_set_style_text_font(wifi_btn_name, FONT_BODY_12, 0);
        lv_obj_set_style_text_color(wifi_btn_name, COLOR_GOLD_LIGHT, 0);
        lv_obj_set_style_text_opa(wifi_btn_name, 245, 0);
        lv_obj_clear_flag(wifi_btn_name, LV_OBJ_FLAG_CLICKABLE);
    }

    // ═══════════════════════════════════════════════════════════════
    // EVENT HANDLERS
    // ═══════════════════════════════════════════════════════════════

    // Tag: 0 = brightness, 1 = volume
    static void onSliderTouch(lv_event_t *e)
    {
        int tag = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
        if (tag < 0 || tag > 1)
            return;

        lv_obj_t *track = lv_event_get_target(e);
        lv_point_t point;
        lv_indev_get_point(lv_indev_get_act(), &point);

        SliderDragState &drag = s_sliderDrag[tag];
        if (!drag.active)
        {
            drag.active = true;
            drag.locked = false;
            drag.vertical = false;
            drag.start = point;
        }

        const int16_t dirLockThresholdPx = 5;
        if (!drag.locked)
        {
            int16_t dx = point.x - drag.start.x;
            int16_t dy = point.y - drag.start.y;
            if (dx < 0)
                dx = -dx;
            if (dy < 0)
                dy = -dy;

            if (dx >= dirLockThresholdPx || dy >= dirLockThresholdPx)
            {
                drag.locked = true;
                drag.vertical = (dy >= dx);
            }
        }

        if (!drag.locked || !drag.vertical)
            return;

        // Calculate absolute X of track using LVGL coords
        lv_area_t track_coords;
        lv_obj_get_coords(track, &track_coords);

        int16_t track_h = lv_obj_get_height(track);
        int16_t local_y = point.y - track_coords.y1;
        int pct = 100 - ((local_y * 100) / track_h);
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;

        if (tag == 0)
        {
            currentBrightness = pct;
            updateSliderVisual(bright_fill, bright_thumb, pct);
            const uint8_t mappedBacklight = static_cast<uint8_t>(MIN_VISIBLE_BACKLIGHT + ((pct * (255 - MIN_VISIBLE_BACKLIGHT)) / 100));
            LvglDisplay::setBacklight(mappedBacklight);
            g_state.brightness = static_cast<uint8_t>(pct);
        }
        else
        {
            if (g_state.muted)
                return;

            currentVolumeLevel = pct;
            updateSliderVisual(vol_fill, vol_thumb, currentVolumeLevel);
            // Apply volume to codec/state immediately (no NVS write during drag)
            VolumeControl::applyRuntime(static_cast<uint8_t>(currentVolumeLevel));
        }
    }

    // Called on RELEASED — persist to NVS (expensive flash write)
    static void onSliderRelease(lv_event_t *e)
    {
        int tag = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));

        if (tag >= 0 && tag <= 1)
            s_sliderDrag[tag] = {};

        if (tag == 1)
        {
            if (g_state.muted)
                return;
            VolumeControl::persist(static_cast<uint8_t>(currentVolumeLevel));
        }
    }

    // Tag: 0 = adhan, 1 = sleep, 2 = ramadan
    static void onToggle(lv_event_t *e)
    {
        int tag = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
        lv_obj_t *tog = lv_event_get_target(e);
        bool newState = !getToggleState(tog);
        setToggleVisual(tog, newState, true);

        switch (tag)
        {
        case 1:
            // ON means "Ekran Hep Açık" (Apple-style positive toggle semantics).
            SettingsManager::setPowerMode(newState ? PowerMode::ALWAYS_ON : PowerMode::SCREEN_OFF);
            g_state.sleepMode = !newState;
            break;
        }
    }

    static void onWiFiBtn(lv_event_t *e)
    {
        // If portal is already running, just navigate to portal page
        if (portal_overlay)
        {
            LvglDisplay::goToPortalPage();
            return;
        }

        setWiFiButtonEnabled(false);
        setWiFiButtonLabel(TXT_LOADING, COLOR_DIM);
        lv_refr_now(NULL);
        if (advancedCallback)
            advancedCallback();
    }

    // ═══════════════════════════════════════════════════════════════
    // PORTAL OVERLAY (AP mode — full-screen WiFi info)
    // ═══════════════════════════════════════════════════════════════

    static void onPortalOverlayClick(lv_event_t *e) { /* no-op, portal is a page now */ }

    static void showPortalOverlay()
    {
        if (!WiFiPortal::isActive())
            return;

        if (!portal_overlay)
        {
            // Create portal as a standalone screen (swipeable page 3)
            portal_overlay = lv_obj_create(nullptr);
            lv_obj_remove_style_all(portal_overlay);
            lv_obj_add_style(portal_overlay, UiTheme::getStyleScreen(), 0);
            lv_obj_clear_flag(portal_overlay, LV_OBJ_FLAG_SCROLLABLE);

            lv_obj_t *title = lv_label_create(portal_overlay);
            lv_label_set_text(title, LocaleTR::toUpperTR("WiFi'ye Bağlanın"));
            lv_obj_set_style_text_color(title, COLOR_GOLD_LIGHT, 0);
            lv_obj_set_style_text_font(title, FONT_BODY_12, 0);
            lv_obj_set_style_text_letter_space(title, 2, 0);
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

            lv_obj_t *wifi_icon = lv_label_create(portal_overlay);
            lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(wifi_icon, COLOR_GOLD, 0);
            lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_20, 0);
            lv_obj_align(wifi_icon, LV_ALIGN_TOP_MID, 0, 75);

            lv_obj_t *ssid_title = lv_label_create(portal_overlay);
            lv_label_set_text(ssid_title, LocaleTR::toUpperTR("Ağ Adı"));
            lv_obj_set_style_text_color(ssid_title, COLOR_DIM, 0);
            lv_obj_set_style_text_opa(ssid_title, 165, 0);
            lv_obj_set_style_text_font(ssid_title, FONT_HEADING_10, 0);
            lv_obj_align(ssid_title, LV_ALIGN_CENTER, 0, -30);

            lv_obj_t *ssid_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(ssid_lbl, WiFiPortal::AP_SSID);
            lv_obj_set_style_text_color(ssid_lbl, COLOR_TEXT, 0);
            lv_obj_set_style_text_font(ssid_lbl, FONT_HIJRI_18, 0);
            lv_obj_align(ssid_lbl, LV_ALIGN_CENTER, 0, -6);

            lv_obj_t *pass_title = lv_label_create(portal_overlay);
            lv_label_set_text(pass_title, LocaleTR::toUpperTR("Şifre"));
            lv_obj_set_style_text_color(pass_title, COLOR_DIM, 0);
            lv_obj_set_style_text_opa(pass_title, 165, 0);
            lv_obj_set_style_text_font(pass_title, FONT_HEADING_10, 0);
            lv_obj_align(pass_title, LV_ALIGN_CENTER, 0, 26);

            lv_obj_t *pass_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(pass_lbl, WiFiPortal::AP_PASSWORD);
            lv_obj_set_style_text_color(pass_lbl, COLOR_GOLD, 0);
            lv_obj_set_style_text_font(pass_lbl, FONT_HIJRI_18, 0);
            lv_obj_align(pass_lbl, LV_ALIGN_CENTER, 0, 50);

            UiComponents::createNavDots(portal_overlay, 2);
        }

        // Navigate to portal screen
        LvglDisplay::goToPortalPage();
    }

    static void clearPortalScreenIfPresent(bool returnToSettingsIfActive)
    {
        if (!portal_overlay)
            return;

        const bool wasActive = (lv_scr_act() == portal_overlay);
        lv_obj_del(portal_overlay);
        portal_overlay = nullptr;
        qr_code_obj = nullptr;
        lastQrData[0] = '\0';

        if (returnToSettingsIfActive && wasActive && scr)
        {
            lv_scr_load(scr);
            lv_refr_now(NULL);
        }
    }

    static void hidePortalOverlay()
    {
        // Portal screen stays alive, just go back to settings if needed
    }

    static void showConnectedQRPage(const char *ip)
    {
        // Delete old portal overlay if it exists (portal→connected transition)
        if (portal_overlay)
        {
            lv_obj_del(portal_overlay);
            portal_overlay = nullptr;
            qr_code_obj = nullptr;
            lastQrData[0] = '\0';
        }

        // Build QR page as a standalone screen (page 3)
        portal_overlay = lv_obj_create(nullptr);
        lv_obj_remove_style_all(portal_overlay);
        lv_obj_add_style(portal_overlay, UiTheme::getStyleScreen(), 0);
        lv_obj_clear_flag(portal_overlay, LV_OBJ_FLAG_SCROLLABLE);

        // Title
        lv_obj_t *title = lv_label_create(portal_overlay);
        lv_label_set_text(title, LocaleTR::toUpperTR(TXT_DEVICE_SETTINGS));
        lv_obj_set_style_text_color(title, COLOR_GOLD_LIGHT, 0);
        lv_obj_set_style_text_font(title, FONT_BODY_12, 0);
        lv_obj_set_style_text_letter_space(title, 2, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 26);

        // QR code: large, centered, with soft rounded frame
        lv_obj_t *qr_frame = lv_obj_create(portal_overlay);
        lv_obj_remove_style_all(qr_frame);
        lv_obj_set_size(qr_frame, 210, 210);
        lv_obj_align(qr_frame, LV_ALIGN_CENTER, 0, 4);
        lv_obj_set_style_bg_color(qr_frame, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(qr_frame, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(qr_frame, 14, 0);
        lv_obj_set_style_shadow_color(qr_frame, lv_color_black(), 0);
        lv_obj_set_style_shadow_width(qr_frame, 20, 0);
        lv_obj_set_style_shadow_opa(qr_frame, 60, 0);
        lv_obj_set_style_shadow_ofs_y(qr_frame, 4, 0);
        lv_obj_clear_flag(qr_frame, LV_OBJ_FLAG_SCROLLABLE);

        qr_code_obj = lv_qrcode_create(qr_frame, 190, lv_color_black(), lv_color_white());
        lv_obj_center(qr_code_obj);

        char url[64];
        snprintf(url, sizeof(url), "http://%s", ip ? ip : "192.168.4.1");
        lv_qrcode_update(qr_code_obj, url, strlen(url));
        strncpy(lastQrData, url, sizeof(lastQrData) - 1);
        lastQrData[sizeof(lastQrData) - 1] = '\0';

        UiComponents::createNavDots(portal_overlay, 2);

        // Navigate to QR page
        LvglDisplay::goToPortalPage();
    }

    // ═══════════════════════════════════════════════════════════════
    // CREATE
    // ═══════════════════════════════════════════════════════════════

    lv_obj_t *create()
    {
        UiTheme::initStyles();

        if (scr)
        {
            lv_obj_del(scr);
            scr = nullptr;
            bright_fill = bright_thumb = nullptr;
            vol_fill = vol_thumb = nullptr;
            tog_sleep = nullptr;
            vol_track = vol_track_icon = nullptr;
            wifi_btn_name = nullptr;
            portal_overlay = nullptr;
            qr_code_obj = nullptr;
            lastQrData[0] = '\0';
        }

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_add_style(scr, getStyleScreen(), 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        // Status bar
        sb_handles = UiComponents::createStatusBar(scr, StatusBarIcon::SETTINGS);
        UiComponents::updateStatusBarCity(sb_handles, "Ayarlar", "");
        UiComponents::createSeparator(scr);

        // Main body
        lv_obj_t *body = lv_obj_create(scr);
        lv_obj_remove_style_all(body);
        lv_obj_set_size(body, SCREEN_W, SCREEN_H - 48);
        lv_obj_set_pos(body, 0, 48);
        lv_obj_set_style_pad_left(body, BODY_PAD_X, 0);
        lv_obj_set_style_pad_right(body, BODY_PAD_X, 0);
        lv_obj_set_style_pad_top(body, 10, 0);
        lv_obj_set_style_pad_bottom(body, 22, 0);
        lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *layout = lv_obj_create(body);
        lv_obj_remove_style_all(layout);
        lv_obj_set_size(layout, lv_pct(100), lv_pct(100));
        lv_obj_clear_flag(layout, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(layout, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(layout, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(layout, SETTINGS_COL_GAP, 0);

        // Left: dual vertical sliders (brightness + volume)
        lv_obj_t *left_col = lv_obj_create(layout);
        lv_obj_remove_style_all(left_col);
        lv_obj_set_size(left_col, LEFT_COL_W, lv_pct(100));
        lv_obj_clear_flag(left_col, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(left_col, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(left_col, SLIDER_CARD_GAP, 0);

        currentBrightness = g_state.brightness;
        auto bright_sw = createSliderCtrl(left_col, "Parlaklık", LV_SYMBOL_EYE_OPEN, currentBrightness, 0);
        bright_fill = bright_sw.fill;
        bright_thumb = bright_sw.thumb;

        currentVolumeLevel = g_state.volume;
        auto vol_sw = createSliderCtrl(left_col, "Ses", LV_SYMBOL_AUDIO, currentVolumeLevel, 1);
        vol_track = vol_sw.track;
        vol_fill = vol_sw.fill;
        vol_thumb = vol_sw.thumb;
        vol_track_icon = vol_sw.icon;
        setVolumeSliderEnabled(!g_state.muted);

        // Right: toggles + WiFi action
        lv_obj_t *right_col = lv_obj_create(layout);
        lv_obj_remove_style_all(right_col);
        lv_obj_set_size(right_col, RIGHT_COL_W, lv_pct(100));
        lv_obj_clear_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(right_col, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(right_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_row(right_col, 6, 0);
        lv_obj_set_style_pad_bottom(right_col, 4, 0);

        {
            createSectionLabel(right_col, "Ekran");

            lv_obj_t *row = createRow(right_col);
            lv_obj_set_style_min_height(row, 48, 0);
            createRowLeft(row, "Sürekli Açık", nullptr);
            tog_sleep = createToggle(row, SettingsManager::getPowerMode() == PowerMode::ALWAYS_ON, 1);
        }

        lv_obj_t *spacer = lv_obj_create(right_col);
        lv_obj_remove_style_all(spacer);
        lv_obj_set_size(spacer, lv_pct(100), 1);
        lv_obj_set_flex_grow(spacer, 1);
        lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

        createQRButton(right_col);

        UiComponents::createNavDots(scr, 2);

        // Set initial WiFi state
        if (Network::isConnected())
            setWiFiButtonState(WifiState::CONNECTED, WiFi.localIP().toString().c_str());
        else
            setWiFiButtonState(WifiState::DISCONNECTED, nullptr);

        return scr;
    }

    // ═══════════════════════════════════════════════════════════════
    // PUBLIC API
    // ═══════════════════════════════════════════════════════════════

    lv_obj_t *getScreen() { return scr; }

    lv_obj_t *getPortalScreen() { return portal_overlay; }

    void setAdvancedCallback(AdvancedCallback cb) { advancedCallback = cb; }

    void setVolumeLevel(int pct)
    {
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;
        currentVolumeLevel = pct;
        updateSliderVisual(vol_fill, vol_thumb, currentVolumeLevel);
    }

    int getVolumeLevel() { return currentVolumeLevel; }

    void setBrightnessLevel(int pct)
    {
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;
        currentBrightness = pct;
        updateSliderVisual(bright_fill, bright_thumb, pct);
    }

    void syncToggles()
    {
        setVolumeSliderEnabled(!g_state.muted);
        if (tog_sleep)
            setToggleVisual(tog_sleep, SettingsManager::getPowerMode() == PowerMode::ALWAYS_ON);
    }

    void updatePowerModeUI()
    {
        if (tog_sleep)
            setToggleVisual(tog_sleep, SettingsManager::getPowerMode() == PowerMode::ALWAYS_ON);
    }

    void setWiFiButtonState(WifiState state, const char *ip)
    {
        if (!scr)
            return;

        setWiFiButtonEnabled(state != WifiState::CONNECTING);

        if (state == WifiState::PORTAL && portal_overlay && qr_code_obj)
        {
            // Replace connected QR page with portal info page when entering AP mode.
            clearPortalScreenIfPresent(false);
        }

        if (state != WifiState::CONNECTED && state != WifiState::PORTAL)
        {
            // Leaving connected/portal state: remove extra page so swipe count returns to normal.
            clearPortalScreenIfPresent(true);
        }

        // Portal overlay: show only in PORTAL state
        if (state == WifiState::PORTAL)
            showPortalOverlay();

        switch (state)
        {
        case WifiState::DISCONNECTED:
            setWiFiButtonLabel(TXT_DEVICE_SETTINGS, COLOR_GOLD_LIGHT);
            break;

        case WifiState::CONNECTING:
            setWiFiButtonLabel(TXT_LOADING, COLOR_DIM);
            break;

        case WifiState::CONNECTED:
            showConnectedQRPage(ip);
            setWiFiButtonLabel(TXT_DEVICE_SETTINGS, COLOR_GOLD_LIGHT);
            break;

        case WifiState::FAILED:
            setWiFiButtonLabel(TXT_RETRY, COLOR_AMBER);
            break;

        case WifiState::PORTAL:
            setWiFiButtonLabel(TXT_PORTAL_ON, COLOR_GOLD_LIGHT);
            break;
        }
    }

    bool isSliderHit(int16_t x, int16_t y)
    {
        auto check = [](lv_obj_t *fill, int16_t px, int16_t py) -> bool
        {
            if (!fill)
                return false;
            lv_obj_t *track = lv_obj_get_parent(fill);
            if (!track)
                return false;
            lv_area_t a;
            lv_obj_get_coords(track, &a);
            const int16_t M = 20;
            return px >= (a.x1 - M) && px <= (a.x2 + M) && py >= (a.y1 - M) && py <= (a.y2 + M);
        };
        return check(bright_fill, x, y) || check(vol_fill, x, y);
    }

    void setStatusBarCity(const char *city, const char *dateAbbrev)
    {
        UiComponents::updateStatusBarCity(sb_handles, city, dateAbbrev);
    }

    void setMuted(bool muted)
    {
        UiComponents::updateStatusBarMute(sb_handles, muted);
        setVolumeSliderEnabled(!muted);
    }

} // namespace UiPageSettings
