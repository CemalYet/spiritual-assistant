/**
 * @file ui_theme.cpp
 * @brief Design System - Style Initialization
 */

#include "ui_theme.h"

namespace UiTheme
{
    // Static styles - initialized once, reused for efficiency
    static lv_style_t style_screen;
    static lv_style_t style_title;
    static lv_style_t style_subtitle;
    static lv_style_t style_card;
    static lv_style_t style_card_label;
    static lv_style_t style_card_time;
    static lv_style_t style_icon_bar;
    static lv_style_t style_icon_btn;
    static lv_style_t style_indicator;
    static lv_style_t style_transparent;
    static bool styles_initialized = false;

    void initStyles()
    {
        if (styles_initialized)
            return;

        // Screen: dark background, pure white text
        lv_style_init(&style_screen);
        lv_style_set_bg_color(&style_screen, lv_color_hex(0x161616));
        lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
        lv_style_set_text_color(&style_screen, lv_color_hex(0xFFFFFF));
        lv_style_set_pad_all(&style_screen, 0);

        // Title (clock): pure white, large
        lv_style_init(&style_title);
        lv_style_set_text_color(&style_title, lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&style_title, &lv_font_montserrat_48);
        lv_style_set_text_letter_space(&style_title, 2);
        lv_style_set_text_align(&style_title, LV_TEXT_ALIGN_CENTER);

        // Subtitle (date): Dimmer for hierarchy
        lv_style_init(&style_subtitle);
        lv_style_set_text_color(&style_subtitle, lv_color_hex(0xAAAAAA));
        lv_style_set_text_font(&style_subtitle, &lv_font_montserrat_16);
        lv_style_set_text_letter_space(&style_subtitle, 1);
        lv_style_set_text_align(&style_subtitle, LV_TEXT_ALIGN_CENTER);

        // Card: darker BG, sky-600 border
        lv_style_init(&style_card);
        lv_style_set_radius(&style_card, CARD_RADIUS);
        lv_style_set_bg_color(&style_card, lv_color_hex(0x1E1E1E));
        lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
        lv_style_set_border_width(&style_card, 2);
        lv_style_set_border_color(&style_card, lv_color_hex(0x0284C7));
        lv_style_set_border_opa(&style_card, LV_OPA_COVER);
        lv_style_set_pad_all(&style_card, 12);
        lv_style_set_shadow_width(&style_card, 0);
        lv_style_set_shadow_color(&style_card, lv_color_hex(0x000000));
        lv_style_set_shadow_opa(&style_card, LV_OPA_TRANSP);
        lv_style_set_shadow_ofs_y(&style_card, 0);

        // Card label: neutral gray for hierarchy
        lv_style_init(&style_card_label);
        lv_style_set_text_color(&style_card_label, lv_color_hex(0x9CA3AF));
        lv_style_set_text_font(&style_card_label, &lv_font_montserrat_14);
        lv_style_set_text_letter_space(&style_card_label, 2);

        // Card time: bright white for emphasis
        lv_style_init(&style_card_time);
        lv_style_set_text_color(&style_card_time, lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&style_card_time, &lv_font_montserrat_32);

        // Icon bar: same as background
        lv_style_init(&style_icon_bar);
        lv_style_set_bg_color(&style_icon_bar, lv_color_hex(0x161616));
        lv_style_set_bg_opa(&style_icon_bar, LV_OPA_COVER);
        lv_style_set_border_width(&style_icon_bar, 0);
        lv_style_set_pad_all(&style_icon_bar, 0);

        // Icon button: transparent
        lv_style_init(&style_icon_btn);
        lv_style_set_bg_opa(&style_icon_btn, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_icon_btn, 0);
        lv_style_set_shadow_width(&style_icon_btn, 0);
        lv_style_set_pad_all(&style_icon_btn, 6);
        lv_style_set_radius(&style_icon_btn, 8);

        // Active indicator: sky-500 accent
        lv_style_init(&style_indicator);
        lv_style_set_bg_color(&style_indicator, lv_color_hex(0x0EA5E9));
        lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
        lv_style_set_radius(&style_indicator, 2);
        lv_style_set_border_width(&style_indicator, 0);

        // Transparent container
        lv_style_init(&style_transparent);
        lv_style_set_bg_opa(&style_transparent, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_transparent, 0);
        lv_style_set_pad_all(&style_transparent, 0);

        styles_initialized = true;
    }

    lv_style_t *getStyleScreen() { return &style_screen; }
    lv_style_t *getStyleTitle() { return &style_title; }
    lv_style_t *getStyleSubtitle() { return &style_subtitle; }
    lv_style_t *getStyleCard() { return &style_card; }
    lv_style_t *getStyleCardLabel() { return &style_card_label; }
    lv_style_t *getStyleCardTime() { return &style_card_time; }
    lv_style_t *getStyleIconBar() { return &style_icon_bar; }
    lv_style_t *getStyleIconBtn() { return &style_icon_btn; }
    lv_style_t *getStyleIndicator() { return &style_indicator; }
    lv_style_t *getStyleTransparent() { return &style_transparent; }

} // namespace UiTheme
