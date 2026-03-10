/**
 * @file ui_components.cpp
 * @brief Reusable UI Components — status bar, separator, nav dots
 */

#include "ui_components.h"
#include "ui_theme.h"
#include "ui_icons.h"
#include "locale_tr.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>

// ── Background pattern (generated once in createSharedAssets) ────────────────
lv_img_dsc_t bg_pattern_dsc;
static uint8_t *motif_buf = nullptr; // heap-allocated pixel data

namespace UiComponents
{
    // ── createStatusBar ──────────────────────────────────────────────────────
    StatusBarHandles createStatusBar(lv_obj_t *parent, StatusBarIcon icon)
    {
        StatusBarHandles h = {};

        lv_obj_t *bar = lv_obj_create(parent);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, 480, 22);
        lv_obj_set_pos(bar, 0, 0);
        noScrollNoBorder(bar);
        lv_obj_set_style_bg_color(bar, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_left(bar, 16, 0);
        lv_obj_set_style_pad_right(bar, 16, 0);
        lv_obj_set_style_pad_top(bar, 8, 0);

        // ── Left: icon + city + date abbrev ──────────────────────────────────
        lv_obj_t *left = lv_obj_create(bar);
        lv_obj_remove_style_all(left);
        noScrollNoBorder(left);
        lv_obj_set_size(left, LV_SIZE_CONTENT, 16);
        lv_obj_align(left, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(left, 6, 0);

        // Leading icon — map pin (location) or gear (settings)
        // U+F3C5 = fa-map-marker-alt (pin icon matching HTML SVG)
        static const char ICON_MAP_PIN[] = "\xEF\x8F\x85"; // U+F3C5
        lv_obj_t *pin = lv_label_create(left);
        lv_obj_set_style_text_font(pin, FONT_HEADING_12, 0);
        lv_obj_set_style_text_color(pin, COLOR_GOLD, 0);
        lv_obj_set_style_text_opa(pin, 190, 0);
        lv_label_set_text(pin, icon == StatusBarIcon::SETTINGS ? LV_SYMBOL_SETTINGS : ICON_MAP_PIN);

        h.lbl_city = lv_label_create(left);
        lv_obj_set_style_text_font(h.lbl_city, FONT_BODY_12, 0);
        lv_obj_set_style_text_color(h.lbl_city, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(h.lbl_city, 2, 0);
        lv_label_set_text(h.lbl_city, "");

        h.lbl_dateabbr = lv_label_create(left);
        lv_obj_set_style_text_font(h.lbl_dateabbr, FONT_HEADING_10, 0);
        lv_obj_set_style_text_color(h.lbl_dateabbr, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(h.lbl_dateabbr, 209, 0);
        lv_label_set_text(h.lbl_dateabbr, "");

        // ── Right: wifi icon + battery outline + percent ─────────────────────
        lv_obj_t *right = lv_obj_create(bar);
        lv_obj_remove_style_all(right);
        noScrollNoBorder(right);
        lv_obj_set_size(right, LV_SIZE_CONTENT, 16);
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(right, 10, 0);

        // WiFi — use LVGL built-in symbol
        h.lbl_wifi = lv_label_create(right);
        lv_obj_set_style_text_font(h.lbl_wifi, FONT_HEADING_12, 0);
        lv_obj_set_style_text_color(h.lbl_wifi, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(h.lbl_wifi, 140, 0);
        lv_label_set_text(h.lbl_wifi, LV_SYMBOL_WIFI);

        // Battery wrapper (w=24 h=11): outline + nub positioned absolutely
        lv_obj_t *bat_wrap = lv_obj_create(right);
        lv_obj_remove_style_all(bat_wrap);
        lv_obj_set_size(bat_wrap, 24, 11);
        lv_obj_set_style_bg_opa(bat_wrap, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(bat_wrap, 0, 0);
        lv_obj_clear_flag(bat_wrap, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *bat_outer = lv_obj_create(bat_wrap);
        lv_obj_remove_style_all(bat_outer);
        lv_obj_set_size(bat_outer, 20, 11);
        lv_obj_set_pos(bat_outer, 0, 0);
        lv_obj_set_style_radius(bat_outer, 2, 0);
        lv_obj_set_style_border_width(bat_outer, 1, 0);
        lv_obj_set_style_border_color(bat_outer, COLOR_GOLD, 0);
        lv_obj_set_style_border_opa(bat_outer, 102, 0);
        lv_obj_set_style_bg_opa(bat_outer, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(bat_outer, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *bat_nub = lv_obj_create(bat_wrap);
        lv_obj_remove_style_all(bat_nub);
        lv_obj_set_size(bat_nub, 3, 6);
        lv_obj_set_pos(bat_nub, 21, 3);
        lv_obj_set_style_radius(bat_nub, 1, 0);
        lv_obj_set_style_bg_color(bat_nub, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(bat_nub, 130, 0);
        lv_obj_set_style_border_width(bat_nub, 0, 0);

        h.bat_fill = lv_obj_create(bat_outer);
        lv_obj_remove_style_all(h.bat_fill);
        lv_obj_set_size(h.bat_fill, 12, 7);
        lv_obj_set_pos(h.bat_fill, 2, 2);
        lv_obj_set_style_radius(h.bat_fill, 1, 0);
        lv_obj_set_style_bg_color(h.bat_fill, COLOR_GREEN, 0);
        lv_obj_set_style_bg_opa(h.bat_fill, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(h.bat_fill, 0, 0);
        lv_obj_clear_flag(h.bat_fill, LV_OBJ_FLAG_SCROLLABLE);

        h.lbl_charge = lv_label_create(bat_outer);
        lv_obj_set_style_text_font(h.lbl_charge, FONT_HEADING_8, 0);
        lv_obj_set_style_text_color(h.lbl_charge, COLOR_GOLD, 0);
        lv_label_set_text(h.lbl_charge, "\xE2\x9A\xA1"); // ⚡ U+26A1
        lv_obj_center(h.lbl_charge);
        lv_obj_add_flag(h.lbl_charge, LV_OBJ_FLAG_HIDDEN);

        h.lbl_bat_pct = lv_label_create(right);
        lv_obj_set_style_text_font(h.lbl_bat_pct, FONT_MONO_10, 0);
        lv_obj_set_style_text_color(h.lbl_bat_pct, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(h.lbl_bat_pct, 209, 0);
        lv_label_set_text(h.lbl_bat_pct, "--%");

        return h;
    }

    // ── createSeparator ──────────────────────────────────────────────────────
    void createSeparator(lv_obj_t *parent)
    {
        // Left half: COLOR_BG → COLOR_GOLD (2px tall, stronger opa + glow)
        lv_obj_t *sl = lv_obj_create(parent);
        lv_obj_remove_style_all(sl);
        lv_obj_set_size(sl, 240, 2);
        lv_obj_set_pos(sl, 0, 27);
        lv_obj_set_style_bg_grad_dir(sl, LV_GRAD_DIR_HOR, 0);
        lv_obj_set_style_bg_color(sl, COLOR_BG, 0);
        lv_obj_set_style_bg_grad_color(sl, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(sl, 230, 0);
        lv_obj_set_style_shadow_color(sl, COLOR_GOLD, 0);
        lv_obj_set_style_shadow_width(sl, 8, 0);
        lv_obj_set_style_shadow_opa(sl, 90, 0);
        lv_obj_set_style_shadow_spread(sl, 0, 0);
        lv_obj_set_style_border_width(sl, 0, 0);
        lv_obj_clear_flag(sl, LV_OBJ_FLAG_SCROLLABLE);

        // Right half: COLOR_GOLD → COLOR_BG
        lv_obj_t *sr = lv_obj_create(parent);
        lv_obj_remove_style_all(sr);
        lv_obj_set_size(sr, 240, 2);
        lv_obj_set_pos(sr, 240, 27);
        lv_obj_set_style_bg_grad_dir(sr, LV_GRAD_DIR_HOR, 0);
        lv_obj_set_style_bg_color(sr, COLOR_GOLD, 0);
        lv_obj_set_style_bg_grad_color(sr, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(sr, 230, 0);
        lv_obj_set_style_shadow_color(sr, COLOR_GOLD, 0);
        lv_obj_set_style_shadow_width(sr, 8, 0);
        lv_obj_set_style_shadow_opa(sr, 90, 0);
        lv_obj_set_style_shadow_spread(sr, 0, 0);
        lv_obj_set_style_border_width(sr, 0, 0);
        lv_obj_clear_flag(sr, LV_OBJ_FLAG_SCROLLABLE);
    }

    static NavClickCallback navClickCallback = nullptr;
    static uint32_t lastNavClick = 0;

    void setNavClickCallback(NavClickCallback cb)
    {
        navClickCallback = cb;
    }

    static void onNavClick(lv_event_t *e)
    {
        uint32_t now = lv_tick_get();
        if (now - lastNavClick < UiTheme::DEBOUNCE_MS)
            return;
        lastNavClick = now;

        int page = (int)(intptr_t)lv_event_get_user_data(e);
        if (navClickCallback)
            navClickCallback(page);
    }

    void createNavBar(lv_obj_t *parent, int activePage)
    {
        using namespace UiTheme;

        lv_obj_t *bar = lv_obj_create(parent);
        lv_obj_remove_style_all(bar);
        lv_obj_add_style(bar, getStyleIconBar(), 0);
        lv_obj_set_size(bar, SCREEN_W, NAV_H);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

        // Top border line
        lv_obj_t *line = lv_obj_create(bar);
        lv_obj_set_size(line, SCREEN_W, 1);
        lv_obj_set_pos(line, 0, 0);
        lv_obj_set_style_bg_color(line, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);
        lv_obj_set_style_border_width(line, 0, 0);

        constexpr int16_t BTN_W = SCREEN_W / 3; // 106px per button

        for (int i = 0; i < 3; i++)
        {
            bool active = (i == activePage);
            lv_color_t col = active ? COLOR_GOLD_LIGHT : COLOR_DIM;

            lv_obj_t *btn = lv_btn_create(bar);
            lv_obj_remove_style_all(btn);
            lv_obj_add_style(btn, getStyleIconBtn(), 0);
            lv_obj_set_size(btn, BTN_W, NAV_H - 4);
            lv_obj_set_pos(btn, i * BTN_W, 4);
            lv_obj_add_event_cb(btn, onNavClick, LV_EVENT_PRESSED, (void *)(intptr_t)i);

            lv_obj_t *icon = lv_obj_create(btn);
            lv_obj_remove_style_all(icon);
            lv_obj_add_style(icon, getStyleTransparent(), 0);
            lv_obj_set_size(icon, NAV_ICON_SIZE, NAV_ICON_SIZE);
            lv_obj_align(icon, LV_ALIGN_CENTER, 0, -4);
            lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);

            if (i == 0)
                UiIcons::drawHomeIcon(icon, col);
            else if (i == 1)
                UiIcons::drawMosqueIcon(icon, col);
            else
                UiIcons::drawMenuIcon(icon, col);

            lv_obj_t *ind = lv_obj_create(btn);
            lv_obj_remove_style_all(ind);
            lv_obj_add_style(ind, getStyleIndicator(), 0);
            lv_obj_set_size(ind, 24, 3);
            lv_obj_align(ind, LV_ALIGN_BOTTOM_MID, 0, -4);
            lv_obj_set_style_bg_opa(ind, active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_clear_flag(ind, LV_OBJ_FLAG_CLICKABLE);
        }
    }

    void createNavDots(lv_obj_t *parent, int activePage, int numDots)
    {
        lv_obj_t *nav = lv_obj_create(parent);
        lv_obj_remove_style_all(nav);
        lv_obj_set_size(nav, LV_SIZE_CONTENT, 8);
        lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, -8);
        lv_obj_set_style_bg_opa(nav, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(nav, 0, 0);
        lv_obj_set_style_pad_all(nav, 0, 0);
        lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(nav, 6, 0);

        for (int i = 0; i < numDots; i++)
        {
            lv_obj_t *dot = lv_obj_create(nav);
            lv_obj_remove_style_all(dot);
            bool active = (i == activePage);
            lv_obj_set_size(dot, active ? 16 : 5, 5);
            lv_obj_set_style_radius(dot, 3, 0);
            lv_obj_set_style_bg_color(dot, COLOR_GOLD, 0);
            lv_obj_set_style_bg_opa(dot, active ? 230 : 82, 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            if (active)
            {
                lv_obj_set_style_shadow_color(dot, COLOR_GOLD, 0);
                lv_obj_set_style_shadow_spread(dot, 3, 0);
                lv_obj_set_style_shadow_opa(dot, 115, 0);
            }
        }
    }

    // ── Shared status-bar data setters ────────────────────────────────────────
    void updateStatusBarCity(const StatusBarHandles &h, const char *city, const char *dateAbbrev)
    {
        if (h.lbl_city && city)
            lv_label_set_text(h.lbl_city, LocaleTR::toUpperTR(city));
        if (h.lbl_dateabbr && dateAbbrev)
            lv_label_set_text(h.lbl_dateabbr, LocaleTR::toUpperTR(dateAbbrev));
    }

    void updateStatusBarWifi(const StatusBarHandles &h, uint8_t bars)
    {
        if (!h.lbl_wifi)
            return;
        // Change color/opacity based on signal strength
        if (bars == 0)
        {
            lv_obj_set_style_text_color(h.lbl_wifi, COLOR_DIM, 0);
            lv_obj_set_style_text_opa(h.lbl_wifi, 80, 0);
        }
        else
        {
            lv_obj_set_style_text_color(h.lbl_wifi, COLOR_GREEN, 0);
            lv_obj_set_style_text_opa(h.lbl_wifi, (bars >= 2) ? 220 : 140, 0);
        }
    }

    void updateStatusBarBattery(const StatusBarHandles &h, uint8_t pct, bool charging)
    {
        if (!h.bat_fill)
            return;
        int32_t w = ((int32_t)pct * 16) / 100;
        if (w < 0)
            w = 0;
        if (w > 16)
            w = 16;
        lv_obj_set_width(h.bat_fill, (lv_coord_t)w);
        lv_color_t col = (pct > 20) ? COLOR_GREEN : lv_color_hex(0xFF4444);
        lv_obj_set_style_bg_color(h.bat_fill, col, 0);

        if (h.lbl_bat_pct)
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d%%", (int)pct);
            lv_label_set_text(h.lbl_bat_pct, buf);
        }

        if (h.lbl_charge)
        {
            if (charging)
                lv_obj_clear_flag(h.lbl_charge, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(h.lbl_charge, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // ── Motif tile generation ────────────────────────────────────────
    // Islamic interlocking circles pattern — matches HTML mockup.
    // 60×60 tile, 9 circles (r=20) at grid intersections.
    // Uses LV_IMG_CF_TRUE_COLOR_ALPHA (3 bytes/px for RGB565+A).
    static constexpr int TILE_W = 60;
    static constexpr int TILE_H = 60;

    // Circle centers in the 60×60 tile (with wrapping from neighbors)
    static constexpr int NUM_CIRCLES = 9;
    static const int16_t CX[NUM_CIRCLES] = {0, 30, 60, 0, 30, 60, 0, 30, 60};
    static const int16_t CY[NUM_CIRCLES] = {0, 0, 0, 30, 30, 30, 60, 60, 60};
    static constexpr float RADIUS = 20.0f;
    static constexpr float STROKE = 1.0f; // visual stroke width in pixels

    void createSharedAssets()
    {
        // LV_IMG_CF_TRUE_COLOR_ALPHA = 3 bytes/pixel on RGB565 (2 color + 1 alpha)
        const size_t bpp = LV_IMG_CF_TRUE_COLOR_ALPHA == LV_IMG_CF_TRUE_COLOR_ALPHA ? 3 : 3;
        const size_t buf_size = TILE_W * TILE_H * bpp;
        uint8_t *buf = (uint8_t *)lv_mem_alloc(buf_size);
        if (!buf)
            return;
        memset(buf, 0, buf_size); // fully transparent

        // Precompute gold in RGB565 big-endian
        lv_color_t gold = COLOR_GOLD; // 0xC9A84C
        uint16_t c16 = gold.full;
        uint8_t c_hi = (uint8_t)(c16 >> 8);
        uint8_t c_lo = (uint8_t)(c16 & 0xFF);

        for (int py = 0; py < TILE_H; py++)
        {
            for (int px = 0; px < TILE_W; px++)
            {
                // Find minimum distance to any circle edge
                float min_dist = 999.0f;
                for (int c = 0; c < NUM_CIRCLES; c++)
                {
                    float dx = (float)px - (float)CX[c];
                    float dy = (float)py - (float)CY[c];
                    float d = sqrtf(dx * dx + dy * dy);
                    float edge_dist = fabsf(d - RADIUS);
                    if (edge_dist < min_dist)
                        min_dist = edge_dist;
                }

                // Anti-aliased stroke: full alpha within STROKE/2, fade to 0 outside
                float half = STROKE * 0.5f;
                uint8_t alpha = 0;
                if (min_dist <= half)
                    alpha = 255;
                else if (min_dist < half + 1.0f)
                    alpha = (uint8_t)(255.0f * (1.0f - (min_dist - half)));

                if (alpha > 0)
                {
                    size_t idx = (py * TILE_W + px) * bpp;
                    buf[idx + 0] = c_lo; // RGB565 low byte
                    buf[idx + 1] = c_hi; // RGB565 high byte
                    buf[idx + 2] = alpha;
                }
            }
        }

        bg_pattern_dsc.header.always_zero = 0;
        bg_pattern_dsc.header.w = TILE_W;
        bg_pattern_dsc.header.h = TILE_H;
        bg_pattern_dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        bg_pattern_dsc.data_size = buf_size;
        bg_pattern_dsc.data = buf;
    }

    void applyMotif(lv_obj_t *scr)
    {
        if (!bg_pattern_dsc.data)
            return;
        lv_obj_set_style_bg_img_src(scr, &bg_pattern_dsc, 0);
        lv_obj_set_style_bg_img_tiled(scr, true, 0);
        lv_obj_set_style_bg_img_opa(scr, 25, 0); // ~10% opacity (closer to HTML 0.09)
    }

    void createAmbientGlow(lv_obj_t *parent)
    {
        // Gold radial glow at top-center — approximates HTML .amb gradient
        lv_obj_t *glow = lv_obj_create(parent);
        lv_obj_remove_style_all(glow);
        lv_obj_set_size(glow, 480, 160);
        lv_obj_set_pos(glow, 0, 0);
        lv_obj_set_style_bg_color(glow, COLOR_GOLD, 0);
        lv_obj_set_style_bg_grad_color(glow, COLOR_BG, 0);
        lv_obj_set_style_bg_grad_dir(glow, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_opa(glow, 50, 0); // visible gold warmth (~20% blend)
        lv_obj_set_style_border_width(glow, 0, 0);
        lv_obj_clear_flag(glow, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }

} // namespace UiComponents
