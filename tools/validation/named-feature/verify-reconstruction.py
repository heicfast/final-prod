#!/usr/bin/env python3
"""verify-reconstruction.py - the byte-for-byte judge for the named-feature
fixture (session: cross-repo harvest, T5 / Phase 3 validation standard).

Compares a reconstruction OUTPUT directory against the manifest recorded
BEFORE deletion:

  invariants (always checked):
    I1 deleted-only      - no live row's path may exist; no live row's
                           SHA-256 may appear as any output file's hash
    I2 exactness         - every EXACT row present at expect.output_rel with
                           exactly its size and SHA-256
    I3 collision-set     - the double-delete pair: 1..2 outputs under the
                           declared dir with the declared stem; every output
                           hash must belong to the pair; distinct outputs
                           must carry distinct hashes
    I4 accountability    - every output file must be accounted for by a
                           deleted manifest row (by SHA-256) - no invented
                           files
    I5 containment       - every output path resolves inside the output
                           root (junction/symlink escape = fail); all path
                           components must be valid Windows names (no
                           trailing dots/spaces, no reserved device names,
                           no <>:"/\\|?* or control chars)

Usage:
  python verify-reconstruction.py --manifest manifest.jsonl --out OUTDIR \
      [--report report.json]
  python verify-reconstruction.py --self-test

Exit code 0 = every check passed; 1 = any failure (also reported in JSON).

Licensed under the MIT license. See LICENSE.
"""

import argparse
import hashlib
import json
import os
import sys
import tempfile

RESERVED_NAMES = {
    "CON", "PRN", "AUX", "NUL",
    "CONIN$", "CONOUT$",
} | {f"COM{i}" for i in range(1, 10)} | {f"LPT{i}" for i in range(1, 10)}
INVALID_CHARS = set('<>:"/\\|?*') | {chr(c) for c in range(0, 0x20)}


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def valid_windows_component(name):
    if not name or name in (".", ".."):
        return False
    if any(c in INVALID_CHARS for c in name):
        return False
    if name[-1] in ". ":
        return False
    stem = name.split(".")[0].upper()
    if stem in RESERVED_NAMES:
        return False
    return True


def walk_outputs(out_root):
    """All files under out_root as (relpath_posix, abspath, size)."""
    results = []
    for base, _dirs, files in os.walk(out_root, followlinks=False):
        for name in files:
            abspath = os.path.join(base, name)
            rel = os.path.relpath(abspath, out_root).replace(os.sep, "/")
            results.append((rel, abspath, os.path.getsize(abspath)))
    return results


def verify(manifest_path, out_root, live_ok=False):
    failures = []
    rows = []
    with open(manifest_path, "r", encoding="utf-8-sig") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            obj = json.loads(line)
            if obj.get("_meta"):
                continue
            rows.append(obj)

    if not os.path.isdir(out_root):
        return {"ok": False, "failures": ["output root does not exist: %s" % out_root],
                "stats": {}}

    out_root_real = os.path.realpath(out_root)
    outputs = walk_outputs(out_root)
    out_by_rel = {rel: (abs, size) for rel, abs, size in outputs}
    out_hashes = {}
    for rel, abspath, size in outputs:
        out_hashes[rel] = (sha256_file(abspath), size)

    live_rows = [r for r in rows if not r.get("deleted")]
    deleted_rows = [r for r in rows if r.get("deleted")]
    exact_rows = [r for r in deleted_rows
                  if r.get("expect", {}).get("class") == "EXACT"
                  and r.get("expect", {}).get("output_rel")]
    collision_rows = [r for r in deleted_rows
                      if r.get("expect", {}).get("class") == "COLLISION-SET"]
    dir_rows = [r for r in deleted_rows if r.get("is_dir")]

    # I5 containment + valid components
    for rel, abspath, _size in outputs:
        if os.path.realpath(abspath) != abspath and \
           not os.path.realpath(abspath).startswith(out_root_real + os.sep):
            failures.append("I5 containment escape: %s" % rel)
        parts = rel.split("/")
        if any(not valid_windows_component(p) for p in parts):
            failures.append("I5 invalid path component: %r" % rel)

    # I1 deleted-only (path and content)
    for r in live_rows:
        rel = r["rel"]
        if rel in out_by_rel:
            failures.append("I1 live file present at path: %s" % rel)
        live_sha = r.get("sha256")
        if live_sha:
            for orel, (h, _s) in out_hashes.items():
                if h == live_sha:
                    failures.append("I1 live content leaked: %s matches live row %s"
                                    % (orel, rel))

    # I2 exactness
    matched_exact = 0
    for r in exact_rows:
        want_rel = r["expect"]["output_rel"]
        if want_rel not in out_hashes:
            failures.append("I2 missing output: %s" % want_rel)
            continue
        h, size = out_hashes[want_rel]
        if size != r["size"]:
            failures.append("I2 size mismatch %s: manifest %d vs output %d"
                            % (want_rel, r["size"], size))
        if h != r["sha256"]:
            failures.append("I2 sha256 mismatch %s: manifest %s vs output %s"
                            % (want_rel, r["sha256"], h))
        if not failures or all("I2 missing" not in f for f in failures):
            matched_exact += 1

    # I3 collision-set (double-delete pair)
    if collision_rows:
        pair_hashes = {r["sha256"] for r in collision_rows}
        exp = collision_rows[0]["expect"]
        cdir, stem, ext = exp["dir"], exp["stem"], exp["ext"]
        candidates = []
        for orel, (h, size) in out_hashes.items():
            parts = orel.split("/")
            if len(parts) == 2 and parts[0] == cdir:
                name = parts[1]
                if name.endswith("." + ext) and name.startswith(stem):
                    candidates.append((orel, h, size))
        if not (1 <= len(candidates) <= len(collision_rows)):
            failures.append("I3 collision-set: expected 1..%d outputs under %s/ "
                            "matching %s*.%s, found %d"
                            % (len(collision_rows), cdir, stem, ext, len(candidates)))
        seen_hashes = set()
        for orel, h, size in candidates:
            if h not in pair_hashes:
                failures.append("I3 collision-set: %s has unknown sha256 %s"
                                % (orel, h))
            if h in seen_hashes:
                failures.append("I3 collision-set: duplicate content at %s" % orel)
            seen_hashes.add(h)
        for r in collision_rows:
            if r["sha256"] not in {h for _o, h, _s in candidates} and len(candidates) > 1:
                failures.append("I3 collision-set: manifest hash %s (%s) not present "
                                "although %d outputs exist"
                                % (r["sha256"][:12], r["rel"], len(candidates)))

    # I4 accountability
    deleted_hashes = {r["sha256"] for r in deleted_rows if r.get("sha256")}
    for orel, (h, _size) in out_hashes.items():
        if h not in deleted_hashes:
            failures.append("I4 unaccounted output file: %s (sha %s)"
                            % (orel, h[:12]))

    # deleted directories must exist in the output (they carried deleted children)
    for r in dir_rows:
        want = r.get("expect", {}).get("output_rel") or r["rel"]
        want_path = os.path.join(out_root, *want.split("/"))
        if not os.path.isdir(want_path):
            failures.append("I2 deleted directory not reconstructed: %s" % want)

    stats = {
        "manifest_rows": len(rows),
        "live_rows": len(live_rows),
        "deleted_rows": len(deleted_rows),
        "exact_rows": len(exact_rows),
        "collision_rows": len(collision_rows),
        "output_files": len(outputs),
        "output_dirs": sum(1 for b, d, _f in os.walk(out_root) for _ in d) if outputs else 0,
        "failures": len(failures),
    }
    return {"ok": not failures, "failures": failures, "stats": stats}


# ---------------------------------------------------------------- self-test

def _self_test():
    """Build a tiny manifest + trees in a tempdir; assert pass/fail behavior."""
    tmp = tempfile.mkdtemp(prefix="nffx-verify-")
    man = os.path.join(tmp, "manifest.jsonl")
    rows = [
        {"_meta": True, "fixture_id": "selftest"},
        {"rel": "a.txt", "size": 4, "sha256": hashlib.sha256(b"AAA\n").hexdigest(),
         "deleted": True, "is_dir": False,
         "expect": {"output_rel": "a.txt", "class": "EXACT", "mode": "EXACT"}},
        {"rel": "b/c.txt", "size": 4, "sha256": hashlib.sha256(b"BBB\n").hexdigest(),
         "deleted": True, "is_dir": False,
         "expect": {"output_rel": "b/c.txt", "class": "EXACT", "mode": "EXACT"}},
        {"rel": "live.txt", "size": 4, "sha256": hashlib.sha256(b"LLL\n").hexdigest(),
         "deleted": False, "is_dir": False, "expect": {"output_rel": None}},
    ]
    with open(man, "w", encoding="utf-8") as f:
        for r in rows:
            f.write(json.dumps(r) + "\n")

    def make_out(with_missing=False, with_live=False, with_badhash=False,
                 with_extra=False):
        out = tempfile.mkdtemp(prefix="nffx-out-", dir=tmp)
        os.makedirs(os.path.join(out, "b"), exist_ok=True)
        if not with_missing:
            with open(os.path.join(out, "a.txt"), "wb") as f:
                f.write(b"AAA\n")
        data = b"XXX\n" if with_badhash else b"BBB\n"
        with open(os.path.join(out, "b", "c.txt"), "wb") as f:
            f.write(data)
        if with_live:
            with open(os.path.join(out, "live.txt"), "wb") as f:
                f.write(b"LLL\n")
        if with_extra:
            with open(os.path.join(out, "extra.txt"), "wb") as f:
                f.write(b"EEE\n")
        return out

    results = []
    results.append(("good", verify(man, make_out())["ok"] is True))
    results.append(("missing", verify(man, make_out(with_missing=True))["ok"] is False))
    results.append(("live-leak", verify(man, make_out(with_live=True))["ok"] is False))
    results.append(("bad-hash", verify(man, make_out(with_badhash=True))["ok"] is False))
    results.append(("extra", verify(man, make_out(with_extra=True))["ok"] is False))
    ok = all(v for _n, v in results)
    for name, v in results:
        print("selftest %-10s %s" % (name, "PASS" if v else "FAIL"))
    print("self-test %s" % ("PASSED" if ok else "FAILED"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest")
    ap.add_argument("--out")
    ap.add_argument("--report")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()

    if args.self_test:
        return _self_test()

    if not args.manifest or not args.out:
        ap.error("--manifest and --out are required (or --self-test)")

    result = verify(args.manifest, args.out)
    # D-PRINT-01 (run 44380763554): a Windows runner's stdout is cp1252; the
    # unicode filename row ('Names/unicode-...') crashed the verdict print
    # with UnicodeEncodeError BEFORE the report was written and BEFORE the
    # VERDICT line - masking the actual judgment. The JSON report is still
    # written utf-8; the stdout echo falls back to ASCII so a failing
    # verdict always reaches the log. The verdict line itself is pure ASCII.
    if args.report:
        with open(args.report, "w", encoding="utf-8") as f:
            json.dump(result, f, indent=2, ensure_ascii=False)
    try:
        print(json.dumps(result, indent=2, ensure_ascii=False))
    except UnicodeEncodeError:
        print(json.dumps(result, indent=2, ensure_ascii=True))
    print("VERDICT: %s" % ("PASS" if result["ok"] else "FAIL"))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
