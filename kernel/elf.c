#include "elf.h"
#include "paging.h"
#include "pmm.h"
#include "printk.h"
#include "string.h"
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
static void map_range(uint32_t vaddr, uint32_t size, int writable) {
    uint32_t start = vaddr & ~0xFFFu;
    uint32_t end = (vaddr + size + PAGE_SIZE - 1) & ~0xFFFu;
    uint32_t flags = PAGE_PRESENT | PAGE_USER;
    if (writable) flags |= PAGE_WRITE;
    for (uint32_t va = start; va < end; va += PAGE_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame) { printk("[elf] OOM %x\n", va); return; }
        paging_map(va, frame, flags);
        memset((void *)va, 0, PAGE_SIZE);
    }
}
uint32_t elf_load(const void *data, uint32_t size) {
    if (size < sizeof(struct elf32_hdr)) return 0;
    const struct elf32_hdr *eh = (const struct elf32_hdr *)data;
    if (*(const uint32_t *)eh->e_ident != ELF_MAGIC) { printk("[elf] bad magic\n"); return 0; }
    if (eh->e_ident[4] != 1 || eh->e_machine != 3) { printk("[elf] not i386\n"); return 0; }
    const uint8_t *base = (const uint8_t *)data;
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const struct elf32_phdr *ph = (const struct elf32_phdr *)(base + eh->e_phoff + i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD) continue;
        if (ph->p_vaddr < 0x08000000u || ph->p_vaddr >= 0x09000000u) continue;
        map_range(ph->p_vaddr, ph->p_memsz, (ph->p_flags & 2) ? 1 : 0);
        if (ph->p_filesz) {
            if (ph->p_offset + ph->p_filesz > size) return 0;
            memcpy((void *)ph->p_vaddr, base + ph->p_offset, ph->p_filesz);
        }
        printk("[elf] LOAD %x\n", ph->p_vaddr);
    }
    printk("[elf] entry=%x\n", eh->e_entry);
    return eh->e_entry;
}
