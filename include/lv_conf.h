/**
 * LVGL v8.3 Configuration for ESP32-S3 with AXS15231B
 * Display: 320x480 QSPI (Waveshare ESP32-S3-Touch-LCD-3.5B)
 * Touch:   AXS15231B capacitive (I2C)
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   DISPLAY SETTINGS
 *====================*/
#define LV_HOR_RES_MAX 480
#define LV_VER_RES_MAX 320
#define LV_DPI_DEF 130

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1 /* Pre-swap for fast writeBytes path via draw16bitBeRGBBitmap */

/*====================
   MEMORY SETTINGS - Use PSRAM
 *====================*/
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <esp_heap_caps.h>
#define LV_MEM_CUSTOM_ALLOC(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LV_MEM_CUSTOM_FREE(p) free(p)
#define LV_MEM_CUSTOM_REALLOC(p, size) heap_caps_realloc(p, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

#define LV_MEM_SIZE (48 * 1024U)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_DISP_DEF_REFR_PERIOD 16
#define LV_INDEV_DEF_READ_PERIOD 20

/*====================
   FEATURE CONFIGURATION
 *====================*/
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/*====================
   FONT USAGE
 *====================*/
/* All built-in Montserrat fonts DISABLED.
 * UI uses Cinzel + DM Mono (src/fonts/), --lcd subpixel rendered.
 * LV_FONT_DEFAULT → Cinzel Regular 14 with Turkish glyphs + FA symbols. */
#define LV_FONT_MONTSERRAT_8 0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_48 0

#ifdef __cplusplus
extern "C"
{
#endif
  extern const struct _lv_font_t font_inter_14;
#ifdef __cplusplus
}
#endif
#define LV_FONT_DEFAULT (&font_inter_14)

/* Subpixel disabled — rotated display makes LCD subpx produce shadows */
#define LV_USE_FONT_SUBPX 0
#if LV_USE_FONT_SUBPX
#define LV_FONT_SUBPX_BGR 0
#endif

#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*====================
   WIDGETS
 *====================*/
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1

/*====================
   EXTRA COMPONENTS
 *====================*/
#define LV_USE_QRCODE 1
#define LV_USE_ANIMIMG 0
#define LV_USE_CALENDAR 0
#define LV_USE_CHART 0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LED 1
#define LV_USE_LIST 1
#define LV_USE_MENU 0
#define LV_USE_METER 1
#define LV_USE_MSGBOX 1
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 1
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_SPAN 1

/*====================
   THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 1
#define LV_THEME_DEFAULT_TRANSITION_TIME 80

/*====================
   LAYOUTS
 *====================*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

#endif /* LV_CONF_H */
