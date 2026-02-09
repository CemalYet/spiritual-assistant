/**
 * @file ui_theme.h
 * @brief Design System - Colors, Fonts, Spacing, Styles
 *
 * DESIGN SYSTEM (Premium IoT - Sky Blue Palette):
 * - Background: #161616 (professional dark gray)
 * - Text Primary: #FFFFFF (pure white for labels)
 * - Text Secondary: #AAAAAA (dimmer gray for date)
 * - Accent: #0EA5E9 (sky-500, premium blue)
 * - Accent Dark: #0284C7 (sky-600, borders)
 * - Inactive: #9CA3AF (neutral gray-400)
 * - Card: #1E1E1E with sky border
 * - Typography: Montserrat 48/32/16/14
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

namespace UiTheme
{
    // ═══════════════════════════════════════════════════════════════
    // COLOR PALETTE - TN Panel Optimized (brighter for viewing angles)
    // ═══════════════════════════════════════════════════════════════
    static constexpr lv_color_t COLOR_BG = LV_COLOR_MAKE(0x16, 0x16, 0x16);            // #161616 Professional dark
    static constexpr lv_color_t COLOR_CARD_BG = LV_COLOR_MAKE(0x20, 0x21, 0x24);       // #202124 Card background
    static constexpr lv_color_t COLOR_TEXT = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF);          // #FFFFFF Pure White
    static constexpr lv_color_t COLOR_SUBTITLE = LV_COLOR_MAKE(0xCF, 0xF4, 0xFF);      // #CFF4FF Bright for TN
    static constexpr lv_color_t COLOR_ACCENT = LV_COLOR_MAKE(0x0E, 0xA5, 0xE9);        // #0EA5E9 Sky-500 premium
    static constexpr lv_color_t COLOR_ACCENT_BRIGHT = LV_COLOR_MAKE(0x0E, 0xA5, 0xE9); // #0EA5E9 Same for consistency
    static constexpr lv_color_t COLOR_ACCENT_DARK = LV_COLOR_MAKE(0x02, 0x84, 0xC7);   // #0284C7 Sky-600 for borders
    static constexpr lv_color_t COLOR_GREEN = LV_COLOR_MAKE(0x00, 0xE6, 0x76);         // #00E676 Green (sync success)
    static constexpr lv_color_t COLOR_AMBER = LV_COLOR_MAKE(0xFF, 0xBF, 0x00);         // #FFBF00 Amber (warning)
    static constexpr lv_color_t COLOR_RED = LV_COLOR_MAKE(0xFF, 0x44, 0x44);           // #FF4444 Red (weak signal)
    static constexpr lv_color_t COLOR_DIM = LV_COLOR_MAKE(0x9C, 0xA3, 0xAF);           // #9CA3AF Neutral gray-400
    static constexpr lv_color_t COLOR_HEADER = LV_COLOR_MAKE(0x7B, 0x7B, 0x8F);        // #7B7B8F Header text gray

    // ═══════════════════════════════════════════════════════════════
    // LAYOUT CONSTANTS - Consistent Spacing Rhythm
    // ═══════════════════════════════════════════════════════════════
    static constexpr int16_t SPACING_SM = 8;  // Small spacing
    static constexpr int16_t SPACING_MD = 16; // Medium spacing
    static constexpr int16_t SPACING_LG = 24; // Large spacing

    static constexpr int16_t CLOCK_Y = 20;       // Hero clock position
    static constexpr int16_t HEADER_Y = 76;      // Location line (date 18px below)
    static constexpr int16_t CARD_Y = 120;       // Prayer card position
    static constexpr int16_t CARD_W = 208;       // Card width (240-32 margin)
    static constexpr int16_t CARD_H = 56;        // Card height
    static constexpr int16_t CARD_RADIUS = 10;   // Design guideline radius
    static constexpr int16_t NAV_H = 64;         // Navigation bar height
    static constexpr int16_t NAV_ICON_SIZE = 32; // Nav icon size
    static constexpr int16_t STATUS_Y = 195;     // Status icons position

    static constexpr uint32_t DEBOUNCE_MS = 200; // Minimum time between clicks

    // ═══════════════════════════════════════════════════════════════
    // STYLE GETTERS - Initialized once, accessed anywhere
    // ═══════════════════════════════════════════════════════════════
    void initStyles();
    lv_style_t *getStyleScreen();
    lv_style_t *getStyleTitle();
    lv_style_t *getStyleSubtitle();
    lv_style_t *getStyleCard();
    lv_style_t *getStyleCardLabel();
    lv_style_t *getStyleCardTime();
    lv_style_t *getStyleIconBar();
    lv_style_t *getStyleIconBtn();
    lv_style_t *getStyleIndicator();
    lv_style_t *getStyleTransparent();

} // namespace UiTheme

#endif // UI_THEME_H
