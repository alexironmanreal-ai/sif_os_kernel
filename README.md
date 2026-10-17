# SIF Kernel v1.7

## Nuevo — GUI minima
- VGA **mode 13h** (320×200×256)
- Escritorio, barra de tareas, ventana, paneles
- Comando: `gui`
- Teclas: `1` Apps · `2` Disk · `3` About · `ESC` volver a texto

```bash
git pull origin main
make clean && make && make run
```

```
SIF> gui
```

En la GUI:
- **1** — aplicaciones
- **2** — estado del disco/SFS
- **3** — about / version
- **ESC** — vuelve a la shell de texto

## Tambien
```
SIF> mkfs / dls / exec hello.elf / ps
```
