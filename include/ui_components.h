/**
 * @file ui_components.h
 * @brief Reusable UI Components
 */

#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <lvgl.h>

// Background pattern tile (generated once in createSharedAssets, defined in ui_components.cpp)
extern lv_img_dsc_t bg_pattern_dsc;

// Status bar leading icon type
enum class StatusBarIcon
{
    LOCATION, // GPS pin (Home, Clock pages)
    SETTINGS  // Gear icon (Settings page)
};

namespace UiComponents
{
    // ── Utility ───────────────────────────────────────────────────────
    // Zero out scroll, border, padding and bg on a plain container.
    inline void noScrollNoBorder(lv_obj_t *o)
    {
        lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_border_width(o, 0, 0);
        lv_obj_set_style_pad_all(o, 0, 0);
        lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    }

    /// Recursively add GESTURE_BUBBLE to all children so swipe gestures
    /// propagate up to the screen-level gesture handler.
    inline void bubbleGestures(lv_obj_t *parent)
    {
        uint32_t cnt = lv_obj_get_child_cnt(parent);
        for (uint32_t i = 0; i < cnt; i++)
        {
            lv_obj_t *child = lv_obj_get_child(parent, i);
            lv_obj_add_flag(child, LV_OBJ_FLAG_GESTURE_BUBBLE);
            bubbleGestures(child);
        }
    }

    // ── Status bar ────────────────────────────────────────────────────
    // Mutable widget handles returned so each screen can call setters.
    struct StatusBarHandles
    {
        lv_obj_t *lbl_city;
        lv_obj_t *lbl_dateabbr;
        lv_obj_t *mute_btn;
        lv_obj_t *lbl_mute_icon;
        lv_obj_t *lbl_mute_text;
    };

    using MuteToggleCallback = void (*)();

    // Build the 22 px status bar (city+date left, wifi+battery right).
    // icon: LOCATION shows GPS pin, SETTINGS shows gear icon.
    StatusBarHandles createStatusBar(lv_obj_t *parent, StatusBarIcon icon = StatusBarIcon::LOCATION);

    // Gold radial ambient glow overlay at top of screen (matches HTML .amb)
    void createAmbientGlow(lv_obj_t *parent);

    // Build the gold gradient separator line at y=25.
    void createSeparator(lv_obj_t *parent);

    // ── Navigation bar ────────────────────────────────────────────────
    using NavClickCallback = void (*)(int page);
    void setNavClickCallback(NavClickCallback cb);

    // 3-icon bottom nav bar (Home / Mosque / Menu) with tap callbacks.
    // activePage 0-2 determines which icon is highlighted.
    void createNavBar(lv_obj_t *parent, int activePage);

    // Nav-dot position indicator pills (bottom-centre).
    // activePage 0-based determines which dot is wide+bright.
    // numDots: total number of dots (default 3 = Home/Clock/Settings).
    void createNavDots(lv_obj_t *parent, int activePage, int numDots = 3);

    // ── Shared assets ─────────────────────────────────────────────
    // Call once after lv_init() to generate bg_pattern_dsc motif tile.
    void createSharedAssets();

    // Apply the tiled Islamic motif overlay to a screen object.
    void applyMotif(lv_obj_t *scr);

    // ── Shared status-bar data setters ────────────────────────────
    // Operate on handles returned by createStatusBar().
    void setMuteToggleCallback(MuteToggleCallback cb);
    void updateStatusBarCity(const StatusBarHandles &h, const char *city, const char *dateAbbrev);
    void updateStatusBarMute(const StatusBarHandles &h, bool muted);

} // namespace UiComponents

#endif // UI_COMPONENTS_H
