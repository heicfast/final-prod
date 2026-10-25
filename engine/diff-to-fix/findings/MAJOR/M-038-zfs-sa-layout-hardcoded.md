# MAJOR M-038 — ZFS SA decode is layout-1-hardcoded; objects with non-1 registered layouts are dropped entirely

**Severity:** MAJOR
**Module:** zfs
**Parameters affected:** B11 (node fields — SA attribute registry), B1 (detection — files skipped), A25 (engine algorithm steps)
**Corpus references:** ZfsSearchDLL_decompiled.c — SA attribute decode is **registry-driven, by NAME**: "ZPL_PARENT" looked up in the SA attribute-registration ZAP :24324, "ZPL_CRTIME" :24807 — attribute offsets resolved through the registry, layout-agnostic.
**Core references:** `src/core/zfs/cr_zfs_engine.cpp:1230-1246` — `if (layout != 1) return nullptr;` (the object is skipped and its dir entry never emitted at :1284-1286), legacy znode fallback :1247-1256. Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-023` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og resolves system-attribute offsets by NAME through the SA attribute-registration ZAP (per-dataset registry), so any registered layout works. Our engine hardcodes SA layout 1 and returns null for anything else — objects carrying a non-1 registered layout (ACL/xattr-bearing files on several OpenZFS versions register alternate layouts) are dropped: their files never appear.

## What the reference does

The by-name lookups ("ZPL_PARENT", "ZPL_CRTIME") into the registration ZAP are quoted — the registry maps attribute names to slots for the object's layout.

## What our core does

`layout == 1` gate at :1235-1246; no registry read; the legacy-znode fallback only covers pre-SA objects.

## Gap

Registry-driven decode vs hardcoded layout. Falsifiable: a pool with objects carrying a registered non-1 layout shows those files under the og and not under ours.

## Impact

Silent missing files on pools where alternate SA layouts are registered (xattr/ACL-heavy workloads on some OpenZFS versions). Related open question OQ-Z1 (dsl offset constants) gates a broader ZFS verification.

## Reproduction / verification

Corpus-derived (quotes first-hand). UNVERIFIED-BY-EXECUTION (needs a pool image with registered non-1 layouts — ztest/zpool with xattrs can produce one).

## Fix plan

1. Read the SA attribute-registration ZAP (`sa_attr_registration` object) per dataset: name → {attr number, bytes length}.
2. Decode the layout registry: layout number → slot list; resolve each attribute's offset from the object's layout entry + the registered lengths.
3. Fall back to layout 1 defaults only when the registry is absent (compat with our current fixtures).
4. Keep the legacy-znode path.

## Tests to add

- `Zfs.SaRegistryDrivenLayout2` — fixture with a registered layout-2 object; assert the file emits with correct parent/crtime (fails today: dropped).
- `Zfs.SaRegistryAbsentFallsBackToLayout1` — pin current behavior for registry-less pools.

## Related findings

M-026 (parallel leg)-context (ZFS supersets recorded in I-007), OQ-Z1..Z5 (tracked-open — the dsl offsets question is the ACUTE-gated sibling).

## Open questions

The exact layout-registry ZAP object number and slot encoding — decode pass on a real pool image (folded into tracked-open OQ-Z).
