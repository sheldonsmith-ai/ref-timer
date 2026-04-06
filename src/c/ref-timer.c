#include <pebble.h>
#include "segment_display.h"
#include "play_clock.h"
#include "game_clock.h"
#include "storage.h"

// ── Constants ─────────────────────────────────────────────────────
#define LONG_PRESS_REPEAT_MS  75

// ── Layers ────────────────────────────────────────────────────────
static Window     *s_window;
static Layer      *s_play_clock_layer;
static TextLayer  *s_game_clock_layer;
static Layer      *s_divider_layer;

// ── Timers ────────────────────────────────────────────────────────
static AppTimer   *s_tick_timer;
static AppTimer   *s_long_press_repeat_timer;

/**
 * @brief Direction (+1 or -1) for the active long-press operation.
 * 
 * Set when a long-press begins, used by the repeat callback to determine
 * whether to increment or decrement the game clock value. Only valid when
 * game_clock_in_edit_mode() returns true.
 */
static int s_long_press_direction;

/**
 * @brief Repeatedly adjusts the game clock while a long-press is held.
 * 
 * Implements auto-repeat functionality: after the initial adjustment,
 * schedules itself to run again after LONG_PRESS_REPEAT_MS. This creates
 * a smooth continuous adjustment experience without requiring repeated
 * button presses.
 * 
 * The callback self-reschedules only if still in edit mode; otherwise
 * the timer chain terminates naturally.
 * 
 * @param context Unused (required by AppTimer signature)
 */
static void prv_long_press_repeat_callback(void *context) {
  if (!game_clock_in_edit_mode()) return;
  game_clock_adjust(s_long_press_direction);
  s_long_press_repeat_timer = app_timer_register(
    LONG_PRESS_REPEAT_MS, prv_long_press_repeat_callback, NULL);
}

/**
 * @brief Begins a long-press adjustment sequence.
 * 
 * Performs the initial adjustment immediately, then schedules the repeat
 * callback. This pattern ensures responsive feedback on the first press
 * while enabling continuous adjustment for held presses.
 */
static void prv_start_long_press_repeat(void) {
  game_clock_adjust(s_long_press_direction);
  s_long_press_repeat_timer = app_timer_register(
    LONG_PRESS_REPEAT_MS, prv_long_press_repeat_callback, NULL);
}

/**
 * @brief Cancels any active long-press repeat sequence.
 * 
 * Safely handles the case where no timer is active by checking for NULL.
 * Must be called when the long-press ends to prevent orphaned callbacks.
 */
static void prv_stop_long_press_repeat(void) {
  if (s_long_press_repeat_timer) {
    app_timer_cancel(s_long_press_repeat_timer);
    s_long_press_repeat_timer = NULL;
  }
}

/**
 * @brief Master timer callback that updates both clocks once per second.
 * 
 * Centralizes the timekeeping logic for the entire application. By having
 * a single source of truth for the 1-second tick, we avoid timer drift
 * that could occur with separate timers. Self-reschedules to maintain
 * continuous operation throughout the app session.
 * 
 * @param context Unused (required by AppTimer signature)
 */
static void prv_tick_handler(void *context) {
  play_clock_tick();
  game_clock_tick();
  s_tick_timer = app_timer_register(1000, prv_tick_handler, NULL);
}

/**
 * @brief UP button single-click handler.
 * 
 * In edit mode: increments the game clock by the current step size.
 * In normal mode: starts the play clock if stopped, resets if running.
 * 
 * @param r Click recognizer reference (unused)
 * @param ctx User context pointer (unused)
 */
static void prv_up_single_handler(ClickRecognizerRef r, void *ctx) {
  if (game_clock_in_edit_mode()) { game_clock_adjust(+1); return; }
  if (play_clock_is_running()) play_clock_reset();
  else                         play_clock_start();
}

/**
 * @brief UP button long-press begin callback.
 * 
 * Initiates continuous time increase for rapid adjustment. Only active
 * in edit mode where changing the game clock makes sense.
 * 
 * @param r Click recognizer reference (unused)
 * @param ctx User context pointer (unused)
 */
static void prv_up_long_press_begin(ClickRecognizerRef r, void *ctx) {
  if (!game_clock_in_edit_mode()) return;
  s_long_press_direction = +1;
  prv_start_long_press_repeat();
}

/**
 * @brief UP button long-press end callback.
 */
static void prv_up_long_press_end(ClickRecognizerRef r, void *ctx) {
  prv_stop_long_press_repeat();
}

/**
 * @brief DOWN button single-click handler.
 * 
 * In edit mode: decrements the game clock by the current step size.
 * In normal mode: toggles the game clock start/stop state.
 * 
 * @param r Click recognizer reference (unused)
 * @param ctx User context pointer (unused)
 */
static void prv_down_single_handler(ClickRecognizerRef r, void *ctx) {
  if (game_clock_in_edit_mode()) { game_clock_adjust(-1); return; }
  if (game_clock_is_running()) game_clock_stop();
  else                         game_clock_start();
}

/**
 * @brief DOWN button long-press begin callback.
 * 
 * Initiates continuous time decrease for rapid downward adjustment.
 * 
 * @param r Click recognizer reference (unused)
 * @param ctx User context pointer (unused)
 */
static void prv_down_long_press_begin(ClickRecognizerRef r, void *ctx) {
  if (!game_clock_in_edit_mode()) return;
  s_long_press_direction = -1;
  prv_start_long_press_repeat();
}

/**
 * @brief DOWN button long-press end callback.
 */
static void prv_down_long_press_end(ClickRecognizerRef r, void *ctx) {
  prv_stop_long_press_repeat();
}

/**
 * @brief SELECT button single-click handler.
 * 
 * Exits edit mode. On coarse-grained exit, the current time becomes
 * the new default and is persisted immediately. This ensures that even
 * if the app crashes before the user exits, their chosen default survives.
 * Fine-grained edits (temporary adjustments) are discarded on exit.
 * 
 * @param r Click recognizer reference (unused)
 * @param ctx User context pointer (unused)
 */
static void prv_select_single_handler(ClickRecognizerRef r, void *ctx) {
  if (!game_clock_in_edit_mode()) return;
  bool was_fine = game_clock_is_edit_fine_grained();
  game_clock_exit_edit_mode();
  // Persist immediately on coarse-grained exit so the new default survives crashes.
  if (!was_fine) storage_save();
}

/**
 * @brief SELECT button long-press handler.
 * 
 * Toggles edit mode: enters if not in edit mode, resets to default if in
 * edit mode. The edit mode type (fine/coarse) is determined by whether
 * the game clock was ever started—this allows setting initial defaults
 * (coarse minute steps) while also adjusting running games (fine seconds).
 * 
 * @param r Click recognizer reference (unused)
 * @param ctx User context pointer (unused)
 */
static void prv_select_long_press_handler(ClickRecognizerRef r, void *ctx) {
  if (game_clock_in_edit_mode()) {
    game_clock_reset_to_default();
    return;
  }
  game_clock_enter_edit_mode(game_clock_ever_started());
}

/**
 * @brief BACK button handler.
 * 
 * Persists current state and exits the app. Saving before exit ensures
 * all state survives app termination, whether normal or crash-induced.
 */
static void prv_back_handler(ClickRecognizerRef r, void *ctx) {
  storage_save();
  window_stack_pop(true);
}

// ── Click Config ──────────────────────────────────────────────────

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_single_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 500,
    prv_up_long_press_begin, prv_up_long_press_end);

  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_single_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 500,
    prv_down_long_press_begin, prv_down_long_press_end);

  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_single_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500,
    prv_select_long_press_handler, NULL);

  window_single_click_subscribe(BUTTON_ID_BACK, prv_back_handler);
}

/**
 * @brief Divider line render callback for separating play and game clocks.
 * 
 * Draws a solid dark gray horizontal bar across the width of the layer.
 * The layer bounds determine the width, so the divider adapts to different
 * screen sizes without hard-coded dimensions.
 * 
 * @param layer The layer being rendered
 * @param ctx Graphics context for drawing commands
 */
static void prv_divider_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

/**
 * @brief Window unload callback that cleans up all allocated resources.
 * 
 * Destroys layers, unloads fonts, and frees GPath objects. The order
 * matters: child layers before parent, GPaths after their using layers.
 * 
 * @param window The window being unloaded (unused but required by signature)
 */
static void prv_window_unload(Window *window) {
  layer_destroy(s_play_clock_layer);
  text_layer_destroy(s_game_clock_layer);
  layer_destroy(s_divider_layer);
  seg_destroy_paths();
  game_clock_deinit();
}

/**
 * @brief Application entry point that loads state and initializes the window.
 * 
 * Load order is critical: storage must load before the window displays so
 * the initial render reflects any persisted state. The tick timer starts
 * immediately to begin timekeeping.
 */
static void prv_init(void) {
  storage_load();

  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  s_tick_timer = app_timer_register(1000, prv_tick_handler, NULL);
}

/**
 * @brief Application cleanup that cancels timers and destroys the window.
 * 
 * Cancels any pending timers before destroying the window to prevent
 * callbacks on invalidated memory. This function should never be called
 * in normal operation (user would need to kill the app).
 */
static void prv_deinit(void) {
  if (s_tick_timer)              app_timer_cancel(s_tick_timer);
  if (s_long_press_repeat_timer) app_timer_cancel(s_long_press_repeat_timer);
  window_destroy(s_window);
}

/**
 * @brief Application entry point.
 * 
 * Sets up the app, enters the Pebble event loop (which never returns),
 * and deinit is unreachable in normal operation. The deinit function
 * exists for completeness and potential future use cases.
 */
int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_play_clock_layer);
  text_layer_destroy(s_game_clock_layer);
  layer_destroy(s_divider_layer);
  seg_destroy_paths();
  game_clock_deinit();
}

// ── App Lifecycle ─────────────────────────────────────────────────

static void prv_init(void) {
  storage_load();

  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  s_tick_timer = app_timer_register(1000, prv_tick_handler, NULL);
}

static void prv_deinit(void) {
  if (s_tick_timer)              app_timer_cancel(s_tick_timer);
  if (s_long_press_repeat_timer) app_timer_cancel(s_long_press_repeat_timer);
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
