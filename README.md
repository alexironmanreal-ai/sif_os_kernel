# SIF Kernel v0.3 — Fase 1+2

## Fix teclado
El stub IRQ leía mal el número de IRQ (offset 40 en vez de 36). Corregido: ahora IRQ1 (teclado) funciona.

## Fase 2
- PMM (bitmap de frames)
- Paging identity map 0-4MiB
- kmalloc (bump allocator)

## Build (WSL)
```bash
git pull
make clean && make && make run
```
