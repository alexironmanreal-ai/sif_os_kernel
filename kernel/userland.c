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

static uint32_t g_ret_esp, g_ret_eip;
static uint32_t g_active_cr3;
static int g_active_pid = -1;

void userland_do_exit(void) {
    /* tear down user address space if we own one */
    if (g_active_cr3 && g_active_cr3 != paging_kernel_pd()) {
        uint32_t doomed = g_active_cr3;
        g_active_cr3 = 0;
        paging_switch(paging_kernel_pd());
        paging_destroy_address_space(doomed);
    }
    if (g_active_pid >= 0) {
        /* mark process zombie via exit path if still current */
        struct task *t = task_current();
        if (t && t->id == g_active_pid)
            task_exit_code(0);
        g_active_pid = -1;
    }

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
    printk("[user] addr-space per process + argv + clean exit\n");
}

static uint32_t setup_argv(uint32_t stack_top, int argc, char **argv) {
    uint32_t sp = stack_top;
    uint32_t str_ptrs[16];
    if (argc > 15)
        argc = 15;

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
    /* envp NULL, argv NULL terminator */
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

static void run_in_space(uint32_t cr3, uint32_t entry, uint32_t stack_top,
                         int argc, char **argv, const char *name) {
    static uint8_t kstack[TASK_KSTACK_SIZE] __attribute__((aligned(16)));
    tss_set_kernel_stack((uint32_t)&kstack[sizeof(kstack)]);

    /* register process slot for ps / wait / kill */
    int pid = task_create_user(name ? name : "user", cr3, entry, stack_top);
    g_active_pid = pid;
    g_active_cr3 = cr3;

    uint32_t user_esp = setup_argv(stack_top, argc, argv);
    printk("[user] pid=%d cr3=%x eip=%x esp=%x argc=%d\n", pid, cr3, entry,
           user_esp, argc);

    __asm__ volatile("mov %%esp, %0" : "=r"(g_ret_esp));
    g_ret_eip = (uint32_t)&&resume_user_exit;
    enter_usermode(entry, user_esp);

resume_user_exit:
    /* ensure we are back on kernel PD */
    if (paging_current_pd() != paging_kernel_pd())
        paging_switch(paging_kernel_pd());
    if (g_active_cr3 && g_active_cr3 != paging_kernel_pd()) {
        paging_destroy_address_space(g_active_cr3);
        g_active_cr3 = 0;
    }
    if (g_active_pid >= 0) {
        /* if SYS_EXIT already zombied via task path, fine; else mark dead */
        g_active_pid = -1;
    }
    printk("[user] volvio al kernel (addr-space liberado)\n");
}

void userland_run_test(void) {
    uint32_t cr3 = paging_create_address_space();
    if (!cr3) {
        printk("[user] no address space\n");
        return;
    }
    paging_switch(cr3);

    uint32_t cp = pmm_alloc_frame();
    if (!cp) {
        paging_switch(paging_kernel_pd());
        paging_destroy_address_space(cr3);
        printk("[user] sin frames\n");
        return;
    }
    uint32_t fl = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    paging_map(USER_CODE_V, cp, fl);
    memset((void *)USER_CODE_V, 0, PAGE_SIZE);
    build_user_image((uint8_t *)USER_CODE_V);

    if (map_user_stack() < 0) {
        paging_switch(paging_kernel_pd());
        paging_destroy_address_space(cr3);
        return;
    }

    char *av[] = { "ring3test", 0 };
    run_in_space(cr3, USER_CODE_V, USER_STACK_V, 1, av, "ring3test");
}

int userland_exec_buf(const void *data, uint32_t size, int argc, char **argv) {
    uint32_t cr3 = paging_create_address_space();
    if (!cr3) {
        printk("[exec] no address space\n");
        return -1;
    }
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
        run_in_space(cr3, entry, USER_STACK_V, 1, av, nm);
    } else {
        run_in_space(cr3, entry, USER_STACK_V, argc, argv, nm);
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
