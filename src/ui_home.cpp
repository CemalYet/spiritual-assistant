/**
 * @file ui_home.cpp
 * @brief HOME Screen - Premium Dark Dashboard
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
 * - Static lv_style_t for efficiency
 */

#include "ui_home.h"
#include <Arduino.h>

// Font declarations
LV_FONT_DECLARE(lv_font_montserrat_48);
LV_FONT_DECLARE(lv_font_montserrat_32);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_12);

namespace UiHome
{
    // ═══════════════════════════════════════════════════════════════
    // STATIC STYLES - Initialized once, reused for efficiency
    // ═══════════════════════════════════════════════════════════════
    static lv_style_t style_screen;
    static lv_style_t style_title;      // Clock
    static lv_style_t style_subtitle;   // Date
    static lv_style_t style_card;       // Prayer card
    static lv_style_t style_card_label; // Card prayer name
    static lv_style_t style_card_time;  // Card time
    static lv_style_t style_icon_bar;   // Bottom nav
    static lv_style_t style_icon_btn;   // Nav button
    static lv_style_t style_indicator;  // Active indicator
    static lv_style_t style_transparent;
    static bool styles_initialized = false;

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

    // ═══════════════════════════════════════════════════════════════
    // LAYOUT CONSTANTS - Consistent Spacing Rhythm
    // ═══════════════════════════════════════════════════════════════
    static constexpr int16_t SPACING_SM = 8;  // Small spacing
    static constexpr int16_t SPACING_MD = 16; // Medium spacing
    static constexpr int16_t SPACING_LG = 24; // Large spacing

    static constexpr int16_t CLOCK_Y = 32;       // Hero clock position
    static constexpr int16_t DATE_Y = 92;        // Date below clock (32+48+12)
    static constexpr int16_t CARD_Y = 120;       // Prayer card position
    static constexpr int16_t CARD_W = 208;       // Card width (240-32 margin)
    static constexpr int16_t CARD_H = 64;        // Card height (smaller)
    static constexpr int16_t CARD_RADIUS = 10;   // Design guideline radius
    static constexpr int16_t NAV_H = 64;         // Navigation bar height (larger for touch)
    static constexpr int16_t NAV_ICON_SIZE = 32; // Nav icon size (larger)
    static constexpr int16_t STATUS_Y = 195;     // Status icons - between card and nav

    // ═══════════════════════════════════════════════════════════════
    // UI ELEMENTS
    // ═══════════════════════════════════════════════════════════════
    static lv_obj_t *scr = nullptr;
    static lv_obj_t *lbl_clock = nullptr;
    static lv_obj_t *lbl_date = nullptr;
    static lv_obj_t *prayer_card = nullptr;
    static lv_obj_t *lbl_prayer_name = nullptr;
    static lv_obj_t *lbl_prayer_time = nullptr;

    static lv_obj_t *icon_wifi = nullptr;
    static lv_obj_t *icon_sync = nullptr;
    static lv_obj_t *btn_mute = nullptr;
    static lv_obj_t *icon_mute = nullptr;

    static lv_obj_t *nav_bar = nullptr;
    static lv_obj_t *nav_btns[3] = {nullptr};
    static lv_obj_t *nav_icons[3] = {nullptr};
    static lv_obj_t *nav_indicators[3] = {nullptr};

    // Prayer times page elements
    static lv_obj_t *prayer_scr = nullptr;
    static lv_obj_t *prayer_nav_bar = nullptr;
    static lv_obj_t *prayer_nav_btns[3] = {nullptr};
    static lv_obj_t *prayer_nav_icons[3] = {nullptr};
    static lv_obj_t *prayer_nav_indicators[3] = {nullptr};
    static lv_obj_t *prayer_time_labels[6] = {nullptr};
    static int currentNextPrayer = -1;

    // ═══════════════════════════════════════════════════════════════
    // CUSTOM ICON PATHS - 2px stroke, rounded ends, 28x28 base
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
    // STATE
    // ═══════════════════════════════════════════════════════════════
    static bool muted = false;
    static bool wifiConnected = false;
    static bool synced = false;
    static bool adhanAvailable = false; // Whether adhan sound file exists (set true when verified)
    static int activeNavPage = 0;
    static NavCallback navCallback = nullptr;

    // State tracking for partial updates (avoid full redraws)
    static int lastHour = -1;
    static int lastMinute = -1;
    static char lastPrayerName[32] = "";
    static char lastPrayerTime[16] = "";
    static char lastDate[64] = "";

    // ═══════════════════════════════════════════════════════════════
    // STYLE INITIALIZATION - Call once before UI creation
    // ═══════════════════════════════════════════════════════════════
    static void initStyles()
    {
        if (styles_initialized)
            return;

        // Screen: dark background, pure white text for TN
        lv_style_init(&style_screen);
        lv_style_set_bg_color(&style_screen, lv_color_hex(0x161616));
        lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
        lv_style_set_text_color(&style_screen, lv_color_hex(0xFFFFFF));
        lv_style_set_pad_all(&style_screen, 0);

        // Title (clock): pure white, large
        lv_style_init(&style_title);
        lv_style_set_text_color(&style_title, lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&style_title, &lv_font_montserrat_48);
        lv_style_set_text_letter_space(&style_title, 2);
        lv_style_set_text_align(&style_title, LV_TEXT_ALIGN_CENTER);

        // Subtitle (date): Dimmer for hierarchy, larger for TN readability
        lv_style_init(&style_subtitle);
        lv_style_set_text_color(&style_subtitle, lv_color_hex(0xAAAAAA));
        lv_style_set_text_font(&style_subtitle, &lv_font_montserrat_16);
        lv_style_set_text_letter_space(&style_subtitle, 1);
        lv_style_set_text_align(&style_subtitle, LV_TEXT_ALIGN_CENTER);

        // Card: darker BG, sky-600 border (high-end look)
        lv_style_init(&style_card);
        lv_style_set_radius(&style_card, CARD_RADIUS);
        lv_style_set_bg_color(&style_card, lv_color_hex(0x1E1E1E)); // Darker for contrast
        lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
        lv_style_set_border_width(&style_card, 2);                      // Thicker border
        lv_style_set_border_color(&style_card, lv_color_hex(0x0284C7)); // Sky-600 darker accent
        lv_style_set_border_opa(&style_card, LV_OPA_COVER);
        lv_style_set_pad_all(&style_card, 12);
        lv_style_set_shadow_width(&style_card, 0); // Disabled for TN
        lv_style_set_shadow_color(&style_card, lv_color_hex(0x000000));
        lv_style_set_shadow_opa(&style_card, LV_OPA_TRANSP);
        lv_style_set_shadow_ofs_y(&style_card, 0);

        // Card label: neutral gray for hierarchy (time is primary)
        lv_style_init(&style_card_label);
        lv_style_set_text_color(&style_card_label, lv_color_hex(0x9CA3AF)); // Gray-400 subdued
        lv_style_set_text_font(&style_card_label, &lv_font_montserrat_14);
        lv_style_set_text_letter_space(&style_card_label, 2);

        // Card time: bright white for emphasis (focus on time)
        lv_style_init(&style_card_time);
        lv_style_set_text_color(&style_card_time, lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&style_card_time, &lv_font_montserrat_32);

        // Icon bar: same as background
        lv_style_init(&style_icon_bar);
        lv_style_set_bg_color(&style_icon_bar, lv_color_hex(0x161616));
        lv_style_set_bg_opa(&style_icon_bar, LV_OPA_COVER);
        lv_style_set_border_width(&style_icon_bar, 0);
        lv_style_set_pad_all(&style_icon_bar, 0);

        // Icon button: transparent
        lv_style_init(&style_icon_btn);
        lv_style_set_bg_opa(&style_icon_btn, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_icon_btn, 0);
        lv_style_set_shadow_width(&style_icon_btn, 0);
        lv_style_set_pad_all(&style_icon_btn, 6);
        lv_style_set_radius(&style_icon_btn, 8);

        // Active indicator: sky-500 accent
        lv_style_init(&style_indicator);
        lv_style_set_bg_color(&style_indicator, lv_color_hex(0x0EA5E9));
        lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
        lv_style_set_radius(&style_indicator, 2);
        lv_style_set_border_width(&style_indicator, 0);

        // Transparent container
        lv_style_init(&style_transparent);
        lv_style_set_bg_opa(&style_transparent, LV_OPA_TRANSP);
        lv_style_set_border_width(&style_transparent, 0);
        lv_style_set_pad_all(&style_transparent, 0);

        styles_initialized = true;
    }

    // ═══════════════════════════════════════════════════════════════
    // ICON DRAWING HELPERS - 2px rounded stroke
    // ═══════════════════════════════════════════════════════════════
    static lv_obj_t *drawLine(lv_obj_t *parent, lv_point_t *pts, uint16_t cnt, lv_color_t col)
    {
        lv_obj_t *ln = lv_line_create(parent);
        lv_line_set_points(ln, pts, cnt);
        lv_obj_set_style_line_color(ln, col, 0);
        lv_obj_set_style_line_width(ln, 2, 0);
        lv_obj_set_style_line_rounded(ln, true, 0);
        return ln;
    }

    static void drawHomeIcon(lv_obj_t *cont, lv_color_t col)
    {
        drawLine(cont, home_roof, 3, col);
        drawLine(cont, home_roof2, 3, col);
        drawLine(cont, home_body, 4, col);
        drawLine(cont, home_door, 4, col);
    }

    static void drawMosqueIcon(lv_obj_t *cont, lv_color_t col)
    {
        lv_obj_clean(cont);

        // Minaret with dome and crescent - classic mosque silhouette
        // 32x32 container

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

    // Draw speaker icon
    // isMuted: true = show X (muted), false = show waves (playing)
    // available: false = show diagonal line (adhan file not available)
    static void drawSpeakerIcon(lv_obj_t *cont, bool isMuted, lv_color_t col, bool available = true)
    {
        lv_obj_clean(cont);
        drawLine(cont, speaker_body, 7, col);

        if (!available)
        {
            // Adhan not available - show diagonal cross line
            static lv_point_t speaker_unavailable[] = {{2, 2}, {26, 26}};
            drawLine(cont, speaker_unavailable, 2, col);
        }
        else if (isMuted)
        {
            // Muted - show X
            drawLine(cont, speaker_mute_x, 2, col);
            drawLine(cont, speaker_mute_x2, 2, col);
        }
        else
        {
            // Sound on - show waves
            drawLine(cont, speaker_wave1, 3, col);
            drawLine(cont, speaker_wave2, 3, col);
        }
    }

    static void drawMenuIcon(lv_obj_t *cont, lv_color_t col)
    {
        drawLine(cont, menu_line1, 2, col);
        drawLine(cont, menu_line2, 2, col);
        drawLine(cont, menu_line3, 2, col);
    }

    static void drawWiFiIcon(lv_obj_t *cont, lv_color_t col, bool connected = true)
    {
        lv_obj_clean(cont);

        // Professional WiFi icon - compact, centered design (32x32 container)
        // All arcs centered at bottom-center, dot directly below smallest arc

        // Outer arc (largest) - centered horizontally
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

        // Center dot - directly below smallest arc
        lv_obj_t *dot = lv_obj_create(cont);
        lv_obj_set_size(dot, 4, 4);
        lv_obj_set_pos(dot, 14, 15); // Closer to arcs
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, col, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);

        // If not connected, draw diagonal cross line through center
        if (!connected)
        {
            static lv_point_t wifi_cross[] = {{6, 2}, {26, 22}}; // Centered diagonal
            drawLine(cont, wifi_cross, 2, col);
        }
    }

    static void drawSyncIcon(lv_obj_t *cont, lv_color_t col, bool synced = true)
    {
        lv_obj_clean(cont);
        drawLine(cont, check_mark, 3, col);

        // If not synced, draw diagonal cross line (same pattern as WiFi)
        if (!synced)
        {
            static lv_point_t sync_cross[] = {{4, 4}, {28, 28}};
            drawLine(cont, sync_cross, 2, col);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // EVENT HANDLERS
    // ═══════════════════════════════════════════════════════════════
    static constexpr uint32_t DEBOUNCE_MS = 200; // Minimum time between clicks
    static uint32_t lastMuteClick = 0;
    static uint32_t lastNavClick = 0;

    static void onMuteClick(lv_event_t *e)
    {
        uint32_t now = lv_tick_get();
        if (now - lastMuteClick < DEBOUNCE_MS)
            return;
        lastMuteClick = now;

        muted = !muted;
        drawSpeakerIcon(icon_mute, muted, muted ? COLOR_DIM : COLOR_ACCENT);
    }

    static void onNavClick(lv_event_t *e)
    {
        uint32_t now = lv_tick_get();
        if (now - lastNavClick < DEBOUNCE_MS)
            return;
        lastNavClick = now;

        int page = (int)(intptr_t)lv_event_get_user_data(e);
        setActiveNav(page);

        // Navigate to page
        if (page == 0)
            showHomePage();
        else if (page == 1)
            showPrayerTimesPage();
        // page 2 = Menu (not implemented yet)

        if (navCallback)
            navCallback(page);
    }

    // ═══════════════════════════════════════════════════════════════
    // UI CREATION - Premium Dark Dashboard Style
    // ═══════════════════════════════════════════════════════════════
    static void createHeroClock()
    {
        // Large bold clock - the star of the screen
        lbl_clock = lv_label_create(scr);
        lv_obj_add_style(lbl_clock, &style_title, 0);
        lv_label_set_text(lbl_clock, "00:00");
        lv_obj_align(lbl_clock, LV_ALIGN_TOP_MID, 0, CLOCK_Y);
    }

    static void createDateLabel()
    {
        // Date below clock - bright for TN panel visibility
        lbl_date = lv_label_create(scr);
        lv_obj_add_style(lbl_date, &style_subtitle, 0);
        lv_label_set_text(lbl_date, "Friday, Jan 30");
        lv_obj_align(lbl_date, LV_ALIGN_TOP_MID, 0, DATE_Y);
    }

    static void createPrayerCard()
    {
        // Card with cyan border and shadow
        prayer_card = lv_obj_create(scr);
        lv_obj_remove_style_all(prayer_card);
        lv_obj_add_style(prayer_card, &style_card, 0);
        lv_obj_set_size(prayer_card, CARD_W, CARD_H);
        lv_obj_align(prayer_card, LV_ALIGN_TOP_MID, 0, CARD_Y);
        lv_obj_clear_flag(prayer_card, LV_OBJ_FLAG_SCROLLABLE);

        // Prayer name - uppercase, left aligned (just the prayer name like "AKSAM")
        lbl_prayer_name = lv_label_create(prayer_card);
        lv_obj_add_style(lbl_prayer_name, &style_card_label, 0);
        lv_label_set_text(lbl_prayer_name, "SABAH");
        lv_obj_align(lbl_prayer_name, LV_ALIGN_LEFT_MID, 0, 0);

        // Prayer time - bright cyan, right aligned
        lbl_prayer_time = lv_label_create(prayer_card);
        lv_obj_add_style(lbl_prayer_time, &style_card_time, 0);
        lv_label_set_text(lbl_prayer_time, "21:45");
        lv_obj_align(lbl_prayer_time, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    static void createStatusRow()
    {
        // Horizontal status row with WiFi, Sync, Mute icons
        lv_obj_t *status_row = lv_obj_create(scr);
        lv_obj_remove_style_all(status_row);
        lv_obj_add_style(status_row, &style_transparent, 0);
        lv_obj_set_size(status_row, 200, 48);
        lv_obj_align(status_row, LV_ALIGN_TOP_MID, 0, STATUS_Y);
        lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(status_row, SPACING_MD, 0);
        lv_obj_clear_flag(status_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        int16_t icon_size = 32; // Icon drawing size
        int16_t cont_size = 48; // Container size (same for all)

        // WiFi icon container (non-clickable, informational only)
        lv_obj_t *wifi_cont = lv_obj_create(status_row);
        lv_obj_remove_style_all(wifi_cont);
        lv_obj_add_style(wifi_cont, &style_transparent, 0);
        lv_obj_set_size(wifi_cont, cont_size, cont_size);
        lv_obj_clear_flag(wifi_cont, LV_OBJ_FLAG_CLICKABLE);

        // Inner icon container for centering
        lv_obj_t *wifi_icon = lv_obj_create(wifi_cont);
        lv_obj_remove_style_all(wifi_icon);
        lv_obj_add_style(wifi_icon, &style_transparent, 0);
        lv_obj_set_size(wifi_icon, icon_size, icon_size);
        lv_obj_center(wifi_icon);
        lv_obj_clear_flag(wifi_icon, LV_OBJ_FLAG_CLICKABLE);
        icon_wifi = wifi_icon;
        drawWiFiIcon(icon_wifi, wifiConnected ? COLOR_ACCENT : COLOR_DIM, wifiConnected);

        // Sync icon container (non-clickable, informational only)
        lv_obj_t *sync_outer = lv_obj_create(status_row);
        lv_obj_remove_style_all(sync_outer);
        lv_obj_add_style(sync_outer, &style_transparent, 0);
        lv_obj_set_size(sync_outer, cont_size, cont_size);
        lv_obj_clear_flag(sync_outer, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *sync_icon = lv_obj_create(sync_outer);
        lv_obj_remove_style_all(sync_icon);
        lv_obj_add_style(sync_icon, &style_transparent, 0);
        lv_obj_set_size(sync_icon, icon_size, icon_size);
        lv_obj_center(sync_icon);
        lv_obj_clear_flag(sync_icon, LV_OBJ_FLAG_CLICKABLE);
        icon_sync = sync_icon;
        drawSyncIcon(icon_sync, synced ? COLOR_ACCENT : COLOR_DIM, synced);

        // Mute button with extended touch area
        btn_mute = lv_btn_create(status_row);
        lv_obj_remove_style_all(btn_mute);
        lv_obj_add_style(btn_mute, &style_icon_btn, 0);
        lv_obj_set_size(btn_mute, cont_size, cont_size); // Same size as other icons
        lv_obj_set_ext_click_area(btn_mute, 8);          // Extra touch area
        lv_obj_add_flag(btn_mute, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn_mute, onMuteClick, LV_EVENT_PRESSED, nullptr);

        lv_obj_t *mute_cont = lv_obj_create(btn_mute);
        lv_obj_remove_style_all(mute_cont);
        lv_obj_add_style(mute_cont, &style_transparent, 0);
        lv_obj_set_size(mute_cont, icon_size, icon_size);
        lv_obj_center(mute_cont);
        lv_obj_clear_flag(mute_cont, LV_OBJ_FLAG_CLICKABLE); // Pass clicks to parent
        icon_mute = mute_cont;
        drawSpeakerIcon(icon_mute, muted, adhanAvailable ? COLOR_ACCENT : COLOR_DIM, adhanAvailable);
    }

    static void createBottomNav()
    {
        // Bottom navigation bar
        nav_bar = lv_obj_create(scr);
        lv_obj_remove_style_all(nav_bar);
        lv_obj_add_style(nav_bar, &style_icon_bar, 0);
        lv_obj_set_size(nav_bar, 240, NAV_H);
        lv_obj_align(nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_clear_flag(nav_bar, LV_OBJ_FLAG_SCROLLABLE);

        // Top border line
        lv_obj_t *top_line = lv_obj_create(nav_bar);
        lv_obj_set_size(top_line, 240, 1);
        lv_obj_set_pos(top_line, 0, 0);
        lv_obj_set_style_bg_color(top_line, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(top_line, LV_OPA_50, 0);
        lv_obj_set_style_border_width(top_line, 0, 0);

        int16_t btn_w = 80;

        for (int i = 0; i < 3; i++)
        {
            // Button with extended touch area
            lv_obj_t *btn = lv_btn_create(nav_bar);
            lv_obj_remove_style_all(btn);
            lv_obj_add_style(btn, &style_icon_btn, 0);
            lv_obj_set_size(btn, btn_w, NAV_H - 4);
            lv_obj_set_pos(btn, i * btn_w, 4);
            lv_obj_set_ext_click_area(btn, 0); // No extended area to avoid overlap
            lv_obj_add_event_cb(btn, onNavClick, LV_EVENT_PRESSED, (void *)(intptr_t)i);
            nav_btns[i] = btn;

            // Icon container - pass clicks to parent
            lv_obj_t *icon_cont = lv_obj_create(btn);
            lv_obj_remove_style_all(icon_cont);
            lv_obj_add_style(icon_cont, &style_transparent, 0);
            lv_obj_set_size(icon_cont, NAV_ICON_SIZE, NAV_ICON_SIZE);
            lv_obj_align(icon_cont, LV_ALIGN_CENTER, 0, -4);
            lv_obj_clear_flag(icon_cont, LV_OBJ_FLAG_CLICKABLE);
            nav_icons[i] = icon_cont;

            // Active indicator (small line below icon) - pass clicks to parent
            lv_obj_t *indicator = lv_obj_create(btn);
            lv_obj_remove_style_all(indicator);
            lv_obj_add_style(indicator, &style_indicator, 0);
            lv_obj_set_size(indicator, 24, 3);
            lv_obj_align(indicator, LV_ALIGN_BOTTOM_MID, 0, -4);
            lv_obj_set_style_bg_opa(indicator, (i == 0) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_clear_flag(indicator, LV_OBJ_FLAG_CLICKABLE);
            nav_indicators[i] = indicator;

            // Draw icons
            lv_color_t col = (i == 0) ? COLOR_ACCENT_BRIGHT : COLOR_DIM;
            if (i == 0)
                drawHomeIcon(icon_cont, col);
            else if (i == 1)
                drawMosqueIcon(icon_cont, col);
            else
                drawMenuIcon(icon_cont, col);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // PUBLIC API
    // ═══════════════════════════════════════════════════════════════
    void init()
    {
        // Initialize styles once
        initStyles();

        scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(scr);
        lv_obj_add_style(scr, &style_screen, 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        createHeroClock();
        createDateLabel();
        createPrayerCard();
        createStatusRow();
        createBottomNav();

        lv_scr_load(scr);
    }

    void setTime(int hour, int minute)
    {
        // Only update if changed (partial update)
        if (hour == lastHour && minute == lastMinute)
            return;
        lastHour = hour;
        lastMinute = minute;

        if (!lbl_clock)
            return;
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
        lv_label_set_text(lbl_clock, buf);
    }

    void setDate(const char *date)
    {
        if (!lbl_date || !date)
            return;
        // Only update if changed (partial update)
        if (strcmp(lastDate, date) == 0)
            return;
        strncpy(lastDate, date, sizeof(lastDate) - 1);
        lv_label_set_text(lbl_date, date);
    }

    void setCalculationMethod(const char *method)
    {
        // Not displayed on main screen - moved to menu
    }

    void setNextPrayer(const char *name, const char *time)
    {
        // Early return if no labels
        if (!lbl_prayer_name || !lbl_prayer_time)
            return;

        // Update prayer name (uppercase)
        if (name)
        {
            char buf[32];
            strncpy(buf, name, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            for (char *p = buf; *p; ++p)
                *p = toupper(*p);

            if (strcmp(lastPrayerName, buf) != 0)
            {
                strncpy(lastPrayerName, buf, sizeof(lastPrayerName) - 1);
                lv_label_set_text(lbl_prayer_name, buf);
            }
        }

        // Update prayer time
        if (time && strcmp(lastPrayerTime, time) != 0)
        {
            strncpy(lastPrayerTime, time, sizeof(lastPrayerTime) - 1);
            lv_label_set_text(lbl_prayer_time, time);
        }
    }

    void setWifiConnected(bool connected, int rssi)
    {
        if (!icon_wifi)
            return;

        wifiConnected = connected;

        // Color by RSSI: >-60=cyan(strong), -60 to -75=amber, <-75=red
        lv_color_t color = COLOR_DIM;
        if (connected)
        {
            color = (rssi > -60) ? COLOR_ACCENT : (rssi > -75) ? COLOR_AMBER
                                                               : COLOR_RED;
        }
        drawWiFiIcon(icon_wifi, color, connected);
    }

    void setNtpSynced(bool s)
    {
        if (!icon_sync || synced == s)
            return;
        synced = s;
        drawSyncIcon(icon_sync, s ? COLOR_ACCENT : COLOR_DIM, s);
    }

    bool isMuted() { return muted; }

    void setAdhanAvailable(bool available)
    {
        if (!icon_mute || adhanAvailable == available)
            return;
        adhanAvailable = available;
        lv_color_t col = available ? (muted ? COLOR_DIM : COLOR_ACCENT) : COLOR_DIM;
        drawSpeakerIcon(icon_mute, muted, col, available);
    }

    void setMuted(bool m)
    {
        if (!icon_mute || muted == m)
            return;
        muted = m;
        if (adhanAvailable)
        {
            drawSpeakerIcon(icon_mute, muted, muted ? COLOR_DIM : COLOR_ACCENT, true);
        }
    }

    void setNavCallback(NavCallback cb)
    {
        navCallback = cb;
    }

    void setActiveNav(int page)
    {
        if (page < 0 || page > 2)
            return;
        activeNavPage = page;

        for (int i = 0; i < 3; i++)
        {
            bool active = (i == page);
            lv_color_t col = active ? COLOR_ACCENT_BRIGHT : COLOR_DIM;

            // Update indicator
            if (nav_indicators[i])
                lv_obj_set_style_bg_opa(nav_indicators[i], active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

            // Redraw icon with new color
            if (!nav_icons[i])
                continue;
            lv_obj_clean(nav_icons[i]);
            switch (i)
            {
            case 0:
                drawHomeIcon(nav_icons[i], col);
                break;
            case 1:
                drawMosqueIcon(nav_icons[i], col);
                break;
            case 2:
                drawMenuIcon(nav_icons[i], col);
                break;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // STATUS SCREENS - Full screen overlays for setup/status
    // ═══════════════════════════════════════════════════════════════
    static lv_obj_t *status_scr = nullptr;

    static void createStatusScreen()
    {
        if (status_scr)
        {
            lv_obj_del(status_scr);
        }
        status_scr = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(status_scr, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(status_scr, LV_OPA_COVER, 0);
    }

    void showConnecting(const char *ssid)
    {
        createStatusScreen();

        // WiFi icon at top
        lv_obj_t *icon_cont = lv_obj_create(status_scr);
        lv_obj_remove_style_all(icon_cont);
        lv_obj_set_size(icon_cont, 64, 64);
        lv_obj_align(icon_cont, LV_ALIGN_CENTER, 0, -60);
        drawWiFiIcon(icon_cont, COLOR_ACCENT, false); // Dim - not connected yet

        // "Connecting..." label
        lv_obj_t *lbl_status = lv_label_create(status_scr);
        lv_label_set_text(lbl_status, "Baglaniyor...");
        lv_obj_set_style_text_color(lbl_status, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl_status, LV_ALIGN_CENTER, 0, 10);

        // SSID label
        lv_obj_t *lbl_ssid = lv_label_create(status_scr);
        lv_label_set_text(lbl_ssid, ssid ? ssid : "");
        lv_obj_set_style_text_color(lbl_ssid, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_ssid, LV_ALIGN_CENTER, 0, 35);

        lv_scr_load(status_scr);
    }

    void showPortal(const char *apName, const char *password, const char *ip)
    {
        createStatusScreen();

        // Title
        lv_obj_t *lbl_title = lv_label_create(status_scr);
        lv_label_set_text(lbl_title, "WiFi Ayari");
        lv_obj_set_style_text_color(lbl_title, COLOR_ACCENT_BRIGHT, 0);
        lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 30);

        // AP Name
        lv_obj_t *lbl_ap_title = lv_label_create(status_scr);
        lv_label_set_text(lbl_ap_title, "Ag Adi:");
        lv_obj_set_style_text_color(lbl_ap_title, COLOR_SUBTITLE, 0);
        lv_obj_set_style_text_font(lbl_ap_title, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_ap_title, LV_ALIGN_TOP_LEFT, 20, 70);

        lv_obj_t *lbl_ap = lv_label_create(status_scr);
        lv_label_set_text(lbl_ap, apName ? apName : "");
        lv_obj_set_style_text_color(lbl_ap, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl_ap, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_ap, LV_ALIGN_TOP_LEFT, 20, 88);

        // Password
        lv_obj_t *lbl_pw_title = lv_label_create(status_scr);
        lv_label_set_text(lbl_pw_title, "Sifre:");
        lv_obj_set_style_text_color(lbl_pw_title, COLOR_SUBTITLE, 0);
        lv_obj_set_style_text_font(lbl_pw_title, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_pw_title, LV_ALIGN_TOP_LEFT, 20, 120);

        lv_obj_t *lbl_pw = lv_label_create(status_scr);
        lv_label_set_text(lbl_pw, password ? password : "");
        lv_obj_set_style_text_color(lbl_pw, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl_pw, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_pw, LV_ALIGN_TOP_LEFT, 20, 138);

        // IP Address
        lv_obj_t *lbl_ip_title = lv_label_create(status_scr);
        lv_label_set_text(lbl_ip_title, "Adres:");
        lv_obj_set_style_text_color(lbl_ip_title, COLOR_SUBTITLE, 0);
        lv_obj_set_style_text_font(lbl_ip_title, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_ip_title, LV_ALIGN_TOP_LEFT, 20, 170);

        lv_obj_t *lbl_ip = lv_label_create(status_scr);
        lv_label_set_text(lbl_ip, ip ? ip : "");
        lv_obj_set_style_text_color(lbl_ip, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_14, 0);
        lv_obj_align(lbl_ip, LV_ALIGN_TOP_LEFT, 20, 188);

        // Instructions
        lv_obj_t *lbl_inst = lv_label_create(status_scr);
        lv_label_set_text(lbl_inst, "Telefonunuzdan\nbu aga baglanin");
        lv_obj_set_style_text_color(lbl_inst, COLOR_DIM, 0);
        lv_obj_set_style_text_font(lbl_inst, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_align(lbl_inst, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lbl_inst, LV_ALIGN_BOTTOM_MID, 0, -40);

        lv_scr_load(status_scr);
    }

    void showMessage(const char *line1, const char *line2)
    {
        createStatusScreen();

        // Line 1
        lv_obj_t *lbl1 = lv_label_create(status_scr);
        lv_label_set_text(lbl1, line1 ? line1 : "");
        lv_obj_set_style_text_color(lbl1, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl1, LV_ALIGN_CENTER, 0, line2 ? -15 : 0);

        // Line 2 (optional)
        if (line2)
        {
            lv_obj_t *lbl2 = lv_label_create(status_scr);
            lv_label_set_text(lbl2, line2);
            lv_obj_set_style_text_color(lbl2, COLOR_SUBTITLE, 0);
            lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_14, 0);
            lv_obj_align(lbl2, LV_ALIGN_CENTER, 0, 15);
        }

        lv_scr_load(status_scr);
    }

    void showError(const char *line1, const char *line2)
    {
        createStatusScreen();

        // Error icon (red X) - simple cross
        lv_obj_t *icon_cont = lv_obj_create(status_scr);
        lv_obj_remove_style_all(icon_cont);
        lv_obj_set_size(icon_cont, 48, 48);
        lv_obj_align(icon_cont, LV_ALIGN_CENTER, 0, -50);

        lv_color_t errorColor = lv_color_hex(0xFF4444);
        static lv_point_t x1[] = {{4, 4}, {44, 44}};
        static lv_point_t x2[] = {{44, 4}, {4, 44}};
        drawLine(icon_cont, x1, 2, errorColor);
        drawLine(icon_cont, x2, 2, errorColor);

        // Line 1 (error title)
        lv_obj_t *lbl1 = lv_label_create(status_scr);
        lv_label_set_text(lbl1, line1 ? line1 : "Hata");
        lv_obj_set_style_text_color(lbl1, lv_color_hex(0xFF4444), 0);
        lv_obj_set_style_text_font(lbl1, &lv_font_montserrat_16, 0);
        lv_obj_align(lbl1, LV_ALIGN_CENTER, 0, 10);

        // Line 2 (optional)
        if (line2)
        {
            lv_obj_t *lbl2 = lv_label_create(status_scr);
            lv_label_set_text(lbl2, line2);
            lv_obj_set_style_text_color(lbl2, COLOR_SUBTITLE, 0);
            lv_obj_set_style_text_font(lbl2, &lv_font_montserrat_14, 0);
            lv_obj_align(lbl2, LV_ALIGN_CENTER, 0, 35);
        }

        lv_scr_load(status_scr);
    }

    // ═══════════════════════════════════════════════════════════════
    // PRAYER TIMES PAGE - Shows all daily prayer times
    // ═══════════════════════════════════════════════════════════════
    static const char *PRAYER_NAMES[6] = {"SABAH", "GUNES", "OGLE", "IKINDI", "AKSAM", "YATSI"};
    static PrayerTimesData cachedPrayerTimes = {"--:--", "--:--", "--:--", "--:--", "--:--", "--:--", -1};

    static void onPrayerNavClick(lv_event_t *e)
    {
        uint32_t now = lv_tick_get();
        if (now - lastNavClick < DEBOUNCE_MS)
            return;
        lastNavClick = now;

        int page = (int)(intptr_t)lv_event_get_user_data(e);
        setActiveNav(page);
        if (page == 0)
            showHomePage();
        else if (page == 1)
            showPrayerTimesPage();
        // page 2 = Menu (not implemented yet)
    }

    static void createPrayerTimesPage()
    {
        // Delete existing page to rebuild with updated data
        if (prayer_scr)
        {
            lv_obj_del(prayer_scr);
            prayer_scr = nullptr;
            prayer_nav_bar = nullptr;
            for (int i = 0; i < 3; i++)
            {
                prayer_nav_btns[i] = nullptr;
                prayer_nav_icons[i] = nullptr;
                prayer_nav_indicators[i] = nullptr;
            }
            for (int i = 0; i < 6; i++)
            {
                prayer_time_labels[i] = nullptr;
            }
        }
        prayer_scr = lv_obj_create(nullptr);
        lv_obj_remove_style_all(prayer_scr);
        lv_obj_add_style(prayer_scr, &style_screen, 0);
        lv_obj_clear_flag(prayer_scr, LV_OBJ_FLAG_SCROLLABLE);

        // Title - "NAMAZ VAKITLERI"
        lv_obj_t *title = lv_label_create(prayer_scr);
        lv_label_set_text(title, "NAMAZ VAKITLERI");
        lv_obj_set_style_text_color(title, COLOR_ACCENT_BRIGHT, 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_letter_space(title, 2, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

        // Prayer times list - 6 rows (36px height for premium spacing + resistive touch)
        int16_t startY = 36;
        int16_t rowH = 36;
        int16_t padX = 16;

        for (int i = 0; i < 6; i++)
        {
            int16_t y = startY + i * rowH;
            bool isNext = (i == cachedPrayerTimes.nextPrayerIndex);

            // Row background for next prayer - controlled hierarchy
            if (isNext)
            {
                lv_obj_t *row_bg = lv_obj_create(prayer_scr);
                lv_obj_remove_style_all(row_bg);
                lv_obj_set_size(row_bg, 224, rowH - 4);
                lv_obj_set_pos(row_bg, 8, y);
                lv_obj_set_style_bg_color(row_bg, COLOR_ACCENT, 0);
                lv_obj_set_style_bg_opa(row_bg, LV_OPA_10, 0); // Subtle fill
                lv_obj_set_style_radius(row_bg, 8, 0);
                lv_obj_set_style_border_width(row_bg, 1, 0);
                lv_obj_set_style_border_color(row_bg, COLOR_ACCENT_DARK, 0); // Darker border
                lv_obj_clear_flag(row_bg, LV_OBJ_FLAG_CLICKABLE);
            }

            // Prayer name (left) - white text always for readability
            lv_obj_t *name_lbl = lv_label_create(prayer_scr);
            lv_label_set_text(name_lbl, PRAYER_NAMES[i]);
            lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(name_lbl, isNext ? COLOR_TEXT : COLOR_DIM, 0);
            lv_obj_set_pos(name_lbl, padX, y + 8);

            // Prayer time (right) - accent only for next, otherwise subtle
            lv_obj_t *time_lbl = lv_label_create(prayer_scr);
            const char *times[6] = {
                cachedPrayerTimes.fajr, cachedPrayerTimes.sunrise, cachedPrayerTimes.dhuhr,
                cachedPrayerTimes.asr, cachedPrayerTimes.maghrib, cachedPrayerTimes.isha};
            lv_label_set_text(time_lbl, times[i]);
            lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(time_lbl, isNext ? COLOR_TEXT : COLOR_DIM, 0);
            lv_obj_align(time_lbl, LV_ALIGN_TOP_RIGHT, -padX, y + 6);
            prayer_time_labels[i] = time_lbl;
        }

        // Bottom nav bar (same as home)
        prayer_nav_bar = lv_obj_create(prayer_scr);
        lv_obj_remove_style_all(prayer_nav_bar);
        lv_obj_add_style(prayer_nav_bar, &style_icon_bar, 0);
        lv_obj_set_size(prayer_nav_bar, 240, NAV_H);
        lv_obj_align(prayer_nav_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_clear_flag(prayer_nav_bar, LV_OBJ_FLAG_SCROLLABLE);

        // Top border line
        lv_obj_t *top_line = lv_obj_create(prayer_nav_bar);
        lv_obj_set_size(top_line, 240, 1);
        lv_obj_set_pos(top_line, 0, 0);
        lv_obj_set_style_bg_color(top_line, COLOR_DIM, 0);
        lv_obj_set_style_bg_opa(top_line, LV_OPA_50, 0);
        lv_obj_set_style_border_width(top_line, 0, 0);

        int16_t btn_w = 80;
        for (int i = 0; i < 3; i++)
        {
            lv_obj_t *btn = lv_btn_create(prayer_nav_bar);
            lv_obj_remove_style_all(btn);
            lv_obj_add_style(btn, &style_icon_btn, 0);
            lv_obj_set_size(btn, btn_w, NAV_H - 4);
            lv_obj_set_pos(btn, i * btn_w, 4);
            lv_obj_set_ext_click_area(btn, 0);
            lv_obj_add_event_cb(btn, onPrayerNavClick, LV_EVENT_PRESSED, (void *)(intptr_t)i);
            prayer_nav_btns[i] = btn;

            // Icon container
            lv_obj_t *icon_cont = lv_obj_create(btn);
            lv_obj_remove_style_all(icon_cont);
            lv_obj_add_style(icon_cont, &style_transparent, 0);
            lv_obj_set_size(icon_cont, NAV_ICON_SIZE, NAV_ICON_SIZE);
            lv_obj_align(icon_cont, LV_ALIGN_CENTER, 0, -4);
            lv_obj_clear_flag(icon_cont, LV_OBJ_FLAG_CLICKABLE);
            prayer_nav_icons[i] = icon_cont;

            // Active indicator
            lv_obj_t *indicator = lv_obj_create(btn);
            lv_obj_remove_style_all(indicator);
            lv_obj_add_style(indicator, &style_indicator, 0);
            lv_obj_set_size(indicator, 24, 3);
            lv_obj_align(indicator, LV_ALIGN_BOTTOM_MID, 0, -4);
            lv_obj_set_style_bg_opa(indicator, (i == 1) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_clear_flag(indicator, LV_OBJ_FLAG_CLICKABLE);
            prayer_nav_indicators[i] = indicator;

            // Draw icons - Cami (1) is active on this page
            lv_color_t col = (i == 1) ? COLOR_ACCENT_BRIGHT : COLOR_DIM;
            if (i == 0)
                drawHomeIcon(icon_cont, col);
            else if (i == 1)
                drawMosqueIcon(icon_cont, col);
            else
                drawMenuIcon(icon_cont, col);
        }
    }

    void setPrayerTimes(const PrayerTimesData &data)
    {
        cachedPrayerTimes = data;
        // If page is already created, update labels
        if (!prayer_scr)
            return;

        const char *times[6] = {data.fajr, data.sunrise, data.dhuhr, data.asr, data.maghrib, data.isha};
        for (int i = 0; i < 6; i++)
        {
            if (prayer_time_labels[i])
            {
                lv_label_set_text(prayer_time_labels[i], times[i]);
                bool isNext = (i == data.nextPrayerIndex);
                lv_obj_set_style_text_color(prayer_time_labels[i], isNext ? COLOR_TEXT : COLOR_DIM, 0);
            }
        }
    }

    void showPrayerTimesPage()
    {
        createPrayerTimesPage();
        lv_scr_load(prayer_scr);
    }

    void showHomePage()
    {
        if (scr)
            lv_scr_load(scr);
    }

    void showHome()
    {
        if (scr)
        {
            lv_scr_load(scr);
        }
        else
        {
            init(); // Create home screen if not exists
        }
    }

    void loop()
    {
        lv_timer_handler();
    }

} // namespace UiHome
