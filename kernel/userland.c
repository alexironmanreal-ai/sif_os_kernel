#include "userland.h"
#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "gdt.h"
#include "string.h"
#include "elf.h"
#include "fs.h"
#include "shell.h"
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define USER_CODE_V 0x08000000u
#define USER_STACK_V 0x08008000u
#define USER_MSG_OFF 0x200
extern void enter_usermode(uint32_t eip, uint32_t esp);
void userland_do_exit(void) {
    __asm__ volatile ("cli\n\tmov $0x10, %ax\n\tmov %ax, %ds\n\tmov %ax, %es\n\tmov %ax, %fs\n\tmov %ax, %gs\n\tmov %ax, %ss\n\t");
    static uint8_t kstack[8192] __attribute__((aligned(16)));
    __asm__ volatile ("mov %0, %%esp" : : "r"((uint32_t)&kstack[sizeof(kstack)]) : "memory");
    tss_set_kernel_stack((uint32_t)&kstack[sizeof(kstack)]);
    printk("[user] EXIT → shell\n");
    __asm__ volatile ("sti");
    shell_run();
    for (;;) __asm__ volatile ("hlt");
}
static void build_user_image(uint8_t *code) {
    const char *msg = "[ring3] hola desde userspace!\n";
    const char *msg2 = "[ring3] sleep ok, bye\n";
    memcpy(code + USER_MSG_OFF, msg, 32);
    memcpy(code + USER_MSG_OFF + 32, msg2, 32);
    uint8_t *p = code;
    uint32_t m1 = USER_CODE_V + USER_MSG_OFF, m2 = USER_CODE_V + USER_MSG_OFF + 32;
    *p++=0xB8; *(uint32_t*)p=2; p+=4; *p++=0xBB; *(uint32_t*)p=m1; p+=4; *p++=0xCD; *p++=0x80;
    *p++=0xB8; *(uint32_t*)p=5; p+=4; *p++=0xBB; *(uint32_t*)p=300; p+=4; *p++=0xCD; *p++=0x80;
    *p++=0xB8; *(uint32_t*)p=2; p+=4; *p++=0xBB; *(uint32_t*)p=m2; p+=4; *p++=0xCD; *p++=0x80;
    *p++=0xB8; *(uint32_t*)p=1; p+=4; *p++=0xCD; *p++=0x80; *p++=0xF4; *p++=0xEB; *p++=0xFD;
}
void userland_init(void) { printk("[user] ring3 + ELF + exit→shell\n"); }
static void run_at(uint32_t entry, uint32_t stack_top) {
    static uint8_t kstack[8192] __attribute__((aligned(16)));
    tss_set_kernel_stack((uint32_t)&kstack[sizeof(kstack)]);
    printk("[user] enter eip=%x esp=%x\n", entry, stack_top);
    enter_usermode(entry, stack_top);
}
void userland_run_test(void) {
    uint32_t cp = pmm_alloc_frame(), sp = pmm_alloc_frame();
    if (!cp || !sp) { printk("[user] sin frames\n"); return; }
    uint32_t fl = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    paging_map(USER_CODE_V, cp, fl);
    paging_map(USER_STACK_V - PAGE_SIZE, sp, fl);
    memset((void *)USER_CODE_V, 0, PAGE_SIZE);
    build_user_image((uint8_t *)USER_CODE_V);
    run_at(USER_CODE_V, USER_STACK_V);
}
int userland_exec_buf(const void *data, uint32_t size) {
    uint32_t entry = elf_load(data, size);
    if (!entry) return -1;
    uint32_t sp = pmm_alloc_frame();
    if (!sp) return -1;
    paging_map(USER_STACK_V - PAGE_SIZE, sp, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    run_at(entry, USER_STACK_V);
    return 0;
}
int userland_exec_file(const char *name) {
    static uint8_t buf[8192]; uint32_t sz = 0;
    if (fs_read(name, buf, sizeof(buf), &sz) < 0) { printk("[exec] no %s\n", name); return -1; }
    printk("[exec] %s (%u)\n", name, sz);
    return userland_exec_buf(buf, sz);
}
