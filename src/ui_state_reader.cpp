/**
 * @file ui_state_reader.cpp
 * @brief UI State Reader Implementation
 *
 * Polls AppState dirty flags and updates UI widgets.
 * Uses LVGL timer for automatic periodic updates.
 */

#include "ui_state_reader.h"
#include "app_state.h"
#include "ui_page_home.h"
#include "ui_page_clock.h"
#include "ui_page_settings.h"
#include "ui_page_status.h"
#include <lvgl.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace UiStateReader
{
    static lv_timer_t *updateTimer = nullptr;
    static lv_obj_t *lastNormalScreen = nullptr;

    // Timer callback - runs every 50ms
    static void timerCallback(lv_timer_t *timer)
    {
        update();
    }

    void init()
    {
        // Create LVGL timer for periodic updates (50ms = 20 FPS state sync)
        if (updateTimer == nullptr)
        {
            updateTimer = lv_timer_create(timerCallback, 50, nullptr);
        }
    }

    void pause()
    {
        if (updateTimer)
            lv_timer_pause(updateTimer);
    }

    void resume()
    {
        if (updateTimer)
            lv_timer_resume(updateTimer);
    }

    void update()
    {
        // Early exit if nothing dirty
        if (g_state.dirty == DirtyFlag::NONE)
        {
            return;
        }

        // ═══════════════════════════════════════════════════
        // STATUS SCREEN (highest priority - overlays everything)
        // ═══════════════════════════════════════════════════
        if (g_state.isDirty(DirtyFlag::STATUS_SCREEN))
        {
            lv_obj_t *active = lv_scr_act();
            if (g_state.statusScreen != StatusScreenType::NONE)
            {
                lv_obj_t *statusScr = UiPageStatus::getScreen();
                if (active && active != statusScr)
                    lastNormalScreen = active;
            }

            switch (g_state.statusScreen)
            {
            case StatusScreenType::CONNECTING:
                UiPageStatus::showConnecting(g_state.statusLine1.c_str());
                break;
            case StatusScreenType::PORTAL:
                UiPageStatus::showPortal(g_state.statusTitle.c_str(),
                                         g_state.statusLine1.c_str(),
                                         g_state.statusLine2.c_str());
                break;
            case StatusScreenType::MESSAGE:
                UiPageStatus::showMessage(g_state.statusTitle.c_str(),
                                          g_state.statusLine1.c_str());
                break;
            case StatusScreenType::ERROR:
                UiPageStatus::showError(g_state.statusTitle.c_str(),
                                        g_state.statusLine1.c_str());
                break;
            case StatusScreenType::NONE:
                // Return to previously active non-status screen if possible.
                if (lastNormalScreen)
                {
                    lv_scr_load(lastNormalScreen);
                }
                else if (UiPageHome::getScreen() != nullptr)
                {
                    lv_scr_load(UiPageHome::getScreen());
                }
                // Mark all dirty to refresh whichever screen becomes active.
                g_state.markDirty(DirtyFlag::ALL & ~DirtyFlag::STATUS_SCREEN);
                break;
            }
            g_state.clearDirty(DirtyFlag::STATUS_SCREEN);
        }

        // Skip normal updates if status screen is showing
        if (g_state.statusScreen != StatusScreenType::NONE)
        {
            g_state.clearAllDirty();
            return;
        }

        // ═══════════════════════════════════════════════════
        // CLOCK SCREEN UPDATES
        // ═══════════════════════════════════════════════════
        if (g_state.isDirty(DirtyFlag::TIME))
        {
            UiPageClock::setTime(g_state.hour, g_state.minute, g_state.second);
            g_state.clearDirty(DirtyFlag::TIME);
        }

        if (g_state.isDirty(DirtyFlag::DATE))
        {
            UiPageClock::setGregorianDate(g_state.gregorianFull.c_str());
            UiPageHome::setStatusBarCity(g_state.location.c_str(), g_state.date.c_str());
            UiPageClock::setStatusBarCity(g_state.location.c_str(), g_state.date.c_str());
            // Settings header: no date, just "Ayarlar" (set once in create)
            g_state.clearDirty(DirtyFlag::DATE);
        }

        if (g_state.isDirty(DirtyFlag::HIJRI))
        {
            UiPageClock::setHijriDate(g_state.hijriDate.c_str());
            // Greeting row removed from Home UI; keep only Ramadan mode detection.
            const char *hijri = g_state.hijriDate.c_str();
            const char *ramPos = strstr(hijri, "Ramazan");
            // Auto-detect Ramadan mode from Hijri month
            g_state.ramadanMode = (ramPos != nullptr);
            g_state.clearDirty(DirtyFlag::HIJRI);
        }

        // ═══════════════════════════════════════════════════
        // HOME PAGE UPDATES
        // ═══════════════════════════════════════════════════
        if (g_state.isDirty(DirtyFlag::LOCATION))
        {
            UiPageHome::setStatusBarCity(g_state.location.c_str(), g_state.date.c_str());
            UiPageClock::setStatusBarCity(g_state.location.c_str(), g_state.date.c_str());
            // Settings header: no date/location update needed
            g_state.clearDirty(DirtyFlag::LOCATION);
        }

        if (g_state.isDirty(DirtyFlag::NEXT_PRAYER))
        {
            UiPageHome::setNextPrayerName(g_state.nextPrayerName.c_str());
            g_state.clearDirty(DirtyFlag::NEXT_PRAYER);
        }

        if (g_state.isDirty(DirtyFlag::COUNTDOWN))
        {
            UiPageHome::setCountdown(g_state.secondsToNext);

            // Forward Ramadan countdown to home page iftar pill
            if (g_state.ramadanMode && !g_state.ramadanCountdownText.empty())
                UiPageHome::setIftarDelta(true, g_state.ramadanCountdownText.c_str());
            else
                UiPageHome::setIftarDelta(false, nullptr);

            g_state.clearDirty(DirtyFlag::COUNTDOWN);
        }

        if (g_state.isDirty(DirtyFlag::PRAYER_TIMES))
        {
            UiPageHome::setPrayerTimes(
                g_state.fajr.c_str(), g_state.sunrise.c_str(),
                g_state.dhuhr.c_str(), g_state.asr.c_str(),
                g_state.maghrib.c_str(), g_state.isha.c_str());
            UiPageHome::setActivePrayerIndex(g_state.activePrayerIndex);
            g_state.clearDirty(DirtyFlag::PRAYER_TIMES);
        }

        if (g_state.isDirty(DirtyFlag::PROGRESS))
        {
            UiPageHome::setActivePrayerProgress(g_state.activePrayerProgress);
            g_state.clearDirty(DirtyFlag::PROGRESS);
        }

        if (g_state.isDirty(DirtyFlag::MUTED))
        {
            UiPageHome::setMuted(g_state.muted);
            UiPageClock::setMuted(g_state.muted);
            UiPageSettings::setMuted(g_state.muted);
            g_state.clearDirty(DirtyFlag::MUTED);
        }

        // NTP_SYNCED, ADHAN_AVAILABLE are not rendered in current UI
        g_state.clearDirty(DirtyFlag::NTP_SYNCED | DirtyFlag::ADHAN_AVAILABLE);

        // ═══════════════════════════════════════════════════
        // SETTINGS PAGE UPDATES
        // ═══════════════════════════════════════════════════
        if (g_state.isDirty(DirtyFlag::WIFI_STATUS))
        {
            UiPageSettings::setWiFiButtonState(g_state.wifiState, g_state.wifiIP.c_str());
            g_state.clearDirty(DirtyFlag::WIFI_STATUS);
        }

        if (g_state.isDirty(DirtyFlag::VOLUME))
        {
            UiPageSettings::setVolumeLevel(g_state.volume);
            g_state.clearDirty(DirtyFlag::VOLUME);
        }

        // ═══════════════════════════════════════════════════
        // SIGNAL & BATTERY (all pages)
        // ═══════════════════════════════════════════════════
        if (g_state.isDirty(DirtyFlag::SIGNAL_BATTERY))
        {
            uint8_t bars = g_state.wifiStrength;
            uint8_t pct = g_state.batteryPct;
            bool chg = g_state.charging;

            UiPageHome::setWifi(bars);
            UiPageHome::setBattery(pct, chg);
            UiPageClock::setWifi(bars);
            UiPageClock::setBattery(pct, chg);
            UiPageSettings::setWifi(bars);
            UiPageSettings::setBattery(pct, chg);

            g_state.clearDirty(DirtyFlag::SIGNAL_BATTERY);
        }
    }

} // namespace UiStateReader
