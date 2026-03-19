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
    static MuteToggleCallback muteToggleCallback = nullptr;
    static uint32_t lastMuteClick = 0;
    static constexpr uint32_t MUTE_DEBOUNCE_MS = 90;

    static void onMuteClick(lv_event_t *e)
    {
        (void)e;
        uint32_t now = lv_tick_get();
        if (now - lastMuteClick < MUTE_DEBOUNCE_MS)
            return;
        lastMuteClick = now;
        if (muteToggleCallback)
            muteToggleCallback();
    }

    // ── createStatusBar ──────────────────────────────────────────────────────
    StatusBarHandles createStatusBar(lv_obj_t *parent, StatusBarIcon icon)
    {
        StatusBarHandles h = {};

        lv_obj_t *bar = lv_obj_create(parent);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, 480, 48);
        lv_obj_set_pos(bar, 0, 0);
        noScrollNoBorder(bar);
        lv_obj_set_style_bg_color(bar, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_left(bar, 16, 0);
        lv_obj_set_style_pad_right(bar, 16, 0);
        lv_obj_set_style_pad_top(bar, 0, 0);

        // ── Left: icon + city (date removed for cleaner top bar) ─────────────
        lv_obj_t *left = lv_obj_create(bar);
        lv_obj_remove_style_all(left);
        noScrollNoBorder(left);
        lv_obj_set_size(left, LV_SIZE_CONTENT, 48);
        lv_obj_align(left, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(left, 8, 0);

        // Leading icon — use built-in LVGL symbols for guaranteed glyph coverage.
        lv_obj_t *pin = lv_label_create(left);
        lv_obj_set_style_text_font(pin, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(pin, COLOR_DIM, 0);
        lv_obj_set_style_text_opa(pin, UiTheme::ICON_OPA_DEFAULT, 0);
        lv_label_set_text(pin, icon == StatusBarIcon::SETTINGS ? LV_SYMBOL_SETTINGS : LV_SYMBOL_GPS);

        h.lbl_city = lv_label_create(left);
        lv_obj_set_style_text_font(h.lbl_city, FONT_BODY_14, 0);
        lv_obj_set_style_text_color(h.lbl_city, COLOR_TEXT, 0);
        lv_obj_set_style_text_letter_space(h.lbl_city, 2, 0);
        lv_label_set_text(h.lbl_city, "");

        h.lbl_dateabbr = nullptr;

        // ── Right: speaker control ───────────────────────────────────────────
        lv_obj_t *right = lv_obj_create(bar);
        lv_obj_remove_style_all(right);
        noScrollNoBorder(right);
        lv_obj_set_size(right, 56, 48);
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(right, 6, 0);
        lv_obj_add_flag(right, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

        h.mute_btn = lv_obj_create(right);
        lv_obj_remove_style_all(h.mute_btn);
        lv_obj_set_size(h.mute_btn, 48, 48);
        lv_obj_set_style_radius(h.mute_btn, 12, 0);
        lv_obj_set_style_bg_color(h.mute_btn, COLOR_BG2, 0);
        lv_obj_set_style_bg_opa(h.mute_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(h.mute_btn, 1, 0);
        lv_obj_set_style_border_color(h.mute_btn, COLOR_BORDER, 0);
        lv_obj_set_style_border_opa(h.mute_btn, 64, 0);
        lv_obj_set_style_pad_left(h.mute_btn, 0, 0);
        lv_obj_set_style_pad_right(h.mute_btn, 0, 0);
        lv_obj_set_style_pad_top(h.mute_btn, 0, 0);
        lv_obj_set_style_pad_bottom(h.mute_btn, 0, 0);
        lv_obj_set_style_pad_column(h.mute_btn, 0, 0);
        lv_obj_clear_flag(h.mute_btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(h.mute_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(h.mute_btn, 20);
        lv_obj_add_event_cb(h.mute_btn, onMuteClick, LV_EVENT_PRESSED, nullptr);

        h.lbl_mute_icon = lv_obj_create(h.mute_btn);
        lv_obj_remove_style_all(h.lbl_mute_icon);
        lv_obj_set_size(h.lbl_mute_icon, UiTheme::ICON_SIZE_STATUS, UiTheme::ICON_SIZE_STATUS);
        lv_obj_clear_flag(h.lbl_mute_icon, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_opa(h.lbl_mute_icon, UiTheme::ICON_OPA_DEFAULT, 0);
        UiIcons::drawSpeakerIcon(h.lbl_mute_icon, false, COLOR_DIM, true);
        lv_obj_center(h.lbl_mute_icon);

        h.lbl_mute_text = nullptr;

        h.lbl_wifi = nullptr;

        updateStatusBarMute(h, false);

        return h;
    }

    // ── createSeparator ──────────────────────────────────────────────────────
    void createSeparator(lv_obj_t *parent)
    {
        // Single gold separator line under status bar.
        lv_obj_t *line = lv_obj_create(parent);
        lv_obj_remove_style_all(line);
        lv_obj_set_size(line, 480, 1);
        lv_obj_set_pos(line, 0, 47);
        lv_obj_set_style_bg_color(line, COLOR_GOLD, 0);
        lv_obj_set_style_bg_opa(line, 170, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
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
            lv_color_t col = active ? COLOR_GOLD : COLOR_DIM;

            lv_obj_t *btn = lv_btn_create(bar);
            lv_obj_remove_style_all(btn);
            lv_obj_add_style(btn, getStyleIconBtn(), 0);
            lv_obj_set_size(btn, BTN_W, NAV_H - 4);
            lv_obj_set_pos(btn, i * BTN_W, 4);
            lv_obj_add_event_cb(btn, onNavClick, LV_EVENT_PRESSED, (void *)(intptr_t)i);

            lv_obj_t *icon = lv_obj_create(btn);
            lv_obj_remove_style_all(icon);
            lv_obj_add_style(icon, getStyleTransparent(), 0);
            lv_obj_set_size(icon, UiTheme::ICON_SIZE_NAV, UiTheme::ICON_SIZE_NAV);
            lv_obj_align(icon, LV_ALIGN_CENTER, 0, -4);
            lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_opa(icon, active ? UiTheme::ICON_OPA_ACTIVE : UiTheme::ICON_OPA_DEFAULT, 0);

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
    void setMuteToggleCallback(MuteToggleCallback cb)
    {
        muteToggleCallback = cb;
    }

    void updateStatusBarCity(const StatusBarHandles &h, const char *city, const char *dateAbbrev)
    {
        if (h.lbl_city && city)
            lv_label_set_text(h.lbl_city, LocaleTR::toUpperTR(city));
        (void)dateAbbrev;
    }

    void updateStatusBarMute(const StatusBarHandles &h, bool muted)
    {
        if (!h.mute_btn || !h.lbl_mute_icon)
            return;

        if (muted)
        {
            lv_obj_set_style_bg_color(h.mute_btn, COLOR_BG2, 0);
            lv_obj_set_style_bg_opa(h.mute_btn, 220, 0);
            lv_obj_set_style_border_color(h.mute_btn, COLOR_AMBER, 0);
            lv_obj_set_style_border_opa(h.mute_btn, 96, 0);
            lv_obj_set_style_opa(h.lbl_mute_icon, UiTheme::ICON_OPA_ACTIVE, 0);
            UiIcons::drawSpeakerIcon(h.lbl_mute_icon, true, COLOR_AMBER, true);
        }
        else
        {
            lv_obj_set_style_bg_color(h.mute_btn, COLOR_BG2, 0);
            lv_obj_set_style_bg_opa(h.mute_btn, 200, 0);
            lv_obj_set_style_border_color(h.mute_btn, COLOR_BORDER, 0);
            lv_obj_set_style_border_opa(h.mute_btn, 64, 0);
            lv_obj_set_style_opa(h.lbl_mute_icon, UiTheme::ICON_OPA_DEFAULT, 0);
            UiIcons::drawSpeakerIcon(h.lbl_mute_icon, false, COLOR_DIM, true);
        }
    }

    void updateStatusBarWifi(const StatusBarHandles &h, uint8_t bars)
    {
        if (!h.lbl_wifi)
            return;
        // Redraw vector icon to avoid font-glyph dependency.
        if (bars == 0)
        {
            UiIcons::drawWiFiIcon(h.lbl_wifi, COLOR_DIM, false);
            lv_obj_set_style_opa(h.lbl_wifi, 130, 0);
        }
        else
        {
            UiIcons::drawWiFiIcon(h.lbl_wifi, COLOR_GREEN, true);
            lv_obj_set_style_opa(h.lbl_wifi, (bars >= 2) ? 255 : 180, 0);
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
