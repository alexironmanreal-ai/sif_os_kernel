#include "gui.h"
#include "bios.h"
#include "shell.h"
#include "gfx.h"
#include "keyboard.h"
#include "mouse.h"
#include "printk.h"
#include "kernel.h"
#include "vga.h"
#include "ata.h"
#include "diskfs.h"
#include "timer.h"
#include "rtc.h"
#include "string.h"

/*
 * SIF Aqua 2 — closer to classic Mac OS X look (320x200)
 */

static int panel;      /* 0 apps 1 disk 2 about */
static int dock_sel;
static int apple_open; /* apple menu visible */

#define CUR_W 9
#define CUR_H 12
static uint8_t cur_under[CUR_W * CUR_H];
static int cur_sx = -1, cur_sy = -1;

static int dock_x, dock_y, dock_ix, dock_iy;

static void cursor_save(int x, int y) {
    for (int j = 0; j < CUR_H; j++)
        for (int i = 0; i < CUR_W; i++)
            cur_under[j * CUR_W + i] = gfx_getpixel(x + i, y + j);
    cur_sx = x;
    cur_sy = y;
}

static void cursor_restore(void) {
    if (cur_sx < 0)
        return;
    for (int j = 0; j < CUR_H; j++)
        for (int i = 0; i < CUR_W; i++)
            gfx_putpixel(cur_sx + i, cur_sy + j, cur_under[j * CUR_W + i]);
    cur_sx = cur_sy = -1;
}

static void cursor_draw(int x, int y) {
    cursor_restore();
    cursor_save(x, y);
    static const char *shape[12] = {
        "X........",
        "XX.......",
        "X.X......",
        "X..X.....",
        "X...X....",
        "X....X...",
        "X.....X..",
        "X..XXXXX.",
        "X.X......",
        "XX.X.....",
        "X..X.....",
        ".X.......",
    };
    for (int j = 0; j < CUR_H; j++)
        for (int i = 0; i < CUR_W; i++) {
            if (shape[j][i] == 'X') {
                gfx_putpixel(x + i, y + j, GFX_BLACK);
                if (i + 1 < CUR_W)
                    gfx_putpixel(x + i + 1, y + j, GFX_WHITE);
            }
        }
}

/* soft mac-like gradient wallpaper */
static void draw_wallpaper(void) {
    for (int y = 0; y < GFX_HEIGHT; y++) {
        uint8_t c;
        if (y < 30)
            c = GFX_LIGHT_CYAN;
        else if (y < 70)
            c = GFX_CYAN;
        else if (y < 110)
            c = GFX_LIGHT_BLUE;
        else if (y < 150)
            c = GFX_BLUE;
        else
            c = GFX_DARK_GREY;
        gfx_hline(0, y, GFX_WIDTH, c);
    }
    /* faint grid of stars */
    for (int i = 0; i < 40; i++) {
        int sx = (i * 37 + 13) % GFX_WIDTH;
        int sy = (i * 53 + 17) % (GFX_HEIGHT - 40);
        if (sy > 16)
            gfx_putpixel(sx, sy, GFX_WHITE);
    }
}

/* desktop icons: HD + Trash */
static void draw_desktop_icons(void) {
    /* Macintosh HD */
    gfx_fillrect(280, 24, 28, 22, GFX_LIGHT_GREY);
    gfx_rect(280, 24, 28, 22, GFX_WHITE);
    gfx_fillrect(284, 28, 20, 12, GFX_DARK_GREY);
    gfx_text(276, 48, "Disk", GFX_WHITE, GFX_BLUE);

    /* Trash */
    gfx_fillrect(284, 64, 20, 24, GFX_LIGHT_GREY);
    gfx_rect(284, 64, 20, 24, GFX_WHITE);
    gfx_fillrect(288, 68, 12, 14, GFX_DARK_GREY);
    gfx_text(276, 90, "Trash", GFX_WHITE, GFX_BLUE);
}

static void draw_menubar(void) {
    gfx_fillrect(0, 0, GFX_WIDTH, 15, GFX_LIGHT_GREY);
    gfx_hline(0, 15, GFX_WIDTH, GFX_DARK_GREY);

    /* Apple-like mark */
    gfx_fillrect(3, 2, 11, 11, GFX_BLACK);
    gfx_text(5, 3, "S", GFX_WHITE, GFX_BLACK);

    gfx_text(18, 3, "Finder", GFX_BLACK, GFX_LIGHT_GREY);
    gfx_text(70, 3, "File", GFX_DARK_GREY, GFX_LIGHT_GREY);
    gfx_text(106, 3, "Edit", GFX_DARK_GREY, GFX_LIGHT_GREY);
    gfx_text(142, 3, "View", GFX_DARK_GREY, GFX_LIGHT_GREY);
    gfx_text(182, 3, "Go", GFX_DARK_GREY, GFX_LIGHT_GREY);

    /* RTC clock */
    struct rtc_time t;
    rtc_read(&t);
    char clk[16];
    clk[0] = (char)('0' + (t.hour / 10) % 10);
    clk[1] = (char)('0' + t.hour % 10);
    clk[2] = ':';
    clk[3] = (char)('0' + (t.minute / 10) % 10);
    clk[4] = (char)('0' + t.minute % 10);
    clk[5] = 0;
    gfx_text(GFX_WIDTH - 44, 3, clk, GFX_BLACK, GFX_LIGHT_GREY);
}

static void draw_apple_menu(void) {
    if (!apple_open)
        return;
    int x = 2, y = 16, w = 110, h = 70;
    gfx_fillrect(x + 2, y + 2, w, h, GFX_DARK_GREY);
    gfx_fillrect(x, y, w, h, GFX_LIGHT_GREY);
    gfx_rect(x, y, w, h, GFX_WHITE);
    gfx_text(x + 6, y + 4, "About SIF", GFX_BLACK, GFX_LIGHT_GREY);
    gfx_hline(x + 4, y + 16, w - 8, GFX_DARK_GREY);
    gfx_text(x + 6, y + 22, "System Prefs", GFX_DARK_GREY, GFX_LIGHT_GREY);
    gfx_text(x + 6, y + 34, "BIOS Setup", GFX_BLACK, GFX_LIGHT_GREY);
    gfx_hline(x + 4, y + 46, w - 8, GFX_DARK_GREY);
    gfx_text(x + 6, y + 52, "Terminal", GFX_BLACK, GFX_LIGHT_GREY);
}

static void draw_traffic(int x, int y) {
    /* glossy traffic lights */
    gfx_fillrect(x, y, 8, 8, GFX_RED);
    gfx_rect(x, y, 8, 8, GFX_DARK_GREY);
    gfx_putpixel(x + 2, y + 2, GFX_LIGHT_RED);
    gfx_fillrect(x + 11, y, 8, 8, GFX_YELLOW);
    gfx_rect(x + 11, y, 8, 8, GFX_DARK_GREY);
    gfx_putpixel(x + 13, y + 2, GFX_WHITE);
    gfx_fillrect(x + 22, y, 8, 8, GFX_GREEN);
    gfx_rect(x + 22, y, 8, 8, GFX_DARK_GREY);
    gfx_putpixel(x + 24, y + 2, GFX_LIGHT_GREEN);
}

static void draw_window_aqua(int x, int y, int w, int h, const char *title) {
    /* drop shadow */
    gfx_fillrect(x + 3, y + 3, w, h, GFX_DARK_GREY);
    /* body */
    gfx_fillrect(x, y, w, h, GFX_LIGHT_GREY);
    gfx_rect(x, y, w, h, GFX_WHITE);
    /* titlebar */
    gfx_fillrect(x + 1, y + 1, w - 2, 15, GFX_LIGHT_GREY);
    gfx_hline(x + 1, y + 15, w - 2, GFX_DARK_GREY);
    /* subtle titlebar lines */
    for (int i = 3; i < 13; i += 2)
        gfx_hline(x + 40, y + i, w - 80, GFX_DARK_GREY);
    draw_traffic(x + 5, y + 4);
    int tw = (int)strlen(title) * 8;
    int tx = x + (w - tw) / 2;
    if (tx < x + 42)
        tx = x + 42;
    gfx_fillrect(tx - 2, y + 3, tw + 4, 10, GFX_LIGHT_GREY);
    gfx_text(tx, y + 4, title, GFX_BLACK, GFX_LIGHT_GREY);
}

static void draw_sidebar_item(int x, int y, const char *label, uint8_t icon,
                              int selected) {
    if (selected) {
        gfx_fillrect(x, y, 78, 13, GFX_LIGHT_BLUE);
        gfx_fillrect(x + 2, y + 2, 8, 8, icon);
        gfx_text(x + 14, y + 2, label, GFX_WHITE, GFX_LIGHT_BLUE);
    } else {
        gfx_fillrect(x + 2, y + 2, 8, 8, icon);
        gfx_text(x + 14, y + 2, label, GFX_BLACK, GFX_LIGHT_GREY);
    }
}

static void draw_app_icon(int x, int y, uint8_t color, const char *name) {
    gfx_fillrect(x, y, 28, 24, color);
    gfx_rect(x, y, 28, 24, GFX_WHITE);
    gfx_fillrect(x + 4, y + 4, 20, 12, GFX_WHITE);
    gfx_text(x - 4, y + 26, name, GFX_DARK_GREY, GFX_WHITE);
}

static void draw_content(void) {
    int wx = 18, wy = 22, ww = 250, wh = 138;
    draw_window_aqua(wx, wy, ww, wh, "Finder");

    /* sidebar */
    gfx_fillrect(wx + 2, wy + 17, 82, wh - 19, GFX_LIGHT_GREY);
    gfx_vline(wx + 84, wy + 17, wh - 19, GFX_DARK_GREY);
    gfx_text(wx + 6, wy + 20, "FAVORITES", GFX_DARK_GREY, GFX_LIGHT_GREY);
    draw_sidebar_item(wx + 2, wy + 34, "Apps", GFX_LIGHT_BLUE, panel == 0);
    draw_sidebar_item(wx + 2, wy + 48, "Disk", GFX_BROWN, panel == 1);
    draw_sidebar_item(wx + 2, wy + 62, "About", GFX_MAGENTA, panel == 2);
    draw_sidebar_item(wx + 2, wy + 76, "BIOS", GFX_BLUE, 0);

    /* content */
    gfx_fillrect(wx + 86, wy + 17, ww - 88, wh - 19, GFX_WHITE);

    int cx = wx + 92;
    int cy = wy + 22;

    if (panel == 0) {
        gfx_text(cx, cy, "Applications", GFX_BLACK, GFX_WHITE);
        draw_app_icon(cx, cy + 16, GFX_LIGHT_BLUE, "hello");
        draw_app_icon(cx + 48, cy + 16, GFX_GREEN, "echo");
        draw_app_icon(cx + 96, cy + 16, GFX_BROWN, "cat");
        draw_app_icon(cx, cy + 56, GFX_CYAN, "ls");
        gfx_text(cx + 48, cy + 64, "Double-click", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx + 48, cy + 74, "soon / use T", GFX_DARK_GREY, GFX_WHITE);
    } else if (panel == 1) {
        gfx_text(cx, cy, "Storage", GFX_BLACK, GFX_WHITE);
        if (!ata_present()) {
            gfx_text(cx, cy + 20, "No disk", GFX_RED, GFX_WHITE);
        } else if (!diskfs_mounted()) {
            gfx_text(cx, cy + 20, "Disk ready", GFX_BROWN, GFX_WHITE);
            gfx_text(cx, cy + 34, "mkfs in Terminal", GFX_LIGHT_BLUE, GFX_WHITE);
        } else {
            gfx_text(cx, cy + 20, "SFS Mounted", GFX_GREEN, GFX_WHITE);
            gfx_text(cx, cy + 34, "dls / dcat", GFX_DARK_GREY, GFX_WHITE);
        }
        gfx_text(cx, cy + 60, "Ramfs always on", GFX_DARK_GREY, GFX_WHITE);
    } else {
        gfx_text(cx, cy, "About This Mac", GFX_BLACK, GFX_WHITE);
        gfx_fillrect(cx, cy + 14, 40, 40, GFX_LIGHT_BLUE);
        gfx_rect(cx, cy + 14, 40, 40, GFX_DARK_GREY);
        gfx_text(cx + 12, cy + 28, "S", GFX_WHITE, GFX_LIGHT_BLUE);
        gfx_text(cx + 48, cy + 18, SIF_KERNEL_NAME, GFX_BLACK, GFX_WHITE);
        gfx_text(cx + 48, cy + 30, SIF_KERNEL_VERSION, GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(cx + 48, cy + 44, "i386 Hobby OS", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 70, "Aqua desktop + PS/2", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 82, "Inspired by Mac OS X", GFX_DARK_GREY, GFX_WHITE);
    }
}

static void dock_icon(int x, int y, uint8_t color, const char *label, int selected) {
    if (selected) {
        gfx_fillrect(x - 3, y - 3, 30, 26, GFX_WHITE);
        gfx_rect(x - 3, y - 3, 30, 26, GFX_LIGHT_CYAN);
    }
    gfx_fillrect(x, y, 24, 20, color);
    gfx_rect(x, y, 24, 20, GFX_WHITE);
    /* gloss */
    gfx_fillrect(x + 2, y + 2, 20, 6, GFX_WHITE);
    if (label && label[0])
        gfx_char(x + 8, y + 8, label[0], color == GFX_WHITE ? GFX_BLACK : GFX_WHITE,
                 color);
}

static void draw_dock(void) {
    int dock_h = 34;
    int dock_w = 220;
    dock_y = GFX_HEIGHT - dock_h - 2;
    dock_x = (GFX_WIDTH - dock_w) / 2;

    /* frosted dock */
    gfx_fillrect(dock_x + 2, dock_y + 2, dock_w, dock_h, GFX_DARK_GREY);
    gfx_fillrect(dock_x, dock_y, dock_w, dock_h, GFX_LIGHT_GREY);
    gfx_rect(dock_x, dock_y, dock_w, dock_h, GFX_WHITE);
    gfx_hline(dock_x, dock_y, dock_w, GFX_CYAN);

    dock_ix = dock_x + 10;
    dock_iy = dock_y + 5;
    dock_icon(dock_ix, dock_iy, GFX_LIGHT_BLUE, "A", dock_sel == 0);
    dock_icon(dock_ix + 32, dock_iy, GFX_BROWN, "D", dock_sel == 1);
    dock_icon(dock_ix + 64, dock_iy, GFX_MAGENTA, "i", dock_sel == 2);
    /* separator */
    gfx_vline(dock_ix + 96, dock_iy, 20, GFX_DARK_GREY);
    dock_icon(dock_ix + 104, dock_iy, GFX_BLUE, "B", dock_sel == 3);
    dock_icon(dock_ix + 136, dock_iy, GFX_BLACK, "T", dock_sel == 4);
    dock_icon(dock_ix + 168, dock_iy, GFX_DARK_GREY, "Q", dock_sel == 5);

    for (int i = 0; i < 5; i++) {
        int dx = dock_ix + (i < 3 ? i * 32 : 104 + (i - 3) * 32) + 10;
        if (i == dock_sel)
            gfx_fillrect(dx, dock_y + dock_h - 5, 4, 2, GFX_WHITE);
    }
}

static void draw_desktop(void) {
    cur_sx = cur_sy = -1;
    draw_wallpaper();
    draw_desktop_icons();
    draw_menubar();
    draw_content();
    draw_dock();
    draw_apple_menu();
}

/* hit ids:
 * 0-4 dock, 5 quit dock, 10-13 sidebar, 20 apple, 21 about menu,
 * 22 bios menu, 23 terminal menu, 30 disk icon, 31 trash, 99 close
 */
static int hit_test(int x, int y) {
    /* apple mark */
    if (x >= 3 && x < 14 && y >= 2 && y < 13)
        return 20;

    if (apple_open) {
        if (x >= 2 && x < 112 && y >= 16 && y < 86) {
            if (y < 32)
                return 21; /* about */
            if (y >= 34 && y < 46)
                return 22; /* bios */
            if (y >= 52 && y < 64)
                return 23; /* terminal */
            return 20;
        }
    }

    /* dock icons 0-4 + quit */
    for (int i = 0; i < 3; i++) {
        int ix = dock_ix + i * 32;
        if (x >= ix && x < ix + 24 && y >= dock_iy && y < dock_iy + 20)
            return i;
    }
    if (x >= dock_ix + 104 && x < dock_ix + 128 && y >= dock_iy && y < dock_iy + 20)
        return 3;
    if (x >= dock_ix + 136 && x < dock_ix + 160 && y >= dock_iy && y < dock_iy + 20)
        return 4;
    if (x >= dock_ix + 168 && x < dock_ix + 192 && y >= dock_iy && y < dock_iy + 20)
        return 5;

    /* sidebar */
    int sx = 20, sy0 = 56;
    for (int i = 0; i < 4; i++) {
        int sy = sy0 + i * 14;
        if (x >= sx && x < sx + 78 && y >= sy && y < sy + 13)
            return 10 + i;
    }

    /* desktop icons */
    if (x >= 280 && x < 308 && y >= 24 && y < 56)
        return 30;
    if (x >= 284 && x < 304 && y >= 64 && y < 98)
        return 31;

    /* red close */
    if (x >= 23 && x < 31 && y >= 26 && y < 34)
        return 99;

    return -1;
}

static int activate(int id) {
    if (id == 20) {
        apple_open = !apple_open;
        draw_desktop();
        return 0;
    }
    if (id == 21) {
        apple_open = 0;
        panel = 2;
        dock_sel = 2;
        draw_desktop();
        return 0;
    }
    if (id == 22 || id == 3 || id == 13) {
        apple_open = 0;
        dock_sel = 3;
        cursor_restore();
        gfx_restore_text();
        bios_run();
        gfx_init_mode13();
        draw_desktop();
        return 0;
    }
    if (id == 23 || id == 4 || id == 5 || id == 99)
        return 1;

    if (id >= 0 && id <= 2) {
        apple_open = 0;
        panel = id;
        dock_sel = id;
        draw_desktop();
        return 0;
    }
    if (id >= 10 && id <= 12) {
        apple_open = 0;
        panel = id - 10;
        dock_sel = panel;
        draw_desktop();
        return 0;
    }
    if (id == 30) {
        apple_open = 0;
        panel = 1;
        dock_sel = 1;
        draw_desktop();
        return 0;
    }
    if (id == 31) {
        /* trash — noop visual */
        apple_open = 0;
        draw_desktop();
        return 0;
    }
    /* click outside closes apple menu */
    if (apple_open) {
        apple_open = 0;
        draw_desktop();
    }
    return 0;
}

void gui_run(void) {
    printk("[gui] Aqua 2 — Mac-style desktop\n");

    panel = 0;
    dock_sel = 0;
    apple_open = 0;

    gfx_init_mode13();
    draw_desktop();

    struct mouse_state ms;
    mouse_get(&ms);
    cursor_draw(ms.x, ms.y);

    uint32_t last_min = 0xFFFFFFFFu;

    for (;;) {
        int quit = 0;

        /* refresh clock each minute */
        struct rtc_time t;
        rtc_read(&t);
        if (t.minute != last_min) {
            last_min = t.minute;
            cursor_restore();
            draw_menubar();
            draw_apple_menu();
            mouse_get(&ms);
            cursor_draw(ms.x, ms.y);
        }

        if (mouse_moved()) {
            mouse_get(&ms);
            int pressed = ms.left_pressed;
            mouse_clear_edges();
            cursor_draw(ms.x, ms.y);

            if (pressed) {
                int id = hit_test(ms.x, ms.y);
                printk("[gui] click %d,%d id=%d\n", ms.x, ms.y, id);
                if (id >= 0) {
                    cursor_restore();
                    if (activate(id))
                        quit = 1;
                    else {
                        mouse_get(&ms);
                        cursor_draw(ms.x, ms.y);
                    }
                } else if (apple_open) {
                    cursor_restore();
                    activate(-1);
                    mouse_get(&ms);
                    cursor_draw(ms.x, ms.y);
                }
            }
        } else {
            mouse_clear_edges();
        }

        if (keyboard_has_input()) {
            int ch = keyboard_read_char();
            if (ch == 27 || ch == 'q' || ch == 'Q')
                quit = 1;
            else if (ch >= '1' && ch <= '5') {
                cursor_restore();
                if (ch == '5')
                    quit = 1;
                else
                    activate(ch - '1');
                if (!quit) {
                    mouse_get(&ms);
                    cursor_draw(ms.x, ms.y);
                }
            }
        }

        if (quit)
            break;

        if (!keyboard_has_input() && !mouse_moved())
            __asm__ volatile("hlt");
    }

    cursor_restore();
    gfx_restore_text();
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    printk("[gui] left desktop\n");
}

void gui_task(void) {
    for (;;) {
        gui_run();
        shell_run();
    }
}
