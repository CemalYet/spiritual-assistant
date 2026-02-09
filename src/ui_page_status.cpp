/**
 * @file ui_page_status.cpp
 * @brief Status Screens - Connecting, Portal, Message, Error
 */

#include "ui_page_status.h"
#include "ui_theme.h"
#include "ui_icons.h"

namespace UiPageStatus
{
    using namespace UiTheme;

    static lv_obj_t *scr = nullptr;

    static void createScreen()
    {
        if (scr)
        {
            // Clean children but keep the screen object
            lv_obj_clean(scr);
        }
        else
        {
            scr = lv_obj_create(NULL);
        }
        lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    }

    void showConnecting(const char *ssid)
    {
        createScreen();

        // WiFi icon at top
        lv_obj_t *icon_cont = lv_obj_create(scr);
        lv_obj_remove_style_all(icon_cont);
        lv_obj_set_size(icon_cont, 64, 64);
        lv_obj_align(icon_cont, LV_ALIGN_CENTER, 0, -60);
        UiIcons::drawWiFiIcon(icon_cont, COLOR_ACCENT, false);

        // "Connecting..." label
        lv_obj_t *lbl_status = lv_label_create(scr);
        lv_label_set_text(lbl_status, "Baglaniyor...");
        lv_obj_set_style_text_color(lbl_status, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl_status, LV_ALIGN_CENTER, 0, 10);

        // SSID label
        lv_obj_t *lbl_ssid = lv_label_create(scr);
        lv_label_set_text(lbl_ssid, ssid ? ssid : "");
        lv_obj_set_style_text_color(lbl_ssid, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_ssid, LV_ALIGN_CENTER, 0, 35);

        lv_scr_load(scr);
    }

    void showPortal(const char *apName, const char *password, const char *ip)
    {
        createScreen();

        // Title
        lv_obj_t *lbl_title = lv_label_create(scr);
        lv_label_set_text(lbl_title, "WiFi Ayari");
        lv_obj_set_style_text_color(lbl_title, COLOR_ACCENT_BRIGHT, 0);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 30);

        // AP Name
        lv_obj_t *lbl_ap_title = lv_label_create(scr);
        lv_label_set_text(lbl_ap_title, "Ag Adi:");
        lv_obj_set_style_text_color(lbl_ap_title, COLOR_SUBTITLE, 0);
        lv_obj_set_style_text_font(lbl_ap_title, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_ap_title, LV_ALIGN_TOP_LEFT, 20, 70);

        lv_obj_t *lbl_ap = lv_label_create(scr);
        lv_label_set_text(lbl_ap, apName ? apName : "");
        lv_obj_set_style_text_color(lbl_ap, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl_ap, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_ap, LV_ALIGN_TOP_LEFT, 20, 88);

        // Password
        lv_obj_t *lbl_pw_title = lv_label_create(scr);
        lv_label_set_text(lbl_pw_title, "Sifre:");
        lv_obj_set_style_text_color(lbl_pw_title, COLOR_SUBTITLE, 0);
        lv_obj_set_style_text_font(lbl_pw_title, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_pw_title, LV_ALIGN_TOP_LEFT, 20, 120);

        lv_obj_t *lbl_pw = lv_label_create(scr);
        lv_label_set_text(lbl_pw, password ? password : "");
        lv_obj_set_style_text_color(lbl_pw, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl_pw, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_pw, LV_ALIGN_TOP_LEFT, 20, 138);

        // IP Address
        lv_obj_t *lbl_ip_title = lv_label_create(scr);
        lv_label_set_text(lbl_ip_title, "Adres:");
        lv_obj_set_style_text_color(lbl_ip_title, COLOR_SUBTITLE, 0);
        lv_obj_set_style_text_font(lbl_ip_title, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_ip_title, LV_ALIGN_TOP_LEFT, 20, 170);

        lv_obj_t *lbl_ip = lv_label_create(scr);
        lv_label_set_text(lbl_ip, ip ? ip : "");
        lv_obj_set_style_text_color(lbl_ip, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_ip, LV_ALIGN_TOP_LEFT, 20, 188);

        // Instructions
        lv_obj_t *lbl_inst = lv_label_create(scr);
        lv_label_set_text(lbl_inst, "Telefonunuzdan\nbu aga baglanin");
        lv_obj_set_style_text_color(lbl_inst, COLOR_DIM, 0);
        lv_obj_set_style_text_font(lbl_inst, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_align(lbl_inst, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl_inst, LV_ALIGN_BOTTOM_MID, 0, -40);

        lv_scr_load(scr);
    }

    void showMessage(const char *line1, const char *line2)
    {
        createScreen();

        lv_obj_t *lbl1 = lv_label_create(scr);
        lv_label_set_text(lbl1, line1 ? line1 : "");
        lv_obj_set_style_text_color(lbl1, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl1, LV_ALIGN_CENTER, 0, line2 ? -15 : 0);

        if (line2)
        {
            lv_obj_t *lbl2 = lv_label_create(scr);
            lv_label_set_text(lbl2, line2);
            lv_obj_set_style_text_color(lbl2, COLOR_SUBTITLE, 0);
            lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_14, 0);
            lv_obj_align(lbl2, LV_ALIGN_CENTER, 0, 15);
        }

        lv_scr_load(scr);
    }

    void showError(const char *line1, const char *line2)
    {
        createScreen();

        // Error icon (red X)
        lv_obj_t *icon_cont = lv_obj_create(scr);
        lv_obj_remove_style_all(icon_cont);
        lv_obj_set_size(icon_cont, 48, 48);
        lv_obj_align(icon_cont, LV_ALIGN_CENTER, 0, -50);

        static lv_point_t x1[] = {{4, 4}, {44, 44}};
        static lv_point_t x2[] = {{44, 4}, {4, 44}};
        UiIcons::drawLine(icon_cont, x1, 2, COLOR_RED);
        UiIcons::drawLine(icon_cont, x2, 2, COLOR_RED);

        // Error title
        lv_obj_t *lbl1 = lv_label_create(scr);
        lv_label_set_text(lbl1, line1 ? line1 : "Hata");
        lv_obj_set_style_text_color(lbl1, COLOR_RED, 0);
        lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl1, LV_ALIGN_CENTER, 0, 10);

        if (line2)
        {
            lv_obj_t *lbl2 = lv_label_create(scr);
            lv_label_set_text(lbl2, line2);
            lv_obj_set_style_text_color(lbl2, COLOR_SUBTITLE, 0);
            lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_14, 0);
            lv_obj_align(lbl2, LV_ALIGN_CENTER, 0, 35);
        }

        lv_scr_load(scr);
    }

} // namespace UiPageStatus
