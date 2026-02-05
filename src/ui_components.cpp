/**
 * @file ui_components.cpp
 * @brief Reusable UI Components - Nav Bar
 */

#include "ui_components.h"
#include "ui_theme.h"
#include "ui_icons.h"

namespace UiComponents
{
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
        lv_obj_set_size(bar, 240, NAV_H);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

        // Top border line
        lv_obj_t *line = lv_obj_create(bar);
        lv_obj_set_size(line, 240, 1);
        lv_obj_set_pos(line, 0, 0);
        lv_obj_set_style_bg_color(line, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);
        lv_obj_set_style_border_width(line, 0, 0);

        for (int i = 0; i < 3; i++)
        {
            bool active = (i == activePage);
            lv_color_t col = active ? COLOR_ACCENT_BRIGHT : COLOR_DIM;

            lv_obj_t *btn = lv_btn_create(bar);
            lv_obj_remove_style_all(btn);
            lv_obj_add_style(btn, getStyleIconBtn(), 0);
            lv_obj_set_size(btn, 80, NAV_H - 4);
            lv_obj_set_pos(btn, i * 80, 4);
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

} // namespace UiComponents
