#pragma once
#include <pebble.h>

// Call once during window load after screen size is known.
void seg_compute_geometry(int screen_w, int screen_h);

// Call after seg_compute_geometry. Creates the GPath objects used for drawing.
void seg_create_paths(void);

// Call during window unload.
void seg_destroy_paths(void);

// Draw a single decimal digit (0–9) with its top-left at (x, y).
// seg_compute_geometry must have been called first.
void seg_draw_digit(GContext *ctx, int x, int y, int digit);

// Accessors for callers that need to compute layout.
int seg_digit_width(void);
int seg_digit_height(void);
int seg_digit_spacing(void);
