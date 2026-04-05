#include "storage.h"
#include "play_clock.h"
#include "game_clock.h"
#include <pebble.h>

#define PERSIST_KEY_GAME_DEFAULT  1
#define PERSIST_KEY_GAME_CURRENT  2
#define PERSIST_KEY_GAME_STATE    3
#define PERSIST_KEY_PLAY_RUNNING  4
#define PERSIST_KEY_PLAY_SECONDS  5
#define PERSIST_KEY_EXIT_TIME     6

#define GAME_CLOCK_DEFAULT (25 * 60)
#define PLAY_CLOCK_START   25

void storage_load(void) {
  int game_default = persist_exists(PERSIST_KEY_GAME_DEFAULT)
    ? persist_read_int(PERSIST_KEY_GAME_DEFAULT)
    : GAME_CLOCK_DEFAULT;
  game_clock_set_default_seconds(game_default);

  if (persist_exists(PERSIST_KEY_GAME_CURRENT)) {
    int stored_seconds = persist_read_int(PERSIST_KEY_GAME_CURRENT);
    int game_state     = persist_read_int(PERSIST_KEY_GAME_STATE);

    if (game_state == 2 && persist_exists(PERSIST_KEY_EXIT_TIME)) {
      int elapsed   = (int)time(NULL) - persist_read_int(PERSIST_KEY_EXIT_TIME);
      int remaining = stored_seconds - elapsed;
      if (remaining <= 0) {
        game_clock_set_total_seconds(game_default);
        game_clock_set_state(0);
      } else {
        game_clock_set_total_seconds(remaining);
        game_clock_set_state(2);
      }
    } else {
      game_clock_set_total_seconds(stored_seconds);
      game_clock_set_state(game_state);
    }
  } else {
    game_clock_set_total_seconds(game_default);
    game_clock_set_state(0);
  }

  if (persist_exists(PERSIST_KEY_PLAY_SECONDS)) {
    play_clock_set_seconds(persist_read_int(PERSIST_KEY_PLAY_SECONDS));
    play_clock_set_running(persist_read_int(PERSIST_KEY_PLAY_RUNNING) == 1);
  } else {
    play_clock_set_seconds(PLAY_CLOCK_START);
    play_clock_set_running(false);
  }
}

void storage_save(void) {
  persist_write_int(PERSIST_KEY_GAME_DEFAULT, game_clock_get_default_seconds());
  persist_write_int(PERSIST_KEY_GAME_CURRENT, game_clock_get_total_seconds());

  int game_state = 0;
  if (game_clock_ever_started() && !game_clock_is_running()) game_state = 1;
  if (game_clock_is_running())                               game_state = 2;
  persist_write_int(PERSIST_KEY_GAME_STATE, game_state);

  if (game_state == 2) {
    persist_write_int(PERSIST_KEY_EXIT_TIME, (int)time(NULL));
  } else {
    persist_delete(PERSIST_KEY_EXIT_TIME);
  }

  persist_write_int(PERSIST_KEY_PLAY_RUNNING, play_clock_is_running() ? 1 : 0);
  persist_write_int(PERSIST_KEY_PLAY_SECONDS, play_clock_get_seconds());
}
