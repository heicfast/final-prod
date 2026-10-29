#!/usr/bin/env python3
"""S68: extract the og RAWSearchDLL static registry initializer — every
magic-key -> analyser binding, from the construct-then-insert pattern in
RAWSearchDLL_decompiled.c (FUN_180006c80 = std::multimap insert).
Each analyser construction block ends by assigning a vftable + an id byte
(local_res8[2] = <id>); the NEXT FUN_180006c80(param_1, local_res10) inserts
the PRECEDING constructed analyser under the key local_res10[0].
So: block-then-key ordering. Walk the region, pair them up."""
import re, sys

SRC = "/home/z/my-project/audit/easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c"
lines = open(SRC, encoding="utf-8", errors="replace").read().splitlines()

# Region: from the first insert (~1700) to the last (~2520)
start, end = 1690, 2530
text = "\n".join(lines[start:end])

# Split into blocks at each insert call
blocks = re.split(r"plVar3 = FUN_180006c80\(param_1,local_res10\);", text)
pairs = []
for i, blk in enumerate(blocks[:-1]):
    # The key comes BEFORE the insert: local_res10[0] = <key>;
    # the analyser was constructed in the text before that key assignment...
    # Actually pattern: [construct analyser X] ... local_res10[0]=KEY; insert(X)
    # So for block i (text before insert i), the analyser = last vftable assigned,
    # and key = the last 'local_res10[0] = KEY' in this block.
    vft = re.findall(r"(\w+(?:::\w+)?)::vftable", blk)
    keys = re.findall(r"local_res10\[0\] = (0x[0-9a-f]+);", blk)
    if not keys:
        continue
    key = keys[-1]
    # analyser: the LAST ::vftable assigned in this block (the concrete class;
    # IRawFileAnalyser/ISpecialRawFileAnalyser are interface prefixes)
    concrete = [v for v in vft if v not in ("IRawFileAnalyser", "ISpecialRawFileAnalyser")]
    analyser = concrete[-1] if concrete else (vft[-1] if vft else "??")
    # the id byte: local_res8[2] = <id> after the concrete vftable
    ids = re.findall(r"local_res8\[2\] = (0x[0-9a-f]+|\d+);", blk)
    idv = ids[-1] if ids else "?"
    pairs.append((key, analyser, idv))

print(f"extracted {len(pairs)} key->analyser bindings")
for key, ana, idv in pairs:
    print(f"  {key:>12}  id={idv:>4}  {ana}")

# also dump the id->format-name mapping if available: look for the id table
# in the census doc later.
