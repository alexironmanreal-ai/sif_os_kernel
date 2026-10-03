# SIF Kernel v0.9 — userspace ring3

## Nuevo
- `enter_usermode` (iret a CS=0x1B SS=0x23)
- paginas USER en 0x08000000
- programa de prueba con `int 0x80` (write, sleep, exit)
- comando shell: **`user`**

```bash
git pull origin main
make clean && make && make run
```

```
SIF> user
[ring3] hola desde userspace!
[ring3] volvi de sleep, exit
[sys] EXIT (userspace fin)
```
