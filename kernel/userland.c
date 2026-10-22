#include "userland.h"
#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "gdt.h"
#include "string.h"
#include "elf.h"
#include "fs.h"
#include "task.h"
#include "security.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define USER_CODE_V 0x08000000u

extern void enter_usermode(uint32_t eip, uint32_t esp);

void userland_do_exit(void) {
    struct task *t = task_current();
    if (t && t->is_user) {
        task_exit_code(0);
        return;
    }
}

void userland_init(void) {
    printk("[user] ASLR + stack guard + isolation\n");
}

static uint32_t setup_argv(uint32_t stack_top, int argc, char **argv) {
    uint32_t sp = stack_top;
    uint32_t str_ptrs[16];
    if (argc > 15)
        argc = 15;
    if (argc < 0)
        argc = 0;

    for (int i = argc - 1; i >= 0; i--) {
        const char *s = argv[i] ? argv[i] : "";
        uint32_t len = 0;
        while (s[len])
            len++;
        sp -= (len + 1);
        sp &= ~3u;
        memcpy((void *)sp, s, len + 1);
        str_ptrs[i] = sp;
    }
    sp -= 4;
    *(uint32_t *)sp = 0;
    sp -= 4;
    *(uint32_t *)sp = 0;
    for (int i = argc - 1; i >= 0; i--) {
        sp -= 4;
        *(uint32_t *)sp = str_ptrs[i];
    }
    sp -= 4;
    *(uint32_t *)sp = (uint32_t)argc;
    return sp;
}

/*
 * Map 2 stack pages + leave 1 unmapped guard page below.
 * Layout: [guard unmapped][stack page][stack page][stack_top]
 */
static int map_user_stack_at(uint32_t stack_top) {
    uint32_t fl = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    for (int i = 1; i <= 2; i++) {
        uint32_t va = stack_top - (uint32_t)i * PAGE_SIZE;
        if (!security_user_range_ok(va, PAGE_SIZE))
            return -1;
        uint32_t frame = pmm_alloc_frame();
        if (!frame)
            return -1;
        paging_map(va, frame, fl);
        memset((void *)va, 0, PAGE_SIZE);
    }
    /* guard: page at stack_top - 3*PAGE intentionally not mapped */
    return 0;
}

int userland_spawn_buf(const void *data, uint32_t size, int argc, char **argv,
                       const char *name) {
    uint32_t saved_pd = paging_current_pd();
    uint32_t cr3 = paging_create_address_space();
    if (!cr3) {
        printk("[spawn] no address space\n");
        return -1;
    }

    paging_switch(cr3);

    uint32_t entry = elf_load(data, size);
    if (!entry) {
        paging_switch(saved_pd);
        paging_destroy_address_space(cr3);
        printk("[spawn] elf_load fail\n");
        return -1;
    }

    uint32_t stack_top = aslr_stack_top();
    if (map_user_stack_at(stack_top) < 0) {
        paging_switch(saved_pd);
        paging_destroy_address_space(cr3);
        return -1;
    }

    char *fallback_av[] = {(char *)(name ? name : "a.out"), 0};
    if (!argv || argc < 1) {
        argv = fallback_av;
        argc = 1;
    }
    uint32_t user_esp = setup_argv(stack_top, argc, argv);

    paging_switch(saved_pd);

    const char *nm = name ? name : (argv[0] ? argv[0] : "user");
    int pid = task_create_user(nm, cr3, entry, user_esp);
    if (pid < 0) {
        paging_destroy_address_space(cr3);
        return -1;
    }

    printk("[spawn] pid=%d '%s' eip=%x esp=%x guard=on\n", pid, nm, entry,
           user_esp);
    return pid;
}

int userland_spawn_file_argv(const char *name, int argc, char **argv) {
    static uint8_t buf[16384];
    uint32_t sz = 0;
    if (fs_read(name, buf, sizeof(buf), &sz) < 0) {
        printk("[spawn] no existe %s\n", name);
        return -1;
    }
    return userland_spawn_buf(buf, sz, argc, argv, name);
}

int userland_spawn_file(const char *name) {
    char *av[] = {(char *)name, 0};
    return userland_spawn_file_argv(name, 1, av);
}

void userland_run_test(void) { (void)0; }

int userland_exec_buf(const void *data, uint32_t size, int argc, char **argv) {
    int pid = userland_spawn_buf(data, size, argc, argv,
                                 argv && argv[0] ? argv[0] : "a.out");
    if (pid < 0)
        return -1;
    int st = 0;
    task_wait(pid, &st);
    return st;
}

int userland_exec_file(const char *name) {
    char *av[] = {(char *)name, 0};
    return userland_exec_file_argv(name, 1, av);
}

int userland_exec_file_argv(const char *name, int argc, char **argv) {
    static uint8_t buf[16384];
    uint32_t sz = 0;
    if (fs_read(name, buf, sizeof(buf), &sz) < 0)
        return -1;
    return userland_exec_buf(buf, sz, argc, argv);
}
