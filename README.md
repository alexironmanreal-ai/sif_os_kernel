# SIF Kernel v1.1 — userland real

## Features
- Userspace build (`userspace/`) + minimal libc (`printf`, `write`, `exit`)
- Real ELF32 linked at `0x08000000`
- stdio FDs: **0=stdin 1=stdout 2=stderr**
- `exec hello.elf` loads embedded userspace program
- EXIT returns to shell

## Build
```bash
git pull origin main
make clean && make && make run
```
Needs: gcc-multilib, make, python3, qemu-system-i386

## Test
```
SIF> ls
SIF> exec hello.elf
SIF> user
```
