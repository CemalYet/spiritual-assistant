/**
 * @file lvgl_display.cpp
 * @brief LVGL Display Driver Implementation
 *
 * Main display system - replaces TftDisplay completely.
 * Uses LovyanGFX for hardware, LVGL for UI, UiHome for screens.
 * Prayer logic handled by main.cpp
 */

#include "lvgl_display.h"
#include <Arduino.h>
#include <lvgl.h>
#include <time.h>
#include <LittleFS.h>
#include <WiFi.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "app_state.h"
#include "ui_state_reader.h"
#include "ui_page_home.h"
#include "ui_page_prayer.h"
#include "ui_page_settings.h"
#include "ui_components.h"
#include "network.h"

// ═══════════════════════════════════════════════════════════════
// LOVANGFX DISPLAY CONFIGURATION
// ═══════════════════════════════════════════════════════════════
class LGFX_Display : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Touch_XPT2046 _touch_instance;

public:
    LGFX_Display(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 14;
            cfg.pin_mosi = 13;
            cfg.pin_miso = 15;
            cfg.pin_dc = 12;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 10;
            cfg.pin_rst = 11;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 320;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel_instance.config(cfg);
        }
        {
            // Touch configuration (XPT2046 resistive touch)
            auto cfg = _touch_instance.config();
            cfg.spi_host = SPI2_HOST; // Same SPI bus as display
            cfg.freq = 1000000;       // 1MHz - slower = more reliable
            cfg.pin_sclk = 14;        // Shared with display
            cfg.pin_mosi = 13;        // Shared with display
            cfg.pin_miso = 15;        // Shared with display
            cfg.pin_cs = 9;           // Touch CS pin
            cfg.pin_int = -1;         // No interrupt pin
            cfg.x_min = 100;          // Wider calibration range
            cfg.x_max = 4000;
            cfg.y_min = 100;
            cfg.y_max = 4000;
            cfg.bus_shared = true; // SPI shared with display
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};

// ═══════════════════════════════════════════════════════════════
// STATIC OBJECTS
// ═══════════════════════════════════════════════════════════════
static LGFX_Display gfx;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = nullptr;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv; // Touch input device driver
static bool initialized = false;

// Turkish localization
static const char *TURKISH_DAYS[] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"};
static const char *TURKISH_MONTHS[] = {"Ocak", "Subat", "Mart", "Nisan", "Mayis", "Haziran",
                                       "Temmuz", "Agustos", "Eylul", "Ekim", "Kasim", "Aralik"};

// ═══════════════════════════════════════════════════════════════
// LVGL CALLBACKS
// ═══════════════════════════════════════════════════════════════
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    gfx.startWrite();
    gfx.setAddrWindow(area->x1, area->y1, w, h);
    gfx.writePixels((uint16_t *)color_p, w * h);
    gfx.endWrite();

    lv_disp_flush_ready(drv);
}

// Touch input callback for LVGL
static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t x, y;

    // Single read - let LVGL handle debouncing
    bool touched = gfx.getTouch(&x, &y);

    if (touched)
    {
        // Touch panel is 90° rotated with different scaling
        // Swap and scale X/Y to match display
        uint16_t raw_x = x, raw_y = y;
        x = 240 - (raw_y * 240 / 320);
        y = 320 - (raw_x * 320 / 240);

        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ═══════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════
static void formatTurkishDate(char *buffer, size_t size)
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        snprintf(buffer, size, "%s, %d %s %d",
                 TURKISH_DAYS[timeinfo.tm_wday],
                 timeinfo.tm_mday,
                 TURKISH_MONTHS[timeinfo.tm_mon],
                 1900 + timeinfo.tm_year);
    }
    else
    {
        snprintf(buffer, size, "Tarih Yok");
    }
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

        // Init display hardware
        gfx.init();
        gfx.setRotation(0);
        gfx.fillScreen(TFT_BLACK);

        // Init LVGL
        lv_init();

        // Allocate draw buffer in PSRAM
        size_t buf_size = 240 * 40; // 40 lines
        buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
        if (!buf1)
        {
            Serial.println("[Display] ERROR: Buffer allocation failed!");
            return false;
        }

        lv_disp_draw_buf_init(&draw_buf, buf1, NULL, buf_size);

        // Register display driver
        lv_disp_drv_init(&disp_drv);
        disp_drv.hor_res = 240;
        disp_drv.ver_res = 320;
        disp_drv.flush_cb = lvgl_flush_cb;
        disp_drv.draw_buf = &draw_buf;
        lv_disp_drv_register(&disp_drv);

        // Register touch input driver
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lvgl_touch_cb;
        lv_indev_drv_register(&indev_drv);

        initialized = true;
        Serial.println("[Display] LVGL initialized with touch");

        // Initialize state reader early so state-driven UI works from the start
        UiStateReader::init();

        return true;
    }

    void loop()
    {
        if (initialized)
        {
            lv_timer_handler();
        }
    }

    void showPrayerScreen()
    {
        // Clear any status screen state before initializing
        g_state.statusScreen = StatusScreenType::NONE;
        g_state.clearDirty(DirtyFlag::STATUS_SCREEN);

        // Initialize state reader (creates LVGL timer for state polling)
        UiStateReader::init();

        // Create all pages once (they stay in memory)
        UiPageHome::create();
        UiPagePrayer::create();
        UiPageSettings::create();

        // Load home screen (pages created above, this just displays home)
        lv_scr_load(UiPageHome::getScreen());

        // Set time via shared state
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            AppStateHelper::setTime(timeinfo.tm_hour, timeinfo.tm_min);
        }
        else
        {
            AppStateHelper::setTime(0, 0);
        }

        // Set Turkish date via shared state
        char dateBuffer[64];
        formatTurkishDate(dateBuffer, sizeof(dateBuffer));
        AppStateHelper::setDate(dateBuffer);

        // Initial loading state
        AppStateHelper::setNextPrayer("YUKLENIYOR", "--:--");

        // Update status icons via shared state
        updateStatus();

        // Navigation callback - just switch screens, no recreation
        UiComponents::setNavClickCallback([](int page)
                                          {
            switch (page)
            {
            case 0:
                lv_scr_load(UiPageHome::getScreen());
                break;
            case 1:
                lv_scr_load(UiPagePrayer::getScreen());
                break;
            case 2:
                lv_scr_load(UiPageSettings::getScreen());
                break;
            } });
    }

    void updateTime()
    {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            AppStateHelper::setTime(timeinfo.tm_hour, timeinfo.tm_min);

            // Update date at midnight
            if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0)
            {
                char dateBuffer[64];
                formatTurkishDate(dateBuffer, sizeof(dateBuffer));
                AppStateHelper::setDate(dateBuffer);
            }
        }
    }

    void updateDate()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("[Display] updateDate: Failed to get time");
            return;
        }
        char dateBuffer[64];
        formatTurkishDate(dateBuffer, sizeof(dateBuffer));
        AppStateHelper::setDate(dateBuffer);
    }

    void updateStatus()
    {
        // NTP sync status
        struct tm timeinfo;
        bool ntpSynced = getLocalTime(&timeinfo, 0);
        AppStateHelper::setNtpSynced(ntpSynced);

        // Adhan file status
        bool adhanExists = LittleFS.exists("/azan.mp3") || LittleFS.exists("/azan.wav");
        AppStateHelper::setAdhanAvailable(adhanExists);
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
                     TURKISH_MONTHS[timeinfo.tm_mon],
                     TURKISH_DAYS[timeinfo.tm_wday]);
        }
        else
        {
            prayerDateBuffer[0] = '\0';
        }
        return prayerDateBuffer;
    }

} // namespace LvglDisplay
