#include "game_clock.h"

#define GAME_CLOCK_DEFAULT (25 * 60)

static int        s_game_total_seconds;
static int        s_game_default_seconds;
static bool       s_game_running;
static bool       s_game_ever_started;
static bool       s_game_2min_warned;
static bool       s_edit_mode;
static bool       s_edit_fine_grained;
static char       s_game_buf[8];
static TextLayer *s_game_clock_layer;
static GFont      s_game_font;

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

void game_clock_init(TextLayer *layer, bool large_screen) {
  s_game_clock_layer = layer;
  s_game_font = fonts_load_custom_font(resource_get_handle(
    large_screen ? RESOURCE_ID_GAME_CLOCK_48 : RESOURCE_ID_GAME_CLOCK_32));
  text_layer_set_font(layer, s_game_font);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  prv_update_display();
}

void game_clock_deinit(void) {
  fonts_unload_custom_font(s_game_font);
}

void game_clock_tick(void) {
  if (!s_game_running) return;
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

  prv_update_display();
}

void game_clock_start(void) {
  s_game_running      = true;
  s_game_ever_started = true;
  if (!s_game_2min_warned && s_game_total_seconds <= 120) {
    s_game_2min_warned = true;
  }
  prv_update_display();
}

void game_clock_stop(void) {
  s_game_running = false;
  prv_update_display();
}

bool game_clock_is_running(void)          { return s_game_running; }
bool game_clock_ever_started(void)        { return s_game_ever_started; }
int  game_clock_get_total_seconds(void)   { return s_game_total_seconds; }
int  game_clock_get_default_seconds(void) { return s_game_default_seconds; }

void game_clock_set_total_seconds(int s)   { s_game_total_seconds = s; }
void game_clock_set_default_seconds(int s) { s_game_default_seconds = s; }

void game_clock_set_state(int state) {
  s_game_running      = (state == 2);
  s_game_ever_started = (state != 0);
  s_game_2min_warned  = (s_game_total_seconds <= 120 && s_game_ever_started);
}

void game_clock_enter_edit_mode(bool fine_grained) {
  s_game_running      = false;
  s_edit_mode         = true;
  s_edit_fine_grained = fine_grained;
  prv_update_display();
}

void game_clock_exit_edit_mode(void) {
  // On coarse-grained edit, the current value becomes the new default.
  if (!s_edit_fine_grained) {
    s_game_default_seconds = s_game_total_seconds;
  }
  s_edit_mode = false;
  prv_update_display();
}

bool game_clock_in_edit_mode(void)         { return s_edit_mode; }
bool game_clock_is_edit_fine_grained(void) { return s_edit_fine_grained; }

void game_clock_reset_to_default(void) {
  s_game_total_seconds = s_game_default_seconds;
  prv_update_display();
}

void game_clock_adjust(int direction) {
  int step = s_edit_fine_grained ? 1 : 60;
  s_game_total_seconds += direction * step;
  if (s_game_total_seconds < 1)    s_game_total_seconds = 1;
  if (s_game_total_seconds > 5999) s_game_total_seconds = 5999;
  prv_update_display();
}
