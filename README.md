# SIF Kernel v1.11.0-bootgui

Arranca **directo al escritorio Aqua** (estilo Mac).

```bash
git fetch origin && git reset --hard origin/main
make clean && make && make run
```

## Al boot
1. Inicializa el kernel (texto breve en serial)
2. Entra al **escritorio grafico**

## Controles desktop
| Tecla | Accion |
|-------|--------|
| **1-3** | Apps / Disk / About |
| **4** | BIOS setup |
| **5** o **ESC** | Terminal (shell) |
| **A/D** | Dock |
| **Enter** | Abrir |

## Desde la shell
```
gui     # volver al escritorio
bios    # setup azul
exec hello.elf
```
