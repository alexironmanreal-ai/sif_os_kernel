# SIF Kernel v0.5

## Fix consola
La shell corre **directo** desde `kernel_main` (ya no depende del scheduler).
Deberias ver banner + `SIF>`.

## Nuevo
- ramfs (`ls`, `cat`)
- ATA PIO detect/read
- shell siempre visible

## Comandos
```
help ps mem ticks yield demo clear
echo hola
ls
cat readme.txt
disk
version
```

## Run
```bash
git pull && make clean && make && make run
```
