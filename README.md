# SIF Kernel 2.2.0-wire

Kernel propio con **wire** (red NE2000).

```bash
git fetch origin && git reset --hard origin/main
make clean && make && make run
```

QEMU incluye `-netdev user -device ne2k_pci`.

## Red (comandos SIF)
```
wire              # estado + MAC
cast hola sif     # broadcast frame ethertype 0x88B5
sniff             # recibir un frame
```

## Otras
```
cells apps
spawn apps.hello
desk
aid
```
