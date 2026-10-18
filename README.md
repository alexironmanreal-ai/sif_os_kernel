# SIF Kernel v1.9.0-bios

## BIOS Setup
Pantalla estilo BIOS clasica (azul, menus Main/Advanced/Boot/Exit).

```bash
git fetch origin && git reset --hard origin/main
make clean && make && make run
```

```
SIF> bios
```

O desde el escritorio:
```
SIF> gui
```
luego tecla **4**.

### Controles BIOS
| Tecla | Accion |
|-------|--------|
| **A / D** | Menu izquierda/derecha |
| **W / S** | Item arriba/abajo |
| **1-4** | Main / Advanced / Boot / Exit |
| **Enter** | Seleccionar (en Exit sale) |
| **ESC / Q** | Salir |

## Tambien
```
exec hello.elf | run | wait | gui | mkfs
```
