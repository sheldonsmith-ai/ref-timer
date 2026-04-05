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

// ── Long-Press State ──────────────────────────────────────────────
static int         s_long_press_direction;

// ── Long Press Repeat Timer ───────────────────────────────────────

static void prv_long_press_repeat_callback(void *context) {
  if (!game_clock_in_edit_mode()) return;
  game_clock_adjust(s_long_press_direction);
  s_long_press_repeat_timer = app_timer_register(
    LONG_PRESS_REPEAT_MS, prv_long_press_repeat_callback, NULL);
}

static void prv_start_long_press_repeat(void) {
  game_clock_adjust(s_long_press_direction);
  s_long_press_repeat_timer = app_timer_register(
    LONG_PRESS_REPEAT_MS, prv_long_press_repeat_callback, NULL);
}

static void prv_stop_long_press_repeat(void) {
  if (s_long_press_repeat_timer) {
    app_timer_cancel(s_long_press_repeat_timer);
    s_long_press_repeat_timer = NULL;
  }
}

// ── Master 1-Second Tick ──────────────────────────────────────────

static void prv_tick_handler(void *context) {
  play_clock_tick();
  game_clock_tick();
  s_tick_timer = app_timer_register(1000, prv_tick_handler, NULL);
}

// ── Button Handlers ───────────────────────────────────────────────

static void prv_up_single_handler(ClickRecognizerRef r, void *ctx) {
  if (game_clock_in_edit_mode()) { game_clock_adjust(+1); return; }
  if (play_clock_is_running()) play_clock_reset();
  else                         play_clock_start();
}

static void prv_up_long_press_begin(ClickRecognizerRef r, void *ctx) {
  if (!game_clock_in_edit_mode()) return;
  s_long_press_direction = +1;
  prv_start_long_press_repeat();
}

static void prv_up_long_press_end(ClickRecognizerRef r, void *ctx) {
  prv_stop_long_press_repeat();
}

static void prv_down_single_handler(ClickRecognizerRef r, void *ctx) {
  if (game_clock_in_edit_mode()) { game_clock_adjust(-1); return; }
  if (game_clock_is_running()) game_clock_stop();
  else                         game_clock_start();
}

static void prv_down_long_press_begin(ClickRecognizerRef r, void *ctx) {
  if (!game_clock_in_edit_mode()) return;
  s_long_press_direction = -1;
  prv_start_long_press_repeat();
}

static void prv_down_long_press_end(ClickRecognizerRef r, void *ctx) {
  prv_stop_long_press_repeat();
}

static void prv_select_single_handler(ClickRecognizerRef r, void *ctx) {
  if (!game_clock_in_edit_mode()) return;
  bool was_fine = game_clock_is_edit_fine_grained();
  game_clock_exit_edit_mode();
  // Persist immediately on coarse-grained exit so the new default survives crashes.
  if (!was_fine) storage_save();
}

static void prv_select_long_press_handler(ClickRecognizerRef r, void *ctx) {
  if (game_clock_in_edit_mode()) {
    game_clock_reset_to_default();
    return;
  }
  game_clock_enter_edit_mode(game_clock_ever_started());
}

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

// ── Divider Layer ─────────────────────────────────────────────────

static void prv_divider_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

// ── Window Lifecycle ──────────────────────────────────────────────

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  int sw = bounds.size.w;
  int sh = bounds.size.h;

  window_set_background_color(window, GColorWhite);

  seg_compute_geometry(sw, sh);
  seg_create_paths();

  // Layout: play clock (~63%), divider, game clock (~37%)
  int play_h    = sh * 63 / 100;
  int divider_y = play_h;
  int game_y    = divider_y + 3;
  int game_h    = sh - game_y;

  // Horizontal inset for round displays (keeps content off curved edges)
  int h_inset = PBL_IF_ROUND_ELSE(8, 0);

  // Play clock — top portion, custom 7-segment drawing
  s_play_clock_layer = layer_create(GRect(0, 2, sw, play_h - 2));
  layer_add_child(root, s_play_clock_layer);
  play_clock_init(s_play_clock_layer);

  // Divider line
  s_divider_layer = layer_create(GRect(h_inset, divider_y, sw - 2 * h_inset, 2));
  layer_set_update_proc(s_divider_layer, prv_divider_update_proc);
  layer_add_child(root, s_divider_layer);

  // Game clock — bottom portion, text layer
  s_game_clock_layer = text_layer_create(GRect(0, game_y, sw, game_h));
  layer_add_child(root, text_layer_get_layer(s_game_clock_layer));
  game_clock_init(s_game_clock_layer, sh >= 200);
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
