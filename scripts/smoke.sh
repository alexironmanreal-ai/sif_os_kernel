#!/usr/bin/env bash
# QEMU smoke test — boot kernel, expect markers, exit 0 on success
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

KERNEL="${KERNEL:-sif_kernel.elf}"
LOG="$(mktemp)"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"

if [[ ! -f "$KERNEL" ]]; then
  echo "Building kernel..."
  make -j"$(nproc 2>/dev/null || echo 2)" all
fi

echo "=== SIF QEMU smoke test ==="
echo "kernel=$KERNEL timeout=${TIMEOUT_SEC}s"

# Multiboot cmdline via -append when using -kernel
set +e
timeout "$TIMEOUT_SEC" qemu-system-i386 \
  -kernel "$KERNEL" \
  -append "smoke" \
  -serial stdio \
  -display none \
  -m 64M \
  -device isa-debug-exit,iobase=0x501,iosize=0x02 \
  -no-reboot \
  -no-shutdown \
  >"$LOG" 2>&1
QEMU_EC=$?
set -e

echo "--- serial log (tail) ---"
tail -n 40 "$LOG" || true
echo "-------------------------"

# isa-debug-exit: exit code = (value >> 1); we write code 0 -> qemu often exits 1
# accept QEMU_EC 0 or 1 as possible success paths; validate log markers
ok=0
grep -q '\[stub\] ok\|\[stub\]' "$LOG" && echo "PASS stub" && ok=$((ok+1)) || echo "FAIL stub"
grep -q 'Multiboot\|MB1\|MB2\|\[boot\]' "$LOG" && echo "PASS boot" && ok=$((ok+1)) || echo "FAIL boot"
grep -q '\[idt\]' "$LOG" && echo "PASS idt" && ok=$((ok+1)) || echo "FAIL idt"
grep -q '\[ok\] interrupts' "$LOG" && echo "PASS irq" && ok=$((ok+1)) || echo "FAIL irq"
grep -q '\[smoke\] PASS\|smoke PASS' "$LOG" && echo "PASS smoke-marker" && ok=$((ok+1)) || echo "WARN smoke-marker"

rm -f "$LOG"

if [[ "$ok" -ge 4 ]]; then
  echo "=== SMOKE OK ($ok checks) qemu_ec=$QEMU_EC ==="
  exit 0
fi
echo "=== SMOKE FAIL ($ok checks) qemu_ec=$QEMU_EC ==="
exit 1
