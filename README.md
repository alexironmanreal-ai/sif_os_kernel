# SIF Kernel v1.0

## Hoy se avanzó
- EXIT userspace → vuelve a shell
- Loader ELF32 i386
- VFS + file descriptors (open/read/write/close)
- Syscalls con retorno en EAX
- `exec hello.elf`, `open`, `fdread`
- Target `make iso`

```bash
git pull origin main
make clean && make && make run
```

```
SIF> user
SIF> ls
SIF> exec hello.elf
SIF> open readme.txt
SIF> fdread 0
```
