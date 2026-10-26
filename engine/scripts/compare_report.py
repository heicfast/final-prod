#!/usr/bin/env python3
"""compare_report.py - side-by-side EaseUS (og) vs rrecov scoring against
the seeded ground-truth manifest.

Inputs:
  --manifest manifest.csv  (relpath,size,sha256,scenario) from bench_seed
  --og       og.csv        (path,size,origin,deleted,...) from compare_og_rsf
  --ours     ours.csv      (path,size,...) from `rrecov tree --csv`
  --out      comparison.md markdown report
  --json     comparison.json

Both products are scored with the same name-level matcher (basename +
exact size) - carved results lose path context, so name+size is the only
fair common denominator. Wipe-scenario files are excluded from the recall
denominator (content destroyed; nobody can recover them).

Licensed under the MIT license. See LICENSE.
"""

import argparse
import csv
import json
import os
import sys
from collections import defaultdict

RECOVERABLE = ("recycle", "perm", "folder")


def load_csv(path, fields):
    rows = []
    try:
        with open(path, "r", encoding="utf-8-sig", newline="") as fh:
            for row in csv.DictReader(fh, fieldnames=fields):
                rows.append(row)
    except OSError:
        pass
    return rows


def normalize_origin(raw):
    """Two CSV dialects meet here (GAP-BENCH-3): `rrecov tree --csv`
    emits the FROZEN numeric enums (cr_node.h 28-36:
    0=Structured 1=Carved 2=Lost 3=Virtual) while compare_og_rsf.py emits
    words (structured|carved). The pre-fix `origin == "carved"` test
    classified ALL 536 carved rows of ours.csv as structured in the
    published report."""
    v = (raw or "").strip().lower()
    if v == "1":
        return "carved"
    if v == "0":
        return "structured"
    if v == "2":
        return "lost"
    if v == "3":
        return "virtual"
    return v or "unknown"


def normalize_deleted(raw):
    """deleted: numeric 0=Active, 1/2/3=deleted-family (Deleted,
    PartiallyOverwritten, Damaged); the og extractor emits
    deleted|active|unknown."""
    v = (raw or "").strip().lower()
    if v == "0":
        return "active"
    if v in ("1", "2", "3"):
        return "deleted"
    return v or "unknown"


def load_ours(path):
    rows = load_csv(path, ["path", "size", "origin", "deleted", "confidence", "extent"])
    out = []
    for r in rows:
        try:
            out.append((r["path"].strip(), int(r["size"] or 0),
                        normalize_origin(r["origin"]), normalize_deleted(r["deleted"])))
        except (ValueError, AttributeError):
            continue
    return out


def load_og(path):
    rows = load_csv(path, ["path", "size", "origin", "deleted", "confidence", "extent"])
    out = []
    for r in rows:
        try:
            out.append((r["path"].strip(), int(r["size"] or 0),
                        normalize_origin(r["origin"]), normalize_deleted(r["deleted"])))
        except (ValueError, AttributeError):
            continue
    return out


def score(manifest, entries):
    """name+size level scoring shared by both products."""
    name_size = set()
    active_name_size = set()
    carved = 0
    structured = 0
    for path, size, origin, deleted in entries:
        base = os.path.basename(path.replace("\\", "/")).lower()
        if base:
            name_size.add((base, size))
            if deleted == "active":
                active_name_size.add((base, size))
        if origin == "carved":
            carved += 1
        elif origin in ("structured", "lost", "virtual"):
            structured += 1
        # unknown dialects count in neither bucket
    per_scenario = defaultdict(lambda: {"total": 0, "hit": 0})
    for m in manifest:
        base = os.path.basename(m["relpath"].replace("\\", "/")).lower()
        s = per_scenario[m["scenario"]]
        s["total"] += 1
        if (base, m["size"]) in name_size:
            s["hit"] += 1
    recall_denom = sum(per_scenario[s]["total"] for s in RECOVERABLE if s in per_scenario)
    recall_num = sum(per_scenario[s]["hit"] for s in RECOVERABLE if s in per_scenario)
    keep = per_scenario.get("keep", {"total": 0, "hit": 0})
    keep_active = 0
    for m in manifest:
        if m["scenario"] != "keep":
            continue
        base = os.path.basename(m["relpath"].replace("\\", "/")).lower()
        if (base, m["size"]) in active_name_size:
            keep_active += 1
    return {
        "entries": len(entries),
        "keep_active": keep_active,
        "structured": structured,
        "carved": carved,
        "per_scenario": {k: dict(v) for k, v in per_scenario.items()},
        "recall": round(recall_num / recall_denom, 4) if recall_denom else 0.0,
        "recall_num": recall_num,
        "recall_denom": recall_denom,
        # GAP-BENCH-1: the keep control requires the rows to be ACTIVE -
        # a deleted duplicate of a keep file is a control FAILURE
        "keep_ok": keep["total"] > 0 and keep_active == keep["total"],
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--og", required=True)
    ap.add_argument("--ours", required=True)
    ap.add_argument("--out", default="comparison.md")
    ap.add_argument("--json", default="comparison.json")
    args = ap.parse_args()

    manifest = []
    with open(args.manifest, "r", encoding="utf-8-sig", newline="") as fh:
        for row in csv.DictReader(fh, fieldnames=["relpath", "size", "sha256", "scenario"]):
            try:
                size = int(row["size"])
            except (TypeError, ValueError):
                continue  # header or malformed line
            manifest.append(
                {
                    "relpath": row["relpath"].strip(),
                    "size": size,
                    "scenario": row["scenario"].strip(),
                }
            )

    og = score(manifest, load_og(args.og))
    ours = score(manifest, load_ours(args.ours))

    def pct(x):
        return f"{x*100:.1f}%"

    lines = [
        "# EaseUS (og) vs rrecov - same seeded disk",
        "",
        "| metric | EaseUS DRW (og) | rrecov (ours) |",
        "|---|---|---|",
        f"| recovered entries | {og['entries']:,} | {ours['entries']:,} |",
        f"| structured | {og['structured']:,} | {ours['structured']:,} |",
        f"| carved | {og['carved']:,} | {ours['carved']:,} |",
        f"| recall (recycle+perm+folder) | {pct(og['recall'])} "
        f"({og['recall_num']}/{og['recall_denom']}) | {pct(ours['recall'])} "
        f"({ours['recall_num']}/{ours['recall_denom']}) |",
        f"| keep-control active | {'OK' if og['keep_ok'] else 'FAIL'} | "
        f"{'OK' if ours['keep_ok'] else 'FAIL'} |",
        "",
        "## Per-scenario name-level recall",
        "",
        "| scenario | og | ours |",
        "|---|---|---|",
    ]
    scenarios = sorted(set(og["per_scenario"]) | set(ours["per_scenario"]))
    for s in scenarios:
        o = og["per_scenario"].get(s, {"total": 0, "hit": 0})
        u = ours["per_scenario"].get(s, {"total": 0, "hit": 0})
        lines.append(
            f"| {s} | {o['hit']}/{o['total']} "
            f"({pct(o['hit']/o['total']) if o['total'] else '-'}) | "
            f"{u['hit']}/{u['total']} "
            f"({pct(u['hit']/u['total']) if u['total'] else '-'}) |"
        )
    lines += [
        "",
        "_wipe-scenario files are excluded from the recall denominator "
        "(content zero-destroyed before deletion - unrecoverable by design). "
        "The og leg is UI-driven; missing og numbers mean the UI automation "
        "did not complete - see the screenshots artifact._",
        "",
    ]
    report = "\n".join(lines)
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write(report)
    with open(args.json, "w", encoding="utf-8") as fh:
        json.dump({"og": og, "ours": ours}, fh, indent=2)
    print(report)
    return 0


if __name__ == "__main__":
    sys.exit(main())
