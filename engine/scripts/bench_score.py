#!/usr/bin/env python3
"""bench_score.py - ground-truth recall/precision scoring for the recovery
benchmark.

Inputs:
  --manifest manifest.csv   (relpath,size,sha256,scenario) from bench_seed.ps1
  --results results.csv     (path,size,origin,deleted,confidence) from
                            `rrecov tree --csv` over the scan session
  --out results.json        machine-readable scores
  --summary summary.md      human/markdown summary (also stdout)

Scoring model:
  keep    -> must appear in the ACTIVE tree (sanity gate)
  recycle -> recovered if found (path or name+size) anywhere
  perm    -> recovered if found anywhere (structured deleted / carved)
  folder  -> recovered if found anywhere
  wipe    -> expected UNRECOVERABLE (content destroyed); any "recovery"
             is a bonus, never counted against recall

Two match levels are reported:
  path_recall - recovered path ends with the manifest relpath
  name_recall - basename + exact size matched (carved results often lose
                their path context; a hit still means usable recovery)

Licensed under the MIT license. See LICENSE.
"""

import argparse
import csv
import json
import os
import sys
from collections import defaultdict


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--results", required=True)
    ap.add_argument("--out", default="scores.json")
    ap.add_argument("--summary", default="summary.md")
    args = ap.parse_args()

    manifest = []
    with open(args.manifest, "r", encoding="utf-8-sig", newline="") as fh:
        for row in csv.DictReader(fh, fieldnames=["relpath", "size", "sha256", "scenario"]):
            # hardened parse (the pattern proven in compare_report.py): a
            # single malformed row (blank line, header, short row -> None
            # fields) must skip, never kill the whole scoring run
            try:
                manifest.append(
                    {
                        "relpath": (row["relpath"] or "").strip(),
                        "size": int(row["size"]),
                        "sha256": (row["sha256"] or "").strip(),
                        "scenario": (row["scenario"] or "").strip(),
                    }
                )
            except (TypeError, ValueError, AttributeError):
                continue  # header or malformed line

    results = []
    with open(args.results, "r", encoding="utf-8-sig", newline="") as fh:
        for row in csv.DictReader(fh, fieldnames=["path", "size", "origin", "deleted", "confidence", "extent"]):
            try:
                results.append(
                    {
                        "path": (row["path"] or "").strip(),
                        "size": int(row["size"] or 0),
                        "origin": (row["origin"] or "").strip(),
                        "deleted": (row["deleted"] or "").strip(),
                        "confidence": (row["confidence"] or "").strip(),
                    }
                )
            except (TypeError, ValueError, KeyError, AttributeError):
                continue  # header or malformed line

    # `rrecov tree --csv` FROZEN schema (cr_node.h 28-36, GAP-BENCH-3):
    #   origin     u32: 0=Structured 1=Carved 2=Lost 3=Virtual
    #   deleted    u32: 0=Active 1=Deleted 2=PartiallyOverwritten 3=Damaged
    #   confidence u32: 0..4
    # The pre-fix word-matching ("deleted","true","1","yes") classified
    # PartiallyOverwritten(2)/Damaged(3) rows as ACTIVE via the accidental
    # "1" collision - the keep gate then passed on deleted duplicates.

    def is_active(deleted_field):
        # numeric enum: ONLY 0 is active; 1/2/3 are deleted-family states
        try:
            return int(deleted_field) == 0
        except ValueError:
            return deleted_field.strip().lower() in ("active", "false", "no")

    # index results: full lowercase path and basename+size
    path_set = set()
    name_size = defaultdict(int)
    active_paths = set()
    active_name_size = set()
    for r in results:
        p = r["path"].replace("\\", "/").lower()
        path_set.add(p)
        name_size[(os.path.basename(p), r["size"])] += 1
        if is_active(r["deleted"]):
            active_paths.add(p)
            active_name_size.add((os.path.basename(p), r["size"]))

    per_scenario = defaultdict(lambda: {"total": 0, "path_hit": 0, "name_hit": 0})
    matched_result_rows = set()

    for m in manifest:
        rel = m["relpath"].replace("\\", "/").lower()
        base = os.path.basename(rel)
        s = per_scenario[m["scenario"]]
        s["total"] += 1
        path_hit = any(p.endswith("/" + rel) or p == rel or p.endswith(rel) for p in path_set)
        name_hit = (base, m["size"]) in name_size
        if path_hit:
            s["path_hit"] += 1
        if name_hit:
            s["name_hit"] += 1
        if path_hit or name_hit:
            # GAP-BENCH-2: mark the RESULT ROWS this manifest entry
            # consumed (per-result matching; the old per-manifest
            # decrement let two manifest entries double-consume one row
            # and drove the counter negative)
            for r in results:
                p = r["path"].replace("\\", "/").lower()
                if p == rel or p.endswith("/" + rel) or p.endswith(rel):
                    matched_result_rows.add(id(r))
                if os.path.basename(p) == base and r["size"] == m["size"]:
                    matched_result_rows.add(id(r))

    # sanity gate: keep files must be ACTIVE (GAP-BENCH-1: the pre-fix
    # gate counted name hits anywhere - a deleted duplicate of a keep
    # file passed the control; active_paths/active_name_size were built
    # and then never used)
    keep = per_scenario.get("keep", {"total": 0, "path_hit": 0, "name_hit": 0})
    keep_active_ok = 0
    for m in manifest:
        if m["scenario"] != "keep":
            continue
        rel = m["relpath"].replace("\\", "/").lower()
        base = os.path.basename(rel)
        if any(p.endswith("/" + rel) or p == rel or p.endswith(rel) for p in active_paths):
            keep_active_ok += 1
        elif (base, m["size"]) in active_name_size:
            keep_active_ok += 1

    scores = {
        "manifest_files": len(manifest),
        "recovered_entries": len(results),
        "per_scenario": {},
        "overall": {},
    }
    for scen, s in sorted(per_scenario.items()):
        total = s["total"] or 1
        scores["per_scenario"][scen] = {
            "total": s["total"],
            "path_recall": round(s["path_hit"] / total, 4),
            "name_recall": round(s["name_hit"] / total, 4),
        }
    recall_denom = sum(
        s["total"] for scen, s in per_scenario.items() if scen in ("recycle", "perm", "folder")
    )
    recall_num = sum(
        per_scenario[scen]["name_hit"] for scen in ("recycle", "perm", "folder") if scen in per_scenario
    )
    scores["overall"] = {
        "recoverable_total": recall_denom,
        "recovered": recall_num,
        "recall": round(recall_num / recall_denom, 4) if recall_denom else 0.0,
        "keep_control_ok": bool(keep_active_ok == keep["total"] and keep["total"] > 0),
        # GAP-BENCH-2: per-RESULT accounting - matched rows / all rows.
        # HINT, not precision: seeded manifests do not enumerate
        # pre-existing disk content, so an unmatched row is a duplicate
        # or an engine artifact, not provably a false positive.
        "matched_result_rows": len(matched_result_rows),
        "precision_hint": round(len(matched_result_rows) / len(results), 4)
        if results
        else 0.0,
    }

    with open(args.out, "w", encoding="utf-8") as fh:
        json.dump(scores, fh, indent=2)

    lines = [
        "# Recovery benchmark scores",
        "",
        f"- manifest files: **{scores['manifest_files']}**",
        f"- recovered entries: **{scores['recovered_entries']}**",
        f"- overall recall (recycle+perm+folder, name-level): "
        f"**{scores['overall']['recall']*100:.1f}%** "
        f"({scores['overall']['recovered']}/{scores['overall']['recoverable_total']})",
        f"- keep-control active: **{'OK' if scores['overall']['keep_control_ok'] else 'FAIL'}**",
        "",
        "| scenario | total | path recall | name recall |",
        "|---|---|---|---|",
    ]
    for scen, s in scores["per_scenario"].items():
        lines.append(
            f"| {scen} | {s['total']} | {s['path_recall']*100:.1f}% | {s['name_recall']*100:.1f}% |"
        )
    summary = "\n".join(lines) + "\n"
    with open(args.summary, "w", encoding="utf-8") as fh:
        fh.write(summary)
    print(summary)

    # exit code: fail the job when the control group is not found active
    return 0 if scores["overall"]["keep_control_ok"] else 3


if __name__ == "__main__":
    sys.exit(main())
