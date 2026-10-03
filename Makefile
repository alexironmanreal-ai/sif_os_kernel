CC := gcc
AS := gcc
LD := ld
CFLAGS := -m32 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -Wall -Wextra -O2 -Iinclude
ASFLAGS := -m32
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib
KERNEL := sif_kernel.elf
OBJS := boot/boot.o arch/x86/gdt_flush.o arch/x86/idt_load.o arch/x86/isr_stub.o arch/x86/isr_ex_stub.o arch/x86/switch.o arch/x86/syscall_stub.o arch/x86/gdt.o arch/x86/idt.o arch/x86/pic.o arch/x86/irq.o arch/x86/isr.o arch/x86/syscall.o drivers/vga.o drivers/serial.o drivers/timer.o drivers/keyboard.o drivers/ata.o drivers/pci.o drivers/rtc.o mm/pmm.o mm/paging.o mm/kmalloc.o lib/printk.o kernel/main.o kernel/task.o kernel/shell.o kernel/fs.o
.PHONY: all clean run
all: $(KERNEL)
$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo Built $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(KERNEL)
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -serial stdio -m 64M -no-reboot -no-shutdown
