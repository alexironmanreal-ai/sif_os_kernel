# SIF Kernel v1.7.1

## GUI (arreglado)
```bash
git pull origin main
make clean && make && make run
```

```
SIF> gui
```

Deberias ver escritorio azul 320x200. Si la ventana de QEMU queda negra, mira el **serial** (terminal):
```
[gfx] mode13 begin
[gfx] mode13 ok
[gui] desktop drawn
```

Teclas: `1` `2` `3` paneles · `ESC` o `q` salir

## Nota QEMU
Usa la ventana grafica de QEMU (no solo -nographic). El Makefile ya abre VGA normal.
