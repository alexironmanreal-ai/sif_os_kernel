#include "userland.h"
#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "gdt.h"
#include "string.h"
#include "elf.h"
#include "fs.h"
#include "task.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define USER_CODE_V  0x08000000u
#define USER_STACK_V 0x08010000u
#define USER_MSG_OFF 0x200

extern void enter_usermode(uint32_t eip, uint32_t esp);

/* legacy blocking return path */
static uint32_t g_ret_esp, g_ret_eip;
static uint32_t g_active_cr3;
static int g_active_pid = -1;
static int g_blocking;

void userland_do_exit(void) {
    struct task *t = task_current();
    if (t && t->is_user) {
        task_exit_code(0);
        return;
    }
    if (g_active_cr3 && g_active_cr3 != paging_kernel_pd()) {
        uint32_t doomed = g_active_cr3;
        g_active_cr3 = 0;
        paging_switch(paging_kernel_pd());
        paging_destroy_address_space(doomed);
    }
    g_active_pid = -1;
    if (!g_blocking)
        return;
    __asm__ volatile(
        "cli\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        "mov %0, %%esp\n\t"
        "sti\n\t"
        "jmp *%1\n\t"
        :
        : "r"(g_ret_esp), "r"(g_ret_eip)
        : "memory", "eax");
}

/* Kernel entry for user tasks: switch segments and iret to ring3 */
static void user_trampoline(void) {
    struct task *t = task_current();
    if (!t || !t->is_user || !t->user_eip) {
        printk("[user] trampoline invalid\n");
        task_exit_code(-1);
    }
    if (t->kstack_top)
        tss_set_kernel_stack(t->kstack_top);
    if (t->cr3)
        paging_switch(t->cr3);

    uint32_t eip = t->user_eip;
    uint32_t esp = t->user_esp;
    printk("[user] enter pid=%d eip=%x esp=%x\n", t->id, eip, esp);
    enter_usermode(eip, esp);
    /* never returns if user runs; if it does: */
    task_exit_code(-1);
}

static void build_user_image(uint8_t *code) {
    const char *msg = "[ring3] hola desde userspace!\n";
    const char *msg2 = "[ring3] sleep ok, bye\n";
    memcpy(code + USER_MSG_OFF, msg, 32);
    memcpy(code + USER_MSG_OFF + 32, msg2, 32);
    uint8_t *p = code;
    uint32_t m1 = USER_CODE_V + USER_MSG_OFF;
    uint32_t m2 = USER_CODE_V + USER_MSG_OFF + 32;
    *p++ = 0xB8;
    *(uint32_t *)p = 2;
    p += 4;
    *p++ = 0xBB;
    *(uint32_t *)p = m1;
    p += 4;
    *p++ = 0xCD;
    *p++ = 0x80;
    *p++ = 0xB8;
    *(uint32_t *)p = 5;
    p += 4;
    *p++ = 0xBB;
    *(uint32_t *)p = 300;
    p += 4;
    *p++ = 0xCD;
    *p++ = 0x80;
    *p++ = 0xB8;
    *(uint32_t *)p = 2;
    p += 4;
    *p++ = 0xBB;
    *(uint32_t *)p = m2;
    p += 4;
    *p++ = 0xCD;
    *p++ = 0x80;
    *p++ = 0xB8;
    *(uint32_t *)p = 1;
    p += 4;
    *p++ = 0xCD;
    *p++ = 0x80;
    *p++ = 0xF4;
    *p++ = 0xEB;
    *p++ = 0xFD;
}

void userland_init(void) {
    printk("[user] async spawn + trampoline + wait\n");
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
    *(uint32_t *)sp = 0; /* envp NULL */
    sp -= 4;
    *(uint32_t *)sp = 0; /* argv NULL */
    for (int i = argc - 1; i >= 0; i--) {
        sp -= 4;
        *(uint32_t *)sp = str_ptrs[i];
    }
    sp -= 4;
    *(uint32_t *)sp = (uint32_t)argc;
    return sp;
}

static int map_user_stack(void) {
    uint32_t fl = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    for (int i = 1; i <= 2; i++) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame)
            return -1;
        paging_map(USER_STACK_V - (uint32_t)i * PAGE_SIZE, frame, fl);
        memset((void *)(USER_STACK_V - (uint32_t)i * PAGE_SIZE), 0, PAGE_SIZE);
    }
    return 0;
}

/* ---- ASYNC SPAWN ---- */
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

    if (map_user_stack() < 0) {
        paging_switch(saved_pd);
        paging_destroy_address_space(cr3);
        return -1;
    }

    char *fallback_av[] = { (char *)(name ? name : "a.out"), 0 };
    if (!argv || argc < 1) {
        argv = fallback_av;
        argc = 1;
    }
    uint32_t user_esp = setup_argv(USER_STACK_V, argc, argv);

    /* back to caller address space before creating task */
    paging_switch(saved_pd);

    const char *nm = name ? name : (argv[0] ? argv[0] : "user");
    int pid = task_create_user(nm, cr3, entry, user_esp);
    if (pid < 0) {
        paging_destroy_address_space(cr3);
        printk("[spawn] task_create_user fail\n");
        return -1;
    }

    printk("[spawn] pid=%d '%s' cr3=%x eip=%x esp=%x\n", pid, nm, cr3, entry,
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
    printk("[spawn] load %s (%u bytes)\n", name, sz);
    return userland_spawn_buf(buf, sz, argc, argv, name);
}

int userland_spawn_file(const char *name) {
    char *av[] = { (char *)name, 0 };
    return userland_spawn_file_argv(name, 1, av);
}

/* ---- BLOCKING (legacy) ---- */
static void run_blocking(uint32_t cr3, uint32_t entry, uint32_t stack_top,
                         int argc, char **argv, const char *name) {
    static uint8_t kstack[TASK_KSTACK_SIZE] __attribute__((aligned(16)));
    tss_set_kernel_stack((uint32_t)&kstack[sizeof(kstack)]);

    g_blocking = 1;
    int pid = task_create_user(name ? name : "user", cr3, entry, stack_top);
    g_active_pid = pid;
    g_active_cr3 = cr3;

    uint32_t user_esp = setup_argv(stack_top, argc, argv);
    /* update task user_esp after argv build */
    if (pid >= 0) {
        /* re-setup was on current (user) PD; store final esp */
        extern struct task *task_current(void);
        (void)task_current;
    }
    printk("[user] blocking pid=%d eip=%x esp=%x\n", pid, entry, user_esp);

    __asm__ volatile("mov %%esp, %0" : "=r"(g_ret_esp));
    g_ret_eip = (uint32_t)&&resume_user_exit;
    enter_usermode(entry, user_esp);

resume_user_exit:
    g_blocking = 0;
    if (paging_current_pd() != paging_kernel_pd())
        paging_switch(paging_kernel_pd());
    if (g_active_cr3 && g_active_cr3 != paging_kernel_pd()) {
        paging_destroy_address_space(g_active_cr3);
        g_active_cr3 = 0;
    }
    g_active_pid = -1;
    printk("[user] blocking return\n");
}

void userland_run_test(void) {
    /* async ring3 test */
    uint32_t saved = paging_current_pd();
    uint32_t cr3 = paging_create_address_space();
    if (!cr3)
        return;
    paging_switch(cr3);
    uint32_t cp = pmm_alloc_frame();
    if (!cp) {
        paging_switch(saved);
        paging_destroy_address_space(cr3);
        return;
    }
    uint32_t fl = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    paging_map(USER_CODE_V, cp, fl);
    memset((void *)USER_CODE_V, 0, PAGE_SIZE);
    build_user_image((uint8_t *)USER_CODE_V);
    if (map_user_stack() < 0) {
        paging_switch(saved);
        paging_destroy_address_space(cr3);
        return;
    }
    char *av[] = { "ring3test", 0 };
    uint32_t usp = setup_argv(USER_STACK_V, 1, av);
    paging_switch(saved);
    int pid = task_create_user("ring3test", cr3, USER_CODE_V, usp);
    printk("[user] test spawned pid=%d\n", pid);
}

int userland_exec_buf(const void *data, uint32_t size, int argc, char **argv) {
    /* prefer async + wait if we have a current task */
    struct task *cur = task_current();
    if (cur && !cur->is_user) {
        int pid = userland_spawn_buf(data, size, argc, argv,
                                     argv && argv[0] ? argv[0] : "a.out");
        if (pid < 0)
            return -1;
        int st = 0;
        task_wait(pid, &st);
        printk("[exec] pid=%d exit=%d\n", pid, st);
        return st;
    }

    uint32_t cr3 = paging_create_address_space();
    if (!cr3)
        return -1;
    paging_switch(cr3);
    uint32_t entry = elf_load(data, size);
    if (!entry) {
        paging_switch(paging_kernel_pd());
        paging_destroy_address_space(cr3);
        return -1;
    }
    if (map_user_stack() < 0) {
        paging_switch(paging_kernel_pd());
        paging_destroy_address_space(cr3);
        return -1;
    }
    const char *nm = (argv && argv[0]) ? argv[0] : "a.out";
    if (!argv || argc < 1) {
        char *av[] = { "a.out", 0 };
        run_blocking(cr3, entry, USER_STACK_V, 1, av, nm);
    } else {
        run_blocking(cr3, entry, USER_STACK_V, argc, argv, nm);
    }
    return 0;
}

int userland_exec_file(const char *name) {
    char *av[] = { (char *)name, 0 };
    return userland_exec_file_argv(name, 1, av);
}

int userland_exec_file_argv(const char *name, int argc, char **argv) {
    static uint8_t buf[16384];
    uint32_t sz = 0;
    if (fs_read(name, buf, sizeof(buf), &sz) < 0) {
        printk("[exec] no existe %s\n", name);
        return -1;
    }
    printk("[exec] %s (%u) argc=%d\n", name, sz, argc);
    return userland_exec_buf(buf, sz, argc, argv);
}
