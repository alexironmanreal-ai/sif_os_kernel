#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[1]
blob = (root / "userspace/bin/hello.elf").read_bytes()
lines = []
for i in range(0, len(blob), 12):
    chunk = blob[i:i+12]
    lines.append(",".join(f"0x{b:02x}" for b in chunk))
body = ",\n  ".join(lines)
out = root / "kernel/hello_elf_data.h"
out.write_text(f"/* auto-generated */\n#ifndef HELLO_ELF_DATA_H\n#define HELLO_ELF_DATA_H\n#include <stdint.h>\nstatic const uint8_t hello_elf_data[] = {{\n  {body}\n}};\nstatic const uint32_t hello_elf_len = {len(blob)};\n#endif\n")
print(f"embedded {len(blob)} bytes")
