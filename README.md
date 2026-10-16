# SIF Kernel v1.4

## Nuevo (v1.4.0-proc)
- **Address space por proceso**: cada `exec` crea un page directory propio
- Kernel mappings compartidos, region user (0x08000000–0x09000000) privada
- Al salir se libera el address space (frames + page tables)
- Tasks con `cr3`, `is_user`, `parent`, `kstack` — base para procesos reales
- `ps` muestra CR3 y si es user/kernel

## Anterior
- SYS_GETDENTS + `ls.elf`
- argv en userspace, exit limpio, `exec FILE args...`

```bash
git pull origin main
make clean && make && make run
```

```
SIF> exec hello.elf
SIF> exec echo.elf hola mundo
SIF> exec ls.elf
SIF> ps
SIF> user
```
