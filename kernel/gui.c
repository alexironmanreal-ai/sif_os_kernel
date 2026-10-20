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
#include "userland.h"
#include "ns.h"

static int panel;
static int dock_sel;
static int apple_open;
static char status_msg[40];

#define CUR_W 9
#define CUR_H 12
static uint8_t cur_under[CUR_W * CUR_H];
static int cur_sx = -1, cur_sy = -1;
static int dock_x, dock_y, dock_ix, dock_iy;
static int content_cx, content_cy;

static const char *app_paths[4] = {
    "apps.hello", "apps.echo", "apps.cat", "apps.ls"
};
static const char *app_labels[4] = { "hello", "echo", "cat", "ls" };
static const uint8_t app_colors[4] = {
    GFX_LIGHT_BLUE, GFX_GREEN, GFX_BROWN, GFX_CYAN
};

static void cursor_save(int x, int y) {
    for (int j = 0; j < CUR_H; j++)
        for (int i = 0; i < CUR_W; i++) {
            int px = x + i, py = y + j;
            if (px >= 0 && px < GFX_WIDTH && py >= 0 && py < GFX_HEIGHT)
                cur_under[j * CUR_W + i] = gfx_getpixel(px, py);
            else
                cur_under[j * CUR_W + i] = 0;
        }
    cur_sx = x;
    cur_sy = y;
}

static void cursor_restore(void) {
    if (cur_sx < 0)
        return;
    for (int j = 0; j < CUR_H; j++)
        for (int i = 0; i < CUR_W; i++) {
            int px = cur_sx + i, py = cur_sy + j;
            if (px >= 0 && px < GFX_WIDTH && py >= 0 && py < GFX_HEIGHT)
                gfx_putpixel(px, py, cur_under[j * CUR_W + i]);
        }
    cur_sx = cur_sy = -1;
}

static void cursor_draw(int x, int y) {
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x > GFX_WIDTH - 2)
        x = GFX_WIDTH - 2;
    if (y > GFX_HEIGHT - 2)
        y = GFX_HEIGHT - 2;

    cursor_restore();
    cursor_save(x, y);

    static const char *shape[12] = {
        "X........", "XX.......", "X.X......", "X..X.....",
        "X...X....", "X....X...", "X.....X..", "X..XXXXX.",
        "X.X......", "XX.X.....", "X..X.....", ".X.......",
    };
    for (int j = 0; j < CUR_H; j++)
        for (int i = 0; i < CUR_W; i++)
            if (shape[j][i] == 'X') {
                int px = x + i, py = y + j;
                if (px >= 0 && px < GFX_WIDTH && py >= 0 && py < GFX_HEIGHT)
                    gfx_putpixel(px, py, GFX_WHITE);
            }
}

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
}

static void draw_desktop_icons(void) {
    gfx_fillrect(280, 24, 28, 22, GFX_LIGHT_GREY);
    gfx_rect(280, 24, 28, 22, GFX_WHITE);
    gfx_text(276, 48, "Vault", GFX_WHITE, GFX_BLUE);
}

static void draw_menubar(void) {
    gfx_fillrect(0, 0, GFX_WIDTH, 15, GFX_LIGHT_GREY);
    gfx_hline(0, 15, GFX_WIDTH, GFX_DARK_GREY);
    gfx_fillrect(3, 2, 11, 11, GFX_BLACK);
    gfx_text(5, 3, "S", GFX_WHITE, GFX_BLACK);
    gfx_text(18, 3, "Cells", GFX_BLACK, GFX_LIGHT_GREY);

    struct rtc_time t;
    rtc_read(&t);
    char clk[8];
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
    gfx_fillrect(2, 16, 110, 70, GFX_LIGHT_GREY);
    gfx_rect(2, 16, 110, 70, GFX_WHITE);
    gfx_text(8, 20, "About SIF", GFX_BLACK, GFX_LIGHT_GREY);
    gfx_text(8, 36, "Setup", GFX_BLACK, GFX_LIGHT_GREY);
    gfx_text(8, 52, "Console", GFX_BLACK, GFX_LIGHT_GREY);
}

static void draw_traffic(int x, int y) {
    gfx_fillrect(x, y, 8, 8, GFX_RED);
    gfx_fillrect(x + 11, y, 8, 8, GFX_YELLOW);
    gfx_fillrect(x + 22, y, 8, 8, GFX_GREEN);
}

static void draw_window_aqua(int x, int y, int w, int h, const char *title) {
    gfx_fillrect(x + 2, y + 2, w, h, GFX_DARK_GREY);
    gfx_fillrect(x, y, w, h, GFX_LIGHT_GREY);
    gfx_rect(x, y, w, h, GFX_WHITE);
    gfx_fillrect(x + 1, y + 1, w - 2, 15, GFX_LIGHT_GREY);
    gfx_hline(x + 1, y + 15, w - 2, GFX_DARK_GREY);
    draw_traffic(x + 5, y + 4);
    gfx_text(x + 40, y + 4, title, GFX_BLACK, GFX_LIGHT_GREY);
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
    gfx_text(x - 4, y + 26, name, GFX_DARK_GREY, GFX_WHITE);
}

static void draw_content(void) {
    int wx = 18, wy = 22, ww = 250, wh = 138;
    draw_window_aqua(wx, wy, ww, wh, "Cellview");

    gfx_fillrect(wx + 2, wy + 17, 82, wh - 19, GFX_LIGHT_GREY);
    gfx_vline(wx + 84, wy + 17, wh - 19, GFX_DARK_GREY);
    draw_sidebar_item(wx + 2, wy + 34, "Apps", GFX_LIGHT_BLUE, panel == 0);
    draw_sidebar_item(wx + 2, wy + 48, "Vault", GFX_BROWN, panel == 1);
    draw_sidebar_item(wx + 2, wy + 62, "About", GFX_MAGENTA, panel == 2);
    draw_sidebar_item(wx + 2, wy + 76, "Setup", GFX_BLUE, 0);

    gfx_fillrect(wx + 86, wy + 17, ww - 88, wh - 19, GFX_WHITE);
    content_cx = wx + 92;
    content_cy = wy + 22;

    if (panel == 0) {
        gfx_text(content_cx, content_cy, "WASD move Enter click", GFX_BLACK, GFX_WHITE);
        for (int i = 0; i < 4; i++) {
            int ix = content_cx + (i % 3) * 48;
            int iy = content_cy + 16 + (i / 3) * 40;
            draw_app_icon(ix, iy, app_colors[i], app_labels[i]);
        }
        if (status_msg[0])
            gfx_text(content_cx, content_cy + 100, status_msg, GFX_LIGHT_BLUE, GFX_WHITE);
    } else if (panel == 1) {
        gfx_text(content_cx, content_cy, "Vault", GFX_BLACK, GFX_WHITE);
        gfx_text(content_cx, content_cy + 20,
                 ata_present() ? "Disk present" : "No disk", GFX_DARK_GREY, GFX_WHITE);
    } else {
        gfx_text(content_cx, content_cy, "About SIF", GFX_BLACK, GFX_WHITE);
        gfx_text(content_cx, content_cy + 20, SIF_KERNEL_VERSION, GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(content_cx, content_cy + 40, "ESC = console", GFX_DARK_GREY, GFX_WHITE);
    }
}

static void dock_icon(int x, int y, uint8_t color, const char *label, int selected) {
    if (selected)
        gfx_rect(x - 2, y - 2, 28, 24, GFX_WHITE);
    gfx_fillrect(x, y, 24, 20, color);
    gfx_rect(x, y, 24, 20, GFX_WHITE);
    if (label && label[0])
        gfx_char(x + 8, y + 6, label[0], GFX_WHITE, color);
}

static void draw_dock(void) {
    int dock_h = 34, dock_w = 220;
    dock_y = GFX_HEIGHT - dock_h - 2;
    dock_x = (GFX_WIDTH - dock_w) / 2;
    gfx_fillrect(dock_x, dock_y, dock_w, dock_h, GFX_LIGHT_GREY);
    gfx_rect(dock_x, dock_y, dock_w, dock_h, GFX_WHITE);
    dock_ix = dock_x + 10;
    dock_iy = dock_y + 5;
    dock_icon(dock_ix, dock_iy, GFX_LIGHT_BLUE, "A", dock_sel == 0);
    dock_icon(dock_ix + 32, dock_iy, GFX_BROWN, "V", dock_sel == 1);
    dock_icon(dock_ix + 64, dock_iy, GFX_MAGENTA, "i", dock_sel == 2);
    dock_icon(dock_ix + 104, dock_iy, GFX_BLUE, "S", dock_sel == 3);
    dock_icon(dock_ix + 136, dock_iy, GFX_BLACK, "C", dock_sel == 4);
    dock_icon(dock_ix + 168, dock_iy, GFX_DARK_GREY, "Q", dock_sel == 5);
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

static int spawn_app(int idx) {
    if (idx < 0 || idx > 3)
        return -1;
    const char *key = ns_store_key(app_paths[idx]);
    if (!key)
        return -1;
    char *argv[1];
    argv[0] = (char *)key;
    int pid = userland_spawn_file_argv(key, 1, argv);
    if (pid >= 0) {
        status_msg[0] = 'o';
        status_msg[1] = 'k';
        status_msg[2] = ' ';
        status_msg[3] = (char)('0' + (pid % 10));
        status_msg[4] = 0;
        printk("[desk] spawn pid %d\n", pid);
    } else {
        status_msg[0] = 'f';
        status_msg[1] = 'a';
        status_msg[2] = 'i';
        status_msg[3] = 'l';
        status_msg[4] = 0;
    }
    return pid;
}

static int hit_test(int x, int y) {
    if (x >= 3 && x < 14 && y >= 2 && y < 13)
        return 20;
    if (apple_open && x >= 2 && x < 112 && y >= 16 && y < 86) {
        if (y < 32)
            return 21;
        if (y >= 34 && y < 46)
            return 22;
        if (y >= 50)
            return 23;
        return 20;
    }
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

    for (int i = 0; i < 4; i++) {
        int sy = 56 + i * 14;
        if (x >= 20 && x < 98 && y >= sy && y < sy + 13)
            return 10 + i;
    }

    if (panel == 0) {
        for (int i = 0; i < 4; i++) {
            int ix = content_cx + (i % 3) * 48;
            int iy = content_cy + 16 + (i / 3) * 40;
            if (x >= ix && x < ix + 28 && y >= iy && y < iy + 24)
                return 40 + i;
        }
    }
    if (x >= 23 && x < 31 && y >= 26 && y < 34)
        return 99;
    return -1;
}

static int activate(int id) {
    if (id >= 40 && id <= 43) {
        apple_open = 0;
        spawn_app(id - 40);
        draw_desktop();
        return 0;
    }
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
    if (apple_open) {
        apple_open = 0;
        draw_desktop();
    }
    return 0;
}

void gui_run(void) {
    printk("[desk] responsive loop\n");
    panel = 0;
    dock_sel = 0;
    apple_open = 0;
    status_msg[0] = 0;

    gfx_init_mode13();
    draw_desktop();

    struct mouse_state ms;
    mouse_get(&ms);
    cursor_draw(ms.x, ms.y);

    for (;;) {
        int quit = 0;

        /* poll every iteration — never block long */
        mouse_poll();

        if (mouse_moved()) {
            mouse_get(&ms);
            int pressed = ms.left_pressed;
            int x = ms.x, y = ms.y;
            mouse_clear_edges();
            cursor_draw(x, y);

            if (pressed) {
                cursor_restore();
                if (activate(hit_test(x, y)))
                    quit = 1;
                else {
                    mouse_get(&ms);
                    cursor_draw(ms.x, ms.y);
                }
            }
        }

        while (keyboard_has_input()) {
            int ch = keyboard_read_char();
            if (ch == 27 || ch == 'q' || ch == 'Q') {
                quit = 1;
                break;
            }
            if (ch == 'w' || ch == 'W')
                mouse_nudge(0, -6);
            else if (ch == 's' || ch == 'S')
                mouse_nudge(0, 6);
            else if (ch == 'a' || ch == 'A')
                mouse_nudge(-6, 0);
            else if (ch == 'd' || ch == 'D')
                mouse_nudge(6, 0);
            else if (ch == '\n' || ch == ' ') {
                mouse_get(&ms);
                cursor_restore();
                if (activate(hit_test(ms.x, ms.y)))
                    quit = 1;
                else {
                    mouse_get(&ms);
                    cursor_draw(ms.x, ms.y);
                }
            } else if (ch >= '1' && ch <= '4') {
                cursor_restore();
                spawn_app(ch - '1');
                draw_desktop();
                mouse_get(&ms);
                cursor_draw(ms.x, ms.y);
            } else if (ch == '5') {
                quit = 1;
                break;
            }

            if (mouse_moved() && !quit) {
                mouse_get(&ms);
                mouse_clear_edges();
                cursor_draw(ms.x, ms.y);
            }
        }

        if (quit)
            break;

        /* short pause only — stay responsive to mouse */
        for (volatile int i = 0; i < 500; i++)
            ;
    }

    cursor_restore();
    gfx_restore_text();
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    printk("[desk] exit\n");
}

void gui_task(void) {
    for (;;) {
        gui_run();
        shell_run();
    }
}
