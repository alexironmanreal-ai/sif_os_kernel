#include "gui.h"
#include "gfx.h"
#include "keyboard.h"
#include "printk.h"
#include "kernel.h"
#include "vga.h"
#include "ata.h"
#include "diskfs.h"
#include "timer.h"
#include "string.h"

static int panel; /* 0=apps 1=disk 2=about */

static void draw_button(int x, int y, int w, int h, const char *label, int selected) {
    uint8_t bg = selected ? GFX_LIGHT_BLUE : GFX_LIGHT_GREY;
    uint8_t fg = selected ? GFX_WHITE : GFX_BLACK;
    uint8_t bd = selected ? GFX_WHITE : GFX_DARK_GREY;
    gfx_fillrect(x, y, w, h, bg);
    gfx_rect(x, y, w, h, bd);
    int tx = x + (w - (int)strlen(label) * 8) / 2;
    if (tx < x + 2)
        tx = x + 2;
    gfx_text(tx, y + (h - 8) / 2, label, fg, bg);
}

static void draw_window(int x, int y, int w, int h, const char *title) {
    gfx_fillrect(x, y, w, h, GFX_LIGHT_GREY);
    gfx_rect(x, y, w, h, GFX_WHITE);
    gfx_fillrect(x + 1, y + 1, w - 2, 12, GFX_BLUE);
    gfx_text(x + 4, y + 3, title, GFX_WHITE, GFX_BLUE);
    /* close box */
    gfx_fillrect(x + w - 12, y + 2, 9, 9, GFX_RED);
    gfx_text(x + w - 11, y + 2, "x", GFX_WHITE, GFX_RED);
}

static void draw_desktop(void) {
    /* wallpaper */
    gfx_fillrect(0, 0, GFX_WIDTH, GFX_HEIGHT - 16, GFX_BLUE);

    /* subtle pattern */
    for (int y = 0; y < GFX_HEIGHT - 16; y += 16)
        for (int x = 0; x < GFX_WIDTH; x += 16)
            gfx_putpixel(x, y, GFX_LIGHT_BLUE);

    /* taskbar */
    gfx_fillrect(0, GFX_HEIGHT - 16, GFX_WIDTH, 16, GFX_DARK_GREY);
    gfx_hline(0, GFX_HEIGHT - 16, GFX_WIDTH, GFX_WHITE);
    gfx_text(4, GFX_HEIGHT - 12, "SIF", GFX_YELLOW, GFX_DARK_GREY);
    gfx_text(40, GFX_HEIGHT - 12, "1:Apps 2:Disk 3:About  ESC:exit",
             GFX_WHITE, GFX_DARK_GREY);

    /* main window */
    draw_window(20, 12, 280, 160, "SIF Desktop");

    /* tab buttons */
    draw_button(28, 30, 56, 14, "Apps", panel == 0);
    draw_button(90, 30, 56, 14, "Disk", panel == 1);
    draw_button(152, 30, 56, 14, "About", panel == 2);

    /* content area */
    gfx_fillrect(28, 50, 264, 112, GFX_WHITE);
    gfx_rect(28, 50, 264, 112, GFX_DARK_GREY);

    if (panel == 0) {
        gfx_text(36, 56, "Applications", GFX_BLACK, GFX_WHITE);
        gfx_text(36, 72, "* hello.elf   (userspace)", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(36, 84, "* echo.elf    (userspace)", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(36, 96, "* cat.elf / ls.elf", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(36, 112, "Shell: exec hello.elf", GFX_BLUE, GFX_WHITE);
        gfx_text(36, 128, "Shell: demo / ps / mem", GFX_BLUE, GFX_WHITE);
        gfx_text(36, 144, "More apps coming soon", GFX_DARK_GREY, GFX_WHITE);
    } else if (panel == 1) {
        gfx_text(36, 56, "Disk / SFS", GFX_BLACK, GFX_WHITE);
        if (!ata_present()) {
            gfx_text(36, 76, "No ATA disk attached", GFX_RED, GFX_WHITE);
            gfx_text(36, 92, "make run uses disk.img", GFX_DARK_GREY, GFX_WHITE);
        } else if (!diskfs_mounted()) {
            gfx_text(36, 76, "Disk present, SFS not mounted", GFX_BROWN, GFX_WHITE);
            gfx_text(36, 92, "Shell: mkfs then dwrite", GFX_BLUE, GFX_WHITE);
        } else {
            gfx_text(36, 76, "SFS mounted OK", GFX_GREEN, GFX_WHITE);
            gfx_text(36, 92, "Shell: dls / dcat / dwrite", GFX_BLUE, GFX_WHITE);
        }
        gfx_text(36, 120, "Ramfs still available (ls)", GFX_DARK_GREY, GFX_WHITE);
    } else {
        gfx_text(36, 56, SIF_KERNEL_NAME, GFX_BLACK, GFX_WHITE);
        gfx_text(36, 72, "Version:", GFX_DARK_GREY, GFX_WHITE);
        gfx_text(100, 72, SIF_KERNEL_VERSION, GFX_BLUE, GFX_WHITE);
        gfx_text(36, 92, "Hobby OS — structure first",
                 GFX_DARK_GREY, GFX_WHITE);
        gfx_text(36, 108, "Then apps & GUI",
                 GFX_DARK_GREY, GFX_WHITE);
        gfx_text(36, 128, "Keys: 1 2 3 panels | ESC quit",
                 GFX_BLACK, GFX_WHITE);
        char up[32];
        uint32_t s = timer_seconds();
        /* tiny uptime string */
        up[0] = 'u';
        up[1] = 'p';
        up[2] = ' ';
        up[3] = '0' + (char)((s / 60) / 10 % 10);
        up[4] = '0' + (char)((s / 60) % 10);
        up[5] = 'm';
        up[6] = ' ';
        up[7] = '0' + (char)((s % 60) / 10);
        up[8] = '0' + (char)(s % 60);
        up[9] = 's';
        up[10] = 0;
        gfx_text(36, 144, up, GFX_DARK_GREY, GFX_WHITE);
    }
}

void gui_run(void) {
    printk("[gui] entering mode 13h — ESC to exit\n");
    panel = 0;
    gfx_init_mode13();
    draw_desktop();

    for (;;) {
        if (!keyboard_has_input()) {
            __asm__ volatile("hlt");
            continue;
        }
        int ch = keyboard_read_char();
        if (!ch)
            continue;
        if (ch == 27) /* ESC */
            break;
        if (ch == '1') {
            panel = 0;
            draw_desktop();
        } else if (ch == '2') {
            panel = 1;
            draw_desktop();
        } else if (ch == '3') {
            panel = 2;
            draw_desktop();
        }
    }

    gfx_restore_text();
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n[gui] back to text mode\n");
}
