# MINOR N-116 — the session-list row ordering diverges: the og sorts LastScanRecordView rows by the embedded scan time (FILETIME comparator) — core byte-sorts by full path, so "row 0 = most recent" host assumptions break

**Severity:** MINOR
**Module / Surface:** session / ABI (the W1a cr_ui_session_list verification)
**Parameters affected:** B17-adjacent (sort model — the ordering is the row contract), A42-adjacent (the two-call pattern's row semantics)
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:432112-432142` (FUN_140273550 — the row comparator: SystemTimeToFileTime over the row+0x20 SYSTEMTIME and FILETIME comparison — **sort by embedded scan time**); `:432549` (the introsort call over the 0x70-stride rows); the enumeration `:432259-432359` (FindFirstFileW over the SaveScan folder, auto_* filter, skip dirs); the per-file record-2 read `:432410-432554` (light records only — record 3/4 never opened).
**Core references:** audited commit `ff879f9` — `src/ui/cr_ui.cpp:381-453` (list_crsf_files — `std::sort` by **full path bytes**, :428 Win/:450 POSIX); `src/app/rrecov.cpp:948` (the CLI sorts rows again by path); the W1a landing record (worklog.md:737) lists "byte-sorted caller-owned POD rows" as the contract (cr_ui.h:864).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** W1a landed (78a6552/eb16335, API-DELTA S3-W1a DONE) citing rsf.md:11 + UIUX/07:21 — those cites support the list's existence and light-read semantics, not the sort key. The decompile-side sort semantics were never read before landing; this session read them first-hand.

## Summary

The W1a session list matches the og on the load-bearing halves (light records only, record-2 fields, embedded save time) but orders rows differently: the og sorts by the embedded scan time (newest-first semantics for "Continue last scanning"), core sorts by path bytes. A host that renders row 0 as the most recent session (the og's banner behavior) gets a different answer than the og's list.

## What the reference does

The og builds its rows then introsorts them with the FILETIME comparator over the record-2 SYSTEMTIME (DRWUI:432112-432142, 432549) — the list is time-ordered, which is what makes the "Continue last scanning" banner pick the right file.

## What our core does

cr_ui_session_list's rows come back path-byte-sorted (cr_ui.cpp:428/450) and the CLI re-sorts by path (rrecov.cpp:948); the contract text (cr_ui.h:864) documents the byte order as if it were the semantics.

## Gap

Falsifiable: two sessions A (older, name sorts first) and B (newer, name sorts second) — the og returns [B, A]; core returns [A, B].

## Impact

Host-side ordering assumptions (the resume banner, the session picker); the divergence is user-observable in any host that maps the og's list onto a UI.

## Reproduction / verification

Decompile-direct (the comparator read verbatim this session) + core source read; both cited. No executed comparison available in this container.

## Fix plan (for the engineering team)

1. Decide the contract: either (a) match the og — sort rows by save_unix descending inside cr_ui_session_list (ties broken by path bytes for determinism) — or (b) keep path order and register it as a deliberate delta in PARITY-DELTAS with the banner guidance ("hosts pick the max save_unix row"). The og-match is the lower-friction fix (the rows already carry save_unix). 2. Update cr_ui.h:864's contract sentence + ABI-CONTRACT §7f + the rrecov sort. 3. Keep the deterministic tie-break documented.

## Tests to add

- `SessionList.OgTimeOrdering` — two sessions out of name order return newest-first; RED pre-fix if the og-match is chosen.
- `SessionList.TieBreakDeterministic` — equal save_unix → path order.
- `SessionList.CliOrderingAgrees` — the verb and the ABI agree.

## Related findings

N-091 (the other W1a/b/c shaping deltas — the registration finding); N-117 (the doc-drift set).

## Open questions

Whether the og's comparator is newest-first or oldest-first (the introsort direction is in the comparator's return convention — one more read of FUN_140273550's caller expectation; the landing fix should pin it with a runtime check or a corpus probe).

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-116 commit — the session-38 minor-first lane)
**Verified by:** `CrUi.SessionListOgTimeOrdering` + `CrUi.SessionListTieBreakDeterministic` + `CrUi.SessionListCliAndAbiOrderAgree` (all new) — OgTimeOrdering EXECUTED RED on pre-fix main (row 0 came back the path-first OLD session), GREEN after (row 0 = the path-LAST NEW session, stamps ordered); full suite 1041/1041 green (linux-release, gcc Werror); clang-format 18.1.8 clean.
**Resolution notes:** Re-verified at current main first: list_crsf_files sorts by path bytes (cr_ui.cpp, both Win and POSIX branches), session_list_impl filled rows in that order, the CLI re-sorted by path (rrecov.cpp:964's own comment pinned the wrong contract) — all three confirmed. The corpus comparator re-read FIRST-HAND this session and the finding's open question is now settled: FUN_140273550 (DRWUI_decompiled.c:432112-432142) converts row+0x20 SYSTEMTIME to FILETIME for both rows and returns !(ftA < ftB) — for the container's less-than insert semantics that is NEWEST-FIRST (the ordering that makes the og's "Continue last scanning" banner pick the right file); the side-calls to FUN_140273610 in both branches are an if/else decompiler artifact (identical bodies) and carry no ordering meaning. The fix (the finding's option (a), the og-match): session_list_impl now decodes EVERY candidate's light summary (as the og reads every record-2), sorts by save_unix DESC with a deterministic path-ascending tie-break (equal stamps and zero-stamp rows - pre-v5 sessions, unreadable files), and hands the host the newest n rows - so the TRUNCATED under-capacity answer is the time-ordered prefix, not an arbitrary path prefix; cr_ui.h's two-call contract and ABI-CONTRACT §7f re-worded to the new row-order semantics; the CLI's sessions verb re-sorts its rows by the same key after the path-sorted enumeration (the enumeration sort stays as the deterministic input; its misleading old comment corrected). Tests: the ordering test isolates two sessions in a dedicated directory with the NEWER one path-sorting LAST (path order and time order disagree by construction, 1.2 s apart so the stamps differ); the tie-break test copies one session under two names; the CLI-agreement test pins the verb's source (the exe is Windows-only - the pin asserts the time-key re-sort exists and the stale path-order contract comment is gone).
