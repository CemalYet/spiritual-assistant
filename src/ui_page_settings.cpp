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
#include "locale_tr.h"
#include "settings_manager.h"
#include "audio_player.h"
#include "app_state.h"
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
    static lv_obj_t *bright_val_lbl = nullptr;
    static lv_obj_t *vol_fill = nullptr;
    static lv_obj_t *vol_thumb = nullptr;
    static lv_obj_t *vol_val_lbl = nullptr;

    // Toggles
    static lv_obj_t *tog_adhan = nullptr;
    static lv_obj_t *tog_sleep = nullptr;

    // WiFi / connect button
    static lv_obj_t *wifi_btn = nullptr;
    static lv_obj_t *wifi_btn_name = nullptr;
    static lv_obj_t *wifi_btn_desc = nullptr;

    // Portal overlay & QR
    static lv_obj_t *portal_overlay = nullptr;
    static lv_obj_t *qr_code_obj = nullptr;
    static lv_obj_t *qr_ip_lbl = nullptr;
    static char lastQrData[128] = "";

    // State
    static int currentVolumeLevel = 80; // 0-100
    static int currentBrightness = 70;  // 0-100
    static AdvancedCallback advancedCallback = nullptr;

    // ═══════════════════════════════════════════════════════════════
    // LAYOUT CONSTANTS (from HTML mockup, scaled to 480×320)
    // ═══════════════════════════════════════════════════════════════
    static constexpr int16_t BODY_PAD_X = 12;
    static constexpr int16_t BODY_TOP = 28;
    static constexpr int16_t ROW_H = LV_SIZE_CONTENT;
    static constexpr int16_t ROW_PAD_X = 12;
    static constexpr int16_t ROW_PAD_Y = 5;
    static constexpr int16_t ROW_RADIUS = 7;
    static constexpr int16_t SECTION_GAP = 2;
    static constexpr int16_t SLIDER_W = 180;
    static constexpr int16_t SLIDER_TRACK_H = 14;
    static constexpr int16_t THUMB_SIZE = 26;
    static constexpr int16_t TOG_W = 52;
    static constexpr int16_t TOG_H = 28;
    static constexpr int16_t TOG_KNOB = 20;
    static constexpr lv_opa_t SECTION_OPA = 179;
    static constexpr lv_opa_t ROW_BG_OPA = 6;
    static constexpr lv_opa_t ROW_BORDER_OPA = 41;

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

    // ═══════════════════════════════════════════════════════════════
    // SLIDER UPDATE HELPERS
    // ═══════════════════════════════════════════════════════════════
    static void updateSliderVisual(lv_obj_t *fill, lv_obj_t *thumb, lv_obj_t *val_lbl, int pct)
    {
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;

        const int16_t track_w = SLIDER_W - 50; // 180 - 10(gap) - 40(val)
        const int16_t fill_w = (pct * track_w) / 100;
        lv_obj_set_width(fill, fill_w > 0 ? fill_w : 1);
        lv_obj_set_x(thumb, fill_w - THUMB_SIZE / 2);

        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", pct);
        lv_label_set_text(val_lbl, buf);
    }

    // ═══════════════════════════════════════════════════════════════
    // TOGGLE HELPERS
    // ═══════════════════════════════════════════════════════════════
    static void setToggleVisual(lv_obj_t *tog, bool on)
    {
        if (!tog)
            return;
        lv_obj_t *knob = lv_obj_get_child(tog, 0);
        if (on)
        {
            lv_obj_set_style_bg_color(tog, COLOR_GOLD, 0);
            lv_obj_set_style_bg_opa(tog, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(tog, COLOR_GOLD, 0);
            if (knob)
                lv_obj_set_x(knob, TOG_W - TOG_KNOB - 4);
        }
        else
        {
            lv_obj_set_style_bg_color(tog, COLOR_GOLD, 0);
            lv_obj_set_style_bg_opa(tog, 26, 0); // rgba(234,201,106,0.10)
            lv_obj_set_style_border_color(tog, COLOR_BORDER, 0);
            if (knob)
                lv_obj_set_x(knob, 4);
        }
    }

    static bool getToggleState(lv_obj_t *tog)
    {
        if (!tog)
            return false;
        lv_obj_t *knob = lv_obj_get_child(tog, 0);
        return knob && (lv_obj_get_x(knob) > TOG_W / 2);
    }

    // ═══════════════════════════════════════════════════════════════
    // WIDGET BUILDERS
    // ═══════════════════════════════════════════════════════════════

    // Section header: "EKRAN", "SES", etc.
    static lv_obj_t *createSectionLabel(lv_obj_t *parent, const char *text)
    {
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text(lbl, LocaleTR::toUpperTR(text));
        lv_obj_set_style_text_font(lbl, FONT_HEADING_10, 0);
        lv_obj_set_style_text_color(lbl, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(lbl, SECTION_OPA, 0);
        lv_obj_set_style_text_letter_space(lbl, 3, 0);
        lv_obj_set_style_pad_left(lbl, 2, 0);
        return lbl;
    }

    // A row card container (horizontal, space-between)
    static lv_obj_t *createRow(lv_obj_t *parent)
    {
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, lv_pct(100), ROW_H);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
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
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        return row;
    }

    // Left side of a row: name + description
    static void createRowLeft(lv_obj_t *row, const char *name, const char *desc)
    {
        lv_obj_t *left = lv_obj_create(row);
        lv_obj_remove_style_all(left);
        lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(left, 0, 0);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *nm = lv_label_create(left);
        lv_label_set_text(nm, LocaleTR::toUpperTR(name));
        lv_obj_set_style_text_font(nm, FONT_BODY_12, 0);
        lv_obj_set_style_text_color(nm, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(nm, 1, 0);

        if (desc && desc[0] != '\0')
        {
            lv_obj_t *ds = lv_label_create(left);
            lv_label_set_text(ds, LocaleTR::toUpperTR(desc));
            lv_obj_set_style_text_font(ds, FONT_HEADING_10, 0);
            lv_obj_set_style_text_color(ds, COLOR_DIM, 0);
            lv_obj_set_style_text_opa(ds, 165, 0);
        }
    }

    // Slider control: track + fill + thumb + value label
    struct SliderWidgets
    {
        lv_obj_t *fill;
        lv_obj_t *thumb;
        lv_obj_t *val_lbl;
    };

    static SliderWidgets createSliderCtrl(lv_obj_t *row, int initialPct, int tag)
    {
        lv_obj_t *ctrl = lv_obj_create(row);
        lv_obj_remove_style_all(ctrl);
        lv_obj_set_size(ctrl, SLIDER_W, LV_SIZE_CONTENT);
        lv_obj_clear_flag(ctrl, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(ctrl, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_set_style_min_height(ctrl, TOG_H, 0); // match toggle height for consistent rows
        lv_obj_set_flex_flow(ctrl, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ctrl, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(ctrl, 10, 0);

        const int16_t track_w = SLIDER_W - 50; // 180 - 10(gap) - 40(val)
        lv_obj_t *track = lv_obj_create(ctrl);
        lv_obj_remove_style_all(track);
        lv_obj_set_size(track, track_w, SLIDER_TRACK_H);
        lv_obj_set_style_bg_color(track, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_bg_opa(track, 38, 0);
        lv_obj_set_style_radius(track, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(track, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_OVERFLOW_VISIBLE);
        lv_obj_clear_flag(track, LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_set_ext_click_area(track, 12);
        lv_obj_add_event_cb(track, onSliderTouch, LV_EVENT_PRESSING,
                            reinterpret_cast<void *>(static_cast<intptr_t>(tag)));
        lv_obj_add_event_cb(track, onSliderTouch, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(tag)));
        lv_obj_add_event_cb(track, onSliderRelease, LV_EVENT_RELEASED,
                            reinterpret_cast<void *>(static_cast<intptr_t>(tag)));

        lv_obj_t *fill = lv_obj_create(track);
        lv_obj_remove_style_all(fill);
        lv_obj_set_height(fill, SLIDER_TRACK_H);
        lv_obj_set_pos(fill, 0, 0);
        lv_obj_set_style_bg_color(fill, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(fill, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *thumb = lv_obj_create(track);
        lv_obj_remove_style_all(thumb);
        lv_obj_set_size(thumb, THUMB_SIZE, THUMB_SIZE);
        lv_obj_set_style_bg_color(thumb, COLOR_GOLD_LIGHT, 0);
        lv_obj_set_style_bg_opa(thumb, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(thumb, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_shadow_color(thumb, COLOR_GOLD, 0);
        lv_obj_set_style_shadow_width(thumb, 10, 0);
        lv_obj_set_style_shadow_opa(thumb, LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_spread(thumb, 0, 0);
        lv_obj_set_y(thumb, -(THUMB_SIZE - SLIDER_TRACK_H) / 2);
        lv_obj_clear_flag(thumb, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *val = lv_label_create(ctrl);
        lv_obj_set_style_text_font(val, FONT_MONO_10, 0);
        lv_obj_set_style_text_color(val, COLOR_GOLD, 0);
        lv_obj_set_style_min_width(val, 40, 0);
        lv_obj_set_style_text_align(val, LV_TEXT_ALIGN_RIGHT, 0);

        SliderWidgets sw{fill, thumb, val};
        updateSliderVisual(fill, thumb, val, initialPct);
        return sw;
    }

    // Toggle switch
    static lv_obj_t *createToggle(lv_obj_t *row, bool initialOn, int tag)
    {
        lv_obj_t *tog = lv_obj_create(row);
        lv_obj_remove_style_all(tog);
        lv_obj_set_size(tog, TOG_W, TOG_H);
        lv_obj_set_style_radius(tog, TOG_H / 2, 0);
        lv_obj_set_style_border_width(tog, 1, 0);
        lv_obj_set_style_bg_opa(tog, LV_OPA_COVER, 0);
        lv_obj_clear_flag(tog, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(tog, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(tog, 14);
        lv_obj_add_event_cb(tog, onToggle, LV_EVENT_CLICKED,
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

    // QR-nav button (gold gradient bg, "Telefondan Yönet")
    static void createQRButton(lv_obj_t *parent)
    {
        wifi_btn = lv_obj_create(parent);
        lv_obj_remove_style_all(wifi_btn);
        lv_obj_set_size(wifi_btn, lv_pct(100), ROW_H);
        lv_obj_set_style_bg_color(wifi_btn, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(wifi_btn, 30, 0);
        lv_obj_set_style_border_width(wifi_btn, 1, 0);
        lv_obj_set_style_border_color(wifi_btn, COLOR_GOLD, 0);
        lv_obj_set_style_border_opa(wifi_btn, 82, 0);
        lv_obj_set_style_radius(wifi_btn, ROW_RADIUS, 0);
        lv_obj_set_style_pad_left(wifi_btn, ROW_PAD_X, 0);
        lv_obj_set_style_pad_right(wifi_btn, ROW_PAD_X, 0);
        lv_obj_set_style_pad_top(wifi_btn, ROW_PAD_Y, 0);
        lv_obj_set_style_pad_bottom(wifi_btn, ROW_PAD_Y, 0);
        lv_obj_clear_flag(wifi_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(wifi_btn, 8);
        lv_obj_add_event_cb(wifi_btn, onWiFiBtn, LV_EVENT_CLICKED, nullptr);
        lv_obj_set_flex_flow(wifi_btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(wifi_btn, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *left = lv_obj_create(wifi_btn);
        lv_obj_remove_style_all(left);
        lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(left, 0, 0);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

        wifi_btn_name = lv_label_create(left);
        lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR("Telefondan Yonet"));
        lv_obj_set_style_text_font(wifi_btn_name, FONT_BODY_12, 0);
        lv_obj_set_style_text_color(wifi_btn_name, COLOR_GOLD_LIGHT, 0);

        wifi_btn_desc = lv_label_create(left);
        lv_label_set_text(wifi_btn_desc, LocaleTR::toUpperTR("Ayarlar \xC2\xB7 Vakitler \xC2\xB7 Guncelleme"));
        lv_obj_set_style_text_font(wifi_btn_desc, FONT_HEADING_10, 0);
        lv_obj_set_style_text_color(wifi_btn_desc, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(wifi_btn_desc, 165, 0);

        lv_obj_t *icon = lv_label_create(wifi_btn);
        lv_label_set_text(icon, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_font(icon, FONT_HEADING_12, 0); // MUST use font with FA glyphs
        lv_obj_set_style_text_color(icon, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(icon, 140, 0);
    }

    // ═══════════════════════════════════════════════════════════════
    // EVENT HANDLERS
    // ═══════════════════════════════════════════════════════════════

    // Tag: 0 = brightness, 1 = volume
    static void onSliderTouch(lv_event_t *e)
    {
        int tag = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
        lv_obj_t *track = lv_event_get_target(e);

        lv_point_t point;
        lv_indev_get_point(lv_indev_get_act(), &point);

        // Calculate absolute X of track using LVGL coords
        lv_area_t track_coords;
        lv_obj_get_coords(track, &track_coords);

        int16_t track_w = lv_obj_get_width(track);
        int16_t local_x = point.x - track_coords.x1;
        int pct = (local_x * 100) / track_w;
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;

        if (tag == 0)
        {
            currentBrightness = pct;
            updateSliderVisual(bright_fill, bright_thumb, bright_val_lbl, pct);
            LvglDisplay::setBacklight(static_cast<uint8_t>((pct * 255) / 100));
            g_state.brightness = static_cast<uint8_t>(pct);
        }
        else
        {
            currentVolumeLevel = pct;
            updateSliderVisual(vol_fill, vol_thumb, vol_val_lbl, currentVolumeLevel);
            // Apply volume to codec immediately (no NVS write during drag)
            ::setVolume(static_cast<uint8_t>(currentVolumeLevel));
            AppStateHelper::setVolume(static_cast<uint8_t>(currentVolumeLevel));
        }
    }

    // Called on RELEASED — persist to NVS (expensive flash write)
    static void onSliderRelease(lv_event_t *e)
    {
        int tag = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
        if (tag == 1)
            SettingsManager::setVolume(static_cast<uint8_t>(currentVolumeLevel));
    }

    // Tag: 0 = adhan, 1 = sleep, 2 = ramadan
    static void onToggle(lv_event_t *e)
    {
        int tag = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
        lv_obj_t *tog = lv_event_get_target(e);
        bool newState = !getToggleState(tog);
        setToggleVisual(tog, newState);

        switch (tag)
        {
        case 0:
            // Toggle = mute/unmute. Per-prayer enable is web-page only.
            g_state.muted = !newState; // toggle ON → unmuted
            g_state.markDirty(DirtyFlag::MUTED);
            SettingsManager::setMuted(!newState);
            break;
        case 1:
            SettingsManager::setPowerMode(newState ? PowerMode::SCREEN_OFF : PowerMode::ALWAYS_ON);
            g_state.sleepMode = newState;
            break;
        }
    }

    static void onWiFiBtn(lv_event_t *e)
    {
        if (LvglDisplay::isGestureActive())
            return; // suppress click during swipe

        // If portal is already running, just navigate to portal page
        if (portal_overlay)
        {
            LvglDisplay::goToPortalPage();
            return;
        }

        if (wifi_btn_name)
            lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR("Baslatiliyor..."));
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

            UiComponents::applyMotif(portal_overlay);

            lv_obj_t *title = lv_label_create(portal_overlay);
            lv_label_set_text(title, LocaleTR::toUpperTR("WiFi Bilgileri"));
            lv_obj_set_style_text_color(title, COLOR_GOLD, 0);
            lv_obj_set_style_text_font(title, FONT_HEADING_12, 0);
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

            lv_obj_t *wifi_icon = lv_label_create(portal_overlay);
            lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(wifi_icon, COLOR_GOLD, 0);
            lv_obj_set_style_text_font(wifi_icon, FONT_PRAYER_24, 0); // font_cinzel_20_sb has FA glyphs
            lv_obj_align(wifi_icon, LV_ALIGN_TOP_MID, 0, 70);

            lv_obj_t *inst = lv_label_create(portal_overlay);
            lv_label_set_text(inst, LocaleTR::toUpperTR("Telefonunuzdan baglanin:"));
            lv_obj_set_style_text_color(inst, COLOR_DIM, 0);
            lv_obj_set_style_text_opa(inst, 165, 0);
            lv_obj_set_style_text_font(inst, FONT_BODY_12, 0);
            lv_obj_align(inst, LV_ALIGN_CENTER, 0, -30);

            lv_obj_t *ssid_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(ssid_lbl, WiFiPortal::AP_SSID);
            lv_obj_set_style_text_color(ssid_lbl, COLOR_TEXT, 0);
            lv_obj_set_style_text_font(ssid_lbl, FONT_HIJRI_18, 0);
            lv_obj_align(ssid_lbl, LV_ALIGN_CENTER, 0, 0);

            lv_obj_t *pass_title = lv_label_create(portal_overlay);
            lv_label_set_text(pass_title, LocaleTR::toUpperTR("Sifre:"));
            lv_obj_set_style_text_color(pass_title, COLOR_DIM, 0);
            lv_obj_set_style_text_opa(pass_title, 165, 0);
            lv_obj_set_style_text_font(pass_title, FONT_BODY_12, 0);
            lv_obj_align(pass_title, LV_ALIGN_CENTER, 0, 30);

            lv_obj_t *pass_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(pass_lbl, WiFiPortal::AP_PASSWORD);
            lv_obj_set_style_text_color(pass_lbl, COLOR_GOLD, 0);
            lv_obj_set_style_text_font(pass_lbl, FONT_HEADING_12, 0);
            lv_obj_align(pass_lbl, LV_ALIGN_CENTER, 0, 50);

            lv_obj_t *ip_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(ip_lbl, LocaleTR::toUpperTR("Sonra tarayicida: 192.168.4.1"));
            lv_obj_set_style_text_color(ip_lbl, COLOR_DIM, 0);
            lv_obj_set_style_text_opa(ip_lbl, 165, 0);
            lv_obj_set_style_text_font(ip_lbl, FONT_MONO_10, 0);
            lv_obj_align(ip_lbl, LV_ALIGN_CENTER, 0, 80);

            UiComponents::createNavDots(portal_overlay, 3, 4);
        }

        // Navigate to portal screen
        LvglDisplay::goToPortalPage();
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
            qr_ip_lbl = nullptr;
            lastQrData[0] = '\0';
        }

        // Build QR page as a standalone screen (page 3)
        portal_overlay = lv_obj_create(nullptr);
        lv_obj_remove_style_all(portal_overlay);
        lv_obj_add_style(portal_overlay, UiTheme::getStyleScreen(), 0);
        lv_obj_clear_flag(portal_overlay, LV_OBJ_FLAG_SCROLLABLE);
        UiComponents::applyMotif(portal_overlay);

        // Title
        lv_obj_t *title = lv_label_create(portal_overlay);
        lv_label_set_text(title, LocaleTR::toUpperTR("Telefondan Yonet"));
        lv_obj_set_style_text_color(title, COLOR_GREEN, 0);
        lv_obj_set_style_text_font(title, FONT_HEADING_12, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

        // Container: QR on left, text on right
        lv_obj_t *box = lv_obj_create(portal_overlay);
        lv_obj_remove_style_all(box);
        lv_obj_set_size(box, 400, 160);
        lv_obj_align(box, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_style_bg_color(box, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(box, 12, 0);
        lv_obj_set_style_border_width(box, 1, 0);
        lv_obj_set_style_border_color(box, COLOR_GREEN, 0);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        // QR code (120x120)
        qr_code_obj = lv_qrcode_create(box, 120, lv_color_black(), lv_color_white());
        lv_obj_align(qr_code_obj, LV_ALIGN_LEFT_MID, 20, 0);

        char url[64];
        snprintf(url, sizeof(url), "http://%s", ip ? ip : "192.168.4.1");
        lv_qrcode_update(qr_code_obj, url, strlen(url));
        strncpy(lastQrData, url, sizeof(lastQrData) - 1);
        lastQrData[sizeof(lastQrData) - 1] = '\0';

        // Right side text: "Telefonla Tara"
        lv_obj_t *scan_lbl = lv_label_create(box);
        lv_label_set_text(scan_lbl, LocaleTR::toUpperTR("Telefonla Tara"));
        lv_obj_set_style_text_color(scan_lbl, COLOR_GREEN, 0);
        lv_obj_set_style_text_font(scan_lbl, FONT_BODY_12, 0);
        lv_obj_align(scan_lbl, LV_ALIGN_TOP_RIGHT, -20, 25);

        // IP address
        qr_ip_lbl = lv_label_create(box);
        lv_label_set_text(qr_ip_lbl, ip ? ip : "");
        lv_obj_set_style_text_color(qr_ip_lbl, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(qr_ip_lbl, FONT_HIJRI_18, 0);
        lv_obj_align(qr_ip_lbl, LV_ALIGN_TOP_RIGHT, -20, 50);

        // Hint
        lv_obj_t *hint = lv_label_create(box);
        lv_label_set_text(hint, LocaleTR::toUpperTR("Ayarlar \xC2\xB7 Vakitler \xC2\xB7 Guncelleme"));
        lv_obj_set_style_text_color(hint, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(hint, 165, 0);
        lv_obj_set_style_text_font(hint, FONT_HEADING_10, 0);
        lv_obj_align(hint, LV_ALIGN_TOP_RIGHT, -20, 80);

        UiComponents::createNavDots(portal_overlay, 3, 4);

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
            bright_fill = bright_thumb = bright_val_lbl = nullptr;
            vol_fill = vol_thumb = vol_val_lbl = nullptr;
            tog_adhan = tog_sleep = nullptr;
            wifi_btn_name = wifi_btn_desc = nullptr;
            portal_overlay = nullptr;
            qr_code_obj = nullptr;
            qr_ip_lbl = nullptr;
            lastQrData[0] = '\0';
        }

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_add_style(scr, getStyleScreen(), 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        UiComponents::applyMotif(scr);

        // Status bar
        sb_handles = UiComponents::createStatusBar(scr, StatusBarIcon::SETTINGS);
        UiComponents::updateStatusBarCity(sb_handles, "Ayarlar", "");
        UiComponents::createSeparator(scr);

        // Scrollable body
        lv_obj_t *body = lv_obj_create(scr);
        lv_obj_remove_style_all(body);
        lv_obj_set_size(body, SCREEN_W, SCREEN_H - 22 - 2);
        lv_obj_set_pos(body, 0, 22 + 2);
        lv_obj_set_style_pad_left(body, BODY_PAD_X, 0);
        lv_obj_set_style_pad_right(body, BODY_PAD_X, 0);
        lv_obj_set_style_pad_top(body, 4, 0);
        lv_obj_set_style_pad_bottom(body, 8, 0);
        lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(body, 3, 0);
        lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

        // ── Section 1: EKRAN ──
        createSectionLabel(body, "EKRAN");
        {
            lv_obj_t *row = createRow(body);
            createRowLeft(row, "Parlaklik", nullptr);
            currentBrightness = g_state.brightness;
            auto sw = createSliderCtrl(row, currentBrightness, 0);
            bright_fill = sw.fill;
            bright_thumb = sw.thumb;
            bright_val_lbl = sw.val_lbl;
        }

        // ── Section 2: SES ──
        createSectionLabel(body, "SES");
        {
            lv_obj_t *row = createRow(body);
            createRowLeft(row, "Ezan Sesi", nullptr);
            tog_adhan = createToggle(row, !g_state.muted, 0);
        }
        {
            lv_obj_t *row = createRow(body);
            createRowLeft(row, "Ses Seviyesi", nullptr);
            currentVolumeLevel = g_state.volume;
            auto sw = createSliderCtrl(row, currentVolumeLevel, 1);
            vol_fill = sw.fill;
            vol_thumb = sw.thumb;
            vol_val_lbl = sw.val_lbl;
        }

        // ── Section 3: GUC & MOD ──
        createSectionLabel(body, "GUC & MOD");
        {
            lv_obj_t *row = createRow(body);
            createRowLeft(row, "Uyku Modu", nullptr);
            tog_sleep = createToggle(row, SettingsManager::getPowerMode() == PowerMode::SCREEN_OFF, 1);
        }

        // ── Section 4: BAGLANTI ──
        createSectionLabel(body, "BAGLANTI");
        createQRButton(body);

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
        updateSliderVisual(vol_fill, vol_thumb, vol_val_lbl, currentVolumeLevel);
    }

    int getVolumeLevel() { return currentVolumeLevel; }

    void setBrightnessLevel(int pct)
    {
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;
        currentBrightness = pct;
        updateSliderVisual(bright_fill, bright_thumb, bright_val_lbl, pct);
    }

    void syncToggles()
    {
        if (tog_adhan)
            setToggleVisual(tog_adhan, !g_state.muted);
        if (tog_sleep)
            setToggleVisual(tog_sleep, SettingsManager::getPowerMode() == PowerMode::SCREEN_OFF);
    }

    void updatePowerModeUI()
    {
        if (tog_sleep)
            setToggleVisual(tog_sleep, SettingsManager::getPowerMode() == PowerMode::SCREEN_OFF);
    }

    void setWiFiButtonState(WifiState state, const char *ip)
    {
        // Portal overlay: show only in PORTAL state
        if (state == WifiState::PORTAL)
            showPortalOverlay();

        switch (state)
        {
        case WifiState::DISCONNECTED:
            if (wifi_btn_name)
                lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR("Telefondan Yonet"));
            if (wifi_btn_desc)
                lv_label_set_text(wifi_btn_desc, LocaleTR::toUpperTR("Ayarlari ac"));
            break;

        case WifiState::CONNECTING:
            if (wifi_btn_name)
                lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR("Baslatiliyor..."));
            if (wifi_btn_desc)
                lv_label_set_text(wifi_btn_desc, "");
            break;

        case WifiState::CONNECTED:
        {
            // Show QR page with device IP
            showConnectedQRPage(ip);

            char buf[48];
            snprintf(buf, sizeof(buf), "Bagli: %s", ip ? ip : "");
            if (wifi_btn_name)
                lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR(buf));
            if (wifi_btn_desc)
            {
                lv_label_set_text(wifi_btn_desc, LocaleTR::toUpperTR("QR Kodu Goster"));
                lv_obj_set_style_text_color(wifi_btn_desc, COLOR_GREEN, 0);
            }
            break;
        }

        case WifiState::FAILED:
            if (wifi_btn_name)
            {
                lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR("Baglanamadi"));
                lv_obj_set_style_text_color(wifi_btn_name, COLOR_AMBER, 0);
            }
            if (wifi_btn_desc)
                lv_label_set_text(wifi_btn_desc, LocaleTR::toUpperTR("Tekrar dene"));
            break;

        case WifiState::PORTAL:
            if (wifi_btn_name)
                lv_label_set_text(wifi_btn_name, LocaleTR::toUpperTR("WiFi Bilgileri"));
            if (wifi_btn_desc)
                lv_label_set_text(wifi_btn_desc, LocaleTR::toUpperTR("Dokunarak goruntule"));
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

    void setWifi(uint8_t bars)
    {
        UiComponents::updateStatusBarWifi(sb_handles, bars);
    }

    void setBattery(uint8_t pct, bool charging)
    {
        UiComponents::updateStatusBarBattery(sb_handles, pct, charging);
    }

} // namespace UiPageSettings
