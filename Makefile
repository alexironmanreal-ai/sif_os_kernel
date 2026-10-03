CC:=gcc
AS:=gcc
LD:=ld
CFLAGS:=-m32 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -Wall -Wextra -O2 -Iinclude
ASFLAGS:=-m32
LDFLAGS:=-m elf_i386 -T linker.ld -nostdlib
KERNEL:=sif_kernel.elf
OBJS:=boot/boot.o arch/x86/gdt_flush.o arch/x86/idt_load.o arch/x86/isr_stub.o arch/x86/isr_ex_stub.o arch/x86/switch.o arch/x86/syscall_stub.o arch/x86/enter_user.o arch/x86/gdt.o arch/x86/idt.o arch/x86/pic.o arch/x86/irq.o arch/x86/isr.o arch/x86/syscall.o drivers/vga.o drivers/serial.o drivers/timer.o drivers/keyboard.o drivers/ata.o drivers/pci.o drivers/rtc.o mm/pmm.o mm/paging.o mm/kmalloc.o lib/printk.o lib/string.o kernel/main.o kernel/task.o kernel/cmd.o kernel/shell.o kernel/fs.o kernel/userland.o kernel/elf.o kernel/vfs.o
.PHONY: all clean run iso userspace embed
all: userspace embed $(KERNEL)
userspace:
	$(MAKE) -C userspace
embed: userspace/bin/hello.elf userspace/bin/echo.elf userspace/bin/cat.elf userspace/bin/ls.elf
	python3 scripts/embed_hello.py
$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo Built $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
clean:
	$(MAKE) -C userspace clean
	rm -f $(OBJS) $(KERNEL) sif.iso
	rm -rf iso_tmp
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -serial stdio -m 64M -no-reboot -no-shutdown
iso: $(KERNEL)
	mkdir -p iso_tmp/boot/grub
	cp $(KERNEL) iso_tmp/boot/sif_kernel.elf
	printf 'set timeout=0\nset default=0\nmenuentry "SIF Kernel" {\n  multiboot /boot/sif_kernel.elf\n  boot\n}\n' > iso_tmp/boot/grub/grub.cfg
	grub-mkrescue -o sif.iso iso_tmp 2>/dev/null || echo "need grub-mkrescue"
