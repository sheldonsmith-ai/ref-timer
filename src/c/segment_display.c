#include "segment_display.h"

/** Computed digit height (pixels) */
static int s_seg_h;

/** Computed digit width (pixels) */
static int s_seg_w;

/** Segment stroke thickness (pixels) */
static int s_seg_t;

/** Gap between adjacent segments (pixels) */
static int s_seg_g;

/** Width of a horizontal hexagonal segment (excluding anchor points) */
static int s_hseg_w;

/** Height of a vertical hexagonal segment (excluding anchor points) */
static int s_vseg_h;

/** Horizontal space between two digits (pixels) */
static int s_digit_spacing;

/** GPath for horizontal hexagonal segments (A, G, D) */
static GPath *s_h_path;

/** GPath for vertical hexagonal segments (F, B, E, C) */
static GPath *s_v_path;

/** Point buffer for horizontal segment path construction */
static GPoint s_h_pts[6];

/** Point buffer for vertical segment path construction */
static GPoint s_v_pts[6];

static const GPathInfo s_h_info = {6, s_h_pts};
static const GPathInfo s_v_info = {6, s_v_pts};

/**
 * @brief 7-segment display digit map using bitmasks.
 * 
 * Each entry corresponds to a decimal digit (0-9). The bits represent
 * which segments should be illuminated:
 * 
 *     -- A -- (bit0)
 *   F |   | B (bits 5, 1)
 *     -- G -- (bit6)
 *   E |   | C (bits 4, 2)
 *     -- D -- (bit3)
 * 
 * A segment is drawn if its corresponding bit is set.
 */
static const uint8_t SEG_DIGITS[] = {
  0b0111111, // 0: A B C D E F
  0b0000110, // 1: B C
  0b1011011, // 2: A B D E G
  0b1001111, // 3: A B C D G
  0b1100110, // 4: B C F G
  0b1101101, // 5: A C D F G
  0b1111101, // 6: A C D E F G
  0b0000111, // 7: A B C
  0b1111111, // 8: A B C D E F G
  0b1101111, // 9: A B C D F G
};

// ── Public API ────────────────────────────────────────────────────

/**
 * @brief Computes segment geometry based on available screen space.
 * 
 * Derives all segment dimensions from the play clock area height (63% of
 * screen height). The digit aspect ratio is fixed at 5:8. The segment
 * thickness is approximately 1/9 of the digit height, clamped to a
 * minimum of 5 pixels for visibility on small screens.
 * 
 * The hexagonal segment shape is created by chamfering the corners of
 * rectangular segments, giving a distinctive modern look that works
 * well at small sizes.
 * 
 * Must be called before seg_create_paths() and seg_draw_digit().
 * 
 * @param screen_w Total screen width in pixels
 * @param screen_h Total screen height in pixels
 */
void seg_compute_geometry(int screen_w, int screen_h) {
  // Play clock occupies ~63% of the screen height
  int play_h = screen_h * 63 / 100;
  
  // Digit height is ~82% of play clock area, rounded to even number
  s_seg_h = (play_h * 82 / 100) & ~1;
  
  // Fixed 5:8 aspect ratio
  s_seg_w = s_seg_h * 5 / 8;
  
  // Segment thickness ≈11% of height, minimum 5px for visibility
  s_seg_t = s_seg_h / 9;
  if (s_seg_t < 5) s_seg_t = 5;
  
  // Always 1px gap between segments
  s_seg_g = 1;
  
  // Digit spacing scales with screen width, minimum 3px
  s_digit_spacing = screen_w / 40;
  if (s_digit_spacing < 3) s_digit_spacing = 3;

  // Calculate the active length of horizontal and vertical segments
  // (excluding overlaps and anchor offsets)
  s_hseg_w = s_seg_w - 2 * (s_seg_t + s_seg_g);
  s_vseg_h = s_seg_h / 2 - s_seg_t - 2 * s_seg_g;

  int seg_c = s_seg_t / 2;  // Chamfer offset

  // Hexagonal horizontal segment — points defined relative to anchor (top-left)
  s_h_pts[0] = GPoint(seg_c,             0);
  s_h_pts[1] = GPoint(s_hseg_w - seg_c,  0);
  s_h_pts[2] = GPoint(s_hseg_w,          s_seg_t / 2);
  s_h_pts[3] = GPoint(s_hseg_w - seg_c,  s_seg_t);
  s_h_pts[4] = GPoint(seg_c,             s_seg_t);
  s_h_pts[5] = GPoint(0,                 s_seg_t / 2);

  // Hexagonal vertical segment — points defined relative to anchor (top-left)
  s_v_pts[0] = GPoint(s_seg_t / 2,  0);
  s_v_pts[1] = GPoint(s_seg_t,      seg_c);
  s_v_pts[2] = GPoint(s_seg_t,      s_vseg_h - seg_c);
  s_v_pts[3] = GPoint(s_seg_t / 2,  s_vseg_h);
  s_v_pts[4] = GPoint(0,            s_vseg_h - seg_c);
  s_v_pts[5] = GPoint(0,            seg_c);
}

/**
 * @brief Creates GPath objects for horizontal and vertical segments.
 * 
 * Must be called after seg_compute_geometry() which populates the
 * point buffers. The created paths are used during rendering to draw
 * all segments efficiently.
 */
void seg_create_paths(void) {
  s_h_path = gpath_create(&s_h_info);
  s_v_path = gpath_create(&s_v_info);
}

/**
 * @brief Destroys GPath objects to free memory.
 * 
 * Must be called during window unload to prevent memory leaks.
 * After this call, the paths are invalid and must be recreated
 * by calling seg_create_paths() before rendering again.
 */
void seg_destroy_paths(void) {
  gpath_destroy(s_h_path);
  gpath_destroy(s_v_path);
  s_h_path = NULL;
  s_v_path = NULL;
}

int seg_digit_width(void)   { return s_seg_w; }
int seg_digit_height(void)  { return s_seg_h; }

/**
 * @brief Returns the horizontal space between two digits.
 * 
 * @return Spacing in pixels (typically 3-10 depending on screen width)
 */
int seg_digit_spacing(void) { return s_digit_spacing; }

// ── Private helpers ───────────────────────────────────────────────

/**
 * @brief Draws a single segment if it should be illuminated.
 * 
 * Places the segment path at the given anchor coordinates and fills it
 * with black color. If the segment is off (on=false), this function
 * returns early without any drawing operations.
 * 
 * @param ctx Graphics context for drawing
 * @param path Precomputed GPath for this segment type
 * @param ax X anchor coordinate (segment-specific)
 * @param ay Y anchor coordinate (segment-specific)
 * @param on True to draw, false to skip
 */
static void prv_draw_seg(GContext *ctx, GPath *path, int ax, int ay, bool on) {
  if (!on) return;
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_move_to(path, GPoint(ax, ay));
  gpath_draw_filled(ctx, path);
}

/**
 * @brief Renders a single decimal digit (0-9) using hexagonal segments.
 * 
 * Looks up the segment bitmask for the given digit, then draws each
 * active segment at the computed position. The top-left corner of the
 * digit bounds is placed at (x, y). The digit is rendered in black
 * with no background fill.
 * 
 * Precondition: seg_compute_geometry() and seg_create_paths() must have
 * been called. Invalid digit values (outside 0-9) produce undefined output.
 * 
 * @param ctx Graphics context for drawing
 * @param x X coordinate of the digit's top-left corner
 * @param y Y coordinate of the digit's top-left corner
 * @param digit Decimal digit to render (0-9)
 */
void seg_draw_digit(GContext *ctx, int x, int y, int digit) {
  uint8_t segs = SEG_DIGITS[digit];
  int half = s_seg_h / 2;

  // Draw horizontal segments (A=top, G=middle, D=bottom)
  prv_draw_seg(ctx, s_h_path, x + s_seg_t + s_seg_g, y,                          !!(segs & (1<<0)));
  prv_draw_seg(ctx, s_h_path, x + s_seg_t + s_seg_g, y + half - s_seg_t / 2,     !!(segs & (1<<6)));
  prv_draw_seg(ctx, s_h_path, x + s_seg_t + s_seg_g, y + s_seg_h - s_seg_t,      !!(segs & (1<<3)));

  // Draw vertical segments (F=top-left, B=top-right, E=bottom-left, C=bottom-right)
  prv_draw_seg(ctx, s_v_path, x,                     y + s_seg_t + s_seg_g,       !!(segs & (1<<5)));
  prv_draw_seg(ctx, s_v_path, x + s_seg_w - s_seg_t, y + s_seg_t + s_seg_g,      !!(segs & (1<<1)));
  prv_draw_seg(ctx, s_v_path, x,                     y + half + s_seg_g,          !!(segs & (1<<4)));
  prv_draw_seg(ctx, s_v_path, x + s_seg_w - s_seg_t, y + half + s_seg_g,         !!(segs & (1<<2)));
}
