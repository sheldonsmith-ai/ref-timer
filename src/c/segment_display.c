#include "segment_display.h"

// ── Geometry globals ──────────────────────────────────────────────
static int s_seg_h;
static int s_seg_w;
static int s_seg_t;
static int s_seg_g;
static int s_hseg_w;
static int s_vseg_h;
static int s_digit_spacing;

// ── Path objects ──────────────────────────────────────────────────
static GPath  *s_h_path;
static GPath  *s_v_path;
static GPoint  s_h_pts[6];
static GPoint  s_v_pts[6];

static const GPathInfo s_h_info = {6, s_h_pts};
static const GPathInfo s_v_info = {6, s_v_pts};

// ── Digit segment bitmask table ───────────────────────────────────
// bit0=A(top) bit1=B(top-right) bit2=C(bottom-right)
// bit3=D(bottom) bit4=E(bottom-left) bit5=F(top-left) bit6=G(middle)
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

void seg_compute_geometry(int screen_w, int screen_h) {
  int play_h      = screen_h * 63 / 100;
  s_seg_h         = (play_h * 82 / 100) & ~1;  // force even
  s_seg_w         = s_seg_h * 5 / 8;
  s_seg_t         = s_seg_h / 9;
  if (s_seg_t < 5) s_seg_t = 5;
  s_seg_g         = 1;
  s_digit_spacing = screen_w / 40;
  if (s_digit_spacing < 3) s_digit_spacing = 3;

  s_hseg_w = s_seg_w - 2 * (s_seg_t + s_seg_g);
  s_vseg_h = s_seg_h / 2 - s_seg_t - 2 * s_seg_g;

  int seg_c = s_seg_t / 2;

  // Hexagonal horizontal segment (anchor = top-left)
  s_h_pts[0] = GPoint(seg_c,             0);
  s_h_pts[1] = GPoint(s_hseg_w - seg_c,  0);
  s_h_pts[2] = GPoint(s_hseg_w,          s_seg_t / 2);
  s_h_pts[3] = GPoint(s_hseg_w - seg_c,  s_seg_t);
  s_h_pts[4] = GPoint(seg_c,             s_seg_t);
  s_h_pts[5] = GPoint(0,                 s_seg_t / 2);

  // Hexagonal vertical segment (anchor = top-left)
  s_v_pts[0] = GPoint(s_seg_t / 2,  0);
  s_v_pts[1] = GPoint(s_seg_t,      seg_c);
  s_v_pts[2] = GPoint(s_seg_t,      s_vseg_h - seg_c);
  s_v_pts[3] = GPoint(s_seg_t / 2,  s_vseg_h);
  s_v_pts[4] = GPoint(0,            s_vseg_h - seg_c);
  s_v_pts[5] = GPoint(0,            seg_c);
}

void seg_create_paths(void) {
  s_h_path = gpath_create(&s_h_info);
  s_v_path = gpath_create(&s_v_info);
}

void seg_destroy_paths(void) {
  gpath_destroy(s_h_path);
  gpath_destroy(s_v_path);
  s_h_path = NULL;
  s_v_path = NULL;
}

int seg_digit_width(void)   { return s_seg_w; }
int seg_digit_height(void)  { return s_seg_h; }
int seg_digit_spacing(void) { return s_digit_spacing; }

// ── Private helpers ───────────────────────────────────────────────

static void prv_draw_seg(GContext *ctx, GPath *path, int ax, int ay, bool on) {
  if (!on) return;
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_move_to(path, GPoint(ax, ay));
  gpath_draw_filled(ctx, path);
}

void seg_draw_digit(GContext *ctx, int x, int y, int digit) {
  uint8_t segs = SEG_DIGITS[digit];
  int half = s_seg_h / 2;

  prv_draw_seg(ctx, s_h_path, x + s_seg_t + s_seg_g, y,                          !!(segs & (1<<0)));
  prv_draw_seg(ctx, s_h_path, x + s_seg_t + s_seg_g, y + half - s_seg_t / 2,     !!(segs & (1<<6)));
  prv_draw_seg(ctx, s_h_path, x + s_seg_t + s_seg_g, y + s_seg_h - s_seg_t,      !!(segs & (1<<3)));

  prv_draw_seg(ctx, s_v_path, x,                     y + s_seg_t + s_seg_g,       !!(segs & (1<<5)));
  prv_draw_seg(ctx, s_v_path, x + s_seg_w - s_seg_t, y + s_seg_t + s_seg_g,      !!(segs & (1<<1)));
  prv_draw_seg(ctx, s_v_path, x,                     y + half + s_seg_g,          !!(segs & (1<<4)));
  prv_draw_seg(ctx, s_v_path, x + s_seg_w - s_seg_t, y + half + s_seg_g,         !!(segs & (1<<2)));
}
