#pragma once

/**
 * @brief Restores all application state from persistent flash storage.
 * 
 * Reads persisted values for game clock (time, state, default) and play
 * clock (time, running). If a game was running when the app backgrounded,
 * calculates elapsed time to restore the correct remaining time.
 * 
 * Missing or corrupted data is handled gracefully by initializing to
 * default values.
 * 
 * Must be called before window creation so the initial render displays
 * the restored state.
 */
void storage_load(void);

/**
 * @brief Persists all application state to flash storage.
 * 
 * Saves current game clock and play clock state, including the timestamp
 * of when a running game was interrupted. This enables correct resumption
 * if the app is backgrounded and resumed later.
 * 
 * Must be called before window popup or app termination to prevent data
 * loss. The function blocks briefly during flash write operations.
 */
void storage_save(void);
