# ref-timer: Multi-File Refactor Design

**Date:** 2026-04-04  
**Goal:** Split `src/c/ref-timer.c` (507 lines) into focused modules for easier navigation, cleaner growth, and learning good C module patterns.

---

## Motivation

- **Navigation (primary):** The single file is hard to jump around in as it grows.
- **Anticipated growth:** Play clock modes (25s, 30s, 25/40s) will expand the play clock logic significantly.
- **Learning:** Practice the standard C module pattern — `static` state in `.c`, clean interface in `.h`.

---

## File Structure

```
src/c/
├── ref-timer.c          # app lifecycle, window setup, button handlers, master tick, main
├── segment_display.c    # 7-seg geometry computation + drawing primitives
├── segment_display.h
├── play_clock.c         # play clock state, tick logic, display update, reset timer
├── play_clock.h
├── game_clock.c         # game clock state, tick logic, display update, edit mode
├── game_clock.h
├── storage.c            # persistent save/load (reads from all modules)
└── storage.h
```

No `wscript` changes needed — it already globs `src/c/**/*.c`.

---

## Module Interfaces

### `segment_display.h`

```c
void seg_compute_geometry(int screen_w, int screen_h);
void seg_create_paths(void);
void seg_destroy_paths(void);
void seg_draw_digit(GContext *ctx, int x, int y, int digit);
int  seg_digit_width(void);
int  seg_digit_height(void);
```

All segment geometry (`s_seg_h`, `s_seg_w`, etc.) and path objects (`s_h_path`, `s_v_path`) stay as `static` globals inside `segment_display.c`. The layer update proc (`prv_play_clock_update_proc`) moves into `play_clock.c` and calls `seg_draw_digit()`.

### `play_clock.h`

```c
void play_clock_init(Layer *layer);
void play_clock_tick(void);
void play_clock_start(void);
void play_clock_reset(void);
bool play_clock_is_running(void);
int  play_clock_get_seconds(void);
void play_clock_set_seconds(int s);
```

Owns: `s_play_seconds`, `s_play_running`, `s_play_reset_timer`, `s_play_clock_layer`, `TWO_BUZZ` vibration pattern.

### `game_clock.h`

```c
void game_clock_init(TextLayer *layer);
void game_clock_tick(void);
void game_clock_start(void);
void game_clock_stop(void);
bool game_clock_is_running(void);
bool game_clock_ever_started(void);
int  game_clock_get_total_seconds(void);
int  game_clock_get_default_seconds(void);
void game_clock_set_total_seconds(int s);
void game_clock_set_default_seconds(int s);
void game_clock_set_state(int state);        // 0=fresh, 1=paused, 2=running
void game_clock_enter_edit_mode(bool fine_grained);
void game_clock_exit_edit_mode(void);
bool game_clock_in_edit_mode(void);
void game_clock_adjust(int direction);
```

Owns: `s_game_total_seconds`, `s_game_default_seconds`, `s_game_running`, `s_game_ever_started`, `s_game_2min_warned`, `s_edit_mode`, `s_edit_fine_grained`, `s_game_clock_layer`, `s_game_font`, `s_game_buf`.

### `storage.h`

```c
void storage_save(void);
void storage_load(void);
```

`storage.c` includes all three module headers and calls their getters/setters directly. The persist keys move here as private `#define`s.

---

## What Stays in `ref-timer.c`

- `main`, `prv_init`, `prv_deinit`
- Window lifecycle: `prv_window_load`, `prv_window_unload`
- Master tick: `prv_tick_handler` (calls `play_clock_tick()` and `game_clock_tick()`)
- All button handlers: `prv_up_*`, `prv_down_*`, `prv_select_*`, `prv_back_handler`
- Click config: `prv_click_config_provider`
- Divider layer: `prv_divider_update_proc`
- Long-press repeat timer: `prv_long_press_repeat_*` (calls `game_clock_adjust()`)
- Window/layer handles: `s_window`, `s_play_clock_layer`, `s_game_clock_layer`, `s_divider_layer`
- Timer handles: `s_tick_timer`, `s_long_press_repeat_timer`

---

## C Module Pattern (for learning)

Each module follows this pattern:

1. **State is `static` in `.c`** — never exposed as `extern` in the header.
2. **Header exposes only functions** — callers interact through the API, not the variables.
3. **One clear owner per piece of state** — no two modules touch the same variable.
4. **Init function receives external dependencies** — e.g. `play_clock_init(Layer *layer)` receives the layer rather than creating it, since window layout is the app's job.

---

## Future: Play Clock Modes

When adding 25s/30s/25-40s rules, changes are contained to `play_clock.c/.h`:

```c
typedef enum { PLAY_MODE_25S, PLAY_MODE_30S, PLAY_MODE_25_40S } PlayClockMode;
void play_clock_set_mode(PlayClockMode mode);
```

`ref-timer.c` calls `play_clock_set_mode()` from a future settings screen. No other module needs to change.

---

## Out of Scope

- No behavior changes — this is a pure structural refactor.
- No new features in this pass.
- Button handlers stay in `ref-timer.c` for now (they're thin and tightly coupled to the window).
