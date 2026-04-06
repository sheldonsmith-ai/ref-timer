#pragma once
#include <pebble.h>

/**
 * @brief Computes all segment dimensions based on available screen space.
 * 
 * Derives digit size, segment thickness, gaps, and spacing from the
 * screen dimensions. The play clock area is 63% of screen height, and
 * digits occupy 82% of that area with a 5:8 aspect ratio.
 * 
 * Must be called before seg_create_paths() and seg_draw_digit().
 * 
 * @param screen_w Total screen width in pixels
 * @param screen_h Total screen height in pixels
 */
void seg_compute_geometry(int screen_w, int screen_h);

/**
 * @brief Creates GPath objects for horizontal and vertical segments.
 * 
 * Uses the point buffers populated by seg_compute_geometry(). The
 * created paths are reused during rendering for all segment drawings.
 * 
 * Must be called after seg_compute_geometry() and before seg_draw_digit().
 */
void seg_create_paths(void);

/**
 * @brief Destroys GPath objects to free memory.
 * 
 * Must be called during window unload to prevent memory leaks.
 */
void seg_destroy_paths(void);

/**
 * @brief Renders a single decimal digit using hexagonal segments.
 * 
 * Draws the digit in black with the top-left corner at (x, y). The
 * segment geometry must have been computed before calling this function.
 * 
 * @param ctx Graphics context for drawing
 * @param x X coordinate of digit's top-left corner
 * @param y Y coordinate of digit's top-left corner
 * @param digit Decimal digit to render (0-9)
 */
void seg_draw_digit(GContext *ctx, int x, int y, int digit);

/**
 * @brief Returns the width of a single digit in pixels.
 * 
 * @return Digit width (typically 30-50 pixels depending on screen size)
 */
int seg_digit_width(void);

/**
 * @brief Returns the height of a single digit in pixels.
 * 
 * @return Digit height (typically 48-80 pixels depending on screen size)
 */
int seg_digit_height(void);

/**
 * @brief Returns the horizontal spacing between two digits in pixels.
 * 
 * @return Spacing in pixels (3-10 depending on screen width)
 */
int seg_digit_spacing(void);
