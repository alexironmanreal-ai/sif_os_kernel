# SIF Kernel — Makefile
# Target: i386 Multiboot ELF (booteable con GRUB / QEMU)

CC      := gcc
AS      := gcc
LD      := ld
CFLAGS  := -m32 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pic -Wall -Wextra -O2 -Iinclude
ASFLAGS := -m32
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

KERNEL  := sif_kernel.elf

OBJS := \
	boot/boot.o \
	arch/x86/gdt_flush.o \
	arch/x86/idt_load.o \
	arch/x86/gdt.o \
	arch/x86/idt.o \
	drivers/vga.o \
	lib/printk.o \
	kernel/main.o

.PHONY: all clean run iso

all: $(KERNEL)

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "Built $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(KERNEL) sif_kernel.iso
	rm -rf isodir

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -serial stdio -no-reboot -no-shutdown

iso: $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/sif_kernel.elf
	printf 'set timeout=0\nset default=0\nmenuentry "SIF Kernel" {\n  multiboot /boot/sif_kernel.elf\n  boot\n}\n' > isodir/boot/grub/grub.cfg
	grub-mkrescue -o sif_kernel.iso isodir
	@echo "ISO: sif_kernel.iso"
