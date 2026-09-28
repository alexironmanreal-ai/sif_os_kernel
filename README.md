# SIF Kernel v0.4 — Tasks + Shell

## Nuevo
- Multitasking cooperativo (context switch)
- Scheduler round-robin + quantum por timer
- Syscalls `int 0x80` (exit/write/yield/getpid)
- Shell interactiva

## Comandos shell
```
help  ps  mem  ticks  yield  demo  clear  echo hola
```

`demo` crea 2 workers que se alternan con yield.

## Build
```bash
git pull && make clean && make && make run
```
