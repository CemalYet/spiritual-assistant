/**
 * @file ui_page_home.h
 * @brief Home Page - Clock, Date, Prayer Card, Status Icons
 */

#ifndef UI_PAGE_HOME_H
#define UI_PAGE_HOME_H

#include <lvgl.h>

namespace UiPageHome
{
    // Create/recreate home screen
    lv_obj_t *create();

    // Get the screen object
    lv_obj_t *getScreen();

    // Update functions
    void setTime(int hour, int minute);
    void setDate(const char *date);
    void setLocation(const char *location);
    void setNextPrayer(const char *name, const char *time);
    void setNtpSynced(bool synced);
    void setAdhanAvailable(bool available);
    void setMuted(bool muted);
    bool isMuted();

} // namespace UiPageHome

#endif // UI_PAGE_HOME_H
