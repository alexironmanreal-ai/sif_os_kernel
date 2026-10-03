# SIF Kernel v0.8 — sistema de comandos

## Novedad
Tabla de comandos con `cmd_register()` + `argc/argv`.
Agregar un comando = 1 funcion + 1 linea de registro.

## Build
```bash
git pull && make clean && make && make run
```

## Comandos
```
help version uname whoami info clear echo
ps mem free ticks uptime date yield demo
ls cat touch write rm hexdump history
disk pci sleep reboot panic true false
```
