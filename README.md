# SIF Kernel v1.3

## Nuevo
- **argv** en userspace (`main(argc, argv)`)
- **EXIT limpio** → vuelve a la shell (sin anidar)
- **getdents** → `ls.elf` lista el ramfs de verdad
- `exec FILE arg1 arg2...`

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
