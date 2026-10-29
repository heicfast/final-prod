#!/usr/bin/env python3
"""S70 lead verification: fit the og's CalcPartition.log scores against the
candidate formulas (denominator with/without +500; dSuggested base; the
count==0 edge). Ground truth: og-codebase/log/CalcPartition.log."""
import math
import re

LOG = "/home/z/my-project/easeus-drw-research/og-codebase/log/CalcPartition.log"

rows = []  # (round, idx, fs, files, folders, score)
round_no = 0
with open(LOG, encoding="utf-8", errors="replace") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        if "dSuggestted" not in line:
            m = re.search(r"round\s*(\d+)", line, re.I)
            if m:
                round_no = int(m.group(1))
            continue
        m = re.match(
            r"\d+:\d+:\d+:\d+\s+(\d+)\.([0-9A-F]+)\((\w+)\)\s+StartSec:(\d+)\s+"
            r"DataAreaStart:(\d+)\s+EndSec:(\d+)\s+totalSec:(\d+)\s+files:(\d+)\s+"
            r"folders:(\d+)\s+dSuggestted:([0-9.]+)",
            line,
        )
        if m:
            rows.append(
                dict(
                    round=round_no,
                    idx=int(m.group(1)),
                    fs=m.group(3),
                    start=int(m.group(4)),
                    files=int(m.group(8)),
                    folders=int(m.group(9)),
                    score=float(m.group(10)),
                )
            )

print(f"parsed {len(rows)} score rows, round tag range {sorted(set(r['round'] for r in rows))}")

def weighted(fs, files, folders):
    if fs == "HFSPLUS":
        return files + folders * 10
    if fs in ("FAT16", "FAT32"):
        return (files + folders) * 50
    return files + folders * 50  # NTFS family / else

by_round = {}
for r in rows:
    by_round.setdefault(r["round"], []).append(r)

for rnd, rlist in sorted(by_round.items()):
    print(f"\n=== round tag {rnd}: {len(rlist)} rows ===")
    leader = max(rlist, key=lambda r: r["score"])
    w_leader = weighted(leader["fs"], leader["files"], leader["folders"])
    print("  leader:", leader["idx"], leader["fs"], "files", leader["files"],
          "folders", leader["folders"], "score", leader["score"], "w", w_leader)
    for d_max in (0, 10, 20, 26, 30, 45, 50, 60):
        max_count = w_leader + d_max
        if max_count <= 1:
            continue
        den = math.log(max_count)
        implied = []
        for r in rlist:
            if r is leader:
                continue
            w = weighted(r["fs"], r["files"], r["folders"])
            implied_count = math.exp(r["score"] * den)
            implied.append((r["idx"], w, implied_count - w))
        intish = sum(1 for _, _, d in implied if abs(d - round(d)) < 0.02 and round(d) >= 0)
        residues = sorted(abs(d - round(d)) for _, _, d in implied)
        med = residues[len(residues) // 2]
        print(f"  d_max={d_max:3d}: int-like {intish}/{len(implied)}, median |frac|={med:.4f}")
    # per-row implied d under the two leading hypotheses
    for d_max in (0, 60):
        max_count = w_leader + d_max
        den = math.log(max_count)
        print(f"  -- implied dSugg under d_max={d_max}:")
        for r in rlist[:6]:
            w = weighted(r["fs"], r["files"], r["folders"])
            d = math.exp(r["score"] * den) - w
            print(f"     idx={r['idx']:3d} w={w:7d} s={r['score']:.6f} -> d={d:9.3f}")
