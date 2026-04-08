#include "game_clock.h"

/** Default game time: 25 minutes in seconds */
#define GAME_CLOCK_DEFAULT (25 * 60)

/** Current remaining time on the game clock */
static int s_game_total_seconds;

/** The default time that resets when the clock expires or is reset */
static int s_game_default_seconds;

/** True when the game clock is actively counting down */
static bool s_game_running;

/** True after the user has started the game clock at least once;
 *  used to determine whether to enter fine-grained or coarse-grained
 *  edit mode on next entry */
static bool s_game_ever_started;

/** True after the 2-minute warning vibration has triggered;
 *  prevents duplicate warnings on clock restart */
static bool s_game_2min_warned;

/** True while in edit mode, which shows the display inverted
 *  (white text on black background) for visibility */
static bool s_edit_mode;

/** True for fine-grained editing (1-second steps), false for
 *  coarse-grained editing (1-minute steps) */
static bool s_edit_fine_grained;

/** Temporary buffer for formatting MM:SS display string */
static char s_game_buf[8];

/** Reference to the text layer that displays the game clock */
static TextLayer *s_game_clock_layer;

/** Handle to the loaded custom font resource */
static GFont s_game_font;

/**
 * @brief Updates the game clock display with current time and color state.
 * 
 * Formats the remaining time as MM:SS and applies the appropriate color
 * scheme based on the current state:
 * - Edit mode: inverted colors (white on black) for visibility
 * - Running: green text
 * - Never started: black text on clear background
 * - Stopped after start: red text to indicate paused state
 * 
 * Call this function after any state change to keep the display current.
 */
static void prv_update_display(void) {
  int minutes = s_game_total_seconds / 60;
  int seconds = s_game_total_seconds % 60;
  snprintf(s_game_buf, sizeof(s_game_buf), "%02d:%02d", minutes, seconds);
  text_layer_set_text(s_game_clock_layer, s_game_buf);

  if (s_edit_mode) {
    text_layer_set_text_color(s_game_clock_layer, GColorWhite);
    text_layer_set_background_color(s_game_clock_layer, GColorBlack);
  } else {
    GColor text_color;
    if      (s_game_running)       text_color = GColorIslamicGreen;
    else if (!s_game_ever_started) text_color = GColorBlack;
    else                           text_color = GColorRed;
    text_layer_set_text_color(s_game_clock_layer, text_color);
    text_layer_set_background_color(s_game_clock_layer, GColorClear);
  }
}

/**
 * @brief Initializes the game clock text layer and loads the appropriate font.
 * 
 * Must be called during window load after the text layer is created and
 * its size is known. The large_screen parameter determines which font
 * resource to load (48pt for large screens, 32pt for small screens).
 * 
 * @param layer Pointer to the pre-created text layer to configure
 * @param large_screen True if screen height >= 200px, selects 48pt font
 */
void game_clock_init(TextLayer *layer, bool large_screen) {
  s_game_clock_layer = layer;
  s_game_font = fonts_load_custom_font(resource_get_handle(
    large_screen ? RESOURCE_ID_GAME_CLOCK_48 : RESOURCE_ID_GAME_CLOCK_32));
  text_layer_set_font(layer, s_game_font);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  prv_update_display();
}

/**
 * @brief Releases the custom font resource allocated by game_clock_init.
 * 
 * Must be called during window unload to prevent memory leaks. After
 * this call, the font handle is invalid and the layer no longer renders.
 */
void game_clock_deinit(void) {
  fonts_unload_custom_font(s_game_font);
}

/**
 * @brief Processes one second of countdown for the game clock.
 * 
 * Should be called once per second by the master tick handler. Decrements
 * the remaining time, triggers the 2-minute warning vibration when
 * appropriate, and handles clock expiration by resetting to the default
 * value and clearing all state flags.
 * 
 * The warning flag prevents duplicate vibrations if the clock is restarted
 * with <= 2 minutes remaining.
 * 
 * @pre game_clock_init() must have been called
 */
void game_clock_tick(void) {
  if (!s_game_running) return;
  s_game_total_seconds--;

  // 2-minute warning: long vibration pulse, one-time trigger
  if (s_game_total_seconds == 120 && !s_game_2min_warned) {
    vibes_long_pulse();
    s_game_2min_warned = true;
  } else if (s_game_total_seconds == 0) {
    // Clock expired: reset and clear all state
    vibes_long_pulse();
    s_game_running       = false;
    s_game_ever_started  = false;
    s_game_2min_warned   = false;
    s_game_total_seconds = s_game_default_seconds;
  }

  prv_update_display();
}

/**
 * @brief Starts the game clock countdown.
 * 
 * Marks the clock as running and ever-started. If the remaining time is
 * 2 minutes or less, immediately sets the warning flag to prevent a
 * duplicate warning on the next tick. Always updates the display.
 */
void game_clock_start(void) {
  s_game_running      = true;
  s_game_ever_started = true;
  if (!s_game_2min_warned && s_game_total_seconds <= 120) {
    s_game_2min_warned = true;
  }
  vibes_short_pulse();
  prv_update_display();
}

/**
 * @brief Stops the game clock without resetting.
 *
 * Halts the countdown but preserves the remaining time and state.
 * The clock can be restarted with game_clock_start().
 */
void game_clock_stop(void) {
  s_game_running = false;
  vibes_short_pulse();
  prv_update_display();
}

bool game_clock_is_running(void)          { return s_game_running; }
bool game_clock_ever_started(void)        { return s_game_ever_started; }
int  game_clock_get_total_seconds(void)   { return s_game_total_seconds; }
int  game_clock_get_default_seconds(void) { return s_game_default_seconds; }

/**
 * @brief Sets the current time in seconds.
 * 
 * Used by storage_load() to restore the persisted time value. The caller
 * is responsible for ensuring the value is within valid bounds (1-5999).
 * 
 * @param s Time in seconds (1-5999)
 */
void game_clock_set_total_seconds(int s) { s_game_total_seconds = s; }

/**
 * @brief Sets the default time used on clock expiration.
 * 
 * Used by storage_load() to restore the persisted default value. This
 * value is also used when the user resets the clock via long-press SELECT.
 * 
 * @param s Default time in seconds (1-5999)
 */
void game_clock_set_default_seconds(int s) { s_game_default_seconds = s; }

/**
 * @brief Restores a previously saved game clock state.
 * 
 * This function reconstructs the full state from a compact persisted
 * representation. It correctly handles the edge case where a running
 * game was interrupted by setting the warning flag based on the restored
 * time value.
 * 
 * State encoding:
 * - 0: Never started (never started, reset state)
 * - 1: Stopped/paused (ever started, not running)
 * - 2: Running (ever started, running)
 * 
 * @param state Encoded state value (0, 1, or 2)
 */
void game_clock_set_state(int state) {
  s_game_running      = (state == 2);
  s_game_ever_started = (state != 0);
  // Pre-set the warning flag if we're restoring a running game with <= 2 min
  s_game_2min_warned  = (s_game_total_seconds <= 120 && s_game_ever_started);
}

/**
 * @brief Enters edit mode to adjust the game clock time.
 * 
 * Stops any running clock and inverts the display for visibility. The
 * fine_grained parameter determines the adjustment granularity:
 * - false (coarse): 1-minute steps, sets new default on exit
 * - true (fine): 1-second steps, does not change default
 * 
 * This mode is entered via long-press SELECT and exited via single SELECT.
 * 
 * @param fine_grained True for 1-second steps, false for 1-minute steps
 */
void game_clock_enter_edit_mode(bool fine_grained) {
  s_game_running      = false;
  s_edit_mode         = true;
  s_edit_fine_grained = fine_grained;
  prv_update_display();
}

/**
 * @brief Exits edit mode and optionally updates the default time.
 * 
 * When exiting from coarse-grained edit, the current time becomes the new
 * default and will be persisted. Fine-grained edits are discarded and do
 * not affect the default. The display returns to normal colors.
 */
void game_clock_exit_edit_mode(void) {
  // Coarse edit (minutes) creates a new default; fine edit (seconds) is temporary
  if (!s_edit_fine_grained) {
    s_game_default_seconds = s_game_total_seconds;
  }
  s_edit_mode = false;
  prv_update_display();
}

bool game_clock_in_edit_mode(void)         { return s_edit_mode; }

/**
 * @brief Checks if the current edit mode uses 1-second granularity.
 * 
 * @return True if fine-grained (1-second steps), false if coarse-grained (1-minute)
 */
bool game_clock_is_edit_fine_grained(void) { return s_edit_fine_grained; }

/**
 * @brief Resets the current time to the stored default value.
 * 
 * Called when the user long-presses SELECT while in edit mode. This
 * provides a quick way to recover from accidental adjustments. The clock
 * remains in edit mode after the reset.
 */
void game_clock_reset_to_default(void) {
  s_game_total_seconds = s_game_default_seconds;
  prv_update_display();
}

/**
 * @brief Adjusts the game clock time by the edit step.
 * 
 * The step size depends on the current edit mode:
 * - Fine-grained: 1 second
 * - Coarse-grained: 60 seconds (1 minute)
 * 
 * The adjusted value is clamped to the valid range (1-5999 seconds, or
 * 0:01 to 99:59) to prevent invalid states. This function is used by
 * both single-press and long-press repeat handlers.
 * 
 * @param direction +1 to increase, -1 to decrease the time
 */
void game_clock_adjust(int direction) {
  int step = s_edit_fine_grained ? 1 : 60;
  s_game_total_seconds += direction * step;
  // Keep within valid range: minimum 1 second, maximum 5999 seconds (99:59)
  if (s_game_total_seconds < 1)    s_game_total_seconds = 1;
  if (s_game_total_seconds > 5999) s_game_total_seconds = 5999;
  prv_update_display();
}
