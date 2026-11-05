# MAJOR M-150 — session load: the tree-record count pre-allocates `8 × count` bytes from an uncapped hostile u32 (up to 34 GB of touched memory) before any per-record parse — the og's per-node factory reader is bounded by buffer exhaustion

**Severity:** MAJOR
**Module:** session (load / resume path)
**Parameters affected:** X49 (memory — the load-path spike), X68-adjacent (hostile-input hardening), S36 (session lifecycle — resume robustness), X53-adjacent (the load's perceived freeze)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-record4--tree-node-grammar.md:16,26` (the record-4 tree blob: "group framing, node-type factory" — the reader consumes per-node records through per-FS factories, so allocation tracks the bytes actually parsed); `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` (the container grammar the core's CRSF mirrors)
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` (the N-131 parent-anchor landing shifted the file; the cites refreshed) — `src/core/session/cr_records.cpp:349` (was :322 at the audited pin) (`const u32 count = c.g32();` — no cap), `:372` (`std::vector<tree::FileNode*> by_id(count, nullptr);` — the value-initialized pre-allocation, every byte written); the container's record-payload cap `src/core/session/cr_crsf.cpp:304` (`CRSF_RECORD_MAX = 64 MiB` — bounds the payload bytes, NOT the in-record count field); the ABI catch boundary `src/ui/cr_ui.cpp:135-143` (`guarded()` maps `std::bad_alloc` → ERR_INTERNAL)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's `deserialize_tree` reads a u32 node count from the session record and immediately pre-allocates `count × 8` bytes of value-initialized memory — up to 34.4 GB at the u32 maximum — before parsing a single node. A truncated or corrupted session file (the realistic producer: a torn write, the M-131 class; or any hostile .crsf) whose count field is garbage triggers a multi-gigabyte zero-fill: on Windows this burns minutes of pagefile thrash (the perceived-freeze class) before `bad_alloc` is caught at the ABI boundary; on Linux overcommit it can OOM-kill the process outright (uncatchable SIGKILL). The og's record-4 reader allocates node-by-node through per-FS factories, so its allocation is bounded by the bytes the buffer actually contains.

## What the reference does

The og's `.rsf` record-4 reader re-materializes the tree blob through per-node factories — the corpus artifact's grammar: the blob is group-framed with a node-type factory per FS family (`rsf-record4--tree-node-grammar.md:16,26`: "Data (per-FS) nodes — the filesystem-specific record for each file/folder … Serialized by `FUN_1400d6fa0` as `[4B type][type-specific body]`"). The reader consumes the serialized bytes sequentially: each node's allocation happens as its record is parsed, so a count field that overruns the buffer fails the parse at buffer exhaustion with only the nodes-that-fit ever allocated. There is no count-scaled up-front allocation anywhere in the og's reader shape.

## What our core does

`src/core/session/cr_records.cpp:347-372` (the re-verified pin):

```cpp
Cursor c{data, len};
const u32 count = c.g32();                     /* <- hostile, uncapped */
if (c.fail)
    return 0x50040001;
...
std::vector<tree::FileNode*> by_id(count, nullptr);   /* <- 8*count bytes, zero-filled */
```

The vector is value-initialized (`nullptr` fill) — every page of the allocation is touched at construction. With `count = 0xFFFFFFFF` that is 34.4 GB of writes: on Windows the allocation itself fails into `std::bad_alloc` after the pagefile grows toward it (minutes of thrash first); on Linux with default overcommit the virtual allocation succeeds and the zero-fill drives the OOM killer (SIGKILL — the `guarded()` catch at `src/ui/cr_ui.cpp:135-143` never sees it). The container layer's protections do not reach this field: `CRSF_RECORD_MAX` (64 MiB, `cr_crsf.cpp:304`) caps the record's decoded payload bytes, and the P1-7 chunk-amplification budget caps the inflate ratio — but a count field inside a small, validly-framed, CRC-correct record still pre-allocates as though 4 billion nodes followed. Each node in the grammar consumes ≥ 10 bytes on the wire (parent id, flags, marker, origin, deleted, confidence, …), so a 64 MiB record can carry at most ~6.7 M nodes; any count beyond that is impossible-but-allocated anyway.

## Gap

Falsifiable: hand-craft a minimal .crsf whose tree record is a valid container frame (magic, CRC, ≤ 64 MiB payload) containing a parseable node stream of 3 nodes preceded by `count = 0xF4240000` (or any value > len/10). The og's reader parses 3 nodes and fails/clamps at buffer exhaustion; the core attempts a 31-GB zero-fill before any node parse — the load stalls (Windows) or the process dies (Linux overcommit).

## Impact

Resume-from-session is a first-class flow (`cr_ui_scan_run` on a resumed controller; the CLI's `--resume`). A torn or corrupted session file — the exact failure mode M-131's truncate-in-place repair addressed on the WRITE side — now hangs or kills the READ side. The realistic frequency is low (corrupt counts are rare), but the blast radius is the whole process, and the failure mode is the user's "stuck or frozen" report. It is also the standard hostile-input class the tree's own test suite already polices for name/role lengths (`Session.HostileNameAndRoleLengthBombs` exists; no count-cap test does).

## Reproduction / verification

Core-side verified first-hand at `3799dee2`: the `g32()` read with no cap (`:349`), the value-initialized vector (`:372`), the container caps that do not cover the field (`cr_crsf.cpp:304-306`), and the ABI catch boundary (`cr_ui.cpp:135-143`). Og-side: the per-node factory reader shape per the record-4 artifact (the count-bounded-by-bytes property follows from sequential consumption; no count pre-allocation appears in the artifact's reader grammar). Not executed to OOM this session (the execution leg is the fix plan's first test, which runs the crafted record under the existing test harness with a small process rlimit); the allocation arithmetic is mechanical (4.29e9 × 8 B = 34.4 GB).

## Fix plan (for the engineering team)

1. Bound the pre-allocation by the wire length: each node consumes at least `MIN_NODE_BYTES` (10 by the current grammar — parent id 4 + flags 2 + marker 1 + origin 1 + deleted 1 + confidence 1; grade byte optional) — so `if (count > (len - 4) / MIN_NODE_BYTES) return 0x50040002;` (corrupt-count failure, the container's resync layer then handles recovery as it does for other record-level corruption).
2. Alternatively (or additionally, for defense in depth): grow `by_id` lazily — reserve a modest initial capacity and `resize` as records parse (the deferred-parent resolution at :355-359 already tolerates out-of-range parents, so a late-growing index is mechanically compatible).
3. Files: `src/core/session/cr_records.cpp` (the count gate or the lazy growth).
4. Tests: `Session.HostileCountBombRefused` (a validly-framed record with count = 0xF4240000 and 3 real nodes → the load fails fast with the corrupt-count code, no allocation spike — assert via a rlimit-guarded child or by instrumenting the vector's max size in a debug hook); `Session.CountExactlyAtWireBoundLoads` (the boundary: count == (len-4)/10 with a full node stream parses).
5. Acceptance criteria: no session load in the suite ever attempts an allocation larger than the record payload's node bound; the existing truncated-tail and hostile-name tests stay green.

## Tests to add

- `Session.HostileCountBombRefused` — RED today (the allocation happens).
- `Session.CountAtWireBoundLoads` (the boundary pin).
- `Session.TornTailCountStillFailsCleanly` (the M-131-class producer: a torn write that lands a garbage count — load fails with the record-level error, process survives).

## Related findings

- M-131 (FIXED — the truncate-in-place torn-write producer on the save side; this finding is the read-side complement).
- N-058 (FIXED — the deferred-parent/truncated-tail load semantics this path shares).

## Open questions

None — the arithmetic and both reader shapes are directly established; the only choice is the gate vs lazy-growth form (the gate is the smaller change and matches the container's existing fail-and-resync discipline).

---

## Resolution

**Status:** FIXED
**Fixed in commit:** the M-150 S80-lane commit (this session; SHA in the commit footer)
**Verified by:** Session.HostileCountBombRefused / Session.CountAtWireBoundFailsCleanlyBeyondIt / Session.TornTailGarbageCountStillFailsCleanly — RED first-hand via the stash dance (HostileCountBombRefused + TornTailGarbageCountStillFailsCleanly FAILED at the pre-fix tree: the count-scaled value-initialization threw before any node parse), GREEN post-fix; the full Session suite 29/29 (the truncated-tail and hostile-name guards stay green); full suite 1627/1627.
**Resolution notes:** The wire-length bound landed (the fix plan's option 1, defense-in-depth per option 2 deemed unnecessary — the gate is exact): each node consumes ≥ 10 wire bytes by the grammar (parent 4 + nflags 2 + marker 1 + origin 1 + deleted 1 + confidence 1), so a count beyond (len-4)/10 cannot fit the record and is refused instantly with ERR_SESSION_FORMAT (0x50040001 — the corrupt-count class shares the truncated-count failure's semantic; the container's resync layer recovers as it does for other record-level corruption). The og's per-node factory reader shape (rsf-record4--tree-node-grammar.md:16,26 — allocation tracks the bytes actually parsed) is restored at the count field: no count-scaled allocation ever happens on the load path. Files: src/core/session/cr_records.cpp (the gate before by_id), tests/unit/test_session.cpp.

---

## S82 convergence addendum (2026-10-29 — a second independent gate, conceded per SA-9; the stronger bound recorded)

A third lane (S82) implemented the gate independently before pulling:
the same wire-bound shape, but with the grammar's TRUE mandatory prefix
(78 bytes: parent 4 + nflags 2 + marker 1 + origin 1 + deleted 1 +
confidence 1 + record_ref 8 + fs_kind 4 + logical 8 + allocated 8 + the
four times 32 + attributes 4 + flags 2 + name_len 2 — the serialize and
deserialize field walks agree) instead of the finding's conservative
10-byte floor, and the distinct 0x50040002 corrupt-count code. Withdrawn
pre-push per SA-9 (the racing `f7432fb4` keeps the seat; the two gates
are functionally equivalent for the finding's population — both refuse
any wire-impossible count before the allocation). The 78-byte
derivation is recorded here as the tighter bound the next audit may
adopt when the 10-byte floor's residual zone (counts in ((len-4)/78,
(len-4)/10] — still record-bounded, at most 8×len/10 bytes of index)
wants closing; the error-code split (0x50040001 shared vs 0x50040002
distinct) is a registry-taste question both codes' uses in cr_crsf.cpp
support.
