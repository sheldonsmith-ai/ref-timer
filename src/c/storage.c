#include "storage.h"
#include "play_clock.h"
#include "game_clock.h"
#include <pebble.h>

/** Persistence key: default game clock time in seconds */
#define PERSIST_KEY_GAME_DEFAULT  1

/** Persistence key: current game clock time in seconds */
#define PERSIST_KEY_GAME_CURRENT  2

/** Persistence key: game clock state (0=never, 1=stopped, 2=running) */
#define PERSIST_KEY_GAME_STATE    3

/** Persistence key: play clock running flag (1=true, 0=false) */
#define PERSIST_KEY_PLAY_RUNNING  4

/** Persistence key: play clock remaining seconds */
#define PERSIST_KEY_PLAY_SECONDS  5

/** Persistence key: timestamp when app backgrounded (for running game) */
#define PERSIST_KEY_EXIT_TIME     6

/** Default game clock value: 25 minutes in seconds */
#define GAME_CLOCK_DEFAULT (25 * 60)


/**
 * @brief Restores all application state from persistent storage.
 * 
 * Reads persisted values for both game and play clocks. If a running game
 * was interrupted, calculates elapsed time since backgrounding to update
 * the remaining time. Missing or corrupted data is handled gracefully by
 * initializing to default values.
 * 
 * This function must be called before window creation so the initial render
 * displays the restored state.
 */
void storage_load(void) {
  // Restore or initialize game clock default
  int game_default = persist_exists(PERSIST_KEY_GAME_DEFAULT)
    ? persist_read_int(PERSIST_KEY_GAME_DEFAULT)
    : GAME_CLOCK_DEFAULT;
  game_clock_set_default_seconds(game_default);

  // Restore or initialize game clock state
  if (persist_exists(PERSIST_KEY_GAME_CURRENT)) {
    int stored_seconds = persist_read_int(PERSIST_KEY_GAME_CURRENT);
    int game_state     = persist_read_int(PERSIST_KEY_GAME_STATE);

    // If game was running when backgrounded, calculate elapsed time
    if (game_state == 2 && persist_exists(PERSIST_KEY_EXIT_TIME)) {
      int elapsed   = (int)time(NULL) - persist_read_int(PERSIST_KEY_EXIT_TIME);
      int remaining = stored_seconds - elapsed;
      if (remaining <= 0) {
        // Time expired while backgrounded: reset to default
        game_clock_set_total_seconds(game_default);
        game_clock_set_state(0);
      } else {
        // Continue running from remaining time
        game_clock_set_total_seconds(remaining);
        game_clock_set_state(2);
      }
    } else {
      // Normal pause or never-running state
      game_clock_set_total_seconds(stored_seconds);
      game_clock_set_state(game_state);
    }
  } else {
    // No persisted state: initialize to default
    game_clock_set_total_seconds(game_default);
    game_clock_set_state(0);
  }

  // Restore or initialize play clock
  if (persist_exists(PERSIST_KEY_PLAY_SECONDS)) {
    play_clock_set_seconds(persist_read_int(PERSIST_KEY_PLAY_SECONDS));
    play_clock_set_running(persist_read_int(PERSIST_KEY_PLAY_RUNNING) == 1);
  } else {
    play_clock_set_running(false);
  }
}

/**
 * @brief Persists all application state to flash storage.
 * 
 * Saves the current state of both game and play clocks. When the game
 * clock is running, records the current time so the app can resume
 * correctly after backgrounding. The exit time is only saved when
 * running to avoid unnecessary writes.
 * 
 * This function blocks briefly during flash write operations. Should be
 * called before app termination or window popup to ensure no data loss.
 */
void storage_save(void) {
  // Game clock: persist default, current time, and running state
  persist_write_int(PERSIST_KEY_GAME_DEFAULT, game_clock_get_default_seconds());
  persist_write_int(PERSIST_KEY_GAME_CURRENT, game_clock_get_total_seconds());

  // Encode state: 0=never started, 1=stopped, 2=running
  int game_state = 0;
  if (game_clock_ever_started() && !game_clock_is_running()) game_state = 1;
  if (game_clock_is_running())                               game_state = 2;
  persist_write_int(PERSIST_KEY_GAME_STATE, game_state);

  // Track when a running game was interrupted for time correction
  if (game_state == 2) {
    persist_write_int(PERSIST_KEY_EXIT_TIME, (int)time(NULL));
  } else {
    // No need to track expiration time when not running
    persist_delete(PERSIST_KEY_EXIT_TIME);
  }

  // Play clock: persist running flag and remaining time
  persist_write_int(PERSIST_KEY_PLAY_RUNNING, play_clock_is_running() ? 1 : 0);
  persist_write_int(PERSIST_KEY_PLAY_SECONDS, play_clock_get_seconds());
}
