#include "play_clock.h"
#include "segment_display.h"

/** Initial value when the play clock starts or resets */
#define PLAY_CLOCK_START 25

/** Delay after expiration before auto-resetting (1 second) */
#define PLAY_EXPIRE_RESET_MS 1000

/** Current remaining time on the play clock (0-25 seconds) */
static int s_play_seconds;

/** True while the play clock is actively counting down */
static bool s_play_running;

/** Timer handle for the auto-reset delay after expiration */
static AppTimer *s_play_reset_timer;

/** Reference to the layer that displays the play clock digits */
static Layer *s_play_clock_layer;

/** Haptic pattern: three 100ms pulses for the 10-second warning */
static const uint32_t TWO_BUZZ_SEGS[] = {100, 100, 100};

static const VibePattern TWO_BUZZ = {
  .durations = TWO_BUZZ_SEGS,
  .num_segments = 3
};

/**
 * @brief Auto-reset callback that restores the play clock after expiration.
 * 
 * Executes PLAY_EXPIRE_RESET_MS milliseconds after the clock reaches zero.
 * This brief pause allows the user to see the expiration state (00) before
 * automatically resetting to the starting value.
 */
static void prv_play_reset_callback(void *context) {
  s_play_reset_timer = NULL;
  s_play_seconds     = PLAY_CLOCK_START;
  layer_mark_dirty(s_play_clock_layer);
}

/**
 * @brief Renders the play clock as one or two digits using segment drawing.
 * 
 * Dynamically calculates the position to center the display within the
 * layer bounds. Single digits (0-9) are centered horizontally; double
 * digits (10-25) are rendered with appropriate spacing between them.
 * 
 * @param layer The layer being rendered
 * @param ctx Graphics context with colors already configured
 */
static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int value   = s_play_seconds;
  int seg_h   = seg_digit_height();
  int seg_w   = seg_digit_width();
  int spacing = seg_digit_spacing();
  int digit_y = (bounds.size.h - seg_h) / 2;  // Vertically centered

  if (value >= 10) {
    // Two digits: center the pair horizontally
    int total_w = 2 * seg_w + spacing;
    int start_x = (bounds.size.w - total_w) / 2;
    seg_draw_digit(ctx, start_x, digit_y, value / 10);
    seg_draw_digit(ctx, start_x + seg_w + spacing, digit_y, value % 10);
  } else {
    // Single digit: center it in the available space
    int start_x = (bounds.size.w - seg_w) / 2;
    seg_draw_digit(ctx, start_x, digit_y, value);
  }
}

/**
 * @brief Initializes the play clock layer with its render callback.
 * 
 * Called during window load after the layer is created. The initial
 * render is triggered by the window's first paint, so we don't call
 * mark_dirty here.
 * 
 * @param layer Pre-created layer to use for play clock display
 */
void play_clock_init(Layer *layer) {
  s_play_clock_layer = layer;
  layer_set_update_proc(layer, prv_update_proc);
  // Initial render handled by window's first paint
}

/**
 * @brief Processes one second of countdown for the play clock.
 * 
 * Called once per second by the master tick handler. Manages haptic
 * warnings at specific thresholds and handles expiration with auto-reset.
 * The play clock uses a custom haptic pattern at 10 seconds (double buzz)
 * and short pulses during the final 5 seconds.
 * 
 * When the clock reaches zero, a long pulse signals expiration and a
 * delayed reset timer schedules automatic restoration to PLAY_CLOCK_START.
 */
void play_clock_tick(void) {
  if (!s_play_running) return;
  s_play_seconds--;

  // Haptic alerts at key milestones:
  if (s_play_seconds == 10) {
    // Warning threshold: custom double-buzz pattern
    vibes_enqueue_custom_pattern(TWO_BUZZ);
  } else if (s_play_seconds >= 1 && s_play_seconds <= 5) {
    // Final seconds: short pulse each second
    vibes_short_pulse();
  } else if (s_play_seconds == 0) {
    // Expired: long pulse and schedule auto-reset
    vibes_long_pulse();
    s_play_running     = false;
    s_play_reset_timer = app_timer_register(
      PLAY_EXPIRE_RESET_MS, prv_play_reset_callback, NULL);
  }

  layer_mark_dirty(s_play_clock_layer);
}

/**
 * @brief Starts the play clock countdown from the current value.
 * 
 * Sets the running flag and schedules the next tick. This is typically
 * called when the user presses the UP button in normal mode. The clock
 * should be at PLAY_CLOCK_START when first started, or at a restored
 * value if resuming from persistence.
 */
void play_clock_start(void) {
  s_play_running = true;
  layer_mark_dirty(s_play_clock_layer);
}

/**
 * @brief Stops the play clock and resets to PLAY_CLOCK_START.
 * 
 * Cancels any pending auto-reset timer to prevent state corruption.
 * Used when the user manually resets the clock or when the clock
 * has been running and needs to be stopped and reset.
 */
void play_clock_reset(void) {
  s_play_running = false;
  s_play_seconds = PLAY_CLOCK_START;
  // Cancel pending auto-reset if user manually reset before expiration
  if (s_play_reset_timer) {
    app_timer_cancel(s_play_reset_timer);
    s_play_reset_timer = NULL;
  }
  layer_mark_dirty(s_play_clock_layer);
}

bool play_clock_is_running(void)   { return s_play_running; }

/**
 * @brief Gets the current remaining time on the play clock.
 * 
 * @return Remaining seconds (0-25)
 */
int  play_clock_get_seconds(void)  { return s_play_seconds; }

/**
 * @brief Restores the play clock time from persisted state.
 * 
 * @param s Seconds to set (should be 0-25)
 */
void play_clock_set_seconds(int s) { s_play_seconds = s; }

/**
 * @brief Restores the play clock running state from persisted state.
 * 
 * @param running True if clock should be running, false if stopped
 */
void play_clock_set_running(bool r) { s_play_running = r; }
