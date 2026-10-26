# MINOR N-078 — truncated carve_note record loads as OK (the only corrupt-record exit that leaves c.fail unset)

**Severity:** MINOR
**Module:** session
**Parameters affected:** S36 (session load error semantics), D12 (session container fidelity)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rsf-format--scan-session.md` (the rsf-record4 per-node validation model — N-058's landing); the N-058 finding's own rule text: "a stream that failed is an error even when the node was created before the failure surfaced"
**Core references:** `src/core/session/cr_records.cpp` `deserialize_tree` — the `nflags & 16` carve_note length check (`if (c.left < size_t(nl) * 2u) break;`) was the ONLY corrupt-record exit that broke without setting `c.fail`
**Status:** FIXED (filed + fixed in the same session — NEW-FINDING discovered during the DFX17 baseline leak repair; renumbered N-077→N-078 when the parallel session-22 lane's own leak finding landed first under N-077)

## Summary

While repairing the LeakSanitizer baseline red (the `FileProperties` leaks from `deserialize_tree`'s corrupt-record early exits — filed by the parallel session-22 lane as N-077, fixed first by their PropsRef landing), the new five-path regression test caught a second, SEMANTIC defect on one of those paths (uncovered by the N-077 leak landing itself): a record whose carve_note length field overstates the remaining bytes breaks the load loop WITHOUT setting `c.fail`, so the container loads as **OK** with the final record silently missing — a silent partial load that violates the N-058 rule the loader itself established ("a stream that failed is an error even when the node was created before the failure surfaced").

## What the reference does

The og's rsf-record4 reader validates per-node and fails the record on structural corruption (the N-058 landing's decoded model: marker validation, per-node bounds checks before allocation, failure surfaced to the caller). The corpus container layer treats a failed record as an error condition the CRC/resync layer can see — never as a clean load.

## What our core did (pre-fix)

Every other corrupt-record exit sets `c.fail` (the name-length bomb and role-length bomb sites set it explicitly; the resident-data and sha128 sites go through `Cursor::bytes()`, which sets it). The carve_note length check at the M-051 provenance block was written as a bare `break` — the single site that forgot. Result: `deserialize_tree` returned `OK` with the last record dropped (and, pre-guard, leaked its `FileProperties` too).

## Gap

Falsifiable claim: a session blob whose last record carries `nflags & 16` with a carve_note length larger than the remaining bytes loads as OK (return code 0) instead of `0x50040001`. Verified directly by the new regression test's case (4) — it failed against the pre-fix tree returning 0 where the error code was expected.

## Impact

Silent partial load on a corrupt session file — the caller believes the session is intact; the final provenance-carrying carved-file record is missing from the tree with no error, no degraded-count, nothing. Same corruption class in any other field (name, role, resident, sha) correctly fails the load.

## Reproduction / verification

`Session.CorruptRecordEarlyExitsReleaseProps` case (4): a hand-crafted single-node blob with `nflags=16`, carve_note length 8, only 2 note bytes present. Pre-fix: `deserialize_tree` returned `OK` (0). Post-fix: `0x50040001`. Found first-hand in this session's baseline-repair work.

## Fix plan (executed)

1. Set `c.fail = true` before the break (root cause: the site's missing fail propagation, not the caller's handling).
2. Regression coverage: the five-path corrupt-record test drives this exact path under the ASan/LSan gate (which also proves the props-leak guard).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-078 commit — union on top of the parallel N-077 PropsRef landing)
**Verified by:** `Session.CorruptRecordEarlyExitsReleaseProps` case (4) — fail-before EXECUTED (returned OK=0 on the pre-fix tree, exactly the finding's falsifiable claim); pass-after returns `0x50040001`. Full suite 614/614 + 21/21 UI + ABI check green, LeakSanitizer clean.
**Resolution notes:** One-line root-cause fix (`c.fail = true` before the break — the same pattern every sibling corrupt-record site already used), discovered and fixed during the DFX17 baseline leak repair in the same function. ID note: this leg first drafted the finding as N-077 (semantic half) while the parallel session-22 lane filed the leak half under N-077 and landed first — this finding renumbered to N-078, the ID collision recorded here for the audit trail. The LEAK half is the parallel lane's N-077 (their PropsRef guard; this leg's equivalent PropsGuard dropped in the union per first-landing-wins — both guards are functionally identical: release-on-every-early-exit, disarm at handoff). The new five-path regression test (Session.CorruptRecordEarlyExitsReleaseProps) pins BOTH behaviors: it drives all five exits under the sanitizer gate AND asserts the error code on every path — on the leak-only landing (N-077 without this fix) its case (4) fails with rc == OK, which is exactly how this defect was caught.
