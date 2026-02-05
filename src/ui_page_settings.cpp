/**
 * @file ui_page_settings.cpp
 * @brief Settings Page - Volume Control, WiFi Button
 */

#include "ui_page_settings.h"
#include "ui_theme.h"
#include "ui_components.h"
#include "settings_manager.h"
#include "app_state.h"
#include "network.h"
#include <WiFi.h>

namespace UiPageSettings
{
    using namespace UiTheme;

    static lv_obj_t *scr = nullptr;
    static lv_obj_t *volume_minus_btn = nullptr;
    static lv_obj_t *volume_plus_btn = nullptr;
    static lv_obj_t *volume_value_lbl = nullptr;
    static lv_obj_t *volume_bar = nullptr;
    static lv_obj_t *wifi_btn = nullptr;
    static lv_obj_t *wifi_btn_lbl = nullptr;

    static int currentVolumeLevel = 4;
    static AdvancedCallback advancedCallback = nullptr;

    // Helper: Apply button style (reduces DRY violation)
    static void applyButtonStyle(const char *text, lv_color_t bgColor, lv_color_t accentColor)
    {
        lv_label_set_text(wifi_btn_lbl, text);
        lv_obj_set_style_bg_color(wifi_btn, bgColor, 0);
        lv_obj_set_style_bg_opa(wifi_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(wifi_btn, accentColor, 0);
        lv_obj_set_style_text_color(wifi_btn_lbl, accentColor, 0);
    }

    // Helper: Create volume button
    static lv_obj_t *createVolumeButton(lv_obj_t *parent, lv_align_t align, int16_t xOfs, int16_t yOfs, const char *label, int delta)
    {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, 50, 50);
        lv_obj_align(btn, align, xOfs, yOfs);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 10, 0);
        lv_obj_set_ext_click_area(btn, 8);
        lv_obj_add_event_cb(btn, onVolumeBtn, LV_EVENT_ALL, (void *)(intptr_t)(delta));

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, label);
        lv_obj_set_style_text_color(lbl, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
        lv_obj_center(lbl);

        return btn;
    }

    static void updateVolumeDisplay()
    {
        if (!volume_value_lbl || !volume_bar)
            return;

        int pct = currentVolumeLevel * 20;
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", pct);
        lv_label_set_text(volume_value_lbl, buf);
        lv_obj_set_width(volume_bar, (currentVolumeLevel * 80) / 5); // 80px max width
    }

    static void onVolumeBtn(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t *btn = lv_event_get_target(e);
        int delta = (int)(intptr_t)lv_event_get_user_data(e);

        if (code == LV_EVENT_PRESSED)
        {
            lv_obj_set_style_bg_color(btn, COLOR_ACCENT, 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            return;
        }
        if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
        {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x222222), 0);
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            return;
        }
        if (code != LV_EVENT_CLICKED)
            return;

        int newLevel = currentVolumeLevel + delta;
        if (newLevel < 0 || newLevel > 5)
            return;

        currentVolumeLevel = newLevel;
        SettingsManager::setVolume(currentVolumeLevel * 20);
        AppStateHelper::setVolume(currentVolumeLevel); // Sync to g_state
        updateVolumeDisplay();

        // Sync muted state to home page
        AppStateHelper::setMuted(currentVolumeLevel == 0);
    }

    static void onWiFiBtn(lv_event_t *e)
    {
        if (advancedCallback)
        {
            advancedCallback();
        }
    }

    lv_obj_t *create()
    {
        UiTheme::initStyles();

        if (scr)
        {
            lv_obj_del(scr);
            scr = nullptr;
            volume_minus_btn = volume_plus_btn = volume_value_lbl = volume_bar = nullptr;
            wifi_btn = wifi_btn_lbl = nullptr;
        }

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_add_style(scr, getStyleScreen(), 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        // Title
        lv_obj_t *title = lv_label_create(scr);
        lv_label_set_text(title, "AYARLAR");
        lv_obj_set_style_text_color(title, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_letter_space(title, 2, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

        // === VOLUME SECTION (No outer box - phantom controls) ===
        constexpr int16_t VOL_Y = 60; // Vertical position for volume row

        // Minus and Plus buttons
        volume_minus_btn = createVolumeButton(scr, LV_ALIGN_TOP_LEFT, 16, VOL_Y, "-", -1);
        volume_plus_btn = createVolumeButton(scr, LV_ALIGN_TOP_RIGHT, -16, VOL_Y, "+", 1);

        // Volume percentage - large white text in center
        volume_value_lbl = lv_label_create(scr);
        lv_obj_set_style_text_color(volume_value_lbl, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(volume_value_lbl, &lv_font_montserrat_24, 0);
        lv_obj_align(volume_value_lbl, LV_ALIGN_TOP_MID, 0, VOL_Y + 8);

        // Slim cyan bar underneath text (2px height)
        lv_obj_t *track = lv_obj_create(scr);
        lv_obj_remove_style_all(track);
        lv_obj_set_size(track, 80, 3);
        lv_obj_align(track, LV_ALIGN_TOP_MID, 0, VOL_Y + 42);
        lv_obj_set_style_bg_color(track, lv_color_hex(0x333333), 0);
        lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(track, 1, 0);
        lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);

        // Volume bar fill
        volume_bar = lv_obj_create(track);
        lv_obj_remove_style_all(volume_bar);
        lv_obj_set_height(volume_bar, 3);
        lv_obj_set_pos(volume_bar, 0, 0);
        lv_obj_set_style_bg_color(volume_bar, COLOR_ACCENT, 0);
        lv_obj_set_style_bg_opa(volume_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(volume_bar, 1, 0);

        updateVolumeDisplay();

        // === ACTION BUTTON (Telefondan Yonet) ===
        wifi_btn = lv_btn_create(scr);
        lv_obj_remove_style_all(wifi_btn);
        lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(wifi_btn, 208, 50);
        lv_obj_align(wifi_btn, LV_ALIGN_TOP_MID, 0, 140);
        lv_obj_set_style_radius(wifi_btn, 10, 0);
        lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0x0D3D4D), 0); // Dark cyan filled
        lv_obj_set_style_bg_opa(wifi_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(wifi_btn, 2, 0);
        lv_obj_set_style_border_color(wifi_btn, COLOR_ACCENT, 0);
        lv_obj_set_ext_click_area(wifi_btn, 8);
        lv_obj_add_event_cb(wifi_btn, onWiFiBtn, LV_EVENT_CLICKED, nullptr);

        // Single centered label
        wifi_btn_lbl = lv_label_create(wifi_btn);
        lv_obj_set_style_text_font(wifi_btn_lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(wifi_btn_lbl, COLOR_ACCENT, 0);
        lv_obj_center(wifi_btn_lbl);
        lv_label_set_text(wifi_btn_lbl, "Telefondan Yonet");

        // Set initial state based on connection
        if (Network::isConnected())
        {
            setWiFiButtonState(WiFiButtonState::CONNECTED, WiFi.localIP().toString().c_str());
        }
        else
        {
            setWiFiButtonState(WiFiButtonState::DISCONNECTED, nullptr);
        }

        UiComponents::createNavBar(scr, 2);

        return scr;
    }

    lv_obj_t *getScreen() { return scr; }

    void setAdvancedCallback(AdvancedCallback cb) { advancedCallback = cb; }

    void setVolumeLevel(int level)
    {
        if (level < 0)
            level = 0;
        if (level > 5)
            level = 5;
        currentVolumeLevel = level;
        updateVolumeDisplay();
    }

    int getVolumeLevel() { return currentVolumeLevel; }

    void setWiFiButtonState(WiFiButtonState state, const char *ip)
    {
        if (!wifi_btn || !wifi_btn_lbl)
            return;

        switch (state)
        {
        case WiFiButtonState::DISCONNECTED:
            applyButtonStyle("Telefondan Yonet", lv_color_hex(0x0D3D4D), COLOR_ACCENT);
            break;

        case WiFiButtonState::CONNECTING:
            applyButtonStyle("Aciliyor...", lv_color_hex(0x2A2A2A), COLOR_DIM);
            break;

        case WiFiButtonState::CONNECTED:
            if (ip && ip[0])
            {
                char buf[24];
                snprintf(buf, sizeof(buf), "Acik: %s", ip);
                applyButtonStyle(buf, lv_color_hex(0x1A3D2E), COLOR_GREEN);
            }
            else
            {
                applyButtonStyle("Ayarlar Acik", lv_color_hex(0x1A3D2E), COLOR_GREEN);
            }
            break;

        case WiFiButtonState::FAILED:
            applyButtonStyle("Baglanamadi", lv_color_hex(0x3D1A1A), COLOR_RED);
            break;

        case WiFiButtonState::PORTAL:
            applyButtonStyle("AP: 192.168.4.1", lv_color_hex(0x0D3D4D), COLOR_ACCENT);
            break;
        }
    }

    void updateWiFiButton(bool connected, const char *ip)
    {
        setWiFiButtonState(connected ? WiFiButtonState::CONNECTED : WiFiButtonState::DISCONNECTED, ip);
    }

} // namespace UiPageSettings
