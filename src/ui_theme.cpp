#include "ui_theme.h"

namespace UiTheme
{
    //       bg        bg2       stripBg   accent    accent2   text      dim       border    green     amber
    static constexpr ThemePalette PALETTES[] = {
        {0x10141C, 0x182131, 0x0D121B, 0xE2B45A, 0xF0CA85, 0xF3ECDD, 0xC8BEA9, 0x343E55, 0x2FA968, 0xD86C0F}, // Altın Gece
        {0xF5F0E8, 0xEDE6D4, 0xEAE4D8, 0xB8860B, 0x7A5C00, 0x2A2200, 0x6A5E48, 0xD0C8B0, 0x1A7A3A, 0xC45000}, // Krem Altın
        {0x1A1810, 0x252015, 0x14120C, 0xD4C898, 0xEDE4C0, 0xF5F0E0, 0xB8B0A0, 0x3A3528, 0x2FA968, 0xD86C0F}, // Gece Krem
    };
    static_assert(sizeof(PALETTES) / sizeof(PALETTES[0]) == static_cast<uint8_t>(ThemeMode::COUNT),
                  "PALETTES size must match ThemeMode::COUNT");

    static lv_style_t style_screen;
    static lv_style_t style_card;
    static lv_style_t style_icon_bar;
    static lv_style_t style_icon_btn;
    static lv_style_t style_indicator;
    static lv_style_t style_transparent;
    static bool styles_initialized = false;
    static ThemeMode current_theme = ThemeMode::ALTIN_GECE;

    const ThemePalette &p()
    {
        return PALETTES[static_cast<uint8_t>(current_theme)];
    }

    static void applyPaletteToStyles()
    {
        lv_style_set_bg_color(&style_screen, COLOR_BG);
        lv_style_set_text_color(&style_screen, COLOR_TEXT);
        lv_style_set_bg_color(&style_card, COLOR_BG2);
        lv_style_set_border_color(&style_card, COLOR_BORDER);
        lv_style_set_bg_color(&style_icon_bar, COLOR_BG);
        lv_style_set_bg_color(&style_indicator, COLOR_GOLD);
    }

    void initStyles()
    {
        if (styles_initialized)
            return;

        lv_style_init(&style_screen);
        lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
        lv_style_set_border_width(&style_screen, 0);
        lv_style_set_pad_all(&style_screen, 0);

        lv_style_init(&style_card);
        lv_style_set_radius(&style_card, CARD_RADIUS);
        lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
        lv_style_set_border_width(&style_card, 1);
        lv_style_set_border_opa(&style_card, 41);
        lv_style_set_pad_all(&style_card, 10);
        lv_style_set_shadow_width(&style_card, 0);

        lv_style_init(&style_icon_bar);
        lv_style_set_bg_opa(&style_icon_bar, LV_OPA_COVER);
        lv_style_set_border_width(&style_icon_bar, 0);
        lv_style_set_pad_all(&style_icon_bar, 0);

        lv_style_init(&style_icon_btn);
        lv_style_set_bg_opa(&style_icon_btn, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_icon_btn, 0);
        lv_style_set_shadow_width(&style_icon_btn, 0);
        lv_style_set_pad_all(&style_icon_btn, 4);
        lv_style_set_radius(&style_icon_btn, 6);

        lv_style_init(&style_indicator);
        lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
        lv_style_set_radius(&style_indicator, 2);
        lv_style_set_border_width(&style_indicator, 0);

        lv_style_init(&style_transparent);
        lv_style_set_bg_opa(&style_transparent, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_transparent, 0);
        lv_style_set_pad_all(&style_transparent, 0);

        applyPaletteToStyles();
        styles_initialized = true;
    }

    void setThemeMode(ThemeMode mode)
    {
        if (mode >= ThemeMode::COUNT)
            return;
        if (current_theme == mode)
            return;

        current_theme = mode;
        if (!styles_initialized)
            return;

        applyPaletteToStyles();
    }

    ThemeMode getThemeMode()
    {
        return current_theme;
    }

    lv_style_t *getStyleScreen() { return &style_screen; }
    lv_style_t *getStyleCard() { return &style_card; }
    lv_style_t *getStyleIconBar() { return &style_icon_bar; }
    lv_style_t *getStyleIconBtn() { return &style_icon_btn; }
    lv_style_t *getStyleIndicator() { return &style_indicator; }
    lv_style_t *getStyleTransparent() { return &style_transparent; }

} // namespace UiTheme
