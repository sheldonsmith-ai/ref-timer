#include "play_clock.h"
#include "segment_display.h"

#define PLAY_CLOCK_START     25
#define PLAY_EXPIRE_RESET_MS 1000

static int       s_play_seconds;
static bool      s_play_running;
static AppTimer *s_play_reset_timer;
static Layer    *s_play_clock_layer;

static const uint32_t TWO_BUZZ_SEGS[] = {100, 100, 100};
static const VibePattern TWO_BUZZ = {
  .durations = TWO_BUZZ_SEGS,
  .num_segments = 3
};

static void prv_play_reset_callback(void *context) {
  s_play_reset_timer = NULL;
  s_play_seconds     = PLAY_CLOCK_START;
  layer_mark_dirty(s_play_clock_layer);
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int value   = s_play_seconds;
  int seg_h   = seg_digit_height();
  int seg_w   = seg_digit_width();
  int spacing = seg_digit_spacing();
  int digit_y = (bounds.size.h - seg_h) / 2;

  if (value >= 10) {
    int total_w = 2 * seg_w + spacing;
    int start_x = (bounds.size.w - total_w) / 2;
    seg_draw_digit(ctx, start_x, digit_y, value / 10);
    seg_draw_digit(ctx, start_x + seg_w + spacing, digit_y, value % 10);
  } else {
    int start_x = (bounds.size.w - seg_w) / 2;
    seg_draw_digit(ctx, start_x, digit_y, value);
  }
}

void play_clock_init(Layer *layer) {
  s_play_clock_layer = layer;
  layer_set_update_proc(layer, prv_update_proc);
  // No mark_dirty here — the window's initial render handles it.
}

void play_clock_tick(void) {
  if (!s_play_running) return;
  s_play_seconds--;

  if (s_play_seconds == 10) {
    vibes_enqueue_custom_pattern(TWO_BUZZ);
  } else if (s_play_seconds >= 1 && s_play_seconds <= 5) {
    vibes_short_pulse();
  } else if (s_play_seconds == 0) {
    vibes_long_pulse();
    s_play_running     = false;
    s_play_reset_timer = app_timer_register(
      PLAY_EXPIRE_RESET_MS, prv_play_reset_callback, NULL);
  }

  layer_mark_dirty(s_play_clock_layer);
}

void play_clock_start(void) {
  s_play_running = true;
  layer_mark_dirty(s_play_clock_layer);
}

void play_clock_reset(void) {
  s_play_running = false;
  s_play_seconds = PLAY_CLOCK_START;
  if (s_play_reset_timer) {
    app_timer_cancel(s_play_reset_timer);
    s_play_reset_timer = NULL;
  }
  layer_mark_dirty(s_play_clock_layer);
}

bool play_clock_is_running(void)   { return s_play_running; }
int  play_clock_get_seconds(void)  { return s_play_seconds; }

void play_clock_set_seconds(int s) { s_play_seconds = s; }
void play_clock_set_running(bool r){ s_play_running = r; }
