#include "userland.h"
#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "gdt.h"
#include "string.h"
#define USER_CODE_V  0x08000000u
#define USER_STACK_V 0x08008000u
#define USER_MSG_OFF 0x200
extern void enter_usermode(uint32_t eip, uint32_t esp);
static void build_user_image(uint8_t *code) {
    const char *msg = "[ring3] hola desde userspace!\n";
    const char *msg2 = "[ring3] volvi de sleep, exit\n";
    memcpy(code + USER_MSG_OFF, msg, 32);
    memcpy(code + USER_MSG_OFF + 32, msg2, 32);
    uint8_t *p = code;
    uint32_t msg_addr = USER_CODE_V + USER_MSG_OFF;
    uint32_t msg2_addr = USER_CODE_V + USER_MSG_OFF + 32;
    *p++ = 0xB8; *(uint32_t *)p = 2; p += 4;
    *p++ = 0xBB; *(uint32_t *)p = msg_addr; p += 4;
    *p++ = 0xCD; *p++ = 0x80;
    *p++ = 0xB8; *(uint32_t *)p = 5; p += 4;
    *p++ = 0xBB; *(uint32_t *)p = 500; p += 4;
    *p++ = 0xCD; *p++ = 0x80;
    *p++ = 0xB8; *(uint32_t *)p = 2; p += 4;
    *p++ = 0xBB; *(uint32_t *)p = msg2_addr; p += 4;
    *p++ = 0xCD; *p++ = 0x80;
    *p++ = 0xB8; *(uint32_t *)p = 1; p += 4;
    *p++ = 0xCD; *p++ = 0x80;
    *p++ = 0xF4; *p++ = 0xEB; *p++ = 0xFD;
}
void userland_init(void) { printk("[user] ring3 listo (cmd: user)\n"); }
void userland_run_test(void) {
    uint32_t code_phys = pmm_alloc_frame();
    uint32_t stack_phys = pmm_alloc_frame();
    if (!code_phys || !stack_phys) { printk("[user] sin frames\n"); return; }
    uint32_t flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    paging_map(USER_CODE_V, code_phys, flags);
    paging_map(USER_STACK_V - PAGE_SIZE, stack_phys, flags);
    memset((void *)USER_CODE_V, 0, PAGE_SIZE);
    build_user_image((uint8_t *)USER_CODE_V);
    static uint8_t kstack[8192] __attribute__((aligned(16)));
    tss_set_kernel_stack((uint32_t)&kstack[8192]);
    printk("[user] entrando ring3 eip=%x esp=%x\n", USER_CODE_V, USER_STACK_V);
    enter_usermode(USER_CODE_V, USER_STACK_V);
    printk("[user] volvio?\n");
}
