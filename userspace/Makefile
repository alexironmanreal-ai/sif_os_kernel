CC:=gcc
AS:=gcc
CFLAGS:=-m32 -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -Wall -Wextra -O2 -Ilib
ASFLAGS:=-m32
LDFLAGS:=-m elf_i386 -T user.ld -nostdlib
LIB:=lib/crt0.o lib/syscall.o lib/unistd.o lib/stdio.o
.PHONY: all clean
all: bin/hello.elf bin/echo.elf bin/cat.elf bin/ls.elf
bin/%.elf: %.o $(LIB) user.ld
	mkdir -p bin
	ld $(LDFLAGS) -o $@ $(LIB) $<
	@echo Built $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@
lib/%.o: lib/%.S
	$(AS) $(ASFLAGS) -c $< -o $@
clean:
	rm -f lib/*.o *.o bin/*.elf
