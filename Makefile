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
	arch/x86/gdt_flush.o arch/x86/idt_load.o arch/x86/isr_stub.o \
	arch/x86/switch.o arch/x86/syscall_stub.o \
	arch/x86/gdt.o arch/x86/idt.o arch/x86/pic.o arch/x86/irq.o \
	arch/x86/syscall.o \
	drivers/vga.o drivers/serial.o drivers/timer.o drivers/keyboard.o \
	drivers/ata.o \
	mm/pmm.o mm/paging.o mm/kmalloc.o \
	lib/printk.o lib/string.o \
	kernel/main.o kernel/task.o kernel/shell.o kernel/fs.o

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
	rm -f $(OBJS) $(KERNEL)

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -serial stdio -m 64M \
		-no-reboot -no-shutdown

# Optional: build a bootable ISO with grub (requires grub-mkrescue)
iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "SIF Kernel" { multiboot /boot/sif_kernel.elf }' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o sif.iso iso 2>/dev/null || echo "grub-mkrescue not found"
