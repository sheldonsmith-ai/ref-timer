#pragma once
#include <pebble.h>

/**
 * @brief Initializes the play clock subsystem.
 * 
 * Associates the play clock with the provided layer and registers the
 * render callback. The initial display is rendered by the window's first
 * paint, so mark_dirty is not called here. This function must be called
 * during window load after the layer is created.
 * 
 * @param layer Pre-created layer for play clock display
 */
void play_clock_init(Layer *layer);

/**
 * @brief Processes one second of countdown for the play clock.
 * 
 * Should be called once per second by a master tick handler. Decrements
 * the remaining time, triggers haptic warnings at 10 seconds and during
 * the final 5 seconds, and handles expiration with auto-reset. No-op
 * when the clock is not running.
 */
void play_clock_tick(void);

/**
 * @brief Starts the play clock countdown.
 * 
 * Sets the running state and triggers a re-render. The clock should
 * be at PLAY_CLOCK_START (25 seconds) when first started.
 */
void play_clock_start(void);

/**
 * @brief Stops and resets the play clock to PLAY_CLOCK_START.
 * 
 * Cancels any pending auto-reset timer. Used when the user manually
 * resets the clock while it's running or stopped.
 */
void play_clock_reset(void);

/**
 * @brief Checks if the play clock is currently running.
 * 
 * @return True if actively counting down, false otherwise
 */
bool play_clock_is_running(void);

/**
 * @brief Gets the current remaining time on the play clock.
 * 
 * @return Remaining seconds (0-25)
 */
int play_clock_get_seconds(void);

/**
 * @brief Restores the play clock time from persisted state.
 * 
 * @param s Seconds to set (should be 0-25)
 */
void play_clock_set_seconds(int s);

/**
 * @brief Restores the play clock running state from persisted state.
 * 
 * @param running True if clock should be running, false if stopped
 */
void play_clock_set_running(bool running);
