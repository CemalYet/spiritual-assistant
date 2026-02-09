/**
 * @file ui_icons.cpp
 * @brief Icon Point Arrays and Drawing Functions
 */

#include "ui_icons.h"
#include "ui_theme.h"

namespace UiIcons
{
    // ═══════════════════════════════════════════════════════════════
    // ICON POINT ARRAYS - 2px stroke, rounded ends, 28x28 base
    // ═══════════════════════════════════════════════════════════════

    // Home icon - simple house outline
    static lv_point_t home_roof[] = {{14, 2}, {2, 12}, {6, 12}};
    static lv_point_t home_roof2[] = {{14, 2}, {26, 12}, {22, 12}};
    static lv_point_t home_body[] = {{6, 12}, {6, 26}, {22, 26}, {22, 12}};
    static lv_point_t home_door[] = {{11, 26}, {11, 18}, {17, 18}, {17, 26}};

    // Menu icon - 3 horizontal lines (hamburger)
    static lv_point_t menu_line1[] = {{4, 7}, {24, 7}};
    static lv_point_t menu_line2[] = {{4, 14}, {24, 14}};
    static lv_point_t menu_line3[] = {{4, 21}, {24, 21}};

    // Speaker icon - clean wedge shape with sound waves
    static lv_point_t speaker_body[] = {{4, 11}, {8, 11}, {14, 6}, {14, 22}, {8, 17}, {4, 17}, {4, 11}};
    static lv_point_t speaker_wave1[] = {{17, 11}, {19, 14}, {17, 17}};
    static lv_point_t speaker_wave2[] = {{20, 8}, {24, 14}, {20, 20}};
    static lv_point_t speaker_mute_x[] = {{17, 9}, {25, 19}};
    static lv_point_t speaker_mute_x2[] = {{17, 19}, {25, 9}};

    // Sync/Check icon
    static lv_point_t check_mark[] = {{4, 14}, {10, 20}, {24, 6}};

    // ═══════════════════════════════════════════════════════════════
    // DRAWING HELPERS
    // ═══════════════════════════════════════════════════════════════

    lv_obj_t *drawLine(lv_obj_t *parent, lv_point_t *pts, uint16_t cnt, lv_color_t col)
    {
        lv_obj_t *ln = lv_line_create(parent);
        lv_line_set_points(ln, pts, cnt);
        lv_obj_set_style_line_color(ln, col, 0);
        lv_obj_set_style_line_width(ln, 2, 0);
        lv_obj_set_style_line_rounded(ln, true, 0);
        return ln;
    }

    void drawHomeIcon(lv_obj_t *cont, lv_color_t col)
    {
        drawLine(cont, home_roof, 3, col);
        drawLine(cont, home_roof2, 3, col);
        drawLine(cont, home_body, 4, col);
        drawLine(cont, home_door, 4, col);
    }

    void drawMosqueIcon(lv_obj_t *cont, lv_color_t col)
    {
        lv_obj_clean(cont);

        // Left minaret
        lv_obj_t *minaret_l = lv_obj_create(cont);
        lv_obj_set_size(minaret_l, 4, 20);
        lv_obj_set_pos(minaret_l, 4, 10);
        lv_obj_set_style_bg_color(minaret_l, col, 0);
        lv_obj_set_style_bg_opa(minaret_l, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(minaret_l, 0, 0);
        lv_obj_set_style_radius(minaret_l, 1, 0);

        // Right minaret
        lv_obj_t *minaret_r = lv_obj_create(cont);
        lv_obj_set_size(minaret_r, 4, 20);
        lv_obj_set_pos(minaret_r, 24, 10);
        lv_obj_set_style_bg_color(minaret_r, col, 0);
        lv_obj_set_style_bg_opa(minaret_r, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(minaret_r, 0, 0);
        lv_obj_set_style_radius(minaret_r, 1, 0);

        // Central dome
        lv_obj_t *dome = lv_arc_create(cont);
        lv_obj_set_size(dome, 18, 18);
        lv_obj_set_pos(dome, 7, 8);
        lv_arc_set_rotation(dome, 180);
        lv_arc_set_bg_angles(dome, 0, 180);
        lv_arc_set_value(dome, 100);
        lv_obj_remove_style(dome, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(dome, 3, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(dome, col, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(dome, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(dome, LV_OBJ_FLAG_CLICKABLE);

        // Small crescent on top
        lv_obj_t *crescent = lv_arc_create(cont);
        lv_obj_set_size(crescent, 8, 8);
        lv_obj_set_pos(crescent, 12, 0);
        lv_arc_set_rotation(crescent, 300);
        lv_arc_set_bg_angles(crescent, 0, 180);
        lv_arc_set_value(crescent, 100);
        lv_obj_remove_style(crescent, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(crescent, 2, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(crescent, col, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(crescent, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(crescent, LV_OBJ_FLAG_CLICKABLE);

        // Base
        lv_obj_t *base = lv_obj_create(cont);
        lv_obj_set_size(base, 28, 3);
        lv_obj_set_pos(base, 2, 28);
        lv_obj_set_style_bg_color(base, col, 0);
        lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(base, 0, 0);
        lv_obj_set_style_radius(base, 1, 0);
    }

    void drawMenuIcon(lv_obj_t *cont, lv_color_t col)
    {
        drawLine(cont, menu_line1, 2, col);
        drawLine(cont, menu_line2, 2, col);
        drawLine(cont, menu_line3, 2, col);
    }

    void drawSpeakerIcon(lv_obj_t *cont, bool isMuted, lv_color_t col, bool available)
    {
        lv_obj_clean(cont);
        drawLine(cont, speaker_body, 7, col);

        if (!available)
        {
            static lv_point_t speaker_unavailable[] = {{2, 2}, {26, 26}};
            drawLine(cont, speaker_unavailable, 2, col);
            return;
        }
        if (isMuted)
        {
            drawLine(cont, speaker_mute_x, 2, col);
            drawLine(cont, speaker_mute_x2, 2, col);
            return;
        }
        // Sound on - show waves
        drawLine(cont, speaker_wave1, 3, col);
        drawLine(cont, speaker_wave2, 3, col);
    }

    void drawWiFiIcon(lv_obj_t *cont, lv_color_t col, bool connected)
    {
        lv_obj_clean(cont);

        // Outer arc (largest)
        lv_obj_t *arc1 = lv_arc_create(cont);
        lv_obj_set_size(arc1, 26, 26);
        lv_obj_set_pos(arc1, 3, 0);
        lv_arc_set_rotation(arc1, 225);
        lv_arc_set_bg_angles(arc1, 0, 90);
        lv_arc_set_value(arc1, 100);
        lv_obj_remove_style(arc1, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc1, 2, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc1, col, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc1, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(arc1, LV_OBJ_FLAG_CLICKABLE);

        // Middle arc
        lv_obj_t *arc2 = lv_arc_create(cont);
        lv_obj_set_size(arc2, 18, 18);
        lv_obj_set_pos(arc2, 7, 4);
        lv_arc_set_rotation(arc2, 225);
        lv_arc_set_bg_angles(arc2, 0, 90);
        lv_arc_set_value(arc2, 100);
        lv_obj_remove_style(arc2, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc2, 2, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc2, col, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc2, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(arc2, LV_OBJ_FLAG_CLICKABLE);

        // Inner arc (smallest)
        lv_obj_t *arc3 = lv_arc_create(cont);
        lv_obj_set_size(arc3, 10, 10);
        lv_obj_set_pos(arc3, 11, 8);
        lv_arc_set_rotation(arc3, 225);
        lv_arc_set_bg_angles(arc3, 0, 90);
        lv_arc_set_value(arc3, 100);
        lv_obj_remove_style(arc3, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(arc3, 2, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc3, col, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc3, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_clear_flag(arc3, LV_OBJ_FLAG_CLICKABLE);

        // Center dot
        lv_obj_t *dot = lv_obj_create(cont);
        lv_obj_set_size(dot, 4, 4);
        lv_obj_set_pos(dot, 14, 15);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, col, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);

        // If not connected, draw diagonal cross
        if (!connected)
        {
            static lv_point_t wifi_cross[] = {{6, 2}, {26, 22}};
            drawLine(cont, wifi_cross, 2, col);
        }
    }

    void drawSyncIcon(lv_obj_t *cont, lv_color_t col, bool synced)
    {
        lv_obj_clean(cont);
        drawLine(cont, check_mark, 3, col);

        if (!synced)
        {
            static lv_point_t sync_cross[] = {{4, 4}, {28, 28}};
            drawLine(cont, sync_cross, 2, col);
        }
    }

} // namespace UiIcons
