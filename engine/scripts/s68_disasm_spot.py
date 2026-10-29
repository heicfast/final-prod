#!/usr/bin/env python3
"""S68: raw x64 disassembly spotter for the og RAWSearchDLL tier-1 checks that
Ghidra mis-decompiled (functions where the buffer param rides rdx but Ghidra
recovered a wrong signature). Prints the compare constants embedded in the
function body. Usage: python3 s68_disasm_spot.py <func_rva_hex> [len]"""
import sys, struct

DLL = "/home/z/my-project/audit/easeus-drw-research/og-codebase/RAWSearchDLL.dll"
data = open(DLL, "rb").read()
sections = [(".text", 0x1000, 0x640d4, 0x400, 0x64200),
            (".rdata", 0x66000, 0x28662, 0x64600, 0x28800)]

def rva2off(rva):
    for n, v, vs, ro, rs in sections:
        if v <= rva < v + vs:
            return ro + (rva - v)
    return None

rva = int(sys.argv[1], 16)
length = int(sys.argv[2]) if len(sys.argv) > 2 else 160
off = rva2off(rva)
b = data[off:off+length]
print(f"; FUN_{rva:x} file-off {off:#x}")
print(f"; hex: {b.hex()}")
# simple length-decoded walk for cmp immediates
i = 0
# Find "cmp dword ptr [rXX+disp], imm32" (81 F9 id) / mov imm32 to stack (C7 ...)
while i < len(b) - 6:
    op = b[i]
    if op == 0xC7 and 0x24 <= b[i+1] <= 0x27:  # mov dword [rsp/...+disp8], imm32 — rough
        # C7 /0 id ; modrm 44 or 04 or 05...
        pass
    i += 1
# ASCII render
print("; ascii:", "".join(chr(c) if 32 <= c < 127 else "." for c in b))
