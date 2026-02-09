/**
 * @file ui_components.h
 * @brief Reusable UI Components
 */

#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <lvgl.h>

namespace UiComponents
{
    // Navigation bar callback type
    using NavClickCallback = void (*)(int page);

    // Set the navigation callback (called when nav button pressed)
    void setNavClickCallback(NavClickCallback cb);

    // Create navigation bar at bottom of screen
    // parent: screen to add nav bar to
    // activePage: 0=Home, 1=Prayer, 2=Settings
    void createNavBar(lv_obj_t *parent, int activePage);

} // namespace UiComponents

#endif // UI_COMPONENTS_H
