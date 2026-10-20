# SIF Kernel 2.1.0

Kernel propio. Namespace de celdas, desk con spawn, registro de dispositivos.

```bash
git fetch origin && git reset --hard origin/main
make clean && make && make run
```

## Desk
- Clic en iconos **hello / echo / cat / ls** → `spawn` real (async)
- Dock, sidebar, menu S

## Consola
```
aid
cells apps
spawn apps.hello
await
devs
link mi.app hello.elf
```

## Rutas SIF
`apps.hello`  `docs.motd`  (separador `.`)
