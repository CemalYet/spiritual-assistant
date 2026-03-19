#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

// ── Color macros — resolve at runtime from active palette ──
#define COLOR_BG lv_color_hex(UiTheme::p().bg)
#define COLOR_BG2 lv_color_hex(UiTheme::p().bg2)
#define COLOR_GOLD lv_color_hex(UiTheme::p().accent)
#define COLOR_GOLD_LIGHT lv_color_hex(UiTheme::p().accent2)
#define COLOR_TEXT lv_color_hex(UiTheme::p().text)
#define COLOR_DIM lv_color_hex(UiTheme::p().dim)
#define COLOR_STRIP_BG lv_color_hex(UiTheme::p().stripBg)
#define COLOR_GREEN lv_color_hex(UiTheme::p().green)
#define COLOR_AMBER lv_color_hex(UiTheme::p().amber)
#define COLOR_BORDER lv_color_hex(UiTheme::p().border)
#define COLOR_BTN_HI lv_color_hex(UiTheme::p().btnHi)
#define COLOR_BTN_LO lv_color_hex(UiTheme::p().btnLo)

// ── Font declarations ──
LV_FONT_DECLARE(font_inter_8);
LV_FONT_DECLARE(font_inter_10);
LV_FONT_DECLARE(font_inter_11);
LV_FONT_DECLARE(font_inter_12_sb);
LV_FONT_DECLARE(font_inter_13);
LV_FONT_DECLARE(font_inter_14);
LV_FONT_DECLARE(font_inter_20_sb);
LV_FONT_DECLARE(font_dmmono_11);
LV_FONT_DECLARE(font_dmmono_16);
LV_FONT_DECLARE(font_dmmono_66);
LV_FONT_DECLARE(font_dmmono_80);

#define FONT_CLOCK_72 (&font_dmmono_80)
#define FONT_MONO_60 (&font_dmmono_66)
#define FONT_MONO_14 (&font_dmmono_16)
#define FONT_MONO_10 (&font_dmmono_11)
#define FONT_PRAYER_24 (&font_inter_20_sb)
#define FONT_HIJRI_18 (&font_inter_14)
#define FONT_BODY_14 (&font_inter_13)
#define FONT_GREETING_13 (&font_inter_13)
#define FONT_HEADING_12 (&font_inter_12_sb)
#define FONT_BODY_12 (&font_inter_12_sb)
#define FONT_HEADING_10 (&font_inter_11)
#define FONT_HEADING_8 (&font_inter_8)
#define FONT_LABEL_MIN FONT_HEADING_10
#define FONT_QIBLA_52 (&font_dmmono_66)

namespace UiTheme
{
    struct ThemePalette
    {
        uint32_t bg;
        uint32_t bg2;
        uint32_t stripBg;
        uint32_t accent;
        uint32_t accent2;
        uint32_t text;
        uint32_t dim;
        uint32_t border;
        uint32_t green;
        uint32_t amber;
        uint32_t btnHi; // raised button gradient top
        uint32_t btnLo; // raised button gradient bottom / pressed
    };

    enum class ThemeMode : uint8_t
    {
        ALTIN_GECE = 0,
        KREM_ALTIN,
        COUNT
    };

    const ThemePalette &p();

    static constexpr int16_t SCREEN_W = 480;
    static constexpr int16_t SCREEN_H = 320;

    static constexpr int16_t SPACING_SM = 6;
    static constexpr int16_t SPACING_MD = 12;
    static constexpr int16_t SPACING_LG = 20;
    static constexpr int16_t NAV_H = 20;
    static constexpr int16_t NAV_ICON_SIZE = 16;
    static constexpr int16_t ICON_SIZE_STATUS = 32;
    static constexpr int16_t ICON_SIZE_CONTROL = 32;
    static constexpr int16_t ICON_SIZE_NAV = 16;
    static constexpr lv_opa_t ICON_OPA_DEFAULT = 210;
    static constexpr lv_opa_t ICON_OPA_ACTIVE = 245;
    static constexpr lv_opa_t ICON_OPA_DISABLED = 140;
    static constexpr int16_t CARD_RADIUS = 8;
    static constexpr uint32_t DEBOUNCE_MS = 140;

    void initStyles();
    void setThemeMode(ThemeMode mode);
    ThemeMode getThemeMode();
    lv_style_t *getStyleScreen();
    lv_style_t *getStyleCard();
    lv_style_t *getStyleIconBar();
    lv_style_t *getStyleIconBtn();
    lv_style_t *getStyleIndicator();
    lv_style_t *getStyleTransparent();

} // namespace UiTheme

#endif // UI_THEME_H
