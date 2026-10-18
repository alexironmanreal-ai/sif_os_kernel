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
#include "string.h"

static int panel;
static int dock_sel;

/* cursor underlay */
#define CUR_W 8
#define CUR_H 10
static uint8_t cur_under[CUR_W * CUR_H];
static int cur_sx = -1, cur_sy = -1;

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
    /* simple arrow */
    static const char *shape[10] = {
        "X.......",
        "XX......",
        "X.X.....",
        "X..X....",
        "X...X...",
        "X....X..",
        "X..XXXXX",
        "X.X.....",
        "XX......",
        "X.......",
    };
    for (int j = 0; j < CUR_H; j++)
        for (int i = 0; i < CUR_W; i++)
            if (shape[j][i] == 'X')
                gfx_putpixel(x + i, y + j, GFX_WHITE);
}

static void draw_wallpaper(void) {
    for (int y = 0; y < GFX_HEIGHT; y++) {
        uint8_t c;
        if (y < 40)
            c = GFX_LIGHT_CYAN;
        else if (y < 100)
            c = GFX_CYAN;
        else if (y < 160)
            c = GFX_LIGHT_BLUE;
        else
            c = GFX_BLUE;
        gfx_hline(0, y, GFX_WIDTH, c);
    }
}

static void draw_menubar(void) {
    gfx_fillrect(0, 0, GFX_WIDTH, 14, GFX_LIGHT_GREY);
    gfx_hline(0, 14, GFX_WIDTH, GFX_DARK_GREY);

    gfx_fillrect(4, 2, 10, 10, GFX_BLACK);
    gfx_text(5, 3, "S", GFX_WHITE, GFX_BLACK);

    gfx_text(20, 3, "SIF", GFX_BLACK, GFX_LIGHT_GREY);
    gfx_text(52, 3, "File", GFX_DARK_GREY, GFX_LIGHT_GREY);
    gfx_text(88, 3, "Edit", GFX_DARK_GREY, GFX_LIGHT_GREY);
    gfx_text(124, 3, "View", GFX_DARK_GREY, GFX_LIGHT_GREY);
    gfx_text(164, 3, "Window", GFX_DARK_GREY, GFX_LIGHT_GREY);

    char clk[8];
    uint32_t sec = timer_seconds();
    uint32_t h = (sec / 3600) % 24;
    uint32_t m = (sec / 60) % 60;
    clk[0] = (char)('0' + h / 10);
    clk[1] = (char)('0' + h % 10);
    clk[2] = ':';
    clk[3] = (char)('0' + m / 10);
    clk[4] = (char)('0' + m % 10);
    clk[5] = 0;
    gfx_text(GFX_WIDTH - 48, 3, clk, GFX_BLACK, GFX_LIGHT_GREY);
}

static void draw_traffic(int x, int y) {
    gfx_fillrect(x, y, 7, 7, GFX_RED);
    gfx_rect(x, y, 7, 7, GFX_DARK_GREY);
    gfx_fillrect(x + 10, y, 7, 7, GFX_YELLOW);
    gfx_rect(x + 10, y, 7, 7, GFX_DARK_GREY);
    gfx_fillrect(x + 20, y, 7, 7, GFX_GREEN);
    gfx_rect(x + 20, y, 7, 7, GFX_DARK_GREY);
}

static void draw_window_aqua(int x, int y, int w, int h, const char *title) {
    gfx_fillrect(x + 3, y + 3, w, h, GFX_DARK_GREY);
    gfx_fillrect(x, y, w, h, GFX_LIGHT_GREY);
    gfx_rect(x, y, w, h, GFX_WHITE);
    gfx_fillrect(x + 1, y + 1, w - 2, 14, GFX_LIGHT_GREY);
    gfx_hline(x + 1, y + 14, w - 2, GFX_DARK_GREY);
    draw_traffic(x + 6, y + 4);
    int tw = (int)strlen(title) * 8;
    int tx = x + (w - tw) / 2;
    if (tx < x + 40)
        tx = x + 40;
    gfx_text(tx, y + 4, title, GFX_BLACK, GFX_LIGHT_GREY);
}

static void draw_sidebar_item(int x, int y, const char *label, int selected) {
    if (selected) {
        gfx_fillrect(x, y, 72, 12, GFX_LIGHT_BLUE);
        gfx_text(x + 4, y + 2, label, GFX_WHITE, GFX_LIGHT_BLUE);
    } else {
        gfx_text(x + 4, y + 2, label, GFX_BLACK, GFX_LIGHT_GREY);
    }
}

static void draw_content(void) {
    int wx = 24, wy = 22, ww = 272, wh = 140;
    draw_window_aqua(wx, wy, ww, wh, "Finder");

    gfx_fillrect(wx + 2, wy + 16, 76, wh - 18, GFX_LIGHT_GREY);
    gfx_vline(wx + 78, wy + 16, wh - 18, GFX_DARK_GREY);
    gfx_text(wx + 6, wy + 20, "Favorites", GFX_DARK_GREY, GFX_LIGHT_GREY);
    draw_sidebar_item(wx + 2, wy + 34, "Apps", panel == 0);
    draw_sidebar_item(wx + 2, wy + 48, "Disk", panel == 1);
    draw_sidebar_item(wx + 2, wy + 62, "About", panel == 2);
    draw_sidebar_item(wx + 2, wy + 76, "BIOS", 0);

    gfx_fillrect(wx + 80, wy + 16, ww - 82, wh - 18, GFX_WHITE);

    int cx = wx + 88;
    int cy = wy + 22;

    if (panel == 0) {
        gfx_text(cx, cy, "Applications", GFX_BLACK, GFX_WHITE);
        gfx_text(cx, cy + 16, "hello.elf", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 28, "echo.elf", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 40, "cat.elf   ls.elf", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 60, "Click dock icons", GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(cx, cy + 74, "T = Terminal", GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(cx, cy + 96, "Mouse enabled", GFX_DARK_GREY, GFX_WHITE);
    } else if (panel == 1) {
        gfx_text(cx, cy, "Storage", GFX_BLACK, GFX_WHITE);
        if (!ata_present()) {
            gfx_text(cx, cy + 20, "No disk", GFX_RED, GFX_WHITE);
        } else if (!diskfs_mounted()) {
            gfx_text(cx, cy + 20, "Disk ready", GFX_BROWN, GFX_WHITE);
            gfx_text(cx, cy + 34, "Terminal: mkfs", GFX_LIGHT_BLUE, GFX_WHITE);
        } else {
            gfx_text(cx, cy + 20, "SFS Mounted", GFX_GREEN, GFX_WHITE);
            gfx_text(cx, cy + 34, "dls / dcat", GFX_DARK_GREY, GFX_WHITE);
        }
    } else {
        gfx_text(cx, cy, "About This Mac", GFX_BLACK, GFX_WHITE);
        gfx_text(cx, cy + 18, SIF_KERNEL_NAME, GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 32, SIF_KERNEL_VERSION, GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(cx, cy + 52, "PS/2 mouse ON", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 66, "i386  Hobby OS", GFX_DARK_GREY, GFX_WHITE);
    }
}

static void dock_icon(int x, int y, uint8_t color, const char *label, int selected) {
    if (selected) {
        gfx_fillrect(x - 2, y - 2, 28, 28, GFX_WHITE);
        gfx_rect(x - 2, y - 2, 28, 28, GFX_LIGHT_BLUE);
    }
    gfx_fillrect(x, y, 24, 20, color);
    gfx_rect(x, y, 24, 20, GFX_DARK_GREY);
    if (label && label[0])
        gfx_char(x + 8, y + 6, label[0], GFX_WHITE, color);
}

/* dock geometry — shared with hit-test */
static int dock_x, dock_y, dock_ix, dock_iy;

static void draw_dock(void) {
    dock_y = GFX_HEIGHT - 32;
    int dock_h = 30;
    int dock_w = 200;
    dock_x = (GFX_WIDTH - dock_w) / 2;

    gfx_fillrect(dock_x, dock_y, dock_w, dock_h, GFX_LIGHT_GREY);
    gfx_rect(dock_x, dock_y, dock_w, dock_h, GFX_WHITE);
    gfx_hline(dock_x, dock_y, dock_w, GFX_DARK_GREY);

    dock_ix = dock_x + 12;
    dock_iy = dock_y + 4;
    dock_icon(dock_ix, dock_iy, GFX_LIGHT_BLUE, "A", dock_sel == 0);
    dock_icon(dock_ix + 36, dock_iy, GFX_BROWN, "D", dock_sel == 1);
    dock_icon(dock_ix + 72, dock_iy, GFX_MAGENTA, "i", dock_sel == 2);
    dock_icon(dock_ix + 108, dock_iy, GFX_BLUE, "B", dock_sel == 3);
    dock_icon(dock_ix + 144, dock_iy, GFX_DARK_GREY, "T", dock_sel == 4);

    for (int i = 0; i < 5; i++) {
        int dx = dock_ix + i * 36 + 10;
        if (i == dock_sel)
            gfx_fillrect(dx, dock_y + 26, 4, 2, GFX_WHITE);
    }
}

static void draw_desktop(void) {
    cur_sx = cur_sy = -1; /* full redraw invalidates underlay */
    draw_wallpaper();
    draw_menubar();
    draw_content();
    draw_dock();
}

/* returns: -1 none, 0-4 dock, 10-13 sidebar Apps/Disk/About/BIOS */
static int hit_test(int x, int y) {
    for (int i = 0; i < 5; i++) {
        int ix = dock_ix + i * 36;
        if (x >= ix && x < ix + 24 && y >= dock_iy && y < dock_iy + 20)
            return i;
    }
    /* sidebar items */
    int sx = 26, sy0 = 56;
    for (int i = 0; i < 4; i++) {
        int sy = sy0 + i * 14;
        if (x >= sx && x < sx + 72 && y >= sy && y < sy + 12)
            return 10 + i;
    }
    /* red traffic close */
    if (x >= 30 && x < 37 && y >= 26 && y < 33)
        return 99;
    return -1;
}

static int activate(int id) {
    if (id >= 0 && id <= 2) {
        panel = id;
        dock_sel = id;
        draw_desktop();
        return 0;
    }
    if (id == 3 || id == 13) {
        dock_sel = 3;
        draw_desktop();
        cursor_restore();
        gfx_restore_text();
        bios_run();
        gfx_init_mode13();
        draw_desktop();
        return 0;
    }
    if (id == 4 || id == 99)
        return 1; /* exit GUI */
    if (id >= 10 && id <= 12) {
        panel = id - 10;
        dock_sel = panel;
        draw_desktop();
    }
    return 0;
}

void gui_run(void) {
    printk("[gui] Aqua + mouse\n");

    panel = 0;
    dock_sel = 0;

    gfx_init_mode13();
    draw_desktop();

    struct mouse_state ms;
    mouse_get(&ms);
    cursor_draw(ms.x, ms.y);
    printk("[gui] cursor at %d,%d\n", ms.x, ms.y);

    for (;;) {
        int redraw_cursor = 0;
        int quit = 0;

        if (mouse_moved()) {
            mouse_get(&ms);
            mouse_clear_edges();
            cursor_draw(ms.x, ms.y);
            redraw_cursor = 1;

            if (ms.left_pressed) {
                int id = hit_test(ms.x, ms.y);
                printk("[gui] click %d,%d -> %d\n", ms.x, ms.y, id);
                if (id >= 0) {
                    cursor_restore();
                    if (activate(id))
                        quit = 1;
                    else {
                        mouse_get(&ms);
                        cursor_draw(ms.x, ms.y);
                    }
                }
            }
        } else {
            mouse_clear_edges();
        }

        if (keyboard_has_input()) {
            int ch = keyboard_read_char();
            if (ch == 27 || ch == 'q' || ch == 'Q')
                quit = 1;
            else if (ch == '1') {
                cursor_restore();
                activate(0);
                mouse_get(&ms);
                cursor_draw(ms.x, ms.y);
            } else if (ch == '2') {
                cursor_restore();
                activate(1);
                mouse_get(&ms);
                cursor_draw(ms.x, ms.y);
            } else if (ch == '3') {
                cursor_restore();
                activate(2);
                mouse_get(&ms);
                cursor_draw(ms.x, ms.y);
            } else if (ch == '4' || ch == 'b' || ch == 'B') {
                cursor_restore();
                activate(3);
                mouse_get(&ms);
                cursor_draw(ms.x, ms.y);
            } else if (ch == '5') {
                quit = 1;
            }
        }

        if (quit)
            break;

        if (!redraw_cursor && !keyboard_has_input() && !mouse_moved())
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
