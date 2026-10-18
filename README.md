# SIF Kernel v1.10.0-aqua

## GUI estilo Mac (Aqua)
- Barra de menu superior
- Ventana tipo Finder (semaforos rojo/amarillo/verde)
- Sidebar + contenido
- Dock inferior

```bash
git fetch origin && git reset --hard origin/main
make clean && make && make run
```

```
SIF> gui
```

| Tecla | Accion |
|-------|--------|
| **1** | Apps |
| **2** | Disk |
| **3** | About |
| **4** | BIOS setup |
| **5** / **ESC** | Volver a shell |
| **A/D** | Mover en el dock |
| **Enter** | Abrir item del dock |

Tambien: `bios` directo desde shell.
