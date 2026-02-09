/**
 * @file ui_state_reader.h
 * @brief UI State Reader - Polls AppState dirty flags
 *
 * Connects shared state to UI modules.
 * Called from LVGL timer or main loop.
 */

#ifndef UI_STATE_READER_H
#define UI_STATE_READER_H

namespace UiStateReader
{
    /**
     * @brief Initialize UI state reader
     * Creates LVGL timer for periodic state polling
     */
    void init();

    /**
     * @brief Process dirty flags and update UI
     * Call this periodically (or let LVGL timer handle it)
     */
    void update();

} // namespace UiStateReader

#endif // UI_STATE_READER_H
