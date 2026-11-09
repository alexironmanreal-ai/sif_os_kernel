# SIF Kernel — Estado real y tiempos (honesto)

## Qué es esto (sin marketing)

SIF es un **kernel de investigación / aprendizaje** en x86-32. Arranca, tiene
consola, procesos user, FS, red toy y algo de seguridad. **No** es un sistema
operativo de escritorio ni de servidor listo para producción.

## Estadísticas de tiempo (desarrollo de este repo)

| Métrica | Valor realista |
|---------|----------------|
| Ventana de calendario (sesiones intensivas en el historial del proyecto) | ~20 días (oct–nov 2026) |
| Intensidad | A rachas: muchas sesiones cortas de feature + fix, no 8h/día fijas |
| Estimación de esfuerzo humano equivalente | **~150–250 horas** de ingeniería enfocada (orden de magnitud) |
| Comparación: hobby kernel usable (boot+shell+ELF) | Típico: semanas a pocos meses a tiempo parcial |
| Comparación: OS “completo” comercial/desktop | Años; el mito de 7–10 años aplica a productos con ecosistema, no a un kernel de lab |

**Por qué no “ya casi terminamos un kernel de 10 años”:**  
Un kernel *usable para estudiar y experimentar* ≠ un kernel *completo* (drivers
estables, FS robusto, red production-grade, multi-CPU, security auditada,
herramientas, ABI estable, apps). Linux lleva **>30 años** y miles de
contribuidores. SIF está en la fase “lab sólido”, no en “reemplazo de sistema”.

## Completitud aproximada (subjetiva, por área)

| Área | % orientativo | Notas |
|------|---------------|--------|
| Boot / CPU básico | 70% | Multiboot, GDT/IDT/PIC OK; 64-bit path incompleto |
| Memoria (PMM/paging/heap) | 55% | HH + spaces; falta PAE/NX, KASLR |
| Procesos / scheduler | 50% | User ELF, wait/kill; preemption por quantum **corregida en 3.1** |
| Syscalls / userland | 40% | Set mínimo; sin libc rica ni muchas apps |
| Filesystems | 45% | Ramfs + SFS disco; VFS unificado básico |
| Red | 35% | Stack toy (ARP…HTTP); no production |
| Seguridad | 40% | Canary, ASLR parcial, SMEP/SMAP; sin NX en PTE |
| Drivers | 30% | ATA, NE2K, kbd, serial, RTC, PCI enum |
| GUI | 5% | Pausada a propósito |
| **Global “kernel de lab usable”** | **~40–45%** | |
| **Global “OS completo”** | **~5–10%** | |

## Hecho vs crítico pendiente

### Hecho (base)
- Boot, interrupciones, consola serial/VGA
- Paging higher-half, tasks, ELF user, syscalls
- Ramfs + SFS, VFS unificado (3.1)
- Red mínima, auditoría de seguridad parcial
- Preemption por timer conectada al scheduler (3.1)

### Pendiente importante (orden recomendado)
1. **PAE + NX** en page tables
2. **ELF PIE** / ASLR de código
3. **VFS** más limpio + permisos + directorios
4. **TCP** usable (estado, retransmit básico)
5. **Más syscalls** (stat, mmap, pipe)
6. **Estabilidad** (fuzz, stress, no panics)
7. Drivers virtio / mejor disco

## Cómo medir progreso de aquí en adelante

No por “versión se ve alta”, sino por:
- tests automáticos (`make smoke`) verdes siempre
- un proceso user que sobrevive N segundos bajo load
- FS que sobrevive reboot con datos
- red que hace ping estable 100/100
- zero page-faults de kernel en uso normal

---
*Documento de realidad, no de hype. Actualizar con cada release serio.*
