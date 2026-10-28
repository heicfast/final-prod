# SEVERE S-052 — LDM span recovery misbases: dynamic-volume nodes keep layout-relative extents while the SafeWriter reads a single physical disk — spanned/striped/mirrored/RAID-5 recovery serves silently wrong bytes (the scan's virtual device does not survive the scan)

**Severity:** SEVERE
**Module / Surface:** ldm (modules/ldm.md) · recovery writer (modules/writer.md) · tree data model
**Parameters affected:** A2 (recovery correctness — byte-for-byte), A8 (multi-volume/cross-partition), A43 (ownership/lifetimes — the device that owns the basis), A57 (wiring/lifecycle — the virtual device's lifetime vs the tree's), A27-adjacent (the span read path, scan half only)
**Corpus references:** `re-workspace/reversed/ghidra-core/LdmManager.mo` decompile (91,475 lines) — the volume-storage class family CONFIRMED PRESENT: `SimpleVolumeStorage`, `SpanVolumeStorage`, `StripeVolumeStorage`, `MirrorVolumeStorage`, `Raid5VolumeStorage` incl. `ComputeDataWithOtherDisk` (a crash-site string) — cited via `include/cr/table/cr_ldm.h:17-24` (the session-5 ground-truth header, first-hand re-read this session); `modules/ldm.md:47` (B8 multi-volume: spanned/mirrored/stripped/RAID-5 resolved — the SCAN-side verdict) and `:71` (A27: spanned/mirror/stripe reads MATCH — the scan-side read path through the virtual device).
**Core references:** audited commit `ea1ac86`-lineage (product tip past `26da616`; the LANE-A triage base `84a1832`, cites re-verified at the reconciliation HEAD) — `include/cr/tree/cr_node.h:400-407` (the rebase doc block: "LDM volume nodes keep their layout-relative basis (multi-disk bases are not expressible; registered limitation)"); `src/core/orchestrator/cr_scan.cpp:1496-1505` (the LDM scan loop: `table::LdmVolumeDevice vol(lay, resolver);` at `:1500` is a STACK object — it does not survive the scan); `src/ui/cr_ui.cpp:3698-3707` (`recovery_create_impl`: the F-1 basis comment at `:3698-3702`, the src selection at `:3703-3705`, the SafeWriter construction at `:3706` — the SafeWriter's `src` is the scan device or its parent disk — there is NO path that hands the writer the LDM virtual device); `src/core/recovery/cr_safewriter.cpp:131-137` + `:379/:409` (the writer reads `source_->read_at(pr->extents[ext_i].offset, …)` — a single `IStorageDevice*`, extents documented "ABSOLUTE device offsets", zero LDM awareness in the file); `src/app/rrecov.cpp:1400-1440` + `src/ui/cr_ui.cpp:1865-1899` (the cross-disk GUID resolver wiring — scan-side only); `docs/CORE-GAP-LIST.md:139` (GAP-S6-15, PARTIAL — the product doc's own registration: "recover from an LDM scan can still misbase").
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core scans LDM dynamic volumes (simple/spanned/striped/mirrored/RAID-5) correctly through a read-only virtual device that maps volume-relative reads onto the member disks via a cross-disk GUID resolver. But that virtual device is a stack object inside the scan loop and dies when the scan ends. The nodes it produced keep LAYOUT-RELATIVE extents — the F-1 rebase explicitly excludes them ("multi-disk bases are not expressible") — and every recovery consumer (SafeWriter, preview) reads through a single physical device. For any multi-disk layout, layout-relative offsets read from the wrong disk (or the wrong region of the right disk): recovered files contain silently wrong bytes. The product's own gap list registers this as PARTIAL (GAP-S6-15, "registered follow-up from the F-1 session") — but no finding on the audit board owns it, and against the og it is a parity divergence, not just an internal gap: the og's `LdmManager.mo` ships the full `SpanVolumeStorage`/`StripeVolumeStorage`/`MirrorVolumeStorage`/`Raid5VolumeStorage` family that serves these reads for the product's whole lifetime.

## What the reference does

The og's LDM subsystem (LdmManager.mo, 91,475-line decompile) reconstructs dynamic volumes as first-class volume-storage objects — `SimpleVolumeStorage`, `SpanVolumeStorage`, `StripeVolumeStorage`, `MirrorVolumeStorage`, `Raid5VolumeStorage` (the class family confirmed present in the decompile; `ComputeDataWithOtherDisk` is findable as a crash-site string, proving the cross-disk reconstruction path is live code, not a stub). These objects are the volume's read basis for the product's entire lifetime: the scan walks them AND the recovery/writer reads through them, so a file's volume-relative extents are always resolved against the correct member-disk mapping. The core's own session-5 ground-truth header records this family as the reference model (`include/cr/table/cr_ldm.h:17-24`), and the audit's scan-side verdicts (modules/ldm.md B8 `M`, A27 `M` — spanned/mirrored/stripped/RAID-5 "resolved", spanned/mirror/stripe reads MATCH) verify the core matched this behavior ON THE SCAN SIDE.

## What our core does

Scan side (correct, verified in-source this session, re-verified at the reconciliation HEAD): `cr_scan.cpp:1481-1505` — on a whole-disk scan of a dynamic disk, the orchestrator parses the LDM database, builds `table::LdmVolumeDevice vol(lay, resolver)` with the platform-provided cross-disk GUID resolver (each member disk's PRIVHEAD GUID → open device; `cr_ui.cpp:1865-1899`, `rrecov.cpp:1400-1440`), and scans the virtual device: files are found and read correctly through the span mapping.

Recovery side (the defect, verified in-source this session):
1. `LdmVolumeDevice` is a stack local in the scan loop (`cr_scan.cpp:1500`) — it is destroyed when the scan completes; nothing equivalent exists at recovery time.
2. The tree nodes it produced keep LAYOUT-RELATIVE extents: `cr_node.h:405-406` — "LDM volume nodes keep their layout-relative basis (multi-disk bases are not expressible; registered limitation)". The F-1 rebase (`rebase_new_extents`/`rebase_subtree_extents`) explicitly skips them.
3. `recovery_create_impl` (`cr_ui.cpp:3661-3667`) wires the SafeWriter's source as `d->dev.get()` — the scan device (for a whole-disk scan: the physical disk) — or `d->hold` (the parent disk) for volume slices. No code path constructs or retains a span-capable device for the writer.
4. The SafeWriter reads `source_->read_at(pr->extents[ext_i].offset, …)` (`cr_safewriter.cpp:379`) against a single `IStorageDevice*` and documents its input contract as "the extents are ABSOLUTE device offsets" (`:131-134`, the R7.2/F12 overlap check). The file has zero LDM awareness (grep `ldm|span` in cr_safewriter.cpp: only the destination-span helper's own name).
5. No guard, no refusal: nothing marks dynamic-volume nodes as unrecoverable — they enqueue and write like any other node.

The product doc itself concedes the state: `docs/CORE-GAP-LIST.md:139` (GAP-S6-15, PARTIAL): "LDM spans (RAID-0/concat across disks) need per-node multi-disk bases; recover from an LDM scan can still misbase. Registered follow-up from the F-1 session."

## Gap

Falsifiable claims:
1. A file node discovered under a spanned/striped/mirrored/RAID-5 dynamic volume carries extents relative to the virtual volume layout (cr_node.h:405-406), and the recovery writer reads those offsets against a single physical disk (cr_ui.cpp:3703-3706 + cr_safewriter.cpp:379) — for any extent that the layout maps outside the first member's identity region, the bytes read are NOT the bytes the scan saw.
2. No refusal path exists: `enqueue_common` treats dynamic-volume nodes identically to absolute-basis nodes (cr_safewriter.cpp:121-137 — the only early-out is destination overlap).
3. The destination-overlap check (R7.2/F12) compares the layout-relative offsets against the DESTINATION volume's physical disk spans — for LDM nodes this comparison is meaningless on both sides (false negatives: a genuine source-on-destination overlap can pass; false positives: innocent nodes can be refused).

## Impact

- **Recovery correctness (user-observable, silent):** users who scan a dynamic disk and recover files from a spanned/striped/mirrored/RAID-5 volume get files with silently wrong content — the worst failure class in the product (the user believes the recovery succeeded; hash-verify is on by default and will catch some cases as VERIFY_FAILED, but the failure is reported per-file AFTER a full write, and Skip-hash configurations never see it).
- **Safety:** the R7.2/F12 destination-overlap refusal — a hard safety invariant — is computed on the wrong basis for these nodes.
- **Scope:** every multi-disk dynamic layout. Simple (single-disk) dynamic volumes are the identity case of the mapping and read correctly through the self disk only when the layout-relative basis happens to coincide with the physical basis; the product doc scopes the residual to spans, and this finding adopts that scoping rather than claiming the simple case is broken.
- **Parity:** the og serves these reads correctly through its volume-storage family for the product's whole lifetime — "ReFS and LDM are first-class" per the standing rules; a silently-wrong recovery path on a first-class surface is not a registerable subset.

## Reproduction / verification

All first-hand this session (lead verification, SA-10): the five-step chain above, each link quoted with file:line in "What our core does". The corpus side is the LdmManager.mo class-family presence (cr_ldm.h:17-24's ground-truth citation, re-read this session) plus the existing scan-side MATCH verdicts (modules/ldm.md B8/A27). NOT executed: an end-to-end span misbase demo requires a two-disk fixture — the fix plan's tests specify it (the LDM test file already builds multi-disk fixtures: `tests/unit/test_ldm.cpp` "Multi-disk fixtures: RAID-5 / mirror / spanned concatenation" — the scan half is tested; the recovery half is not, which is itself the gap).

## Fix plan (for the engineering team)

1. **Make the basis expressible (architectural):** add a per-node (or per-root) basis descriptor to the tree model — minimally `struct ExtentBasis { const table::LdmLayout* layout; }` on the volume root's nodes, or a shared `IBasisResolver` handle. The F-1 rebase block (cr_node.h:389-407) gains a fourth case: LDM nodes carry their layout reference instead of a numeric base.
2. **Keep the mapping alive past scan end:** the orchestrator should move each scanned `LdmVolumeDevice` (plus its resolver's member devices) into a scan-owned registry that outlives the scan (owned by the session/snapshot, like `ldm_members` already is at `cr_ui.cpp:838` — extend that ownership to the volume devices, not just the member disks).
3. **Wire the writer:** `recovery_create_impl` (and the preview twin at `:3504-3540`) consults the node's basis: LDM-basis nodes read through the retained virtual device (correct mapping); absolute nodes read through the physical device, unchanged. `SafeWriter` itself can stay single-device if the ABI opens the right device per enqueue — the cleaner shape is a small `IMultiBasisReader` wrapper the writer receives once.
4. **Fix the overlap check:** R7.2/F12 must map LDM-basis extents to physical spans (via the layout's member extents) before comparing against `dest_spans_` — the N-013 multi-span helper already models the destination side correctly.
5. **Tests (fail-before/pass-after):**
   - `Ldm.SpanRecoveryReadsThroughVirtualBasis` — two-disk span fixture (extend the existing multi-disk fixture builder): plant a file whose extents live on disk 2; recover; assert byte-exact content (fails today: bytes come from disk 1's offsets).
   - `Ldm.StripeRecoveryRoundTrips` — striped layout, same assertion.
   - `Ldm.Raid5RecoveryWithMissingMember` — RAID-5 with one member absent: recovery must surface the reconstruct-on-failure semantics the scan side already implements (modules/ldm.md A27), not silent garbage.
   - `Ldm.OverlapRefusalMapsToPhysicalSpans` — an LDM-basis node whose physical span intersects the destination volume must be REFUSED (fails today: the layout-relative offset comparison can pass).
   - `Ldm.PreviewMatchesRecovery` — preview and recovery read the same bytes for an LDM-basis node.
6. **Contract updates:** ABI-CONTRACT §7 (recovery) gains the LDM-basis note (nodes from dynamic-volume scans recover through the span mapping; the basis is part of the session state, so a saved .crsf must persist enough of the layout to re-arm the resolver on resume — check the crsf volume-info record already persists LDM layout data; if not, extend it, append-only).
7. **Acceptance:** GAP-S6-15 flips CLOSED (CORE-GAP-LIST.md:139 + the closure order at :149 item 7); the multi-disk recovery tests green on the Windows lane; the modules/ldm.md B8/A27 rows gain the recovery-side `M`.

## Tests to add

(Embedded in the fix plan, step 5 — the five named tests.)

## Related findings

- **A-002 (FIXED)** — the GPT dynamic-disk GUID constants (the LDM parse prerequisite).
- **M-030 (FIXED)** — the 200-disk resolver cap (the cross-disk member registry this finding's fix builds on).
- **modules/ldm.md B8/A27** — the scan-side MATCH verdicts this finding complements (the recovery-side half was never scored).
- **GAP-S6-15** — the product doc's own PARTIAL registration (this finding is its audit-board owner).
- **M-072** — the query-axes finding (unrelated; ID-adjacent only in the driver-install module's stale numbering — see N-120/driver-install repairs).

## Open questions

1. Whether SIMPLE single-disk dynamic volumes read correctly through the physical-disk basis in every case (the layout-relative → physical identity assumption) — the fix plan's basis descriptor makes this explicit either way; a dedicated `Ldm.SimpleVolumeRecoveryRoundTrip` test should pin it during the same landing.
2. Whether the .crsf session format already persists the LDM layout (the crsf VOLUME_INFO record's current fields were not audited for LDM content this session) — fix-plan step 6 flags it as a check-then-extend.


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

cite-refresh map: cr_node.h F-1 block :403-425 (exclusion :420-421); recovery_create_impl cr_ui.cpp:3800-3847 (src selection :3841-3843, no LDM path); preview twins :3535-3590; arm_ldm_resolver cr_ui.cpp:1881-1917; CLI twin rrecov.cpp:1478-1510; cr_scan.cpp:1500 the stack-object death (exact); cr_safewriter.cpp:379/:409/:132-138 (exact); CORE-GAP-LIST.md:139 GAP-S6-15 PARTIAL (exact). All five chain links verified standing.
