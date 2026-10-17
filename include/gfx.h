#ifndef SIF_GFX_H
#define SIF_GFX_H
#include <stdint.h>

#define GFX_WIDTH  320
#define GFX_HEIGHT 200
#define GFX_FB     ((uint8_t *)0xA0000)

/* palette indices we use */
enum gfx_color {
    GFX_BLACK = 0,
    GFX_BLUE = 1,
    GFX_GREEN = 2,
    GFX_CYAN = 3,
    GFX_RED = 4,
    GFX_MAGENTA = 5,
    GFX_BROWN = 6,
    GFX_LIGHT_GREY = 7,
    GFX_DARK_GREY = 8,
    GFX_LIGHT_BLUE = 9,
    GFX_LIGHT_GREEN = 10,
    GFX_LIGHT_CYAN = 11,
    GFX_LIGHT_RED = 12,
    GFX_LIGHT_MAGENTA = 13,
    GFX_YELLOW = 14,
    GFX_WHITE = 15,
};

void gfx_init_mode13(void);
void gfx_restore_text(void);
int gfx_is_graphics(void);

void gfx_clear(uint8_t color);
void gfx_putpixel(int x, int y, uint8_t color);
uint8_t gfx_getpixel(int x, int y);
void gfx_fillrect(int x, int y, int w, int h, uint8_t color);
void gfx_rect(int x, int y, int w, int h, uint8_t color);
void gfx_hline(int x, int y, int w, uint8_t color);
void gfx_vline(int x, int y, int h, uint8_t color);
void gfx_char(int x, int y, char c, uint8_t fg, uint8_t bg);
void gfx_text(int x, int y, const char *s, uint8_t fg, uint8_t bg);

#endif
