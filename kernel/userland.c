#include "userland.h"
#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "gdt.h"
#include "string.h"
#include "elf.h"
#include "fs.h"
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define USER_CODE_V 0x08000000u
#define USER_STACK_V 0x08010000u
#define USER_MSG_OFF 0x200
extern void enter_usermode(uint32_t eip, uint32_t esp);
static uint32_t g_ret_esp, g_ret_eip;
void userland_do_exit(void) {
    __asm__ volatile (
        "cli\n\tmov $0x10, %%ax\n\tmov %%ax, %%ds\n\tmov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\tmov %%ax, %%gs\n\tmov %%ax, %%ss\n\t"
        "mov %0, %%esp\n\tsti\n\tjmp *%1\n\t"
        : : "r"(g_ret_esp), "r"(g_ret_eip) : "memory", "eax");
}
static void build_user_image(uint8_t *code) {
    const char *msg = "[ring3] hola desde userspace!\n";
    const char *msg2 = "[ring3] sleep ok, bye\n";
    memcpy(code + USER_MSG_OFF, msg, 32);
    memcpy(code + USER_MSG_OFF + 32, msg2, 32);
    uint8_t *p = code;
    uint32_t m1 = USER_CODE_V + USER_MSG_OFF, m2 = USER_CODE_V + USER_MSG_OFF + 32;
    *p++=0xB8;*(uint32_t*)p=2;p+=4;*p++=0xBB;*(uint32_t*)p=m1;p+=4;*p++=0xCD;*p++=0x80;
    *p++=0xB8;*(uint32_t*)p=5;p+=4;*p++=0xBB;*(uint32_t*)p=300;p+=4;*p++=0xCD;*p++=0x80;
    *p++=0xB8;*(uint32_t*)p=2;p+=4;*p++=0xBB;*(uint32_t*)p=m2;p+=4;*p++=0xCD;*p++=0x80;
    *p++=0xB8;*(uint32_t*)p=1;p+=4;*p++=0xCD;*p++=0x80;*p++=0xF4;*p++=0xEB;*p++=0xFD;
}
void userland_init(void) { printk("[user] ring3 + argv + clean exit\n"); }
static uint32_t setup_argv(uint32_t stack_top, int argc, char **argv) {
    uint32_t sp = stack_top, str_ptrs[16];
    if (argc > 15) argc = 15;
    for (int i = argc - 1; i >= 0; i--) {
        const char *s = argv[i] ? argv[i] : "";
        uint32_t len = 0; while (s[len]) len++;
        sp -= (len + 1); sp &= ~3u;
        memcpy((void *)sp, s, len + 1);
        str_ptrs[i] = sp;
    }
    sp -= 4; *(uint32_t *)sp = 0;
    sp -= 4; *(uint32_t *)sp = 0;
    for (int i = argc - 1; i >= 0; i--) { sp -= 4; *(uint32_t *)sp = str_ptrs[i]; }
    sp -= 4; *(uint32_t *)sp = (uint32_t)argc;
    return sp;
}
static void run_at(uint32_t entry, uint32_t stack_top, int argc, char **argv) {
    static uint8_t kstack[8192] __attribute__((aligned(16)));
    tss_set_kernel_stack((uint32_t)&kstack[sizeof(kstack)]);
    uint32_t user_esp = setup_argv(stack_top, argc, argv);
    printk("[user] enter eip=%x esp=%x argc=%d\n", entry, user_esp, argc);
    __asm__ volatile ("mov %%esp, %0" : "=r"(g_ret_esp));
    g_ret_eip = (uint32_t)&&resume_user_exit;
    enter_usermode(entry, user_esp);
resume_user_exit:
    printk("[user] volvio al kernel\n");
}
void userland_run_test(void) {
    uint32_t cp = pmm_alloc_frame(), sp = pmm_alloc_frame();
    if (!cp || !sp) { printk("[user] sin frames\n"); return; }
    uint32_t fl = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    paging_map(USER_CODE_V, cp, fl);
    paging_map(USER_STACK_V - PAGE_SIZE, sp, fl);
    uint32_t sp2 = pmm_alloc_frame();
    if (sp2) paging_map(USER_STACK_V - 2 * PAGE_SIZE, sp2, fl);
    memset((void *)USER_CODE_V, 0, PAGE_SIZE);
    build_user_image((uint8_t *)USER_CODE_V);
    char *av[] = { "ring3test", 0 };
    run_at(USER_CODE_V, USER_STACK_V, 1, av);
}
int userland_exec_buf(const void *data, uint32_t size, int argc, char **argv) {
    uint32_t entry = elf_load(data, size);
    if (!entry) return -1;
    uint32_t stack_phys = pmm_alloc_frame();
    if (!stack_phys) return -1;
    uint32_t fl = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    paging_map(USER_STACK_V - PAGE_SIZE, stack_phys, fl);
    uint32_t sp2 = pmm_alloc_frame();
    if (sp2) paging_map(USER_STACK_V - 2 * PAGE_SIZE, sp2, fl);
    if (!argv || argc < 1) {
        char *av[] = { "a.out", 0 };
        run_at(entry, USER_STACK_V, 1, av);
    } else run_at(entry, USER_STACK_V, argc, argv);
    return 0;
}
int userland_exec_file(const char *name) {
    char *av[] = { (char *)name, 0 };
    return userland_exec_file_argv(name, 1, av);
}
int userland_exec_file_argv(const char *name, int argc, char **argv) {
    static uint8_t buf[16384]; uint32_t sz = 0;
    if (fs_read(name, buf, sizeof(buf), &sz) < 0) {
        printk("[exec] no existe %s\n", name); return -1;
    }
    printk("[exec] %s (%u) argc=%d\n", name, sz, argc);
    return userland_exec_buf(buf, sz, argc, argv);
}
