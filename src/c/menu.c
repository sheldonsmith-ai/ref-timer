#include "menu.h"
#include <pebble.h>

// ============================================================================
// menu.c — Play Clock Type Selection
// ============================================================================
//
// Presents a SimpleMenu on launch so the referee can choose which play-clock
// standard to use.  The selection is stored in memory and resets every time
// the app launches (no persistence required for this setting).
// ============================================================================

// ── Static State ─────────────────────────────────────────────────────────────
// Window and menu-layer handles for the type-selection screen.
static Window           *s_type_window;
static SimpleMenuLayer  *s_type_menu_layer;
static SimpleMenuSection s_type_sections[1];
static SimpleMenuItem    s_type_items[4];

// Index into s_type_items[] of the user's current choice.
// 0 = 40/25s  (NCAA/NFHS Football)
// 1 = 30/10s  (NCAA Flag Football)
// 2 = 25s     (Generic 25-second clock)
// 3 = 30s     (Generic 30-second clock)
// Defaults to 0 until the user explicitly picks a type.
static int s_selected_clock_type = 0;

// ── Callback: User Tapped a Menu Item ────────────────────────────────────────
// Stores the selected index and pops the menu off the stack,
// revealing the main play/game clock screen.
static void prv_type_selected(int index, void *context) {
  (void) context;  // unused
  s_selected_clock_type = index;
  window_stack_pop(true);
}

// ── Window Load: Build the Menu ──────────────────────────────────────────────
// Called once when the window first appears.  Constructs the menu items,
// packs them into a single section, and attaches a SimpleMenuLayer.
static void prv_type_window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  // Populate each menu item with title, optional subtitle, and the shared
  // selection callback.
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

  // Bundle all items into a single section titled "Play Clock".
  s_type_sections[0] = (SimpleMenuSection){
    .title     = "Play Clock",
    .items     = s_type_items,
    .num_items = 4,
  };

  // Create and attach the SimpleMenuLayer.
  s_type_menu_layer = simple_menu_layer_create(
    bounds, window, s_type_sections, 1, NULL);
  layer_add_child(root, simple_menu_layer_get_layer(s_type_menu_layer));
}

// ── Window Unload: Tear Down ─────────────────────────────────────────────────
// Cleans up the SimpleMenuLayer when the window is destroyed.
static void prv_type_window_unload(Window *window) {
  (void) window;  // unused
  simple_menu_layer_destroy(s_type_menu_layer);
  s_type_menu_layer = NULL;
}

// ============================================================================
// Public API
// ============================================================================

// Initialize and push the type-selection window onto the window stack.
// Must be called before menu_get_clock_type().
void menu_init(void) {
  s_type_window = window_create();
  window_set_window_handlers(s_type_window, (WindowHandlers){
    .load   = prv_type_window_load,
    .unload = prv_type_window_unload,
  });
  window_stack_push(s_type_window, true);
}

// Destroy the menu window and release all associated resources.
// Safe to call even if menu_init was never called (idempotent).
void menu_deinit(void) {
  if (s_type_window) {
    window_destroy(s_type_window);
    s_type_window = NULL;
  }
}

// Return the index of the clock type the user selected (or 0 if none yet).
// The caller (main app loop) uses this to initialise the play-clock duration.
int menu_get_clock_type(void) {
  return s_selected_clock_type;
}
