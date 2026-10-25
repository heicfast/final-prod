# MINOR N-031 — carve dispatch deltas: og u32→u16→linear→sign-DB order, sector-relative stride, zero-sector skip, only_common gate

**Severity:** MINOR
**Module:** carve
**Parameters affected:** A25 (engine algorithm steps — dispatch order), A28 (validator logic), B1 (detection surface on 4Kn), X47/X50 (scan cost), C19 (registry keys)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` :9385-9633 (scan loop FUN_18000c820), :9730-9930 (dispatch FUN_18000cf60), :9933+ (u16 path FUN_18000d3e0), :10068+ (sign-DB fallback FUN_18000d670); disassembly at VA 0x18000cadc-0x18000cb34 proving the dispatch's return value and the NULL→fallback edge
**Core references:** audited commit `34b3441` — `src/core/carve/cr_carve.cpp` :77-184 (grammar layer: first-byte dispatch over `kTrialOrder`), :186-301 (raw-sign layer as the second pass, `emitted_at` ownership), :28 `WINDOW = 1 MiB`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's carve dispatch is now fully decoded (this resolves OQ-T8's mechanics): per sector it tries the u32 4CC registry (exact match on the first 4 bytes, only when non-zero), then the u16 registry (first 2 bytes), then a linear walk of every analyser, and only when all of that misses does it fall to the raw_signs.json layer. The core's layering (grammar validators first with a first-byte dispatch; raw-sign layer second, honoring grammar-owned positions) is **semantically equivalent** on the layering question. Four mechanical deltas remain: the og strides by device sector size (4096 on 4Kn, not our fixed 512), the og fast-skips all-zero sectors, the og has a `config/config.ini [User] only_common` gate we lack, and within-grammar collision order is registry-chain order rather than our specificity table.

## What the reference does

- **Dispatch order** (FUN_18000cf60): (1) read `config/config.ini`, section `User`, key `only_common` (once-flag); (2) if not only_common and the first u32 is non-zero, `std::map<u32, Analyser*>` exact lookup; the node's analyser chain is tried (vt+0x40 fast validator, else vt+0x10 Analyse; acceptance recorded); (3) u16 registry lookup (FUN_18000d3e0, skipped when the first u32 is zero); (4) linear iteration of the entire registry; (5) on total miss, the caller (scan loop at VA 0x18000cadc: `call 0x18000cf60; test rax,rax; je → FUN_18000d670`) invokes the raw_signs.json sign-DB probe.
- **Stride**: one probe per **device sector** (sector size from the storage device; 512 default, 4096 on 4Kn volumes).
- **Zero-skip**: leading/trailing zero-dword metrics computed per sector (first u32 zero → count 4; a backward 7-dword walk) feed a fast-skip for all-zero sectors.
- **Gate**: the only_common setting restricts dispatch to the linear common chain — a runtime product toggle.

## What our core does

- Grammar layer: a first-byte filter over a fixed specificity-ordered table (`kTrialOrder`, 34 formats; DOCX>ZIP, CR2>DNG>TIFF) at **every byte** offset in the window (`cr_carve.cpp:78-93`).
- Raw-sign layer: a second pass at **512-byte stride** regardless of device geometry (`:192`), skipping positions the grammar layer emitted (`emitted_at`), with the highest-confidence sign owning the position.
- No zero-sector skip; no runtime config gate (CarveFlags only).

## Gap

Falsifiable: (a) on a 4Kn device the og probes raw-sign candidates only at 4096-byte boundaries while the core probes every 512 — the core is a strict superset there (probe positions ⊇ og's), so this is not a recall loss but a behavioral and cost difference; (b) the og never dispatches analysers on an all-zero sector, the core runs its full pipeline there; (c) the og honors only_common, the core has no equivalent; (d) shared-magic grammar families resolve in registry order (og) vs specificity order (core) — e.g. the og funnels all `II*`/`MM*` TIFF variants into one analyser that classifies internally, where the core tries CR2→DNG→TIFF in table order.

## Impact

Detection surface: none proven worse for the core (its stride is finer on 4Kn; its grammar coverage is S-007's separate gap). Performance: the zero-sector skip is a real scan-cost optimization the og has on typical zero-heavy unallocated regions (X47/X50). The collision-order difference can relabel TIFF-family files (ORF/RW2/DNG vs plain TIFF) in rare dual-valid cases — user-visible naming only. The only_common gate is a missing runtime toggle (product-level, not engine-level).

## Reproduction / verification

Decompiled + disassembled evidence, first-hand this session: `evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md` §4 (scan-loop quotes, dispatch chain, the 0x18000cadc disassembly proving the NULL→sign-DB edge). Core cites at `34b3441`. The 4Kn stride difference is additionally implied by the device-vtable sector-size reads in the scan loop (:9490-9520) — not execution-verified (needs a 4Kn fixture + og run).

## Fix plan (for the engineering team)

1. **Zero-sector skip** (recommended, pure win): in `scan_one_window`, before the grammar dispatch loop, test the window's first and last u32 (or an SSE/AVX all-zero check over the stride range); if a 512-byte stride range is all zero, skip the raw-sign probe for it (the grammar layer's magic dispatch already costs almost nothing on zeros — profile before skipping it there too).
2. **Stride semantics**: document (cr_rawsigns.h) that the og probes at device-sector stride and the core deliberately probes at 512; on 4Kn the core is the superset. No code change required unless bench parity on 4Kn matters.
3. **only_common**: file under the runtime-config feature family (with RawConfig.ini per-extension toggles — see the filter-model audit) rather than fixing here.
4. **Collision order**: when S-007 adds the missing grammar formats, order the shared-magic families to mirror the og's single-analyser-internal-classification where known (TIFF family first candidate CR2, then DNG, then plain TIFF — current order already matches; keep the pin test).
5. Tests: `Carve.ZeroSectorSkipStats` (windows_skipped-style counter for zero strides, if implemented); `Carve.TiffFamilyOrder` pin (exists today — keep green).

## Tests to add

`Carve.ZeroSectorSkipStats` (if §5.1 implemented); pin `Carve.TiffFamilyOrder` already exists — verify it stays.

## Related findings

S-015 (units), N-030 (confidence), S-007 (missing grammar formats), OQ-T8 (RESOLVED — N-031 is the decode record).

## Open questions

Whether the og's u32/u16 analyser chains apply secondary ordering among chained analysers at the same key (the chain walk was decoded structurally, not per-key); low impact given the core's table covers the same formats.
