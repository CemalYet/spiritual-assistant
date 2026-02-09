#include "display_ticker.h"
#include "lvgl_display.h"
#include <Arduino.h>

namespace DisplayTicker
{
    static int s_lastMinute = -1;

    void tick()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
            return;

        int currentMinute = timeinfo.tm_hour * 60 + timeinfo.tm_min;

        if (currentMinute != s_lastMinute)
        {
            s_lastMinute = currentMinute;
            LvglDisplay::updateTime();
            LvglDisplay::updateStatus();
        }
    }

} // namespace DisplayTicker
