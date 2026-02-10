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
#include "ui_page_prayer.h"
#include "ui_page_settings.h"
#include "ui_page_status.h"
#include <lvgl.h>

namespace UiStateReader
{
    static lv_timer_t *updateTimer = nullptr;

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
                // Return to home screen (only if pages are created)
                if (UiPageHome::getScreen() != nullptr)
                {
                    lv_scr_load(UiPageHome::getScreen());
                    // Mark all dirty to refresh home screen
                    g_state.markDirty(DirtyFlag::ALL & ~DirtyFlag::STATUS_SCREEN);
                }
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
        // HOME PAGE UPDATES
        // ═══════════════════════════════════════════════════
        if (g_state.isDirty(DirtyFlag::TIME))
        {
            UiPageHome::setTime(g_state.hour, g_state.minute);
            g_state.clearDirty(DirtyFlag::TIME);
        }

        if (g_state.isDirty(DirtyFlag::DATE))
        {
            UiPageHome::setDate(g_state.date.c_str());
            g_state.clearDirty(DirtyFlag::DATE);
        }

        if (g_state.isDirty(DirtyFlag::LOCATION))
        {
            UiPageHome::setLocation(g_state.location.c_str());
            g_state.clearDirty(DirtyFlag::LOCATION);
        }

        if (g_state.isDirty(DirtyFlag::NEXT_PRAYER))
        {
            UiPageHome::setNextPrayer(g_state.nextPrayerName.c_str(), g_state.nextPrayerTime.c_str());
            g_state.clearDirty(DirtyFlag::NEXT_PRAYER);
        }

        if (g_state.isDirty(DirtyFlag::NTP_SYNCED))
        {
            UiPageHome::setNtpSynced(g_state.ntpSynced);
            g_state.clearDirty(DirtyFlag::NTP_SYNCED);
        }

        if (g_state.isDirty(DirtyFlag::ADHAN_AVAILABLE))
        {
            UiPageHome::setAdhanAvailable(g_state.adhanAvailable);
            g_state.clearDirty(DirtyFlag::ADHAN_AVAILABLE);
        }

        if (g_state.isDirty(DirtyFlag::MUTED))
        {
            UiPageHome::setMuted(g_state.muted);
            g_state.clearDirty(DirtyFlag::MUTED);
        }

        // ═══════════════════════════════════════════════════
        // PRAYER PAGE UPDATES
        // ═══════════════════════════════════════════════════
        if (g_state.isDirty(DirtyFlag::PRAYER_TIMES))
        {
            UiPagePrayer::PrayerTimesData data = {
                g_state.fajr.c_str(),
                g_state.sunrise.c_str(),
                g_state.dhuhr.c_str(),
                g_state.asr.c_str(),
                g_state.maghrib.c_str(),
                g_state.isha.c_str(),
                g_state.activePrayerIndex};
            UiPagePrayer::setPrayerTimes(data);
            g_state.clearDirty(DirtyFlag::PRAYER_TIMES);
        }

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
    }

} // namespace UiStateReader
