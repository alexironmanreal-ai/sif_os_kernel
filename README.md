# SIF Kernel v1.6

## Nuevo (v1.6.0-disk) — Filesystem en disco
- **ATA WRITE PIO** (lectura + escritura)
- **SFS** (SIF Simple File System): superblock + tabla de archivos + bloques de datos
- Comandos: `mkfs` `mount` `dls` `dcat` `dwrite` `drm` `sync`
- QEMU usa `disk.img` (se crea solo, 32 MB)

## Probar disco

```bash
git pull origin main
make clean && make && make run
```

```
SIF> mkfs
SIF> dwrite hola.txt hola desde el disco
SIF> dls
SIF> dcat hola.txt
SIF> sync
```

Reinicia QEMU (`make run` de nuevo) y:
```
SIF> mount
SIF> dls
SIF> dcat hola.txt
```

## Tambien
```
SIF> exec hello.elf
SIF> ls          # ramfs
SIF> dls         # disco
SIF> ps
```
