# Seguridad SIF Kernel v1.14

## Implementado
- **uaccess**: `access_ok`, `copy_from_user`, `copy_to_user`
- User solo puede tocar `[USER_BASE, USER_TOP)` (0x08000000–0x09000000)
- Syscalls copian buffers a kernel antes de usarlos
- Paths: sin `/`, sin `..`, sin control chars, longitud limitada
- I/O max 4 KiB por syscall
- `sleep` max 60s
- `kill` no puede matar pid <= 0 (idle)
- FD tables por proceso; stdin/out/err no se cierran
- Syscalls desconocidas → -1

## No es (aun)
- ASLR, SMEP/SMAP, stack canaries completos
- Usuarios/permisos multiuser
- Cifrado de disco
- Seccomp-like filtros finos

Hobby OS: frontera ring3 + validacion de punteros, no fortaleza militar.
