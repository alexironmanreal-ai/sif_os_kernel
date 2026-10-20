CC:=gcc
AS:=gcc
LD:=ld
CFLAGS:=-m32 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -Wall -Wextra -O2 -Iinclude
ASFLAGS:=-m32
LDFLAGS:=-m elf_i386 -T linker.ld -nostdlib
KERNEL:=sif_kernel.elf
DISK:=disk.img
OBJS:=boot/boot.o boot/stub.o arch/x86/gdt_flush.o arch/x86/idt_load.o arch/x86/isr_stub.o arch/x86/isr_ex_stub.o arch/x86/isr_spurious.o arch/x86/switch.o arch/x86/syscall_stub.o arch/x86/enter_user.o arch/x86/longmode.o arch/x86/entry64.o arch/x86/cpu.o arch/x86/gdt.o arch/x86/idt.o arch/x86/pic.o arch/x86/irq.o arch/x86/isr.o arch/x86/syscall.o drivers/vga.o drivers/gfx.o drivers/serial.o drivers/timer.o drivers/keyboard.o drivers/mouse.o drivers/ata.o drivers/pci.o drivers/rtc.o drivers/ne2k.o mm/pmm.o mm/paging.o mm/kmalloc.o lib/printk.o lib/string.o kernel/main.o kernel/multiboot.o kernel/smoke.o kernel/task.o kernel/cmd.o kernel/shell.o kernel/fs.o kernel/ns.o kernel/dev.o kernel/wire.o kernel/diskfs.o kernel/gui.o kernel/bios.o kernel/userland.o kernel/elf.o kernel/vfs.o kernel/sync.o kernel/uaccess.o net/net.o net/arp.o net/ipv4.o net/icmp.o
.PHONY: all clean run run64 iso smoke userspace embed disk
all: userspace embed $(KERNEL)
userspace:
	$(MAKE) -C userspace
embed: userspace/bin/hello.elf userspace/bin/echo.elf userspace/bin/cat.elf userspace/bin/ls.elf
	python3 scripts/embed_hello.py
$(KERNEL): $(OBJS) linker.ld include/user_elfs.h
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo Built $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
kernel/fs.o: include/user_elfs.h
disk:
	@test -f $(DISK) || (dd if=/dev/zero of=$(DISK) bs=1M count=32 status=none && echo "created $(DISK)")
clean:
	$(MAKE) -C userspace clean
	rm -f $(OBJS) $(KERNEL) sif.iso
	rm -rf iso_tmp
run: $(KERNEL) disk
	qemu-system-i386 -kernel $(KERNEL) -serial stdio -m 64M \
		-drive file=$(DISK),format=raw,if=ide \
		-netdev user,id=n0 -device ne2k_pci,netdev=n0 \
		-no-reboot -no-shutdown
run64: $(KERNEL) disk
	qemu-system-x86_64 -kernel $(KERNEL) -serial stdio -m 64M \
		-drive file=$(DISK),format=raw,if=ide \
		-netdev user,id=n0 -device ne2k_pci,netdev=n0 \
		-no-reboot -no-shutdown
smoke: $(KERNEL)
	chmod +x scripts/smoke.sh
	./scripts/smoke.sh
iso: $(KERNEL)
	mkdir -p iso_tmp/boot/grub
	cp $(KERNEL) iso_tmp/boot/sif_kernel.elf
	printf 'set timeout=3\nset default=0\n\nmenuentry "SIF Kernel" {\n  multiboot /boot/sif_kernel.elf\n  boot\n}\n' > iso_tmp/boot/grub/grub.cfg
	grub-mkrescue -o sif.iso iso_tmp 2>/dev/null || echo "need grub-mkrescue"
