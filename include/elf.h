#ifndef SIF_ELF_H
#define SIF_ELF_H
#include <stdint.h>
#include <stddef.h>
#define ELF_MAGIC 0x464C457Fu
struct elf32_hdr {
    uint8_t e_ident[16];
    uint16_t e_type, e_machine;
    uint32_t e_version, e_entry, e_phoff, e_shoff, e_flags;
    uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} __attribute__((packed));
struct elf32_phdr {
    uint32_t p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
} __attribute__((packed));
#define PT_LOAD 1
uint32_t elf_load(const void *data, uint32_t size);
#endif
