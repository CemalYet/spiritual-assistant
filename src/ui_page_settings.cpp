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
#include "wifi_portal.h"
#include <WiFi.h>
#include <lvgl.h>
#include <extra/libs/qrcode/lv_qrcode.h>
#include <cstring>

namespace UiPageSettings
{
    using namespace UiTheme;

    static lv_obj_t *scr = nullptr;

    // QR Code overlay for Portal mode (text info only, no QR)
    static lv_obj_t *portal_overlay = nullptr;
    static lv_obj_t *volume_minus_btn = nullptr;
    static lv_obj_t *volume_plus_btn = nullptr;
    static lv_obj_t *volume_value_lbl = nullptr;
    static lv_obj_t *volume_bar = nullptr;
    static lv_obj_t *wifi_btn = nullptr;
    static lv_obj_t *wifi_btn_lbl = nullptr;

    // Inline QR code area (shown when connected, replaces button)
    static lv_obj_t *inline_qr_container = nullptr;
    static lv_obj_t *inline_qr_code = nullptr;
    static lv_obj_t *inline_qr_ip_lbl = nullptr;
    static char lastInlineQrData[128] = "";

    static int currentVolumeLevel = 4;
    static AdvancedCallback advancedCallback = nullptr;

    // Forward declarations
    static void onVolumeBtn(lv_event_t *e);
    static void onPortalOverlayClick(lv_event_t *e);

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
        // Immediate visual feedback - show "Starting..." state
        setWiFiButtonState(WifiState::CONNECTING, nullptr);

        // Force LVGL to redraw NOW before callback blocks
        lv_refr_now(NULL);

        // Trigger the callback - portal/server will start
        // Final state update happens via ui_state_reader when ready
        if (advancedCallback)
        {
            advancedCallback();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // PORTAL INFO OVERLAY
    // ═══════════════════════════════════════════════════════════════

    static void hidePortalOverlay(); // Forward declaration
    static void showPortalOverlay(); // Forward declaration

    static void onPortalOverlayClick(lv_event_t *e)
    {
        hidePortalOverlay();
    }

    static void showPortalOverlay()
    {
        // Only show overlay in Portal mode (WiFi info)
        // Connected mode uses inline QR instead
        bool isPortalMode = WiFiPortal::isActive();

        if (!isPortalMode)
        {
            // Not in portal mode - don't show overlay (inline QR handles connected state)
            return;
        }

        // First time: create overlay with WiFi info (no QR code)
        if (!portal_overlay)
        {
            portal_overlay = lv_obj_create(lv_scr_act());
            lv_obj_remove_style_all(portal_overlay);
            lv_obj_set_size(portal_overlay, 240, 320);
            lv_obj_set_pos(portal_overlay, 0, 0);
            lv_obj_set_style_bg_color(portal_overlay, lv_color_hex(0x161616), 0);
            lv_obj_set_style_bg_opa(portal_overlay, LV_OPA_COVER, 0);
            lv_obj_clear_flag(portal_overlay, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(portal_overlay, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(portal_overlay, onPortalOverlayClick, LV_EVENT_CLICKED, nullptr);

            // Title
            lv_obj_t *title = lv_label_create(portal_overlay);
            lv_label_set_text(title, "WiFi Bilgileri");
            lv_obj_set_style_text_color(title, COLOR_ACCENT, 0);
            lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

            // WiFi icon
            lv_obj_t *wifi_icon = lv_label_create(portal_overlay);
            lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
            lv_obj_set_style_text_color(wifi_icon, COLOR_ACCENT, 0);
            lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_24, 0);
            lv_obj_align(wifi_icon, LV_ALIGN_TOP_MID, 0, 70);

            // Instruction
            lv_obj_t *inst = lv_label_create(portal_overlay);
            lv_label_set_text(inst, "Telefonunuzdan baglanin:");
            lv_obj_set_style_text_color(inst, COLOR_DIM, 0);
            lv_obj_set_style_text_font(inst, &lv_font_montserrat_12, 0);
            lv_obj_align(inst, LV_ALIGN_CENTER, 0, -40);

            // SSID (network name)
            lv_obj_t *ssid_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(ssid_lbl, "AdhanSettings");
            lv_obj_set_style_text_color(ssid_lbl, COLOR_TEXT, 0);
            lv_obj_set_style_text_font(ssid_lbl, &lv_font_montserrat_20, 0);
            lv_obj_align(ssid_lbl, LV_ALIGN_CENTER, 0, -10);

            // Password title
            lv_obj_t *pass_title = lv_label_create(portal_overlay);
            lv_label_set_text(pass_title, "Sifre:");
            lv_obj_set_style_text_color(pass_title, COLOR_SUBTITLE, 0);
            lv_obj_set_style_text_font(pass_title, &lv_font_montserrat_12, 0);
            lv_obj_align(pass_title, LV_ALIGN_CENTER, 0, 25);

            // Password value
            lv_obj_t *pass_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(pass_lbl, "12345678");
            lv_obj_set_style_text_color(pass_lbl, COLOR_ACCENT, 0);
            lv_obj_set_style_text_font(pass_lbl, &lv_font_montserrat_16, 0);
            lv_obj_align(pass_lbl, LV_ALIGN_CENTER, 0, 50);

            // IP hint
            lv_obj_t *ip_lbl = lv_label_create(portal_overlay);
            lv_label_set_text(ip_lbl, "Sonra tarayicida: 192.168.4.1");
            lv_obj_set_style_text_color(ip_lbl, COLOR_DIM, 0);
            lv_obj_set_style_text_font(ip_lbl, &lv_font_montserrat_12, 0);
            lv_obj_align(ip_lbl, LV_ALIGN_CENTER, 0, 90);

            // Dismiss hint
            lv_obj_t *dismiss = lv_label_create(portal_overlay);
            lv_label_set_text(dismiss, "Kapatmak icin dokun");
            lv_obj_set_style_text_color(dismiss, COLOR_SUBTITLE, 0);
            lv_obj_set_style_text_font(dismiss, &lv_font_montserrat_12, 0);
            lv_obj_align(dismiss, LV_ALIGN_BOTTOM_MID, 0, -30);
        }

        // Show overlay
        lv_obj_clear_flag(portal_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(portal_overlay);
    }

    static void hidePortalOverlay()
    {
        if (portal_overlay)
        {
            lv_obj_add_flag(portal_overlay, LV_OBJ_FLAG_HIDDEN);
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
            inline_qr_container = inline_qr_code = inline_qr_ip_lbl = nullptr;
            lastInlineQrData[0] = '\0';
            portal_overlay = nullptr; // Reset overlay (will be recreated on demand)
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

        // === ACTION BUTTON (Telefondan Yonet) - shown when disconnected ===
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

        // === INLINE QR CODE AREA - shown when connected (replaces button) ===
        inline_qr_container = lv_obj_create(scr);
        lv_obj_remove_style_all(inline_qr_container);
        lv_obj_set_size(inline_qr_container, 220, 130);
        lv_obj_align(inline_qr_container, LV_ALIGN_TOP_MID, 0, 120);
        lv_obj_set_style_bg_color(inline_qr_container, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_bg_opa(inline_qr_container, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(inline_qr_container, 12, 0);
        lv_obj_set_style_border_width(inline_qr_container, 1, 0);
        lv_obj_set_style_border_color(inline_qr_container, COLOR_GREEN, 0);
        lv_obj_clear_flag(inline_qr_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(inline_qr_container, LV_OBJ_FLAG_HIDDEN); // Hidden by default

        // Small QR code (70x70)
        inline_qr_code = lv_qrcode_create(inline_qr_container, 70, lv_color_black(), lv_color_white());
        lv_obj_align(inline_qr_code, LV_ALIGN_LEFT_MID, 12, 0);

        // Right side: "Scan to connect" title
        lv_obj_t *qr_title = lv_label_create(inline_qr_container);
        lv_label_set_text(qr_title, "Telefonla Tara");
        lv_obj_set_style_text_color(qr_title, COLOR_GREEN, 0);
        lv_obj_set_style_text_font(qr_title, &lv_font_montserrat_14, 0);
        lv_obj_align(qr_title, LV_ALIGN_TOP_RIGHT, -12, 20);

        // IP address below title
        inline_qr_ip_lbl = lv_label_create(inline_qr_container);
        lv_label_set_text(inline_qr_ip_lbl, "192.168.1.1");
        lv_obj_set_style_text_color(inline_qr_ip_lbl, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(inline_qr_ip_lbl, &lv_font_montserrat_16, 0);
        lv_obj_align(inline_qr_ip_lbl, LV_ALIGN_TOP_RIGHT, -12, 42);

        // Hint text
        lv_obj_t *qr_hint = lv_label_create(inline_qr_container);
        lv_label_set_text(qr_hint, "Ayarlari ac");
        lv_obj_set_style_text_color(qr_hint, COLOR_DIM, 0);
        lv_obj_set_style_text_font(qr_hint, &lv_font_montserrat_12, 0);
        lv_obj_align(qr_hint, LV_ALIGN_TOP_RIGHT, -12, 65);

        // Set initial state based on connection
        if (Network::isConnected())
        {
            setWiFiButtonState(WifiState::CONNECTED, WiFi.localIP().toString().c_str());
        }
        else
        {
            setWiFiButtonState(WifiState::DISCONNECTED, nullptr);
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

    void setWiFiButtonState(WifiState state, const char *ip)
    {
        if (!wifi_btn || !wifi_btn_lbl)
            return;

        // Helper to show button, hide inline QR
        auto showButton = [&]()
        {
            lv_obj_clear_flag(wifi_btn, LV_OBJ_FLAG_HIDDEN);
            if (inline_qr_container)
                lv_obj_add_flag(inline_qr_container, LV_OBJ_FLAG_HIDDEN);
        };

        // Helper to show inline QR, hide button
        auto showInlineQR = [&](const char *ipAddr)
        {
            lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_HIDDEN);
            if (inline_qr_container && inline_qr_code && inline_qr_ip_lbl)
            {
                lv_obj_clear_flag(inline_qr_container, LV_OBJ_FLAG_HIDDEN);

                // Update IP label
                lv_label_set_text(inline_qr_ip_lbl, ipAddr ? ipAddr : "");

                // Update QR code with URL
                char qrData[64];
                snprintf(qrData, sizeof(qrData), "http://%s", ipAddr ? ipAddr : "192.168.4.1");
                if (strcmp(lastInlineQrData, qrData) != 0)
                {
                    lv_qrcode_update(inline_qr_code, qrData, strlen(qrData));
                    strncpy(lastInlineQrData, qrData, sizeof(lastInlineQrData) - 1);
                }
            }
        };

        // Portal overlay: show only in PORTAL state, hide in all others
        if (state == WifiState::PORTAL)
            showPortalOverlay();
        else
            hidePortalOverlay();

        switch (state)
        {
        case WifiState::DISCONNECTED:
            showButton();
            lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
            applyButtonStyle("Telefondan Yonet", lv_color_hex(0x0D3D4D), COLOR_ACCENT);
            break;

        case WifiState::CONNECTING:
            showButton();
            applyButtonStyle("Baslatiliyor...", lv_color_hex(0x2A2A2A), COLOR_DIM);
            lv_obj_clear_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
            break;

        case WifiState::CONNECTED:
            if (ip && ip[0])
            {
                showInlineQR(ip);
            }
            else
            {
                showButton();
                lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
                applyButtonStyle(LV_SYMBOL_EYE_OPEN " Ayarlar Acik", lv_color_hex(0x1A3D2E), COLOR_GREEN);
            }
            break;

        case WifiState::FAILED:
            showButton();
            lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
            applyButtonStyle("Baglanamadi", lv_color_hex(0x3D1A1A), COLOR_RED);
            break;

        case WifiState::PORTAL:
            showButton();
            lv_obj_add_flag(wifi_btn, LV_OBJ_FLAG_CLICKABLE);
            applyButtonStyle("WiFi Bilgileri", lv_color_hex(0x0D3D4D), COLOR_ACCENT);
            break;
        }
    }

} // namespace UiPageSettings
