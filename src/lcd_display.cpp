#include "lcd_display.h"
#include "config.h"
#include "prayer_types.h"
#include "settings_manager.h"

// Custom LCD characters

// Clock: Minimalist circle with hands at 3:00
uint8_t CHAR_CLOCK[8] = {
    0b00000,
    0b01110, //  ***
    0b10001, // *   *
    0b10101, // * * * (Hand Up)
    0b10111, // * *** (Hand Right)
    0b10001, // *   *
    0b01110, //  ***
    0b00000};

// Mosque: Refined Dome
uint8_t CHAR_MOSQUE[8] = {
    0b00000,
    0b00100, //   *
    0b01110, //  ***
    0b11111, // *****
    0b11111, // *****
    0b10001, // *   *
    0b11111, // *****
    0b00000};

// Speaker: Sound on
uint8_t CHAR_SPEAKER[8] = {
    0b00001, //     *
    0b00011, //    **
    0b01111, //  ****
    0b01111, //  ****
    0b01111, //  ****
    0b00011, //    **
    0b00001, //     *
    0b00000};

// Muted: Speaker with X
uint8_t CHAR_MUTED[8] = {
    0b00001, //     *
    0b10011, // *  **
    0b01111, //  ****
    0b00111, //   ***
    0b01111, //  ****
    0b10011, // *  **
    0b00001, //     *
    0b00000};

LCDDisplay::LCDDisplay()
    : lcd(Config::LCD_ADDRESS, Config::LCD_COLS, Config::LCD_ROWS) {}

void LCDDisplay::init()
{
    if constexpr (!Config::LCD_ENABLED)
        return;

    lcd.init();
    lcd.backlight();
    lcd.createChar(0, CHAR_CLOCK);
    lcd.createChar(1, CHAR_MOSQUE);
    lcd.createChar(2, CHAR_SPEAKER);
    lcd.createChar(3, CHAR_MUTED);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
}

void LCDDisplay::update(const CurrentTime &now,
                        const std::optional<PrayerType> &nextPrayer,
                        const DailyPrayers &prayers)
{
    if constexpr (!Config::LCD_ENABLED)
    {
        return;
    }

    static int lastMinute = -1;
    static PrayerType lastPrayer = PrayerType::COUNT;

    const bool minuteChanged = (now._minutes != lastMinute);
    const bool prayerChanged = (nextPrayer && lastPrayer != *nextPrayer);

    if (!minuteChanged && !prayerChanged && !needsRefresh)
    {
        return;
    }

    lastMinute = now._minutes;
    if (nextPrayer)
    {
        lastPrayer = *nextPrayer;
    }
    needsRefresh = false;

    lcd.clear();

    // Row 0: [Clock] HH:MM DD Mon
    lcd.setCursor(0, 0);
    lcd.write((uint8_t)0); // Clock
    lcd.print(" ");
    lcd.print(now._hhMM.data());
    lcd.print(" ");
    lcd.print(CurrentTime::getCurrentDate().data());

    // Row 1: [Mosque] PrayerName HH:MM
    lcd.setCursor(0, 1);
    lcd.write((uint8_t)1); // Mosque

    if (!nextPrayer)
    {
        // After Isha, show tomorrow's Fajr time
        const auto &fajrTime = prayers[PrayerType::Fajr];
        lcd.print(" ");
        lcd.print(getPrayerName(PrayerType::Fajr).data());
        lcd.print(" ");
        lcd.print(fajrTime.value.data());

        // Show speaker icon for Fajr
        lcd.print(" ");
        lcd.write(SettingsManager::getAdhanEnabled(PrayerType::Fajr) ? (uint8_t)2 : (uint8_t)3);
        return;
    }

    const auto &nextTime = prayers[*nextPrayer];
    lcd.print(" ");
    lcd.print(getPrayerName(*nextPrayer).data());
    lcd.print(" ");
    lcd.print(nextTime.value.data());

    // Show speaker icon (muted icon for disabled or Sunrise)
    lcd.print(" ");
    lcd.write(SettingsManager::getAdhanEnabled(*nextPrayer) ? (uint8_t)2 : (uint8_t)3);
}

void LCDDisplay::showError(const char *line1, const char *line2)
{
    if constexpr (!Config::LCD_ENABLED)
        return;

    lcd.clear();

    // Center text on 16-char display
    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void LCDDisplay::showMessage(const char *line1, const char *line2)
{
    if constexpr (!Config::LCD_ENABLED)
        return;

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void LCDDisplay::forceRefresh()
{
    needsRefresh = true;
}
