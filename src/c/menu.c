#include "menu.h"
#include <pebble.h>

// ── Persist Keys ──────────────────────────────────────────────────────────────
#define PERSIST_KEY_CLOCK_TYPE   7
#define PERSIST_KEY_REMEMBER_DAY 8
#define SECONDS_PER_DAY          86400

// ── State ─────────────────────────────────────────────────────────────────────
static Window           *s_type_window;
static SimpleMenuLayer  *s_type_menu_layer;
static SimpleMenuSection s_type_sections[1];
static SimpleMenuItem    s_type_items[4];

static Window           *s_remember_window;
static SimpleMenuLayer  *s_remember_menu_layer;
static SimpleMenuSection s_remember_sections[1];
static SimpleMenuItem    s_remember_items[2];

static int  s_selected_clock_type;
static bool s_selection_made;

// ── Remember Window ───────────────────────────────────────────────────────────

static void prv_remember_selected(int index, void *context) {
  persist_write_int(PERSIST_KEY_CLOCK_TYPE, s_selected_clock_type);
  if (index == 0) {
    // "Yes" — remember for the rest of today
    persist_write_int(PERSIST_KEY_REMEMBER_DAY,
                      (int32_t)(time(NULL) / SECONDS_PER_DAY));
  } else {
    // "No" — clear any previous remember flag
    persist_delete(PERSIST_KEY_REMEMBER_DAY);
  }
  s_selection_made = true;
  window_stack_pop(true);
}

static void prv_remember_window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  s_remember_items[0] = (SimpleMenuItem){
    .title    = "Yes",
    .callback = prv_remember_selected,
  };
  s_remember_items[1] = (SimpleMenuItem){
    .title    = "No",
    .subtitle = "Ask again next time",
    .callback = prv_remember_selected,
  };
  s_remember_sections[0] = (SimpleMenuSection){
    .title     = "Don't ask again today?",
    .items     = s_remember_items,
    .num_items = 2,
  };

  s_remember_menu_layer = simple_menu_layer_create(
    bounds, window, s_remember_sections, 1, NULL);
  layer_add_child(root, simple_menu_layer_get_layer(s_remember_menu_layer));
}

static void prv_remember_window_unload(Window *window) {
  simple_menu_layer_destroy(s_remember_menu_layer);
  s_remember_menu_layer = NULL;
}

// ── Type-Selection Window ─────────────────────────────────────────────────────

static void prv_type_selected(int index, void *context) {
  s_selected_clock_type = index;
  window_stack_push(s_remember_window, true);
}

static void prv_type_window_appear(Window *window) {
  if (s_selection_made) {
    s_selection_made = false;
    window_stack_pop(true);
  }
}

static void prv_type_window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  s_type_items[0] = (SimpleMenuItem){
    .title    = "40/25s",
    .callback = prv_type_selected,
  };
  s_type_items[1] = (SimpleMenuItem){
    .title    = "30/10s",
    .callback = prv_type_selected,
  };
  s_type_items[2] = (SimpleMenuItem){
    .title    = "25s",
    .callback = prv_type_selected,
  };
  s_type_items[3] = (SimpleMenuItem){
    .title    = "30s",
    .callback = prv_type_selected,
  };
  s_type_sections[0] = (SimpleMenuSection){
    .title     = "Play Clock",
    .items     = s_type_items,
    .num_items = 4,
  };

  s_type_menu_layer = simple_menu_layer_create(
    bounds, window, s_type_sections, 1, NULL);
  layer_add_child(root, simple_menu_layer_get_layer(s_type_menu_layer));
}

static void prv_type_window_unload(Window *window) {
  simple_menu_layer_destroy(s_type_menu_layer);
  s_type_menu_layer = NULL;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool menu_should_show(void) {
  if (!persist_exists(PERSIST_KEY_REMEMBER_DAY)) return true;
  int32_t today        = (int32_t)(time(NULL) / SECONDS_PER_DAY);
  int32_t remember_day = persist_read_int(PERSIST_KEY_REMEMBER_DAY);
  return (today != remember_day);
}

void menu_init(void) {
  s_selection_made = false;

  s_remember_window = window_create();
  window_set_window_handlers(s_remember_window, (WindowHandlers){
    .load   = prv_remember_window_load,
    .unload = prv_remember_window_unload,
  });

  s_type_window = window_create();
  window_set_window_handlers(s_type_window, (WindowHandlers){
    .load   = prv_type_window_load,
    .appear = prv_type_window_appear,
    .unload = prv_type_window_unload,
  });
  window_stack_push(s_type_window, true);
}

void menu_deinit(void) {
  if (s_remember_window) {
    window_destroy(s_remember_window);
    s_remember_window = NULL;
  }
  if (s_type_window) {
    window_destroy(s_type_window);
    s_type_window = NULL;
  }
}

int menu_get_clock_type(void) {
  if (persist_exists(PERSIST_KEY_CLOCK_TYPE)) {
    return persist_read_int(PERSIST_KEY_CLOCK_TYPE);
  }
  return 2;  // default: 25s
}
