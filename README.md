# SIF Kernel v1.8.0-async

## Exec asincrono
```
SIF> exec hello.elf          # lanza y vuelve al prompt
SIF> ps                      # ves el proceso user
SIF> wait                    # espera a que termine
SIF> run hello.elf           # spawn + wait (sincrono)
```

```bash
git fetch origin
git reset --hard origin/main
make clean && make && make run
```

Version al boot: **1.8.0-async**
