#pragma once
#include <pebble.h>

/**
 * @brief Initializes the game clock subsystem.
 * 
 * Loads the appropriate custom font based on screen size, configures
 * the text layer, and renders the initial state. This function must
 * be called during window load, after the text layer is created and
 * its bounds are known.
 * 
 * @param layer Pre-created text layer to configure
 * @param large_screen True for screens >= 200px height, selects 48pt font
 */
void game_clock_init(TextLayer *layer, bool large_screen);

/**
 * @brief Releases resources allocated by game_clock_init().
 * 
 * Unloads the custom font and invalidates the layer. Must be called
 * during window unload to prevent memory leaks.
 */
void game_clock_deinit(void);

/**
 * @brief Processes one second of countdown.
 * 
 * Should be called once per second by a master tick handler. Updates
 * the remaining time, triggers warnings at milestones, and handles
 * expiration. No-op when the clock is not running.
 */
void game_clock_tick(void);

/**
 * @brief Starts the game clock countdown.
 * 
 * Sets the running state and marks the clock as having been started.
 * If the remaining time is <= 2 minutes, the 2-minute warning flag
 * is set to prevent a duplicate warning on the next tick.
 */
void game_clock_start(void);

/**
 * @brief Stops the game clock without resetting.
 * 
 * Halts the countdown while preserving the remaining time and state.
 * The clock can be restarted with game_clock_start().
 */
void game_clock_stop(void);

/**
 * @brief Checks if the game clock is currently running.
 * 
 * @return True if actively counting down, false otherwise
 */
bool game_clock_is_running(void);

/**
 * @brief Checks if the game clock has ever been started.
 * 
 * This distinguishes between a fresh game (no edits yet) and a game
 * that has been paused and is being edited. Used to determine whether
 * to enter fine-grained or coarse-grained edit mode.
 * 
 * @return True if game_clock_start() has been called at least once
 */
bool game_clock_ever_started(void);

/**
 * @brief Gets the current remaining time on the game clock.
 * 
 * @return Time in seconds (1-5999)
 */
int game_clock_get_total_seconds(void);

/**
 * @brief Gets the default time used on clock expiration or reset.
 * 
 * @return Default time in seconds (1-5999)
 */
int game_clock_get_default_seconds(void);

/**
 * @brief Restores the current time from persisted state.
 * 
 * Used by storage_load() to set the game clock after reading from flash.
 * The caller is responsible for ensuring the value is within valid bounds.
 * 
 * @param s Time in seconds (1-5999)
 */
void game_clock_set_total_seconds(int s);

/**
 * @brief Restores the default time from persisted state.
 * 
 * Used by storage_load() to set the default after reading from flash.
 * 
 * @param s Default time in seconds (1-5999)
 */
void game_clock_set_default_seconds(int s);

/**
 * @brief Restores the complete game clock state from a compact encoding.
 * 
 * Reconstructs running, ever_started, and warning states from a single
 * persisted value. Correctly handles the edge case where a running game
 * is restored with <= 2 minutes remaining by pre-setting the warning flag.
 * 
 * @param state Encoded state: 0=never started, 1=stopped, 2=running
 */
void game_clock_set_state(int state);

/**
 * @brief Enters edit mode to adjust the game clock time.
 * 
 * Stops any running clock and inverts the display for visibility. The
 * fine_grained parameter determines the adjustment granularity:
 * - false (coarse): 1-minute steps, current time becomes new default on exit
 * - true (fine): 1-second steps, does not modify default
 * 
 * @param fine_grained True for 1-second steps, false for 1-minute steps
 */
void game_clock_enter_edit_mode(bool fine_grained);

/**
 * @brief Exits edit mode and commits changes.
 * 
 * When exiting from coarse-grained edit, the current time becomes the
 * new default. Fine-grained edits are discarded. The display returns
 * to normal colors. Storage_save() should be called after this function
 * to persist the new default.
 */
void game_clock_exit_edit_mode(void);

/**
 * @brief Checks if currently in edit mode.
 * 
 * @return True if the game clock is in edit mode (display inverted)
 */
bool game_clock_in_edit_mode(void);

/**
 * @brief Checks if the current edit mode uses 1-second granularity.
 * 
 * @return True for fine-grained editing (1-sec steps), false for coarse (1-min)
 */
bool game_clock_is_edit_fine_grained(void);

/**
 * @brief Adjusts the game clock time by the edit step size.
 * 
 * Uses the current edit mode's step size (1 sec or 1 min). The adjusted
 * value is clamped to the valid range (1-5999 seconds). This function
 * is called by both single-press and long-press repeat handlers.
 * 
 * @param direction +1 to increase, -1 to decrease
 */
void game_clock_adjust(int direction);

/**
 * @brief Resets the current time to the stored default value.
 * 
 * Used when the user long-presses SELECT while in edit mode. The clock
 * remains in edit mode after the reset.
 */
void game_clock_reset_to_default(void);
