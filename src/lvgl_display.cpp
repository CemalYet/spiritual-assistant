/**
 * @file lvgl_display.cpp
 * @brief LVGL Display Driver Implementation
 *
 * Main display system for Waveshare ESP32-S3-Touch-LCD-3.5B.
 * Uses Arduino_GFX (QSPI AXS15231B) + LVGL for UI.
 * Touch via I2C capacitive AXS15231B driver.
 */

#include "lvgl_display.h"
#include <Arduino.h>
#include <lvgl.h>
#include <time.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <Wire.h>

#include <databus/Arduino_ESP32QSPI.h>
#include <display/Arduino_AXS15231B.h>
#include "esp_lcd_touch_axs15231b.h"

#include "tft_config.h"
#include "tca_expander.h"
#include "power_manager.h"
#include "app_state.h"
#include "ui_state_reader.h"
#include "ui_page_home.h"
#include "ui_page_clock.h"
#include "ui_page_settings.h"
#include "ui_components.h"
#include "network.h"
#include "locale_tr.h"

// ═══════════════════════════════════════════════════════════════
// DISPLAY HARDWARE  (Waveshare AXS15231B QSPI — native 320×480 portrait)
// ═══════════════════════════════════════════════════════════════

// Native resolution (hardware is portrait)
static const uint16_t GFX_W = 320;
static const uint16_t GFX_H = 480;

// Logical resolution for LVGL (landscape)
static const uint16_t SCR_W = 480;
static const uint16_t SCR_H = 320;

// Single switch for opposite-landscape orientation.
// true  = rotate UI 180° within landscape (other side)
// false = current orientation
static constexpr bool FLIP_LANDSCAPE_180 = true;
static constexpr uint16_t TOUCH_ROTATION = FLIP_LANDSCAPE_180 ? 3 : 1;

static Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    QSPI_CS, QSPI_CLK, QSPI_D0, QSPI_D1, QSPI_D2, QSPI_D3);

// Hardware stays in native portrait — software rotation in flush_cb
static Arduino_GFX *gfx = new Arduino_AXS15231B(
    bus, -1 /* RST */, 0 /* Native Portrait */, false /* IPS */,
    GFX_W, GFX_H);

// ═══════════════════════════════════════════════════════════════
// STATIC OBJECTS
// ═══════════════════════════════════════════════════════════════
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = nullptr; // LVGL render buffer (persistent, full-screen landscape)
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static bool initialized = false;

// ═══════════════════════════════════════════════════════════════
// LVGL CALLBACKS
// ═══════════════════════════════════════════════════════════════

// Reverse pixel order in-place for 180° flip (fast — ~0.5ms on ESP32)
static void reverseBuffer16(uint16_t *buf, uint32_t count)
{
    uint32_t i = 0, j = count - 1;
    while (i < j)
    {
        uint16_t tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
        i++;
        j--;
    }
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    // direct_mode: buf1 is persistent full-screen landscape buffer.
    // LVGL only re-renders dirty widgets into buf1.
    // On last flush, send entire buf1 to display via draw16bitBeRGBBitmapR1
    // which does 90° CW rotation DURING the QSPI DMA transfer — no rot_buf needed.
    if (lv_disp_flush_is_last(drv))
    {
        if (FLIP_LANDSCAPE_180)
        {
            // 180° flip: reverse buffer, send via proven R1 DMA, then restore for LVGL direct_mode.
            const uint32_t pixelCount = (uint32_t)SCR_W * SCR_H;
            reverseBuffer16((uint16_t *)buf1, pixelCount);
            gfx->draw16bitBeRGBBitmapR1(0, 0, (uint16_t *)buf1, SCR_W, SCR_H);
            reverseBuffer16((uint16_t *)buf1, pixelCount);
        }
        else
        {
            gfx->draw16bitBeRGBBitmapR1(0, 0, (uint16_t *)buf1, SCR_W, SCR_H);
        }
    }
    lv_disp_flush_ready(drv);
}

// Touch input callback for LVGL (I2C capacitive — AXS15231B)
static lv_point_t touchStart = {0, 0};
static bool touchActive = false;
static bool gestureActive = false; // suppress clicks during swipe
static bool swipeFired = false;    // prevent double-fire per touch

static const int SWIPE_MIN_PX = 40; // minimum horizontal distance for swipe
static int currentPage = 0;
static bool suppressSwipe = false; // set when touch interacts with a widget — blocks swipe for entire session
static int touchFrames = 0;        // frames since touch began — delays swipe so LVGL can acquire widget
static bool touchStartOnSlider = false;
static void goToPage(int page); // forward declaration

// Dynamic page count: 3 normally, 4 when portal screen is active
static int getNumPages()
{
    return UiPageSettings::getPortalScreen() ? 4 : 3;
}

static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{ // Ignore touch reads when screen is off — touch IC generates phantom events in SLPIN
    if (!PowerManager::isScreenOn())
    {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    touch_data_t touch_data;
    bsp_touch_read();

    if (bsp_touch_get_coordinates(&touch_data))
    {
        // Touch library rotation maps portrait touch coordinates to current landscape orientation.
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_data.coords[0].x;
        data->point.y = touch_data.coords[0].y;
        if (!touchActive)
        {
            touchStart.x = data->point.x;
            touchStart.y = data->point.y;
            touchActive = true;
            gestureActive = false;
            swipeFired = false;
            suppressSwipe = false;
            touchFrames = 0;
            touchStartOnSlider = (currentPage == 2 && UiPageSettings::isSliderHit(touchStart.x, touchStart.y));
        }
        ++touchFrames;

        // Detect intent from direction first, then decide whether slider should own the gesture.
        if (!swipeFired && !suppressSwipe)
        {
            int dx = data->point.x - touchStart.x;
            int dy = data->point.y - touchStart.y;
            int adx = dx < 0 ? -dx : dx;
            int ady = dy < 0 ? -dy : dy;

            if (touchStartOnSlider && ady > adx + 8 && ady >= 16)
            {
                suppressSwipe = true;
            }
            else if (adx >= SWIPE_MIN_PX && adx > ady + 3)
            {
                swipeFired = true;
                gestureActive = true;
                if (dx < 0)
                    goToPage(currentPage + 1);
                else
                    goToPage(currentPage - 1);
            }
        }
        PowerManager::reportActivity();
    }
    else
    {
        touchActive = false;
        touchStartOnSlider = false;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ═══════════════════════════════════════════════════════════════
// SWIPE GESTURE — horizontal swipe to switch screens
// ═══════════════════════════════════════════════════════════════

static lv_obj_t *getScreenForPage(int page)
{
    switch (page)
    {
    case 0:
        return UiPageHome::getScreen();
    case 1:
        return UiPageClock::getScreen();
    case 2:
        return UiPageSettings::getScreen();
    case 3:
        return UiPageSettings::getPortalScreen();
    default:
        return UiPageHome::getScreen();
    }
}

static void goToPage(int page)
{
    int numPages = getNumPages();
    if (page < 0)
        page = 0;
    if (page >= numPages)
        page = numPages - 1;
    if (page == currentPage)
        return;
    // Partial updates + software rotation: page switch is fast enough.
    // lv_scr_load marks full screen dirty = one full flush, then back to partials.
    currentPage = page;
    lv_obj_t *scr = getScreenForPage(page);
    lv_scr_load(scr);
    lv_refr_now(NULL);
}

// Static buffer for prayer date (used by formatPrayerDate)
static char prayerDateBuffer[48];

namespace LvglDisplay
{
    bool begin()
    {
        if (initialized)
            return true;

        Serial.println("[Display] Initializing LVGL...");

        // Reset display via TCA9554 EXIO1 (LCD_RST)
        TcaExpander::setPinMode(EXIO_LCD_RST, OUTPUT);
        TcaExpander::writePin(EXIO_LCD_RST, HIGH);
        delay(10);
        TcaExpander::writePin(EXIO_LCD_RST, LOW);
        delay(10);
        TcaExpander::writePin(EXIO_LCD_RST, HIGH);
        delay(200);

        // Init display hardware (QSPI AXS15231B)
        if (!gfx->begin())
        {
            Serial.println("[Display] ERROR: gfx->begin() failed!");
            return false;
        }

        // No setRotation() — AXS15231B MADCTL rotation broken over QSPI.
        // Software rotation in flush_cb handles landscape→portrait transform.
        gfx->fillScreen(RGB565_BLACK);

        // Backlight on
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, HIGH);

        // Init LVGL
        lv_init();

        // Init capacitive touch (I2C, rotation follows selected landscape orientation)
        bsp_touch_init(&Wire, TOUCH_RST, TOUCH_ROTATION, SCR_W, SCR_H);

        // Buffer in PSRAM for LVGL rendering (no rotation buffer needed)
        size_t buf_size = SCR_W * SCR_H;
        buf1 = (lv_color_t *)heap_caps_aligned_alloc(32, buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
        if (!buf1)
        {
            Serial.println("[Display] ERROR: Buffer allocation failed!");
            return false;
        }
        memset(buf1, 0, buf_size * sizeof(lv_color_t));
        Serial.printf("[Display] Buffer allocated: %uB in PSRAM (no rot_buf needed)\n",
                      buf_size * sizeof(lv_color_t));

        lv_disp_draw_buf_init(&draw_buf, buf1, NULL, buf_size); // Single buffer

        // Register display driver — logical 480×320 landscape
        // Manual rotation in flush_cb handles portrait hardware output
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = SCR_W; // 480 (logical landscape)
        disp_drv.ver_res = SCR_H; // 320 (logical landscape)
        disp_drv.flush_cb = lvgl_flush_cb;
        disp_drv.draw_buf = &draw_buf;
        disp_drv.direct_mode = 1; // Persistent buffer — LVGL only re-renders dirty widgets
        lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

        // Register touch input driver
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lvgl_touch_cb;
        lv_indev_drv_register(&indev_drv);

        initialized = true;
        Serial.println("[Display] LVGL initialized with touch");

        // Initialize state reader early so boot status screens work
        UiStateReader::init();

        return true;
    }

    void loop()
    {
        if (initialized)
            lv_timer_handler();
    }

    void showPrayerScreen()
    {
        // Clear any status screen state before initializing
        g_state.statusScreen = StatusScreenType::NONE;
        g_state.clearDirty(DirtyFlag::STATUS_SCREEN);

        // Create shared assets (motif tile) before screens
        UiComponents::createSharedAssets();

        // Create all pages once (they stay in memory)
        UiPageHome::create();
        UiPageClock::create();
        UiPageSettings::create();

        // Load home screen
        currentPage = 0;
        lv_scr_load(UiPageHome::getScreen());
        lv_refr_now(NULL);

        // Fresh pages need current state — mark all dirty so UiStateReader pushes everything
        g_state.markDirty(DirtyFlag::ALL & ~DirtyFlag::STATUS_SCREEN);

        // Navigation callback - just switch screens, no recreation
        UiComponents::setNavClickCallback([](int page)
                                          {
            if (page < 0 || page >= getNumPages() || page == currentPage) return;
            currentPage = page;
            lv_scr_load(getScreenForPage(page)); });
    }

    const char *formatPrayerDate(int dayOffset)
    {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            // Add dayOffset using tm_mday - mktime normalizes automatically
            // Handles: 31 Jan + 1 = 1 Feb, 28/29 Feb + 1 = 1 Mar, 31 Dec + 1 = 1 Jan
            if (dayOffset > 0)
            {
                timeinfo.tm_mday += dayOffset;
                mktime(&timeinfo); // Normalizes overflows (e.g., day 32 → next month)
            }
            snprintf(prayerDateBuffer, sizeof(prayerDateBuffer), "%d %s %s",
                     timeinfo.tm_mday,
                     LocaleTR::MONTHS[timeinfo.tm_mon],
                     LocaleTR::DAYS[timeinfo.tm_wday]);
        }
        else
        {
            prayerDateBuffer[0] = '\0';
        }
        return prayerDateBuffer;
    }

    void setBacklight(uint8_t brightness)
    {
        analogWrite(TFT_BL, brightness);
    }

    void displayOff()
    {
        gfx->displayOff();
    }

    void displayOn()
    {
        gfx->displayOn();
    }

    bool isGestureActive()
    {
        return gestureActive;
    }

    void goToPortalPage()
    {
        lv_obj_t *portalScr = UiPageSettings::getPortalScreen();
        if (!portalScr)
            return;
        currentPage = 3;
        lv_scr_load(portalScr);
        lv_refr_now(NULL);
    }

    bool leavePortalPageIfActive()
    {
        if (currentPage != 3)
            return false;

        lv_obj_t *settingsScr = UiPageSettings::getScreen();
        if (!settingsScr)
            return false;

        currentPage = 2;
        lv_scr_load(settingsScr);
        lv_refr_now(NULL);
        return true;
    }

} // namespace LvglDisplay
