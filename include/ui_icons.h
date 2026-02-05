/**
 * @file ui_icons.h
 * @brief Icon Point Arrays and Drawing Functions
 *
 * All icons: 28x28 base size, 2px stroke, rounded ends
 */

#ifndef UI_ICONS_H
#define UI_ICONS_H

#include <lvgl.h>

namespace UiIcons
{
    // Core drawing helper
    lv_obj_t *drawLine(lv_obj_t *parent, lv_point_t *pts, uint16_t cnt, lv_color_t col);

    // Icon drawing functions
    void drawHomeIcon(lv_obj_t *cont, lv_color_t col);
    void drawMosqueIcon(lv_obj_t *cont, lv_color_t col);
    void drawMenuIcon(lv_obj_t *cont, lv_color_t col);
    void drawSpeakerIcon(lv_obj_t *cont, bool isMuted, lv_color_t col, bool available = true);
    void drawWiFiIcon(lv_obj_t *cont, lv_color_t col, bool connected = true);
    void drawSyncIcon(lv_obj_t *cont, lv_color_t col, bool synced = true);

} // namespace UiIcons

#endif // UI_ICONS_H
