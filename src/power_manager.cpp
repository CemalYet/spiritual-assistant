#include "power_manager.h"
#include "tft_config.h"
#include "settings_manager.h"
#include "lvgl_display.h"
#include "audio_player.h"
#include "ui_state_reader.h"

#include "rtc_manager.h"

#include "prayer_engine.h"
#include "current_time.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

static constexpr uint8_t INVALID_LOCK = 0xFF;
static constexpr uint32_t FALLBACK_SLEEP_SEC = 3600;
static constexpr uint32_t MAX_SLEEP_SEC = 8 * 3600;
static constexpr int SECONDS_PER_DAY = 86400;
static constexpr uint64_t USEC_PER_SEC = 1'000'000ULL;
static constexpr int WAKE_EARLY_SEC = 5;

static uint32_t computeSleepSeconds()
{
    int nextSec = PrayerEngine::getNextPrayerSeconds();
    if (nextSec < 0)
        return FALLBACK_SLEEP_SEC;

    int nowSec = CurrentTime::now()._seconds;
    int sleepSec = nextSec - nowSec;

    if (PrayerEngine::isShowingTomorrow())
        sleepSec += SECONDS_PER_DAY;

    sleepSec -= WAKE_EARLY_SEC;

    if (sleepSec <= 0)
        return FALLBACK_SLEEP_SEC;

    if (static_cast<uint32_t>(sleepSec) > MAX_SLEEP_SEC)
        return MAX_SLEEP_SEC;

    return static_cast<uint32_t>(sleepSec);
}

static PowerManager::State currentState = PowerManager::State::ACTIVE;
static uint32_t lastActivityMs = 0;

struct WakeLockEntry
{
    const char *name;
    bool active;
};
static WakeLockEntry wakeLocks[PowerManager::MAX_WAKE_LOCKS] = {};

static PowerMode cachedMode = PowerMode::ALWAYS_ON;

static uint32_t getDimTimeout()
{
    if (cachedMode == PowerMode::ALWAYS_ON)
        return PowerManager::DIM_TIMEOUT_MS;
    return PowerManager::DIM_TIMEOUT_SAVER_MS;
}

static const char *modeStr(PowerMode m)
{
    switch (m)
    {
    case PowerMode::ALWAYS_ON:
        return "ALWAYS_ON";
    case PowerMode::SCREEN_OFF:
        return "SCREEN_OFF";
    default:
        return "?";
    }
}

namespace PowerManager
{

    void init()
    {
        cachedMode = SettingsManager::getPowerMode();
        lastActivityMs = millis();
        LvglDisplay::setBacklight(ACTIVE_BRIGHTNESS);
        currentState = State::ACTIVE;
        Serial.printf("[Power] Init — mode=%s, brightness=%u\n", modeStr(cachedMode), ACTIVE_BRIGHTNESS);
    }

    void tick()
    {
        if (hasActiveWakeLocks())
            return;

        cachedMode = SettingsManager::getPowerMode();
        const uint32_t idleMs = millis() - lastActivityMs;

        switch (currentState)
        {
        case State::ACTIVE:
            if (cachedMode == PowerMode::ALWAYS_ON)
                break;

            if (idleMs >= getDimTimeout())
            {
                LvglDisplay::setBacklight(DIM_BRIGHTNESS);
                currentState = State::DIM;
                Serial.printf("[Power] ACTIVE → DIM (idle %u ms, mode=%s)\n", idleMs, modeStr(cachedMode));
            }
            break;

        case State::DIM:
            if (cachedMode == PowerMode::ALWAYS_ON)
            {
                LvglDisplay::setBacklight(ACTIVE_BRIGHTNESS);
                currentState = State::ACTIVE;
                Serial.println("[Power] DIM → ACTIVE (mode=ALWAYS_ON)");
                break;
            }

            if (idleMs >= SCREEN_OFF_TIMEOUT_MS)
            {
                LvglDisplay::setBacklight(0);
                LvglDisplay::displayOff();
                UiStateReader::pause();
                currentState = State::SCREEN_OFF;
                Serial.printf("[Power] DIM → SCREEN_OFF (idle %u ms, mode=%s)\n", idleMs, modeStr(cachedMode));
            }
            break;

        case State::SCREEN_OFF:
            if (cachedMode == PowerMode::SCREEN_OFF)
            {
                Serial.println("[Power] SCREEN_OFF — entering light sleep...");
                enterLightSleep();
                auto cause = esp_sleep_get_wakeup_cause();
                Serial.printf("[Power] Light sleep woke — reason=%d\n", static_cast<int>(cause));
                if (cause == ESP_SLEEP_WAKEUP_GPIO)
                {
                    Serial.println("[Power] GPIO wake → restoring screen");
                }
                else
                {
                    Serial.println("[Power] Timer wake → restoring screen for adhan");
                }
                wakeScreen();
            }
            break;
        }
    }

    void reportActivity()
    {
        lastActivityMs = millis();

        if (currentState == State::SCREEN_OFF)
        {
            Serial.println("[Power] Touch while SCREEN_OFF → waking screen");
            wakeScreen();
            return;
        }

        if (currentState == State::DIM)
        {
            LvglDisplay::setBacklight(ACTIVE_BRIGHTNESS);
            currentState = State::ACTIVE;
            Serial.println("[Power] Touch while DIM → ACTIVE");
        }
    }

    void enterLightSleep()
    {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        audioPlayerSuspend();

        gpio_wakeup_enable(WAKE_PIN, GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();

        uint32_t sleepSec = computeSleepSeconds();
        esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(sleepSec) * USEC_PER_SEC);
        Serial.printf("[Power] Light sleep for %u s\n", sleepSec);
        Serial.flush();

        esp_light_sleep_start();

        // Re-init UART — USB-CDC drops during light sleep
        Serial.begin(115200);
        delay(50);

        // Re-anchor system time to external RTC after light sleep.
        if (!RtcManager::setSystemClockFromRTC())
        {
            Serial.println("[Power] RTC read failed after wake — scheduling immediate NTP sync");
            RtcManager::postponeSync(0);
        }

        audioPlayerResume();
    }

    State getState()
    {
        return currentState;
    }

    bool isScreenOn()
    {
        return currentState == State::ACTIVE || currentState == State::DIM;
    }

    uint8_t acquireWakeLock(const char *name)
    {
        for (uint8_t i = 0; i < MAX_WAKE_LOCKS; ++i)
        {
            if (!wakeLocks[i].active)
            {
                wakeLocks[i].name = name;
                wakeLocks[i].active = true;
                Serial.printf("[Power] Wake lock acquired: %s (slot %u)\n", name, i);
                return i;
            }
        }
        Serial.printf("[Power] WARNING: No free wake lock for: %s\n", name);
        return INVALID_LOCK;
    }

    void releaseWakeLock(uint8_t id)
    {
        if (id >= MAX_WAKE_LOCKS)
            return;
        if (wakeLocks[id].active)
        {
            Serial.printf("[Power] Wake lock released: %s (slot %u)\n", wakeLocks[id].name, id);
            wakeLocks[id].active = false;
            wakeLocks[id].name = nullptr;
        }
    }

    bool hasActiveWakeLocks()
    {
        for (uint8_t i = 0; i < MAX_WAKE_LOCKS; ++i)
        {
            if (wakeLocks[i].active)
                return true;
        }
        return false;
    }

    void wakeScreen()
    {
        LvglDisplay::displayOn();
        LvglDisplay::setBacklight(ACTIVE_BRIGHTNESS);
        UiStateReader::resume();
        lastActivityMs = millis();
        currentState = State::ACTIVE;
        Serial.println("[Power] Screen woken → ACTIVE");
    }

} // namespace PowerManager
