# SIF Kernel 2.0.0-ns

Kernel propio. No es un clon de Linux ni de Windows.

## Namespace de celdas
Rutas con **punto**: `apps.hello`  `docs.motd`

## Comandos SIF (consola)
| Comando | Que hace |
|---------|----------|
| **aid** | ayuda |
| **cells** [prefijo] | listar celdas |
| **glance** ruta | leer |
| **forge** ruta | crear vacia |
| **scribe** ruta texto | escribir |
| **erase** ruta | borrar |
| **spawn** ruta | lanzar ELF |
| **await** [id] | esperar proceso |
| **halt** id | matar proceso |
| **pulse** | procesos |
| **vault** | memoria |
| **desk** | escritorio |
| **setup** | pantalla setup |
| **tick** | uptime |
| **when** | fecha RTC |

```bash
git fetch origin && git reset --hard origin/main
make clean && make && make run
```

Ejemplos:
```
cells apps
glance docs.motd
spawn apps.hello
await
```
