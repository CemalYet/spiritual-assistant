#pragma once
#include <LiquidCrystal_I2C.h>
#include <optional>
#include "prayer_types.h"
#include "daily_prayers.h"
#include "current_time.h"

class LCDDisplay
{
private:
    LiquidCrystal_I2C lcd;

public:
    LCDDisplay();
    void init();
    void update(const CurrentTime &now,
                const std::optional<PrayerType> &nextPrayer,
                const DailyPrayers &prayers);
    void showError(const char *line1, const char *line2);
    void showMessage(const char *line1, const char *line2);
};
