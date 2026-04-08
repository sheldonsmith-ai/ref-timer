#include "menu.h"
#include <pebble.h>

// ── State ─────────────────────────────────────────────────────────────────────
static Window           *s_type_window;
static SimpleMenuLayer  *s_type_menu_layer;
static SimpleMenuSection s_type_sections[1];
static SimpleMenuItem    s_type_items[4];

// In-memory selection; reset each launch (0=40/25s default until user picks)
static int s_selected_clock_type = 0;

// ── Type-Selection Window ─────────────────────────────────────────────────────

static void prv_type_selected(int index, void *context) {
  s_selected_clock_type = index;
  window_stack_pop(true);
}

static void prv_type_window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  s_type_items[0] = (SimpleMenuItem){
    .title    = "40/25s",
    .subtitle = "NCAA/NFHS Football",
    .callback = prv_type_selected,
  };
  s_type_items[1] = (SimpleMenuItem){
    .title    = "30/10s",
    .subtitle = "NCAA Flag Football",
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

void menu_init(void) {
  s_type_window = window_create();
  window_set_window_handlers(s_type_window, (WindowHandlers){
    .load   = prv_type_window_load,
    .unload = prv_type_window_unload,
  });
  window_stack_push(s_type_window, true);
}

void menu_deinit(void) {
  if (s_type_window) {
    window_destroy(s_type_window);
    s_type_window = NULL;
  }
}

int menu_get_clock_type(void) {
  return s_selected_clock_type;
}
