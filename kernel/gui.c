#include "gui.h"
#include "bios.h"
#include "shell.h"
#include "gfx.h"
#include "keyboard.h"
#include "printk.h"
#include "kernel.h"
#include "vga.h"
#include "ata.h"
#include "diskfs.h"
#include "timer.h"
#include "task.h"
#include "string.h"

static int panel;
static int dock_sel;

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
        gfx_text(cx, cy + 60, "Dock 5 = Terminal", GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(cx, cy + 74, "ESC = Terminal", GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(cx, cy + 96, "Click QEMU video win", GFX_DARK_GREY, GFX_WHITE);
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
        gfx_text(cx, cy + 60, "Ramfs: ls / cat", GFX_DARK_GREY, GFX_WHITE);
    } else {
        gfx_text(cx, cy, "About This Mac", GFX_BLACK, GFX_WHITE);
        gfx_text(cx, cy + 18, SIF_KERNEL_NAME, GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 32, SIF_KERNEL_VERSION, GFX_LIGHT_BLUE, GFX_WHITE);
        gfx_text(cx, cy + 52, "Boots to desktop", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 66, "i386  Hobby OS", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(cx, cy + 90, "Inspired by Mac OS", GFX_DARK_GREY, GFX_WHITE);
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

static void draw_dock(void) {
    int dock_y = GFX_HEIGHT - 32;
    int dock_h = 30;
    int dock_w = 200;
    int dock_x = (GFX_WIDTH - dock_w) / 2;

    gfx_fillrect(dock_x, dock_y, dock_w, dock_h, GFX_LIGHT_GREY);
    gfx_rect(dock_x, dock_y, dock_w, dock_h, GFX_WHITE);
    gfx_hline(dock_x, dock_y, dock_w, GFX_DARK_GREY);

    int ix = dock_x + 12;
    int iy = dock_y + 4;
    dock_icon(ix, iy, GFX_LIGHT_BLUE, "A", dock_sel == 0);
    dock_icon(ix + 36, iy, GFX_BROWN, "D", dock_sel == 1);
    dock_icon(ix + 72, iy, GFX_MAGENTA, "i", dock_sel == 2);
    dock_icon(ix + 108, iy, GFX_BLUE, "B", dock_sel == 3);
    dock_icon(ix + 144, iy, GFX_DARK_GREY, "T", dock_sel == 4);

    for (int i = 0; i < 5; i++) {
        int dx = ix + i * 36 + 10;
        if (i == dock_sel)
            gfx_fillrect(dx, dock_y + 26, 4, 2, GFX_WHITE);
    }
}

static void draw_desktop(void) {
    draw_wallpaper();
    draw_menubar();
    draw_content();
    draw_dock();
}

void gui_run(void) {
    printk("[gui] entering Aqua desktop\n");
    printk("[gui] FOCUS the QEMU *video* window\n");
    printk("[gui] keys: 1-5  ESC=shell\n");

    panel = 0;
    dock_sel = 0;

    gfx_init_mode13();
    draw_desktop();
    printk("[gui] desktop drawn — waiting keys\n");

    uint32_t last_beat = timer_seconds();

    for (;;) {
        if (task_needs_resched()) {
            task_clear_resched();
            task_yield();
        }

        /* serial heartbeat so the log does not look frozen */
        uint32_t now = timer_seconds();
        if (now != last_beat) {
            last_beat = now;
            printk("[gui] alive %us (ESC=shell)\n", now);
        }

        if (!keyboard_has_input()) {
            __asm__ volatile("hlt");
            continue;
        }
        int ch = keyboard_read_char();
        if (!ch)
            continue;

        printk("[gui] key %d\n", ch);

        if (ch == 27 || ch == 'q' || ch == 'Q')
            break;

        if (ch == '1') {
            panel = 0;
            dock_sel = 0;
            draw_desktop();
        } else if (ch == '2') {
            panel = 1;
            dock_sel = 1;
            draw_desktop();
        } else if (ch == '3') {
            panel = 2;
            dock_sel = 2;
            draw_desktop();
        } else if (ch == '4' || ch == 'b' || ch == 'B') {
            dock_sel = 3;
            draw_desktop();
            gfx_restore_text();
            bios_run();
            gfx_init_mode13();
            draw_desktop();
        } else if (ch == '5') {
            dock_sel = 4;
            draw_desktop();
            break;
        } else if (ch == 'a' || ch == 'A') {
            dock_sel = (dock_sel + 4) % 5;
            if (dock_sel < 3)
                panel = dock_sel;
            draw_desktop();
        } else if (ch == 'd' || ch == 'D') {
            dock_sel = (dock_sel + 1) % 5;
            if (dock_sel < 3)
                panel = dock_sel;
            draw_desktop();
        } else if (ch == '\n' || ch == '\r') {
            if (dock_sel == 3) {
                gfx_restore_text();
                bios_run();
                gfx_init_mode13();
                draw_desktop();
            } else if (dock_sel == 4) {
                break;
            } else {
                panel = dock_sel;
                draw_desktop();
            }
        }
    }

    gfx_restore_text();
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    printk("[gui] Terminal — type 'gui' to return\n");
}

void gui_task(void) {
    printk("[desktop] task running\n");
    task_yield();
    for (;;) {
        gui_run();
        shell_run();
    }
}
