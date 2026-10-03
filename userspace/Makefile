CC:=gcc
AS:=gcc
CFLAGS:=-m32 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -Wall -Wextra -O2 -Ilib
ASFLAGS:=-m32
LDFLAGS:=-m elf_i386 -T user.ld -nostdlib
OBJS:=lib/crt0.o lib/syscall.o lib/unistd.o lib/stdio.o hello.o
ELF:=bin/hello.elf
.PHONY: all clean
all: $(ELF)
$(ELF): $(OBJS) user.ld
	mkdir -p bin
	ld $(LDFLAGS) -o $@ $(OBJS)
	@echo Built $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(ELF)
