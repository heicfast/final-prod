# MINOR N-027 — Carve/writer minors (session-2 group)

**Severity:** MINOR (grouped) · **Modules:** carve, writer · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** D13, B10, O38, P42, A28
**Audited commit:** `e93decf` · corpus `03ad46c`

**N-014a — Duplicate boundary emission at scan end.** `src/core/orchestrator/cr_scan.cpp:577-578` calls `emit_boundary_events_(…)` twice back-to-back (both lines from 56cce87). FilesFound events double-emit at teardown (ErrorRange is CAS-deduped). Benign counters/noise; a one-line deletion. Test: `ScanEvents.SingleBoundaryAtTeardown`.

**N-014b — Carved-file naming split inside the core.** The orchestrator names carve results `Carved%06u.%s` (`cr_scan.cpp:1053-1058`) while the raw-sign layer hardcodes `File%06u.%s` (`cr_carve.cpp:265`) — one scan can emit two name families. The og uses a single UI-callback policy ("File0001.jpg", `artifacts/SearchDLL/rawsearch-carving--deep-dive.md:75`). Fix: one namer, one format (recommend the og's `File%04d` shape only if display parity is wanted; at minimum eliminate the split). Test: `Carve.SingleNamingFamily`.

**N-014c — Runtime per-format gating config surface.** og: `config/config.ini` key `only_common` gates whole-map dispatch (`FUN_18000cf60 :9766-9842`) and `RawConfig.ini [Filter]` per-extension toggles (`GetPrivateProfileSectionA("Filter", …)` `:13121+`; shipped defaults disable STL/VPJ carving). Core `CarveFlags` has no per-format enable/disable. Fix (optional): a data-driven per-format toggle read from the shipped config; register the absence as a reviewed choice if not wanted. Tests: `Carve.FormatToggleHonored`.

**N-014d — ZIP padding semantic inversion (refines N-008).** og `FUN_1800176e0 :18462-18500`: absorbs **non-empty trailing sectors after a broken record chain** (salvage; break condition `[s-4]==0 && [s-8]==0 && [s-12]==0 && [s-16]==0`, cap ~4 GiB via `0x200000000×0x14`), and **never inflates a cleanly-ended archive**. Core `cr_analysers.cpp:641-655` absorbs **zero 16-byte blocks after the EOCD end** (cap `end + 5,242,880`) — inflating intact ZIPs with zero slack while offering no broken-chain salvage. N-008's (a)/(b) claims confirmed; its direction claim corrected here. Also: og version gates are two-tier (quick `:18288-96` `≤0x13 || ==0x62`; full first-record `≤10` + namelen ∈ [1..259] `:18389-99`); core has none. Fix: adopt the og's trailing-16-of-sector zero test on the salvage path and drop the zero-block padding absorption. Tests: `Carve.ZipIntactNotInflated`, `Carve.ZipBrokenChainSalvaged`, `Carve.ZipVersionGates`.

**Related:** A-004 (same 56cce87 code) · S-007/OQ-T8 resolution (dispatch + signature table) · N-008 (the original ZIP finding — see its session-2 update)

**Open questions:** None.
