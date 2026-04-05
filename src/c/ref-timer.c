#include <pebble.h>
#include "segment_display.h"
#include "play_clock.h"

// ── Persistent Storage Keys ───────────────────────────────────────
#define PERSIST_KEY_GAME_DEFAULT  1
#define PERSIST_KEY_GAME_CURRENT  2
#define PERSIST_KEY_GAME_STATE    3
#define PERSIST_KEY_PLAY_RUNNING  4
#define PERSIST_KEY_PLAY_SECONDS  5

// ── Constants ─────────────────────────────────────────────────────
#define PLAY_CLOCK_START        25
#define GAME_CLOCK_DEFAULT      (25 * 60)
#define LONG_PRESS_REPEAT_MS    75

// ── Layers & Fonts ────────────────────────────────────────────────
static Window     *s_window;
static Layer      *s_play_clock_layer;
static TextLayer  *s_game_clock_layer;
static Layer      *s_divider_layer;
static GFont       s_game_font;

// ── Timers ────────────────────────────────────────────────────────
static AppTimer   *s_tick_timer;
static AppTimer   *s_long_press_repeat_timer;

// ── Game Clock State ──────────────────────────────────────────────
static int         s_game_total_seconds;
static int         s_game_default_seconds;
static bool        s_game_running;
static bool        s_game_ever_started;
static bool        s_game_2min_warned;

// ── Edit Mode State ───────────────────────────────────────────────
static bool        s_edit_mode;
static bool        s_edit_fine_grained;
static int         s_long_press_direction;

// ── String Buffer ─────────────────────────────────────────────────
static char        s_game_buf[8];

// ── Display Update ────────────────────────────────────────────────

static void prv_update_game_display(void) {
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

// ── Game Clock Adjustment ─────────────────────────────────────────

static void prv_adjust_game_clock(int direction) {
  int step = s_edit_fine_grained ? 1 : 60;
  s_game_total_seconds += direction * step;
  if (s_game_total_seconds < 1)    s_game_total_seconds = 1;
  if (s_game_total_seconds > 5999) s_game_total_seconds = 5999;
  prv_update_game_display();
}

// ── Persistent State ──────────────────────────────────────────────

static void prv_save_state(void) {
  persist_write_int(PERSIST_KEY_GAME_DEFAULT, s_game_default_seconds);
  persist_write_int(PERSIST_KEY_GAME_CURRENT, s_game_total_seconds);

  int game_state = 0;
  if (s_game_ever_started && !s_game_running) game_state = 1;
  if (s_game_running)                         game_state = 2;
  persist_write_int(PERSIST_KEY_GAME_STATE, game_state);

  persist_write_int(PERSIST_KEY_PLAY_RUNNING, play_clock_is_running() ? 1 : 0);
  persist_write_int(PERSIST_KEY_PLAY_SECONDS, play_clock_get_seconds());
}

static void prv_load_state(void) {
  if (persist_exists(PERSIST_KEY_GAME_DEFAULT)) {
    s_game_default_seconds = persist_read_int(PERSIST_KEY_GAME_DEFAULT);
  } else {
    s_game_default_seconds = GAME_CLOCK_DEFAULT;
  }

  if (persist_exists(PERSIST_KEY_GAME_CURRENT)) {
    s_game_total_seconds = persist_read_int(PERSIST_KEY_GAME_CURRENT);
    int state            = persist_read_int(PERSIST_KEY_GAME_STATE);
    s_game_running       = (state == 2);
    s_game_ever_started  = (state != 0);
    s_game_2min_warned   = (s_game_total_seconds <= 120 && s_game_ever_started);
  } else {
    s_game_total_seconds = s_game_default_seconds;
    s_game_running       = false;
    s_game_ever_started  = false;
    s_game_2min_warned   = false;
  }

  if (persist_exists(PERSIST_KEY_PLAY_SECONDS)) {
    play_clock_set_seconds(persist_read_int(PERSIST_KEY_PLAY_SECONDS));
    play_clock_set_running(persist_read_int(PERSIST_KEY_PLAY_RUNNING) == 1);
  } else {
    play_clock_set_seconds(PLAY_CLOCK_START);
    play_clock_set_running(false);
  }
}

// ── Long Press Repeat Timer ───────────────────────────────────────

static void prv_long_press_repeat_callback(void *context) {
  if (!s_edit_mode) return;
  prv_adjust_game_clock(s_long_press_direction);
  s_long_press_repeat_timer = app_timer_register(
    LONG_PRESS_REPEAT_MS, prv_long_press_repeat_callback, NULL);
}

static void prv_start_long_press_repeat(void) {
  prv_adjust_game_clock(s_long_press_direction);
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

  if (s_game_running) {
    s_game_total_seconds--;

    if (s_game_total_seconds == 120 && !s_game_2min_warned) {
      vibes_long_pulse();
      s_game_2min_warned = true;
    } else if (s_game_total_seconds == 0) {
      vibes_long_pulse();
      s_game_running       = false;
      s_game_ever_started  = false;
      s_game_2min_warned   = false;
      s_game_total_seconds = s_game_default_seconds;
    }

    prv_update_game_display();
  }

  s_tick_timer = app_timer_register(1000, prv_tick_handler, NULL);
}

// ── Button Handlers ───────────────────────────────────────────────

static void prv_up_single_handler(ClickRecognizerRef r, void *ctx) {
  if (s_edit_mode) {
    prv_adjust_game_clock(+1);
    return;
  }
  if (play_clock_is_running()) play_clock_reset();
  else                         play_clock_start();
}

static void prv_up_long_press_begin(ClickRecognizerRef r, void *ctx) {
  if (!s_edit_mode) return;
  s_long_press_direction = +1;
  prv_start_long_press_repeat();
}

static void prv_up_long_press_end(ClickRecognizerRef r, void *ctx) {
  prv_stop_long_press_repeat();
}

static void prv_down_single_handler(ClickRecognizerRef r, void *ctx) {
  if (s_edit_mode) {
    prv_adjust_game_clock(-1);
    return;
  }
  if (s_game_running) {
    s_game_running = false;
  } else {
    s_game_running      = true;
    s_game_ever_started = true;
    if (!s_game_2min_warned && s_game_total_seconds <= 120) {
      s_game_2min_warned = true;
    }
  }
  prv_update_game_display();
}

static void prv_down_long_press_begin(ClickRecognizerRef r, void *ctx) {
  if (!s_edit_mode) return;
  s_long_press_direction = -1;
  prv_start_long_press_repeat();
}

static void prv_down_long_press_end(ClickRecognizerRef r, void *ctx) {
  prv_stop_long_press_repeat();
}

static void prv_select_single_handler(ClickRecognizerRef r, void *ctx) {
  if (!s_edit_mode) return;
  if (!s_edit_fine_grained) {
    s_game_default_seconds = s_game_total_seconds;
    persist_write_int(PERSIST_KEY_GAME_DEFAULT, s_game_default_seconds);
  }
  s_edit_mode = false;
  prv_update_game_display();
}

static void prv_select_long_press_handler(ClickRecognizerRef r, void *ctx) {
  if (s_edit_mode) return;
  s_game_running      = false;
  s_edit_mode         = true;
  s_edit_fine_grained = s_game_ever_started;
  prv_update_game_display();
}

static void prv_back_handler(ClickRecognizerRef r, void *ctx) {
  prv_save_state();
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
  window_long_click_subscribe(BUTTON_ID_SELECT, 700,
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
  bool large_screen = (sh >= 200);
  s_game_font = fonts_load_custom_font(resource_get_handle(
    large_screen ? RESOURCE_ID_GAME_CLOCK_48 : RESOURCE_ID_GAME_CLOCK_32));
  s_game_clock_layer = text_layer_create(GRect(0, game_y, sw, game_h));
  text_layer_set_font(s_game_clock_layer, s_game_font);
  text_layer_set_text_alignment(s_game_clock_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_game_clock_layer));

  prv_update_game_display();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_play_clock_layer);
  text_layer_destroy(s_game_clock_layer);
  layer_destroy(s_divider_layer);
  seg_destroy_paths();
  fonts_unload_custom_font(s_game_font);
}

// ── App Lifecycle ─────────────────────────────────────────────────

static void prv_init(void) {
  prv_load_state();

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
