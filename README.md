# SIF Kernel v1.5

## Nuevo (v1.5.0-fd)
- **Tabla de FDs por proceso**: cada proceso user tiene sus propios 0/1/2 + opens
- **brk por proceso**: el heap (`sbrk`) ya no es global
- **mutex** cooperativo con cola de espera (`include/sync.h`)
- Al salir se cierran FDs y se libera el address space
- `ps` muestra tambien el BRK del proceso

## Anterior (v1.4)
- Address space por proceso, CR3 switch, getdents, argv, exec con args

```bash
git pull origin main
make clean && make && make run
```

```
SIF> exec hello.elf
SIF> exec cat.elf readme.txt
SIF> exec ls.elf
SIF> ps
```
