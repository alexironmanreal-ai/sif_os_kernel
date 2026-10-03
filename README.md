# SIF Kernel v0.7

## 4 avances
1. **GDT ring3 + TSS** — base userspace
2. **PCI scan** — comando `pci`
3. **Syscalls** — sleep/time/open/read (DPL3)
4. **Shell** — sleep, hexdump, reboot, uptime, pci

```bash
git pull && make clean && make && make run
```

```
pci
sleep 500
hexdump readme.txt
uptime
reboot
```
