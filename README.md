# SIF Kernel v1.2

## 7 features this round
1. Page fault handler (demand-map user + diagnostics)
2. Kernel pages without USER bit
3. wait / zombie + kill
4. Blocking sleep in scheduler
5. Syscalls: kill, wait, brk/sbrk
6. stdin in userland (fgets/getchar)
7. User programs: hello, echo, cat, ls

```bash
make clean && make && make run
```

```
SIF> exec hello.elf
SIF> exec echo.elf
SIF> exec cat.elf
SIF> exec ls.elf
SIF> kill 2
SIF> wait
```
