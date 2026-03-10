/**
 * @file ui_theme.cpp
 * @brief Design System — Style Initialization (Dark Navy/Gold palette)
 */

#include "ui_theme.h"

namespace UiTheme
{
    static lv_style_t style_screen;
    static lv_style_t style_card;
    static lv_style_t style_icon_bar;
    static lv_style_t style_icon_btn;
    static lv_style_t style_indicator;
    static lv_style_t style_transparent;
    static bool styles_initialized = false;

    void initStyles()
    {
        if (styles_initialized)
            return;

        // Screen: deep navy bg, parchment text
        lv_style_init(&style_screen);
        lv_style_set_bg_color(&style_screen, COLOR_BG);
        lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
        lv_style_set_text_color(&style_screen, COLOR_TEXT);
        lv_style_set_border_width(&style_screen, 0);
        lv_style_set_pad_all(&style_screen, 0);

        // Card: BG2 with subtle gold border
        lv_style_init(&style_card);
        lv_style_set_radius(&style_card, CARD_RADIUS);
        lv_style_set_bg_color(&style_card, COLOR_BG2);
        lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
        lv_style_set_border_width(&style_card, 1);
        lv_style_set_border_color(&style_card, COLOR_BORDER);
        lv_style_set_border_opa(&style_card, 41);
        lv_style_set_pad_all(&style_card, 10);
        lv_style_set_shadow_width(&style_card, 0);

        // Icon bar: matches screen bg (nav strip)
        lv_style_init(&style_icon_bar);
        lv_style_set_bg_color(&style_icon_bar, COLOR_BG);
        lv_style_set_bg_opa(&style_icon_bar, LV_OPA_COVER);
        lv_style_set_border_width(&style_icon_bar, 0);
        lv_style_set_pad_all(&style_icon_bar, 0);

        // Icon button: transparent with press feedback
        lv_style_init(&style_icon_btn);
        lv_style_set_bg_opa(&style_icon_btn, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_icon_btn, 0);
        lv_style_set_shadow_width(&style_icon_btn, 0);
        lv_style_set_pad_all(&style_icon_btn, 4);
        lv_style_set_radius(&style_icon_btn, 6);

        // Active indicator: gold pill
        lv_style_init(&style_indicator);
        lv_style_set_bg_color(&style_indicator, COLOR_GOLD);
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
    lv_style_t *getStyleCard() { return &style_card; }
    lv_style_t *getStyleIconBar() { return &style_icon_bar; }
    lv_style_t *getStyleIconBtn() { return &style_icon_btn; }
    lv_style_t *getStyleIndicator() { return &style_indicator; }
    lv_style_t *getStyleTransparent() { return &style_transparent; }

} // namespace UiTheme
