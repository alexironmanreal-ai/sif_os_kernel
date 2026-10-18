#include "bios.h"
#include "gfx.h"
#include "keyboard.h"
#include "printk.h"
#include "kernel.h"
#include "vga.h"
#include "ata.h"
#include "diskfs.h"
#include "timer.h"
#include "pmm.h"
#include "kmalloc.h"
#include "string.h"

/* Award/AMI style palette use */
#define BIOS_BG      GFX_BLUE
#define BIOS_FG      GFX_WHITE
#define BIOS_TITLE   GFX_YELLOW
#define BIOS_HI_BG   GFX_LIGHT_GREY
#define BIOS_HI_FG   GFX_BLACK
#define BIOS_CYAN    GFX_CYAN
#define BIOS_DIM     GFX_LIGHT_CYAN
#define BIOS_BORDER  GFX_CYAN

enum {
    MENU_MAIN = 0,
    MENU_ADV,
    MENU_BOOT,
    MENU_EXIT,
    MENU_COUNT
};

static int menu_sel;
static int item_sel;

static const char *menu_names[MENU_COUNT] = {
    "Main", "Advanced", "Boot", "Exit"
};

static void bios_box(int x, int y, int w, int h) {
    gfx_fillrect(x, y, w, h, BIOS_BG);
    gfx_rect(x, y, w, h, BIOS_BORDER);
    gfx_rect(x + 1, y + 1, w - 2, h - 2, BIOS_BORDER);
}

static void bios_hline_double(int x, int y, int w) {
    gfx_hline(x, y, w, BIOS_BORDER);
    gfx_hline(x, y + 1, w, BIOS_BORDER);
}

static void draw_header(void) {
    gfx_fillrect(0, 0, GFX_WIDTH, 18, BIOS_BG);
    gfx_text(8, 5, "SIF BIOS Setup Utility", BIOS_TITLE, BIOS_BG);
    gfx_text(200, 5, SIF_KERNEL_VERSION, BIOS_DIM, BIOS_BG);
    bios_hline_double(0, 18, GFX_WIDTH);
}

static void draw_footer(void) {
    gfx_fillrect(0, GFX_HEIGHT - 28, GFX_WIDTH, 28, BIOS_BG);
    bios_hline_double(0, GFX_HEIGHT - 28, GFX_WIDTH);
    gfx_text(8, GFX_HEIGHT - 24, "↑↓ item  ←→ menu  Enter select  ESC quit",
             BIOS_DIM, BIOS_BG);
    gfx_text(8, GFX_HEIGHT - 14, "F10 Save & Exit (demo)   F9 Load Defaults",
             BIOS_DIM, BIOS_BG);
}

static void draw_menu_bar(void) {
    int x = 8;
    for (int i = 0; i < MENU_COUNT; i++) {
        int w = (int)strlen(menu_names[i]) * 8 + 12;
        if (i == menu_sel) {
            gfx_fillrect(x, 22, w, 12, BIOS_HI_BG);
            gfx_text(x + 4, 24, menu_names[i], BIOS_HI_FG, BIOS_HI_BG);
        } else {
            gfx_text(x + 4, 24, menu_names[i], BIOS_FG, BIOS_BG);
        }
        x += w + 4;
    }
}

static void fmt_u(char *out, uint32_t n) {
    char tmp[12];
    int i = 0;
    if (n == 0) {
        out[0] = '0';
        out[1] = 0;
        return;
    }
    while (n && i < 11) {
        tmp[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    int j = 0;
    while (i > 0)
        out[j++] = tmp[--i];
    out[j] = 0;
}

static void draw_item(int y, const char *label, const char *value, int selected) {
    uint8_t bg = selected ? BIOS_HI_BG : BIOS_BG;
    uint8_t fg = selected ? BIOS_HI_FG : BIOS_FG;
    gfx_fillrect(12, y, GFX_WIDTH - 24, 11, bg);
    gfx_text(16, y + 1, label, fg, bg);
    if (value)
        gfx_text(160, y + 1, value, selected ? BIOS_HI_FG : BIOS_CYAN, bg);
}

static void draw_main_panel(void) {
    bios_box(8, 38, GFX_WIDTH - 16, GFX_HEIGHT - 70);

    char buf[32];
    uint32_t free_f = pmm_free_frames();
    uint32_t used_f = pmm_used_frames();
    uint32_t tot_f = pmm_total_frames();
    uint32_t kb = tot_f * 4;

    const char *items_l[] = {
        "System Time",
        "System Date",
        "Kernel Version",
        "CPU Type",
        "Base Memory",
        "Total Frames",
        "Free Frames",
        "ATA Disk",
        "SFS Status",
    };
    char vals[9][28];
    uint32_t sec = timer_seconds();
    /* fake clock from uptime */
    uint32_t h = (sec / 3600) % 24;
    uint32_t m = (sec / 60) % 60;
    uint32_t s = sec % 60;
    vals[0][0] = (char)('0' + h / 10);
    vals[0][1] = (char)('0' + h % 10);
    vals[0][2] = ':';
    vals[0][3] = (char)('0' + m / 10);
    vals[0][4] = (char)('0' + m % 10);
    vals[0][5] = ':';
    vals[0][6] = (char)('0' + s / 10);
    vals[0][7] = (char)('0' + s % 10);
    vals[0][8] = 0;
    strcpy(vals[1], "2026-10-17");
    strncpy(vals[2], SIF_KERNEL_VERSION, 27);
    vals[2][27] = 0;
    strcpy(vals[3], "i386 (QEMU)");
    fmt_u(buf, kb);
    strcpy(vals[4], buf);
    strcat(vals[4], " KB");
    fmt_u(vals[5], tot_f);
    fmt_u(vals[6], free_f);
    (void)used_f;
    strcpy(vals[7], ata_present() ? "Present" : "Not Found");
    if (!ata_present())
        strcpy(vals[8], "N/A");
    else if (diskfs_mounted())
        strcpy(vals[8], "Mounted");
    else
        strcpy(vals[8], "Not Formatted");

    int n = 9;
    if (item_sel >= n)
        item_sel = n - 1;
    if (item_sel < 0)
        item_sel = 0;

    for (int i = 0; i < n; i++)
        draw_item(44 + i * 12, items_l[i], vals[i], i == item_sel);
}

static void draw_adv_panel(void) {
    bios_box(8, 38, GFX_WIDTH - 16, GFX_HEIGHT - 70);
    const char *items[] = {
        "Paging", "Scheduler", "Syscalls", "User Mode",
        "Async Exec", "VFS / Ramfs", "Disk SFS", "Graphics 13h",
    };
    const char *vals[] = {
        "Enabled", "Preemptive", "int 0x80", "Ring 3",
        "Spawn+Wait", "Enabled", "Enabled", "Enabled",
    };
    int n = 8;
    if (item_sel >= n)
        item_sel = n - 1;
    for (int i = 0; i < n; i++)
        draw_item(44 + i * 12, items[i], vals[i], i == item_sel);
}

static void draw_boot_panel(void) {
    bios_box(8, 38, GFX_WIDTH - 16, GFX_HEIGHT - 70);
    const char *items[] = {
        "1st Boot Device", "2nd Boot Device", "Boot Message",
        "Quiet Boot", "Shell on Boot",
    };
    const char *vals[] = {
        "HDD0", "Disabled", "SIF Kernel",
        "Disabled", "Enabled",
    };
    int n = 5;
    if (item_sel >= n)
        item_sel = n - 1;
    for (int i = 0; i < n; i++)
        draw_item(44 + i * 12, items[i], vals[i], i == item_sel);
    gfx_text(16, 120, "Boot order is demo only (no NVRAM)", BIOS_DIM, BIOS_BG);
}

static void draw_exit_panel(void) {
    bios_box(8, 38, GFX_WIDTH - 16, GFX_HEIGHT - 70);
    const char *items[] = {
        "Save & Exit Setup",
        "Exit Without Saving",
        "Load Setup Defaults",
        "Discard Changes",
    };
    int n = 4;
    if (item_sel >= n)
        item_sel = n - 1;
    for (int i = 0; i < n; i++)
        draw_item(48 + i * 14, items[i], 0, i == item_sel);
    gfx_text(16, 120, "Enter = confirm   ESC = cancel",
             BIOS_DIM, BIOS_BG);
}

static void draw_all(void) {
    gfx_clear(BIOS_BG);
    draw_header();
    draw_menu_bar();
    switch (menu_sel) {
    case MENU_MAIN: draw_main_panel(); break;
    case MENU_ADV:  draw_adv_panel(); break;
    case MENU_BOOT: draw_boot_panel(); break;
    case MENU_EXIT: draw_exit_panel(); break;
    default: break;
    }
    draw_footer();
}

void bios_run(void) {
    printk("[bios] setup utility - ESC to exit\n");
    menu_sel = 0;
    item_sel = 0;

    gfx_init_mode13();
    draw_all();

    for (;;) {
        if (!keyboard_has_input()) {
            /* refresh clock on Main roughly */
            __asm__ volatile("hlt");
            continue;
        }
        int ch = keyboard_read_char();
        if (!ch)
            continue;

        if (ch == 27) /* ESC */
            break;

        /* number keys jump menu */
        if (ch == '1') { menu_sel = MENU_MAIN; item_sel = 0; draw_all(); continue; }
        if (ch == '2') { menu_sel = MENU_ADV;  item_sel = 0; draw_all(); continue; }
        if (ch == '3') { menu_sel = MENU_BOOT; item_sel = 0; draw_all(); continue; }
        if (ch == '4') { menu_sel = MENU_EXIT; item_sel = 0; draw_all(); continue; }

        /* WASD / hjkl style + letters since no raw scancodes for arrows in shell map */
        if (ch == 'a' || ch == 'A') {
            menu_sel = (menu_sel + MENU_COUNT - 1) % MENU_COUNT;
            item_sel = 0;
            draw_all();
        } else if (ch == 'd' || ch == 'D') {
            menu_sel = (menu_sel + 1) % MENU_COUNT;
            item_sel = 0;
            draw_all();
        } else if (ch == 'w' || ch == 'W') {
            item_sel--;
            if (item_sel < 0)
                item_sel = 0;
            draw_all();
        } else if (ch == 's' || ch == 'S') {
            item_sel++;
            draw_all();
        } else if (ch == '\n' || ch == '\r') {
            if (menu_sel == MENU_EXIT) {
                if (item_sel == 0 || item_sel == 1)
                    break; /* save&exit or exit */
                if (item_sel == 2) {
                    item_sel = 0;
                    menu_sel = MENU_MAIN;
                    draw_all();
                }
            } else {
                /* toggle feel: just redraw */
                draw_all();
            }
        } else if (ch == 'q' || ch == 'Q') {
            break;
        } else {
            /* refresh main for live clock when any other key */
            if (menu_sel == MENU_MAIN)
                draw_all();
        }
    }

    gfx_restore_text();
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    printk("[bios] exited setup\n");
}
