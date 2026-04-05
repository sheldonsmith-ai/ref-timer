#pragma once
#include <pebble.h>

// Call from window load after text layer and screen size are known.
// Loads the font, sets it on the layer, and renders the initial state.
void game_clock_init(TextLayer *layer, bool large_screen);

// Call from window unload to free the font.
void game_clock_deinit(void);

// Call each second from the master tick handler.
void game_clock_tick(void);

// Toggle running state (start sets ever_started and updates 2-min warning).
void game_clock_start(void);
void game_clock_stop(void);

bool game_clock_is_running(void);
bool game_clock_ever_started(void);
int  game_clock_get_total_seconds(void);
int  game_clock_get_default_seconds(void);

// Used by storage module to restore persisted state.
void game_clock_set_total_seconds(int s);
void game_clock_set_default_seconds(int s);
// state: 0 = never started, 1 = paused, 2 = running
void game_clock_set_state(int state);

// Edit mode: enter with fine_grained=false (coarse, 1-min steps) or true (1-sec steps).
// On coarse exit, the current time becomes the new default.
void game_clock_enter_edit_mode(bool fine_grained);
void game_clock_exit_edit_mode(void);
bool game_clock_in_edit_mode(void);
bool game_clock_is_edit_fine_grained(void);

// Increment or decrement current time by the edit step. direction: +1 or -1.
void game_clock_adjust(int direction);
