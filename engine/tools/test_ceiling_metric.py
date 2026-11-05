#!/usr/bin/env python3
"""
test_ceiling_metric.py - regression tests for the GAP-S6-5 physical-ceiling
block in bench/metrics/compute_metrics.py.

The ceiling math only ever runs against real decay-scenario artifacts on the
Windows bench hosts - which means a broken implementation would surface as a
misleading recall verdict (or never at all). This self-test exercises
compute_ceiling() against synthetic manifest/ceiling shapes covering every
documented behavior:

  PASS path  - a decay state where 3 of 5 ground-truth files are physically
               intact and 2 of those 3 were recovered: intact_share = 0.6,
               recall_of_physically_intact = 2/3.
  MISMATCH   - a file whose post-decay range hash differs from its manifest
               sha256 (the decay fillers overwrote it) is NOT intact, even
               when it was "recovered" (recovered_of_intact must not count
               it - the naive denominator is exactly the overstatement this
               metric exists to correct).
  SHORT READ - a probe that hit the volume end (short_read) is excluded
               from the intact set and counted separately.
  KEEP/PATH  - manifest rel paths normalize case/backslashes exactly like
               the scorer's norm(); ceiling keys produced by the PowerShell
               probe use forward-slash rels verbatim.
  EMPTY      - no ceiling artifact (None / {}) -> None block (the scorer
               keeps its pre-GAP-S6-5 shape for non-decay scenarios).

Run:  python tools/test_ceiling_metric.py     (from the repo root)
Exit:  0 = all cases behave, 1 = a regression.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, os.pardir, "bench", "metrics"))

from compute_metrics import compute_ceiling  # noqa: E402

FAILURES = []


def expect(cond, label, detail=""):
    if cond:
        print(f"  ok: {label}")
    else:
        print(f"  FAIL: {label} {detail}")
        FAILURES.append(label)


def gt_entry(rel, sha):
    return {"rel": rel, "name": os.path.basename(rel), "sha256": sha, "keep": False, "size": 100}


def main():
    sha_a = "a" * 64
    sha_b = "b" * 64
    sha_c = "c" * 64
    sha_d = "d" * 64
    sha_e = "e" * 64

    print("case 1: intact subset + recall-of-intact")
    gt = [gt_entry("seed/a.jpg", sha_a), gt_entry("seed/b.jpg", sha_b),
          gt_entry("seed/c.jpg", sha_c), gt_entry("seed/d.jpg", sha_d),
          gt_entry("seed/e.jpg", sha_e)]
    # post-decay: a/b/c intact, d overwritten by fillers, e short-read at vol end
    ceil = {
        "seed/a.jpg": {"post_decay_sha256": sha_a, "short_read": False},
        "seed/b.jpg": {"post_decay_sha256": sha_b, "short_read": False},
        "seed/c.jpg": {"post_decay_sha256": sha_c, "short_read": False},
        "seed/d.jpg": {"post_decay_sha256": "f" * 64, "short_read": False},
        "seed/e.jpg": {"post_decay_sha256": sha_e, "short_read": True},
    }
    recovered = {"seed/a.jpg", "seed/b.jpg", "seed/d.jpg"}
    blk = compute_ceiling(ceil, gt, recovered)
    expect(blk is not None, "block produced")
    expect(blk["files_ground_truth"] == 5, "gt count 5", str(blk))
    expect(blk["physically_intact"] == 3, "intact = 3 (a/b/c; d mismatched, e short-read)", str(blk))
    expect(blk["intact_share"] == 0.6, "intact_share 0.6", str(blk))
    expect(blk["short_read_files"] == 1, "short_read counted separately", str(blk))
    expect(blk["recovered_of_intact"] == 2, "recovered_of_intact = 2 (d recovered but NOT intact)", str(blk))
    expect(blk["recall_of_physically_intact"] == round(2 / 3, 4), "recall_of_intact 2/3", str(blk))

    print("case 2: case/backslash normalization parity with the scorer's norm()")
    gt2 = [gt_entry("Seed\\Sub\\X.Png", sha_a)]
    ceil2 = {"seed/sub/x.png": {"post_decay_sha256": sha_a.upper(), "short_read": False}}
    blk2 = compute_ceiling(ceil2, gt2, {"seed/sub/x.png"})
    expect(blk2 is not None and blk2["physically_intact"] == 1, "normalized keys match + uppercase hash tolerated", str(blk2))

    print("case 3: files without extents are outside the probe, not intact-by-default")
    gt3 = [gt_entry("seed/a.jpg", sha_a), gt_entry("seed/noext.jpg", sha_b)]
    ceil3 = {"seed/a.jpg": {"post_decay_sha256": sha_a, "short_read": False}}
    blk3 = compute_ceiling(ceil3, gt3, set())
    expect(blk3["files_with_extents"] == 1, "only the probed file counted", str(blk3))
    expect(blk3["physically_intact"] == 1, "probed file intact", str(blk3))
    expect(blk3["files_ground_truth"] == 2, "gt still 2 (share denominator honest)", str(blk3))

    print("case 4: empty/absent ceiling keeps the legacy scorer shape")
    expect(compute_ceiling(None, gt, set()) is None, "None records -> None block")
    expect(compute_ceiling({}, gt, set()) is None, "empty records -> None block")

    print("case 5: nothing intact -> recall_of_physically_intact is None, not 0.0")
    ceil5 = {"seed/a.jpg": {"post_decay_sha256": "0" * 64, "short_read": False}}
    blk5 = compute_ceiling(ceil5, gt[:1], set())
    expect(blk5["physically_intact"] == 0, "zero intact", str(blk5))
    expect(blk5["recall_of_physically_intact"] is None, "None (division guarded)", str(blk5))

    print()
    if FAILURES:
        print(f"test_ceiling_metric: {len(FAILURES)} REGRESSION(S): {FAILURES}")
        return 1
    print("test_ceiling_metric: all ceiling behaviors verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
