/**
 * @file ui_theme.h
 * @brief Design System — Dark Navy/Gold Palette, Landscape 480×320
 *
 * Color macros are global (#define) — include once, use anywhere.
 * Font macros are global (#define) under HAS_CUSTOM_FONTS guard.
 * Layout constants, DEBOUNCE_MS, and shared style getters live in
 * namespace UiTheme.
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

// ────────────────────────────────────────────────────────────────
// COLOR PALETTE — deep navy background, warm gold accents
// ────────────────────────────────────────────────────────────────
#define COLOR_BG lv_color_hex(0x07091A)         // deep navy
#define COLOR_BG2 lv_color_hex(0x0C1028)        // slightly lighter navy
#define COLOR_GOLD lv_color_hex(0xC9A84C)       // warm gold
#define COLOR_GOLD_LIGHT lv_color_hex(0xEAC96A) // bright gold (countdown, active)
#define COLOR_TEXT lv_color_hex(0xF0E8D0)       // warm parchment white
#define COLOR_DIM lv_color_make(240, 232, 208)  // parchment — use opa 165
#define COLOR_DIM2 lv_color_make(240, 232, 208) // parchment — use opa 97
#define COLOR_STRIP_BG lv_color_hex(0x04060A)   // ultra-dark strip background
#define COLOR_GREEN lv_color_hex(0x2ECC71)      // battery OK / sync success
#define COLOR_AMBER lv_color_hex(0xE07B00)      // urgent countdown < 10 min
#define COLOR_BORDER lv_color_hex(0xC9A84C)     // gold dividers — use opa 41-46

// ────────────────────────────────────────────────────────────────
// FONT DECLARATIONS & ROLE MAP
// 2-family modern type system:
//   Display / all text → Inter (clean sans-serif, highly readable)
//   Tabular digits     → DM Mono (monospace countdown & times)
// 11 font files in src/fonts/ — generated via lv_font_conv --lcd.
// Turkish glyphs (ıİşŞöÖüÜğĞçÇ) + FontAwesome (GPS/WiFi/Settings).
// ────────────────────────────────────────────────────────────────
// Inter — display & body text
LV_FONT_DECLARE(font_cinzel_8);
LV_FONT_DECLARE(font_cinzel_10);
LV_FONT_DECLARE(font_cinzel_11);
LV_FONT_DECLARE(font_cinzel_12_sb);
LV_FONT_DECLARE(font_cinzel_13);
LV_FONT_DECLARE(font_cinzel_14);
LV_FONT_DECLARE(font_cinzel_20_sb);
// DM Mono — tabular digits
LV_FONT_DECLARE(font_dmmono_11);
LV_FONT_DECLARE(font_dmmono_16);
LV_FONT_DECLARE(font_dmmono_66);
LV_FONT_DECLARE(font_dmmono_80);

// Font role map — AUTHORITATIVE. Use ONLY these defines in all UI code.
#define FONT_CLOCK_72 (&font_dmmono_80)      // clock hero digits (HTML=80px)
#define FONT_MONO_60 (&font_dmmono_66)       // countdown "00:00:00" (HTML=66px)
#define FONT_MONO_14 (&font_dmmono_16)       // seconds :45, IP, hijri mono (HTML=16px)
#define FONT_MONO_10 (&font_dmmono_11)       // strip times, battery%, slider values (HTML=11px)
#define FONT_PRAYER_24 (&font_cinzel_20_sb)  // prayer names, WiFi icon (HTML=20px SemiBold)
#define FONT_HIJRI_18 (&font_cinzel_14)      // Hijri date, SSID (HTML=14px)
#define FONT_BODY_14 (&font_cinzel_13)       // Gregorian date (HTML=13px)
#define FONT_GREETING_13 (&font_cinzel_13)   // greeting "Hayırlı Geceler —" (HTML=13px)
#define FONT_HEADING_12 (&font_cinzel_12_sb) // strip names, headings, QR host (HTML=12px SB + FA)
#define FONT_BODY_12 (&font_cinzel_11)       // city, body, instruction text (HTML=11px)
#define FONT_HEADING_10 (&font_cinzel_10)    // ALL-CAPS section labels (HTML=10px)
#define FONT_HEADING_8 (&font_cinzel_8)      // tiny headers, battery label (HTML=8px)
#define FONT_QIBLA_52 (&font_dmmono_66)      // qibla degree (reuse 66px mono)

// ────────────────────────────────────────────────────────────────
// NAMESPACE — layout constants + shared style getters
// ────────────────────────────────────────────────────────────────
namespace UiTheme
{
    // Screen dimensions — Waveshare 3.5" landscape 480×320
    static constexpr int16_t SCREEN_W = 480;
    static constexpr int16_t SCREEN_H = 320;

    // Spacing rhythm
    static constexpr int16_t SPACING_SM = 6;
    static constexpr int16_t SPACING_MD = 12;
    static constexpr int16_t SPACING_LG = 20;

    // Nav dot strip (bottom 20px)
    static constexpr int16_t NAV_H = 20;
    static constexpr int16_t NAV_ICON_SIZE = 14;

    // Card defaults
    static constexpr int16_t CARD_RADIUS = 8;

    // Debounce
    static constexpr uint32_t DEBOUNCE_MS = 200;

    // Style getters — call initStyles() once before first use.
    void initStyles();
    lv_style_t *getStyleScreen();      // dark navy bg, parchment text
    lv_style_t *getStyleCard();        // BG2 bg, subtle gold border
    lv_style_t *getStyleIconBar();     // bottom nav strip bg
    lv_style_t *getStyleIconBtn();     // transparent clickable button
    lv_style_t *getStyleIndicator();   // gold active indicator pill
    lv_style_t *getStyleTransparent(); // zero bg/border/pad container

} // namespace UiTheme

#endif // UI_THEME_H
