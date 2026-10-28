# SEVERE S-047 — the MPEG-TS / M2TS carve family is absent: the og's two frame-sync scanners (188/192-byte packet auto-detect, 0x47 sync, 20 KiB minimum extent, x10 cap) are fully decoded — core has no TS analyser at all

**Severity:** SEVERE
**Module / Surface:** carve / the S-007 batch-2 remainder (modules/carve.md)
**Parameters affected:** A1 (detection correctness), A2 (recovery correctness — recorded video never carved), A28-adjacent (the list-layer sync-scanner family), B19-adjacent
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:3143-3149` (CMTSAnalyser id 0x76 registration, list layer) and `:3173-3179` (CM2TSAnalyser id 0x74); the chunk handler `:35578-35677` (CMTS: chunk length must exceed 0x1FF; packet-size auto-detection — sync 0x47 at +4 with byte@+5 in {0x40,0x41,'P',0x10} → 192-byte BDAV/M2TS packets (this+0x110 = 0xc0); sync 0x47 at +0 with byte@+1 in the same set → 188-byte TS packets (0xbc)); the packet walk `:38457-38536` (strides the detected size, verifies 0x47 at packet start (+4 for 192-mode), ends the run on sync loss committing the accumulated extent, marks all-0xFF null packets, checks up to 0x400 packets per chunk, flushes every 0x20 synced packets, **minimum extent 0x5000 (20 KiB) or size=0/reject**, cap = controller global x 10, extension "M2TS" stamped for id 0x74). Census context: the batch-1 addendum's "MPEG-TS: numeric key 284 semantics undecoded" was a mis-lead — 284 (0x11C) is the FreeHand 11 key (BruteForce::CFH11Analyser, :1951-1962), which core already implements as FMT_FH11; the TS analysers are list-layer and have no magic key (session-8 erratum E54).
**Core references:** audited commit `ff879f9` — `src/core/carve/cr_registry.cpp:370-389` — FMT_MPEGPS (00 00 01 BA) and FMT_MPEGES (00 00 01 B3) exist; there is no TS row (no 0x47-sync analyser, grep-verified across src/core/carve); cr_signatures.h has no FMT_TS/FMT_M2TS.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** S-007 batch-1 addendum deferred MPEG-TS with the cause "numeric key 284 semantics undecoded". This session decoded it: 284 is FH11 (already implemented), the TS analysers are list-layer sync scanners with a fully decoded grammar. No prior finding covers the TS absence.

## Summary

MPEG-TS (.ts) and M2TS (.m2ts/.mts — AVCHD camcorder footage, broadcast captures) are absent from core's carve reach: no 0x47-sync scanner exists. The og implements the family as two list-layer frame-sync analysers (CMTS id 0x76 / CM2TS id 0x74) whose grammar this session decoded end-to-end: first-chunk packet-size auto-detection (188 vs 192 byte strides by sync position), per-packet 0x47 verification, sync-loss run commitment, a 20 KiB minimum extent, and a x10 cap. The batch-1 deferral cause ("numeric key 284") is resolved as a census mis-lead. The finding formalizes the batch-2 remainder per the session-8 mandate.

## What the reference does

The two list-layer analysers (registered at :3143-3149/:3173-3179) generate their own candidates by chunked scanning rather than magic-key dispatch. CMTS's chunk handler (:35578-35677) requires a >512-byte first chunk and auto-detects the packet size from where the 0x47 sync sits (+0 → 188-byte TS, +4 → 192-byte BDAV/M2TS, with a {0x40,0x41,'P',0x10} byte following the sync). The shared packet walk (:38457-38536) strides the detected size verifying 0x47 at each packet boundary, treats an all-0xFF u64 as a null packet, commits the accumulated extent on sync loss, enforces a 20 KiB minimum (else reject with size 0), caps at the controller's global carve size x 10, and stamps the M2TS extension for the 0x74 analyser.

## What our core does

Core carves MPEG-PS (00 00 01 BA) and MPEG-ES (00 00 01 B3) only. No analyser scans for the 0x47 sync pattern, so .ts/.m2ts/.mts files — including AVCHD camcorder video, one of the highest-value recovery targets in the og's video category — are never carved. There is no partial support: the family is wholly absent.

## Gap

Falsifiable: a 64 KiB stream of 188-byte TS packets (0x47 sync every 188 bytes, payload bytes in the accepted set) is carved by the og (CMTS, size >= 0x5000 accepted); core carves nothing — no 0x47-sync scanner exists.

## Impact

Recovery correctness: camcorder/broadcast video recovery is a flagship consumer scenario; whole families (AVCHD .mts/.m2ts, broadcast .ts) are silently unrecoverable in deep scan. Carve-reach report: list-carve-formats omits a live og family. The sync-scanner architecture (candidate generation by the analyser itself, not by magic-key dispatch) is also the enabler for the remaining list-layer formats — implementing the shared walk unblocks several batch-2 rows.

## Reproduction / verification

Decompile-direct: the registrations, the auto-detect, and the packet walk were read this session at the cited lines and are quoted in the corpus field; the packet-size constants (0xc0=192, 0xbc=188) and the accepted byte set {0x40,0x41,0x50,0x10} are verbatim from the decompile. Fix verification should synthesize a 0x47-strided stream fixture and require the carve with the exact minimum-extent semantics.

## Fix plan (for the engineering team)

1. Implement a list-layer sync-scanner seam in the carve engine (candidate generation by the analyser — the og's architecture for the family; batch-1's static gates only cover the keyed dispatch). 2. Add FMT_TS and FMT_M2TS ("ts"/"m2ts", display "MPEG Transport Stream"/"AVCHD M2TS"). 3. Implement the shared packet walk per the decode: auto-detect 188/192 by sync position + the following byte set, per-packet 0x47 verify, null-packet (all-0xFF) tolerance, run-commit on sync loss, min extent 0x5000, cap x10, M2TS extension stamping for the 192-byte family. 4. Wire the two ids into kTrialOrder (the static_assert membership gate). 5. Update the S-007 batch-2 addendum to point here.

## Tests to add

- `Carve.MpegTs188PacketStreamAccepted` — 0x47 every 188 bytes, 64 KiB, carves as FMT_TS with the walked extent.
- `Carve.M2Ts192PacketStreamAccepted` — sync at +4 per packet, carves as FMT_M2TS with the "m2ts" extension.
- `Carve.TsBelowMinimumExtentRejected` — a 4 KiB synced run rejects (min 0x5000).
- `Carve.TsSyncLossCommitsRun` — sync loss after >= 20 KiB commits the partial extent (salvage semantics).
- `Carve.TsNullPacketTolerated` — all-0xFF null packets do not end the run.
- Hostile: random 0x47-sparse buffers must not fire (the min-extent + per-packet verify gates); ASan over the chunk walk.

## Related findings

S-007 (batch-2 parent); S-041 (school 1, landed `b92ebef`) (the other SEVERE batch-2 row); M-098 (HIK — the og's other MPEG-PS-derived list analyser); erratum E54 (the "numeric key 284" mis-lead correction).

## Open questions

The CMTS filename template (:35641-35668) and the deep validator FUN_18002e6b0 were partially decoded (the sync-count condition at param_5+0x14 < 9 is unresolved) — the landed implementation should keep the decoded gates and record the deep-validator tail as a follow-up decode, mirroring the batch-1 discipline for undecoded size walks.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** `a3abfbe`
**Verified by:** `Carve.MpegTs188PacketStreamAccepted` (a 0x47-strided 188-byte stream carves as FMT_TS with the exact walked extent), `Carve.M2Ts192PacketStreamAccepted` (the BDAV 192-byte family carves as FMT_M2TS with the "m2ts" extension stamp), `Carve.TsBelowMinimumExtentRejected` (a 4 KiB synced run rejects — the 0x5000 gate), `Carve.TsSyncLossCommitsRun` (salvage semantics: the committed run at the last complete packet), `Carve.TsNullPacketTolerated` (all-0xFF null packets keep the run; the "+null" evidence marker), `Carve.TsSparseSyncNoiseRejected` (non-strided 0x47 sprinkles never fire). Full suite 1043/1043 green (Linux release); UBSan and ASan lanes clean over the carve surface (117 tests each); the fuzz_carve harness covers the new validators via the registry (the clang libFuzzer lane runs on CI — no clang in this container).
**Resolution notes:** The corpus was re-read first-hand before the fix: the registrations (CMTS id 0x76 at :3143-3149, CM2TS id 0x74 at :3173-3179), the chunk handler FUN_18002e430 (:35578-35677 — the >0x1FF chunk gate, the auto-detect byte-set {0x40,0x41,'P',0x10} checked at +4 FIRST then +0, the 0xc0/0xbc stride constants), and the packet walk FUN_180032460 (:38457-38536 — per-packet sync verify at the stride, the all-0xFF null-packet marker at +8..+15, run-commit on sync loss, the 0x5000 minimum, the controller-global × 10 cap, the M2TS extension stamp for id 0x74). Architecture: the og's keyless list-layer scanners port onto the core's keyed dispatch through the 1-byte 0x47 magic — FMT_TS anchors the sync at +0 (188-byte stride), FMT_M2TS at +4 (192-byte stride, the TP_extra_header in front), mirroring the og's own detect order (the +4 check first; M2TS precedes TS in kTrialOrder). Each family's grammar rejects the other's streams within two packets (the strides drift apart immediately), so the N-070 best-pick resolves shared positions without a dedicated arbitration layer. Both rows carry suppress_overlap (the repeating-sync family — the MPEG-audio precedent): mid-stream syncs inside an emitted stream are the same stream, exactly the og's one-walker-run-one-file posture. The walk is block-buffered per the P1-12 discipline with a 2 KiB FIRST block: the 1-byte magic means every 0x47+follow-byte pair in the window reaches the walk, and noise candidates must die for 2 KiB of traffic rather than a full 1 MiB probe block (the run needs 109+ packets to clear 0x5000, so the small block can only cheap-reject — measured: the sparse-noise hostile fixture stays at 54 ms for 1,769 candidates). Min extent 0x5000 / cap = 200,000,000 × 10 (the og's controller-global × 10, the DMG family's 0xBEBC200 global being the same constant). The undecoded residuals stay recorded as the finding's open questions: the CMTS filename template (:35641-35668) and the deep validator FUN_18002e6b0's sync-count condition (param_5+0x14 < 9) — neither blocks the landed grammar, and the null-packet flag's og consumer is undecoded, so the "+null" marker rides the N-089 evidence note without altering acceptance/size semantics.

---

## Post-landing decode addendum (FUN_180032670 — the CM2TS probe; same session)

While verifying the M-092/M-093 text-school decodes, the second TS function was read first-hand (`RAWSearchDLL_decompiled.c:38556-38619`): the CM2TS **probe** (not the size walk) requires TWO consecutive syncs (byte@+0xC4 for 192-mode, byte@+0xBC for 188-mode — the next packet's sync) AND non-null first packets (`param_2[1] != -1 && param_2[0x19]/+0xC4 != -1` — the payload qwords at +8 and at the second packet's +8), plus a RawConfig gate (FUN_180010f50 with the DAT_18006be6c string) on the 188-mode branch. Net deltas vs the landed grammar:

1. **Two-sync confirm** — acceptance-equivalent with the landed walk (any run reaching the 0x5000 minimum has two consecutive syncs; sub-minimum runs reject under both).
2. **Null-first-packets reject (the M2TS row only)** — the og's CM2TS probe REJECTS candidates whose first two packets are null (all-0xFF payload qwords), even though the og's own size walk tolerates null packets mid-run. A real broadcast/camcorder stream that begins with null-padding packets (common) is therefore MISSED by the og's M2TS probe. The landed core walk keeps the walk's tolerance on both rows — a deliberate superset registered as **PARITY-DELTAS D-107** (recall-over-strictness; the CMTS chain in the og accepts the same streams).
3. **The CM2TS 188-mode config gate** — the og's M2TS analyser can also claim 188-stride streams when a RawConfig extension toggle (DAT_18006be6c) is enabled; the core's FMT_M2TS row is anchored at the +4 sync (192-mode) only, and 188-stride streams always claim FMT_TS ("ts"). The stamped-extension difference on 188-stride streams under that undecoded toggle posture is immaterial to recall (CMTS carves them either way) and is noted here rather than ported (the toggle's shipped default is undecoded — the RawConfig artifact names 34 toggles without per-analyser defaults).

None of these changes the landed grammar's acceptance for real streams; the finding stays FIXED with this addendum as the residual decode record.
