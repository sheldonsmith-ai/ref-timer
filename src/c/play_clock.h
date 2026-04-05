#pragma once
#include <pebble.h>

// Call from window load after the layer is created.
// Registers the layer update proc.
void play_clock_init(Layer *layer);

// Call each second from the master tick handler.
void play_clock_tick(void);

// Start the countdown.
void play_clock_start(void);

// Stop and reset to 25 seconds, cancelling any pending reset timer.
void play_clock_reset(void);

bool play_clock_is_running(void);
int  play_clock_get_seconds(void);

// Used by storage module to restore persisted state.
void play_clock_set_seconds(int s);
void play_clock_set_running(bool running);
