# SIF Kernel v1.3

## Nuevo
- **SYS_GETDENTS** implementado (ls.elf lista el ramfs de verdad)
- **argv** en userspace (`main(argc, argv)`)
- **EXIT limpio** → vuelve a la shell (sin anidar)
- `exec FILE arg1 arg2...` pasa argv completo
- Embed de todos los ELFs (hello/echo/cat/ls) en el ramfs

```bash
git pull origin main
make clean && make && make run
```

```
SIF> exec hello.elf
SIF> exec echo.elf hola mundo
SIF> exec cat.elf readme.txt
SIF> exec ls.elf
```
