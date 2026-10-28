# MINOR N-105 — Auto-save naming: the og's folder-scan "Folder" name suffix has no core equivalent

**Severity:** MINOR
**Module:** session (auto-save naming)
**Parameters affected:** D12-adjacent (naming grammar), C19-adjacent
**Corpus references:** `DRWUI_decompiled.c` :442881-886 — the auto-name's volume part gains `L"Folder"` when the scan context's `vtable+0x110` flag is set (the folder-scan variant); naming chain :442887-900 (`auto_<vol>[_Folder]_<M>_<D>_<H>_<m>_<s>.rsf`)
**Core references:** audited `ff879f9` — `src/core/session/cr_crsf.cpp:363-403` (`auto_session_name[_at]` — label + timestamp only; no folder variant), used by cr_ui.cpp:2005 and rrecov.cpp:1703
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's auto-session name distinguishes folder scans with a "Folder"
suffix on the volume part; the core's N-061 naming has no such
variant.

## Gap / Impact / Verification

Falsifiable: a folder-scan auto-save in the og yields
`auto_<vol>Folder_<ts>.rsf`; the core yields `auto_<label>_<ts>.crsf`
regardless of scan kind. Impact: cosmetic naming difference; only
observable when (if) the core gains a folder-scan mode.

## Fix plan

When a folder-scan flag exists in ScanConfig, thread it into
`auto_session_name` (suffix before the timestamp). Until then, record
the variant as deferred in the N-061 row.

## Tests to add

AutoSessionNameFolderSuffix (when the flag exists).

## Related findings

S-044 (the auto-save gating divergence), N-100.

## Open questions

Whether `vtable+0x110` marks folder scans specifically (the flag's
exact predicate — the decode shows the condition, the mode's name is
inferred from the suffix string).

---

## Resolution

**Status:** FIXED-ALREADY
**Fixed in commit:** (landed by the parallel S-044 lane's rider; classified this session — the session-38 minor-first lane)
**Verified by:** the live-trail reproduction: PARITY-DELTAS **D-106** (read this session) is exactly this finding's demanded deferral record — "the auto-save Folder-scan naming variant (`Folder` suffix) is unimplementable until a folder-scan mode exists (S-044/N-105 deferral, REGISTERED GAP — surface absent on both engine sides)" with the og's `vtable+0x110` decode (:442883-886), the core's no-folder-scan-mode state, and the go-forward rule ("recorded so a future folder-scan landing knows the naming contract. Verdict: NEEDS-WORK (only when folder-scan lands)").
**Resolution notes:** The finding's own fix plan was "record the variant as deferred in the N-061 row until a folder-scan flag exists" — the S-044 lane landed the stronger form (a dedicated D-row in the central register, cross-referenced to N-105 by name). Nothing remained to edit beyond this classification record; `AutoSessionNameFolderSuffix` stays the test for the future folder-scan landing, exactly as D-106 prescribes.
