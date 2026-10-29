# Documentation inventory — Session-84 Phase 0 (baseline e8dcaba8)

Agent D (Task T2-D) — documentation inventory + first-pass accuracy flags.
Repo: `/home/z/my-project/core_recovery`, branch `main`, HEAD `e8dcaba8` (2026-10-29).
Tracked `.md` files inventoried: **622** (the task brief said 621; `git ls-files '*.md'` returns 622 at this HEAD — one file was added since the brief was written).

## Method

1. **Full census** — `git ls-files '*.md'` → 622 files. A bash loop (`/tmp/docinv.sh`) emitted `path | git log -1 --format="%h %cs %s" (subject ≤80ch) | first heading from the first 30 lines` for every file into `/tmp/doc_inventory.tsv`; that TSV was programmatically converted into the Full inventory table below (category **A** = deep-read, **B** = skimmed long tail).
2. **Deep reads (77 files, category A)** — root contract/status docs (12), `docs/` core contracts (17), `docs/parity/*` (8), `docs/adr/` (1), `diff-to-fix/` top-level protocol docs (5), the two conflict-marker finding files, and all 33 `diff-to-fix/modules/*.md`. Full or near-full reads for the root/docs contracts; head + status-banner + verdict-table reads for the uniform module/parity audit family.
3. **First-pass accuracy probes** — cheap greps against `src/`, `include/`, `tests/`, `.github/workflows/`, `tools/`: symbol existence (≈25 sampled symbols), file-existence for every doc-cited path, count derivations re-run through the repo's own `tools/doc_truth_gate.py` logic (exports/error-codes/event-types), conflict-marker scan over all tracked `.md`, commit-existence checks for cited hashes.
4. **Conflict-marker sweep** — `git grep -E '^(<{7}|={7}|>{7})( |$)' -- '*.md'` over the whole tree; every hit opened and classified (committed corruption vs. deliberate quote inside a code fence).
5. **Flags** — MATCHES-CODE (probes agree with the doc at HEAD or the doc is a correctly pinned/dated historical audit), SUSPECT (specific claim contradicted/drifted), STALE (documents a removed surface or carries outdated counts without a supersession banner), CONFLICT-CORRUPTED (committed git conflict markers).

## Deep-read docs

### Root

#### README.md
- Scope: product overview, quick start, architecture diagram, status, doc index. Last commit `41d88297` 2026-10-28.
- Claims: nine filesystem engines; 139-format signature table; presets `win-x64-release`/`linux-release`; verbs list/scan/tree/recover/health; `ci-local.sh --san/--fuzz`; 4-lane CI; worklog/ is the session-prompt archive "the chronological worklog.md was retired 2026-10-28"; cites GAP-P1-1 / N-036 as open gaps.
- Flag: **MATCHES-CODE** (probes: presets exist in CMakePresets.json; all 6 core verbs present in src/app/rrecov.cpp; 139 formats pinned at include/cr/carve/cr_rawsigns.h:5 and src/core/carve/cr_carve.cpp:930; `--san/--fuzz` at ci-local.sh:10-11; every linked doc exists).
- Note (mild): "worklog.md was retired 2026-10-28" — worklog.md in fact received the HEAD commit itself (`e8dcaba8`, Session-84 open) and carries committed conflict markers (see findings); "retired" is not accurate as "no longer committed".

#### RELEASE-NOTES.md
- Scope: vNext + v0.4.1-parity release notes. Last commit `0f5d57fb` 2026-10-28.
- Claims: CI is FOUR lanes (M-088 correction of the "three" text); FloorVersion pinned at two live sites both v0.2.4; driver pin v0.2.2 in the API-benchmark lane (v0.4.1-era); 926/926 tests at tag; rda-test-sign.cer + MANIFEST.json assets; honest gaps GAP-P1-1/N-036/ZFS multi-vdev/HFS+ UNVERIFIED.
- Flag: **MATCHES-CODE** (4 workflows exist: checks/release/test-benchmark/installer.yml; FloorVersion v0.2.4 at .github/workflows/release.yml:291 and installer.yml:118; M-120's test_carve.cpp conflict markers — the notes' "green" caveat class — are gone: 0 markers at HEAD).

#### CONTRIBUTING.md
- Scope: contribution contract — local gate, build lanes, test/gate inventory, commit style. Last commit `8943993c` 2026-10-27.
- Claims: `ci-local.sh`/`ci-local.ps1` + `.githooks` pre-commit/pre-push; 4 build presets; "no warning suppression without a docs/BUILD-SUPPRESSIONS.md row (currently zero entries)"; CI includes "Linux + TSan + UBSan + the ONE Windows MSVC lane, the libFuzzer campaign".
- Flag: **SUSPECT** (two claims drifted):
  - "currently zero entries" is false — BUILD-SUPPRESSIONS.md carries row S-1 (landed `05e67863` 2026-10-28, two days after this doc's last commit).
  - CONTRIBUTING.md:63-67 describes the pre-consolidation Linux/TSan/UBSan/fuzz CI lanes as live, while README:125-137, RELEASE-NOTES.md:20-37 and CI-CURRENT.md say the ubuntu sanitizer/fuzz lanes were dropped (windows-only directive). CI-LOCAL.md carries an explicit M-088 supersession banner; CONTRIBUTING.md does not.
  - Everything else verified: .githooks/pre-commit + pre-push exist; presets exist; tools/win_syntax_check.sh exists.

#### SECURITY.md
- Scope: BitLocker legitimate-unlock-only policy, consent model, transport boundary, Win7 floor. Last commit `22e96576` 2026-10-26.
- Claims: `src/win/cr_bitlocker.cpp` narrow surface; both CLI+ABI pre-unlock gate (M-033); no FVEK extraction; `tools/verify_pe_um.py` Win7 import gate.
- Flag: **MATCHES-CODE** (cr_bitlocker.cpp exists; verify_pe_um.py + tools/test_verify_pe_um.py exist; M-033 surfaces live: `--bitlocker-fvek`, `cr_ui_open_volume_with_key`, `DecryptingDevice` — 5 src files).
- Note: "no decryption code at all" is NOT claimed here (that's the stale F13 row in error-fix-registry.md); SECURITY.md's "user-FVEK in-stack XTS-AES decryption" phrasing in README is consistent.

#### AUDIT.md
- Scope: Session-5 verification & reconciliation audit (2026-10-23), module audit matrix rows 1-49, rejected findings, fix waves. Last commit `148cf8bf` 2026-10-23.
- Claims: resolution-status banner lists landing commits 58091ba/8084cbb/bc51e3b/32b31b5/2b340d5/9b61246/f63d471/af4bc8e/0db53c8; 4 verification rounds (ZFS/btrfs-ext4/LDM+SafeWriter+NTFS/merged-tree).
- Flag: **MATCHES-CODE** (all 9 cited commits verified present with matching subjects; the doc is explicitly dated/pinned — a historical audit record, superseded operationally by diff-to-fix/ but not misleading).

#### AUDIT-ARCH.md
- Scope: Refactor-A deep architecture-parity audit, pinned at `08b44ac`. Last commit `d8f68083` 2026-10-25 (the N-041 resolution edit).
- Claims: 18-row module-boundary matrix; Dimension-2 layering "PASS (0)" for core-includes-OS-headers; dead-code register 26 entries; 13 findings ARCH-1..13 (ARCH-1 RdaClient dead → RESOLVED N-041 deleted); architecture.md:80-81 "save-on-cancel" false-at-pin.
- Flag: **MATCHES-CODE at its pin** — but one central claim no longer holds at HEAD: Dimension 2's "core must not include OS/win headers → PASS (0)" is contradicted by `src/core/session/cr_lastscan.cpp:12` (`#include <windows.h>` under `#ifdef _WIN32`, landed with M-154 in `b2bc1970` 2026-10-29). Also affects docs/architecture.md (see findings). RdaClient deletion verified (only comment mentions remain: include/cr/win/cr_rda.h:89,120; src/win/cr_transport.cpp:6,353,369).

#### AUDIT-FALLBACKS.md
- Scope: Refactor-B fallback/degraded-path census, pinned `08b44ac`, 2026-10-23. Last commit `ffd9dc84`.
- Claims: verdicts REMOVE/MATCH/RETAIN-AS-SAFETY per fallback site; FB-T06/FB-T07 REMOVE verdicts against the (then-live) RdaClient silent-degrade branches; transport duality confirmed as og behavior with decompile pins.
- Flag: **MATCHES-CODE (historical, pinned)** — FB-T06/T07's subject code was deleted by N-041; the doc is an audit trail input, not a current-state claim.

#### AUDIT-FAT.md
- Scope: Refactor-D FAT12/16/32+exFAT deep-parity audit, pinned `08b44ac`, 2026-10-24. Last commit `ffd9dc84`.
- Claims: 14-row parameter table with MATCH/MISMATCH verdicts (FAT-1..10, EXFAT-1..10 findings), dead-code register (20 items), UNVERIFIED register (11), fix order.
- Flag: **MATCHES-CODE (historical, pinned)** — verdicts were the input queue for the W2 fix wave; CAPABILITY-MATRIX rows FAT/exFAT DONE cite FAT-1..10/EXFAT-1..6 as fixed.

#### AUDIT-LINUX-FS.md
- Scope: Refactor-E ext4/HFS+/Btrfs/ZFS/ReFS deep-parity audit, pinned `2ea5d45`. Last commit `5de2e2b0` 2026-10-29.
- Claims: per-engine MATCH% (ext4 90, HFS+ 85, Btrfs 70, ZFS 92, ReFS 75); ReFS full MSB chain confirmed first-class; UNVERIFIED register 11 items.
- Flag: **MATCHES-CODE (historical, pinned)** — the M-142 hfsplus fix rider landed in `5de2e2b0` (the last commit), consistent with its own text.

#### AUDIT-NTFS.md
- Scope: Refactor-C NTFS deep-parity audit, pinned `08b44ac`, 2026-10-24. Last commit `c741160f` 2026-10-25.
- Claims: parameter-by-parameter table (constants/fixups/runlist/MFT walk), dead-code register, 10 UNVERIFIED items, priority action list (NTFS-1..).
- Flag: **MATCHES-CODE (historical, pinned)**.

#### AUDIT-PIPELINE.md
- Scope: Refactor-F pipeline-parity audit (carve/lost-partition/sessions/writer/health/RawFixer/preview/query/report/LDM), pinned `08b44ac`/`be8875f`. Last commit `a4bd4052` 2026-10-28.
- Claims: raw-sign table byte-exact 139/199/62 vs og raw_signs.json; suite 179/179 green at pin; FileFilter.xml 1,481 unique extensions (doc-cited 1,489 needs recount); CARVE-2/3/9 gaps open at pin.
- Flag: **MATCHES-CODE (historical, pinned)** — the 139/199/62 counts re-confirmed by cr_rawsigns_data.cpp constants (kFormatCount=139) cited in CAPABILITY-MATRIX row 63.

#### worklog.md (root)
- Scope: the chronological session worklog (12k+ lines). Last commit `e8dcaba8` 2026-10-29 (HEAD — Session-84 open).
- Claims: per-session narrative trail.
- Flag: **CONFLICT-CORRUPTED** — committed markers at lines 12069/12135/12168 (`<<<<<<< HEAD` … `=======` … `>>>>>>> 09a18f9f`), the same S82 racing-merge block as PARITY-DELTAS. Also contradicts README's "retired 2026-10-28" (still committed at HEAD).

### docs/ — core contracts

#### docs/ABI-CONTRACT.md
- Scope: normative cr_ui C ABI contract — ownership, threading, errors, events, stability. Last commit `0ba5f14e` 2026-10-29 (M-073).
- Claims: header wins on disagreement; §7a carve-format registry; §7f–7k session-list/load-events/autosave/thumbnail/image-create/folder-filter surfaces; §8 SQLite sidecar REJECTED-BY-REFERENCE; §9 append-only stability policy; 91 exports implied via API docs.
- Flag: **MATCHES-CODE** — spot-checked surfaces exist (`cr_ui_carve_format_count/info`, `cr_ui_session_list`, `cr_ui_thumbnail_generate`, `cr_ui_apply_lost_partition`, `cr_ui_scan_busy/stopping` — all present in include/cr/cr_ui.h); the previously-false §8 SQLite claim is now explicitly REJECTED-BY-REFERENCE (A-ABI-2 executed).

#### docs/API.md
- Scope: single entry point for every programming surface (CLI, cr_ui ABI, C++ core). Last commit `497b1277` 2026-10-29.
- Claims: CLI grammar (11 verbs); exit codes 0-4; `cr_ui_abi_version()` = 3; "91 exported functions … 92 `CR_UI_API` lines minus 3 `#define` macros"; threading contract at cr_scan.h:110-113; consent-bearing flag list.
- Flag: **SUSPECT (two drifts)**:
  - CLI grammar omits the `archive-list` and `image` verbs that exist (src/app/rrecov.cpp:3904 `if (cmd == L"archive-list")`, :3914 `if (cmd == L"image")`) — CORE-API-CURRENT correctly counts 13 verbs.
  - "92 CR_UI_API lines" is wrong arithmetic: the header has 94 CR_UI_API-containing lines, 3 are #defines → 91 declarations (headline 91 is correct and doc_truth_gate-enforced; the "92" intermediate figure was not updated when two racing export appends landed — exactly the class `497b1277` was repairing).
  - Otherwise verified: ABI v3 (cr_ui.h:81), `rrecov info` never existed ("info" only as bench mode, rrecov.cpp:3009), exit-4 driver gate documented in DRIVER-REQUIRED.md.

#### docs/CORE-API-CURRENT.md
- Scope: current public API surface census with per-module inventories. Last commit `497b1277` 2026-10-29.
- Claims: headline counts — C++ public headers **82**; C ABI **91 exports** (94 CR_UI_API lines − 3 #defines); CLI **13 verbs**; error codes **45 + OK**; errmod **13**; ABI event types **12**; supersession banners for pre-driver-only sections.
- Flag: **SUSPECT (one count)**:
  - "82 headers under include/cr/**" is false at HEAD: `git ls-files 'include/cr/**/*.h' | wc -l` = **91** (79 at the original `22e9657` pin; the header count is NOT guarded by doc_truth_gate, unlike the other five counts which I re-derived through the gate's own logic: exports=91 ✓, codes=45 ✓, events=12 ✓, errmod=13 ✓, verbs=13 ✓).
  - Appendix directory census is stale (diff-to-fix "344 files" → 459 tracked files now; worklog "47 files" → 125) but sits under explicit historical banners.

#### docs/API-SURFACE-AUDIT.md
- Scope: the og's 37 decoded API operations vs the cr_ui ABI, session core-api 2026-10-26. Last commit `497b1277` 2026-10-29.
- Claims: 91 exports (same "92 lines" formula as API.md); A-ABI-1..6 findings all EXECUTED/DONE; ops #7/#8 MATCH (carve enumeration); op #20 MATCH on 6-of-7 groups (tag NOT-EXPOSED); M-023 (category taxonomy) remains WRONG/engine-scope.
- Flag: **SUSPECT (same nit)** — "92 CR_UI_API lines − 3 #defines" (actual 94); A-ABI-3's "(86 at the 2026-10-29 re-derive)" is historical phrasing (exempt). All §5 DONE items verified present in code.

#### docs/CAPABILITY-MATRIX.md
- Scope: per-capability status vs the reference — the "single source of truth". Last commit `ed14732f` 2026-10-29.
- Claims: driver prerequisite DONE+ (D-DRV-1..4); NTFS LZNT1 DONE (N-074) with XPRESS/LZX MISSING (GAP-P1-1 "shared limitation"); ReFS/LDM first-class DONE; HFS+ attributes/journal DONE (S-002/S-003/S-021); apply/boot-chain/check/RawFixer DONE rows; S-050 file-repair line EXCLUDED; 4-lane CI row.
- Flag: **MATCHES-CODE** with two editorial defects:
  - The "Lost-partition discovery" row appears TWICE verbatim (lines ~56 and ~58) — a duplicated table row.
  - Its GAP-P1-1 cross-reference framing ("MISSING (GAP-P1-1)") vs CORE-GAP-LIST's GAP-P1-1 row "CLOSED (464d8e5)" — the LZNT1 half is closed (verified: `lznt1_payload_decode`, `expand_compressed_data`, test `NtfsVolume.Lznt1MsXcaDocumentedVector` at tests/unit/test_ntfs_volume.cpp:2414); the XPRESS/LZX half has no row of its own, so README's "GAP-P1-1 … open" phrasing now cites a CLOSED gap id. The capability statement itself (XPRESS/LZX not implemented) is true — grep finds no xpress codec in src/core/ntfs.
  - Symbol probes 10/10: query_driver_status, cr_ui_driver_status, DecryptingDevice, cr_ui_open_volume_with_key, cr_ui_unlock_volume_with_dictionary, cr_ui_apply_lost_partition, bitlocker-fvek/dict, and 9 cited test files all exist.

#### docs/PARITY-DELTAS.md
- Scope: AUTHORITATIVE deliberate-divergence register, ~150 `### D-xx` rows in sections A→Z+. Last commit `b2bc1970` 2026-10-29 (the S82 registration batch itself).
- Claims: registry rule "a delta that is not in this registry is a bug, not a delta"; renumber ledgers for born-collided D-seats (gate-checked); D-155..D-162 = the S82 batch rows.
- Flag: **CONFLICT-CORRUPTED** — committed markers at **lines 4677, 4678, 4682, 4707, 4741, 4742, 4743, 4863** (nested/duplicated blocks, ~187 lines). The S82 rows (D-155 I-048 combined, D-160/D-161/D-162 renumbered registers, D-158/D-159 scope/posture decisions) exist ONLY inside the corrupted tail — both sides of the racing `09a18f9f` merge are present, and the two conflict copies disagree on register numbering (this file's racing block says D-156/D-157; CROSS-REF-MATRIX's and worklog.md's racing blocks say D-161/D-162 for the same registers). Also a pre-existing duplicate seat `### D-52` (twice). This is the highest-severity doc defect in the repo: the authoritative register's tail is unresolvable without git archaeology.

#### docs/BUILD-SUPPRESSIONS.md
- Scope: the warning-suppression table with rationale/scope. Last commit `05e67863` 2026-10-28.
- Claims: "**This file intentionally carries ZERO entries**" (line 7) — then carries row **S-1** (cr_thumbnail.cpp stb_image.h pragma fence, MSVC 4100/4189/… + gcc -Wmaybe-uninitialized etc.).
- Flag: **SUSPECT (self-contradiction)** — the banner text was not flipped when S-1 landed in the same commit `05e67863`. The S-1 row itself MATCHES code (src/core/preview/cr_thumbnail.cpp:34-45 pragma warning/GCC diagnostic push wrapping only the vendored stb include). Two cross-docs repeat the stale "zero" claim: CONTRIBUTING.md:75 ("currently zero entries") and docs/CI-LOCAL.md:46 ("carries zero rows").

#### docs/CI-CURRENT.md
- Scope: the current 4-lane windows-only CI contract. Last commit `41d88297` 2026-10-28.
- Claims: 4 lanes (checks/release/test-benchmark/installer) with triggers; FloorVersion pinned at two live sites both v0.2.4 (doc-truth-gate-enforced); checks-lane components 1-11; scripts/assets inventory.
- Flag: **MATCHES-CODE** with one stale filename: line ~153 cites `tools/gen_fat16_junk_fixture.py` — the tracked file is `tools/gen_junk_gate_fixture.py` (checks.yml:318 cites the correct name; CI-SCENARIOS.md is cited as the detail record). All other cited files verified: 6 .github/scripts/*, benchmark-matrix.json, bench/seed/generate_seed.ps1, bench/baseline-candidate.json, bench/root-causes.json, tools/doc_truth_gate.py, tools/validate_report.py.

#### docs/CI-LOCAL.md
- Scope: the local validation entry point + CI mirror mapping. Last commit `41d88297` 2026-10-28.
- Claims: M-088 CI-shape supersession note at top; 10 components; hooks; driver-only lint forbids `rda_available`/`win_pe_environment`/`open_harddisk_volume`/`prefer_rda` returning; "docs/BUILD-SUPPRESSIONS.md carries zero rows".
- Flag: **SUSPECT (one claim)** — "carries zero rows" is stale (S-1 exists). The driver-only lint claim verified: the four primitives appear only in comments (cr_transport.h:77, cr_winenv.h:30, cr_transport.cpp:13/441/540). Hooks and entry points exist.

#### docs/PARAMETERS.md
- Scope: every tunable with defaults and bounds. Last commit `995ba5dc` 2026-10-28.
- Claims: scan flag table incl. `--bitlocker-fvek`/`--bitlocker-dict`; `bench … [--no-rda]`; ScanConfig defaults (weights 40/35/25); engine constants (139 raw signs, caps 256/12/48/1024/80); consent list.
- Flag: **STALE** — (1) line 51 still lists `--no-rda` as a live bench flag; it is REMOVED (rrecov.cpp:2932/3517 comments; API.md:53 documents the removal); (2) the CLI table misses post-2026-10-28 surfaces: `--filter-folder` (M-073, rrecov.cpp), `--autosave-dir`, `--pause-stdin`, the `driver` verb and its `--service`/`--json` flags, and the `archive-list`/`image` verbs — despite the title "every tunable".

#### docs/FORMATS.md
- Scope: format guide — engines, containers, carving, .crsf, report, repair artifacts. Last commit `1f9f41cf` 2026-10-28.
- Claims: per-family coverage rows (all DONE in CAPABILITY-MATRIX terms); Btrfs re-scan cache OFF-by-default caller-supplied (D-89); UDF recognition-only (M-044); .crsf chunked-zlib CRC records.
- Flag: **MATCHES-CODE** (engine sources exist per family; cr_btrfs_cache.h present; consistent with CAPABILITY-MATRIX and AUDIT-* cross-refs).

#### docs/error-fix-registry.md
- Scope: reference design defects corrected in this rewrite (F1-F16). Last commit `046889d7` 2026-10-23.
- Claims: 16 rows; F13 "BitLocker path … By directive: detection + legitimate Windows unlock only (R8); **no decryption code at all**".
- Flag: **STALE** — F13's "no decryption code at all" was true at writing (2026-10-23) but M-033 later landed in-stack XTS-AES decryption with a user-supplied FVEK (`DecryptingDevice` in 5 src files; `--bitlocker-fvek` CLI; `cr_ui_open_volume_with_key` ABI; documented in SECURITY.md-era README, API.md:117, CAPABILITY-MATRIX §3). The row was never updated or annotated.

#### docs/ground-truth-contracts.md
- Scope: anti-hallucination anchor — verified constants/structures/algorithms with corpus cites. Last commit `e58bde10` 2026-10-27.
- Claims: §0 two-stage recognition contract (N-050) with per-family probes in `probe_family`; §1-§11 per-domain contracts; LDM §6 note "v0.2 shipped recognition only, the full decode landed with session 5+".
- Flag: **MATCHES-CODE** — the ARCH-5 stale "recognition only through v0.2" wording was reworded to historical form (line 119); probe_family exists (cr_scan.cpp); recognition seam logging (N-048) present.

#### docs/roadmap.md
- Scope: version roadmap v0.1/v0.2 shipped, v0.3 candidates, deferred. Last commit `2dd6e011` 2026-10-23.
- Claims: v0.2 DONE list incl. "Recovery benchmark (benchmark.yml)" and "EaseUS comparison (comparison.yml)"; LDM row DONE with landed note; v0.3 candidates (wizard UI landed core-side; file repair; WinPE builder…).
- Flag: **STALE (mild)** — benchmark.yml/comparison.yml workflows no longer exist (dropped in the 2026-10-28 windows-only CI consolidation; CI-CURRENT.md records the deletion). Read as v0.2 history it is accurate, but nothing points the reader at the consolidation; last commit predates 6 days of landings (M-073, S-050 exclusion, M-154…).

#### docs/UI-READINESS-PLAN.md
- Scope: plan closing G-01…G-13 with per-issue design/acceptance/status. Last commit `1f08c20d` 2026-10-25.
- Claims: status IMPLEMENTED for G-01/G-02/G-02b; #1 LANDED (2a93c45) "63-export surface"; design baseline "rrecov scan passes CancelToken* = nullptr and never saves on cancel".
- Flag: **STALE (in part, unbanned)** — the "never saves on cancel" baseline line was superseded by S-057/N-211 (stop-window auto-save now lands; the doc_truth_gate `check_header_contract_phrases` exists precisely to keep the retired phrase out of headers — rrecov.cpp was the third site). The "63-export surface" count is historical-at-landing (fine). No supersession banner on the baseline section, unlike CORE-API-CURRENT's banners.

#### docs/CORE-GAP-LIST.md
- Scope: live code-verified open-gap register. Last commit `046889d7`-lineage (2026-10-23; verified current rows).
- Claims: 67 GAP- mentions; GAP-P1-1 row marked **CLOSED (464d8e5)** with the LZNT1+XPRESS/LZX silent-corruption description; "GAP-P1-1 is the only recovery-output P1 still open" phrasing in CAPABILITY-MATRIX §4.
- Flag: **MATCHES-CODE** — commit 464d8e5 exists ("feat(ntfs)!: GAP-P1-1 - LZNT1 decompression + the compressed-$DATA envelope"); tests present. Cross-doc tension: README/CAPABILITY-MATRIX still use "GAP-P1-1" as the open-XPRESS/LZX id while the gap-list closes GAP-P1-1 — the XPRESS/LZX residue has no fresh gap id.

### docs/parity/* (8 files) and docs/adr/

All eight `docs/parity/PARITY-*.md` are read-only parity-audit trail documents from the Parity-A/B waves (2026-10-15..28), each pinned to an old core commit (`2473c03`/`d42bb00`/`183dfcd`-lineage) — flags: **MATCHES-CODE (historical, pinned)**, with two nits:
- `PARITY-CORE-FS2.md` — header says "**Date:** 2025 (session)" (a typo for 2026) and pins a foreign path (`/home/z/my-project/repos/core_recovery` working tree, unpinned hash) — the weakest pin discipline of the set.
- `PARITY-ARCH.md` / `PARITY-ORCH.md` — their layering/coverage claims were re-verified later by AUDIT-ARCH (which corrected PARITY-ARCH:152's RdaClient claim); the corrections are recorded inside AUDIT-ARCH, not retro-edited — consistent with the trail's append-only convention.
- `PARITY-DELTAS-draft.md` — explicitly marked FIRST DRAFT, superseded by docs/PARITY-DELTAS.md.
- `docs/adr/ADR-UI-SNAPSHOT.md` — ACCEPTED, cites `include/cr/tree/cr_snapshot.h` — file exists. **MATCHES-CODE**.

### diff-to-fix/ — protocol docs

#### diff-to-fix/README.md
- Scope: the parity audit-trail roadmap with per-session banners (S69/S79 newest). Last commit `04fb5e88` 2026-10-29.
- Claims: board at S69 filing "32 active / 169 FIXED / 4 tracked-open; next free IDs M-155+, N-217+, I-049+, A-016+"; liveness battery 28/28; S69/S79 filing lists (M-149..M-154, N-211..N-216, I-048…).
- Flag: **MATCHES-CODE (trail)** — on-disk: findings/FIXED = 197 files, active finding files = 10 (findings/MAJOR 8 + findings/MINOR 2), tracked-open = 4; board numbers are census-of-record, not file counts (combined/renamed findings explain the delta); every spot-checked finding file (I-048, M-120, M-151-family) exists.

#### diff-to-fix/SUMMARY.md
- Scope: executive summary, newest-first session deltas (S79 banner on top, S83/S82/S81… below). Last commit `0cf6142d` 2026-10-29.
- Claims: latest board "0 ACUTE · 1 SEVERE (S-068) · 17 MAJOR · 10 MINOR · 1 INFO active / 176 FIXED / 4 tracked-open"; A-016/N-218 filed+fixed; M-073 conceded-verified; M-156 X3F correction.
- Flag: **MATCHES-CODE (trail)** — with an observation: the board's 29 active findings vs 10 active finding FILES on disk (the remainder live only as ledger/matrix rows — consistent with the "remove-the-fixed" mandate but worth a board-count reconciliation pass); S79-on-top ordering reflects seat-renumber chronology, not session number.

#### diff-to-fix/SURFACE-INVENTORY.md
- Scope: gate document — corpus-derived surface inventory with audit-coverage statuses. Last commit `ed14732f` 2026-10-29.
- Claims: 282 inventoried corpus docs → session-9 rewalk "274 artifact files / 244 unique basenames; 92 cited by literal basename / 110 in any form / 134 never-cited"; NEVER-AUDITED rows retired session-by-session.
- Flag: **MATCHES-CODE (corpus-side unverifiable from this repo)** — internal currency passes are self-consistent; the EuShare "5 artifacts was wrong (2)" correction is recorded in place.

#### diff-to-fix/CROSS-REF-MATRIX.md
- Scope: module × parameter verdict matrix, newest-first lane sections. Last commit `b2bc1970` 2026-10-29.
- Claims: per-lane verdict rows (S83 M-156 X3F union acceptance; S81 N-145/N-216/N-213/N-215/N-211 FIXED; S80 M-155 marker repair; S69 deep-dive rows).
- Flag: **CONFLICT-CORRUPTED** — committed markers at **lines 39-46**: the M-154 row exists in BOTH forms (`DIVERGENCE-MAJOR` open on the HEAD side vs `MATCH — M-154 FIXED (S80 lane)` on the racing side), and the racing block's register numbering (D-161/D-162) disagrees with PARITY-DELTAS' racing block (D-156/D-157). Rows immediately after the conflict are duplicated verbatim (M-151/N-182 rows appear twice). Committed by `b2bc1970`.

#### diff-to-fix/FIXED-LEDGER.md
- Scope: master index of resolved findings (476 rows; "264 finding rows + 20 resolved OQ rows at session-39 open"). Last commit `b2bc1970` 2026-10-29.
- Claims: every ID navigable with severity/module/fixing commit; purge convention documented.
- Flag: **CONFLICT-CORRUPTED** — committed markers at **lines 656-669** with a nested/triple-marker structure (`<<<<<<< HEAD` at 656, `=======` at 662 enclosing only a `---`, `>>>>>>> 0fac15de` at 665, then more rows, `>>>>>>> 09a18f9f` at 669): HEAD side carries I-048/M-152/N-145 rows, racing side carries I-048/M-111/M-114 rows — the S82 batch's ledger rows are stranded inside the conflict.

#### diff-to-fix/findings/FIXED/I-048-three-registers-selected-dead-sumary-remote-ext-size-32bit.md
- Scope: the I-048 finding file (three registers). Last commit 2026-10-29 lineage.
- Flag: **CONFLICT-CORRUPTED** — markers at lines 52-62 (the same `09a18f9f` racing block).

#### diff-to-fix/findings/FIXED/M-120-committed-conflict-markers-red-main.md
- Scope: the MAJOR finding that documented committed conflict markers in tests/unit/test_carve.cpp (1fb7603b era).
- Flag: **MATCHES-CODE (historical)** — it QUOTES markers deliberately inside a ```cpp fence (not corruption); its subject markers are repaired at HEAD (0 markers in test_carve.cpp; last commit `0cf6142d` "fix(carve): M-107's CRW member…"). The doc correctly predicted the recurrence class — which has now recurred (see findings: the S82 batch committed markers in 5 files).

### diff-to-fix/modules/*.md (33 files)

Uniform per-module audit-verdict worksheets; all last-committed 2026-10-25..29 (current at HEAD). Flags below are first-pass; no corruption found in any module file.

| Module file | Last commit | Scope / key claims | Flag |
|---|---|---|---|
| modules/abi.md | f392d27d 10-29 | cr_ui ABI verdicts | MATCHES-CODE (surfaces verified via cr_ui.h probes) |
| modules/apply.md | d7856484 10-29 | consent-gated lost-partition apply (cr/apply, D-98a..d) | MATCHES-CODE (cr_apply.h, test_apply.cpp exist) |
| modules/boot-repair.md | 532c6811 10-29 | M-040 boot-chain family (bcdfix/nvramfix/bootfix) | MATCHES-CODE (test_bcdfix/test_nvramfix exist) |
| modules/btrfs.md | 86dadf2b 10-28 | Btrfs engine verdicts | MATCHES-CODE (historical verdicts, pinned) |
| modules/carve.md | 0cf6142d 10-29 | carve verdicts — the M-107/M-156 X3F+CRW walks, 139-sign table | MATCHES-CODE (cr_analysers.cpp/cr_rawsigns_data.cpp cited symbols exist) |
| modules/check-phase.md | 370d6595 10-28 | M-041 damage-audit check phase | MATCHES-CODE (cr_fscheck.*, test_fscheck.cpp) |
| modules/cli.md | 842a587e 10-29 | rrecov CLI verdicts | MATCHES-CODE |
| modules/driver-install.md | bb2cd8a0 10-28 | packaging/installer surface | MATCHES-CODE (installer.yml exists) |
| modules/err.md | d122d5d5 10-28 | error registry + logging seam (N-048) | MATCHES-CODE (cr_log.h seam exists) |
| modules/exfat.md | 2d614cf2 10-29 | exFAT verdicts (S-067 clamp FIX lineage) | MATCHES-CODE |
| modules/ext4.md | 67d2bf25 10-29 | ext4 verdicts (M-151/M-152) | MATCHES-CODE |
| modules/fat.md | 9b4a9302 10-29 | FAT verdicts (S-068 lineage) | MATCHES-CODE |
| modules/health.md | c7312269 10-28 | health verdicts; S-024 doc-truth sub-item (cr_health.h:8 claim vs issued set) | MATCHES-CODE |
| modules/hfsplus.md | 5de2e2b0 10-29 | HFS+ verdicts (M-142 rider landed in same commit) | MATCHES-CODE |
| modules/imaging.md | f392d27d 10-29 | S-030 image targets | MATCHES-CODE (image stack in include/cr) |
| modules/ldm.md | 1a0cb035 10-29 | tables+LDM; N-104 fixed the dangling `table.md` ref | MATCHES-CODE |
| modules/lost-partition.md | 1ecd0875 10-29 | lost-partition verdicts (M-149 ghost-score) | MATCHES-CODE |
| modules/mom-capabilities.md | 4f44db49 10-28 | never-audited MOM/product surfaces | MATCHES-CODE (historical census) |
| modules/ntfs.md | 5e1dca99 10-29 | NTFS verdicts (716L, the largest) | MATCHES-CODE |
| modules/orchestration.md | 033c5d7c 10-29 | scan orchestrator verdicts (503L) | MATCHES-CODE |
| modules/orchestrator.md | 92ae836c 10-29 | 27-line S81 addendum (N-216) with NO H1 | SUSPECT (editorial): near-duplicate filename of orchestration.md; an addendum file without its own title — likely should have been a section of orchestration.md |
| modules/preview.md | 4298a48d 10-29 | preview verdicts (M-028/M-116) | MATCHES-CODE (cr_preview.* exists) |
| modules/query.md | 59a431d3 10-29 | query verdicts; starts with "## S12 addendum", no H1 | MATCHES-CODE (minor editorial: no H1) |
| modules/rawfixer.md | cf137fc6 10-25 | RawFixer verdicts | MATCHES-CODE (pinned historical) |
| modules/refs.md | 36c34557 10-29 | ReFS verdicts (S-020/M-022 attribute layer) | MATCHES-CODE |
| modules/reghive.md | 602dd170 10-28 | offline registry-hive module | MATCHES-CODE (include/cr/reghive/* 5 headers) |
| modules/session.md | b2bc1970 10-29 | session verdicts; M-154 lifecycle | MATCHES-CODE with duplication fallout: TWO near-identical "M-154 FIXED" addenda present (S80 at :495 and Session-82 at :523) — racing-lane residue; cr_lastscan.cpp/LastScanInfo landed (verified) |
| modules/storage.md | 1a0cb035 10-29 | storage/slice/device verdicts | MATCHES-CODE |
| modules/transport.md | 995ba5dc 10-28 | win transport verdicts (post N-041) | MATCHES-CODE |
| modules/tree.md | f392d27d 10-29 | result-tree verdicts (N-166/N-218 live_files contract) | MATCHES-CODE |
| modules/visualization.md | c55bd459 10-25 | query/preview/disk-map verdicts | MATCHES-CODE (pinned) |
| modules/writer.md | bf097882 10-29 | writer + report verdicts (WRITER-1/2 closed) | MATCHES-CODE |
| modules/zfs.md | aab031ac 10-28 | ZFS verdicts (S-034 ring) | MATCHES-CODE |

## Full inventory

622 tracked markdown files. Columns: path | last commit touching it (`%h %cs %s`, subject truncated) | scope one-liner (first heading, truncated) | category (A = deep-read, B = skimmed long tail).

| Path | Last commit | Scope | Cat |
|---|---|---|---|
| AUDIT-ARCH.md | d8f68083 2026-10-25 fix(N-041): dead-surface deletion — RdaClient + structured-open… | AUDIT-ARCH — Deep Architecture-Parity Audit (Refactor-A) | A |
| AUDIT-FALLBACKS.md | ffd9dc84 2026-10-23 audit(parity): six deep-parity audits + PARITY-REFACTOR-PLAN ga… | AUDIT-FALLBACKS — Refactor-B fallback/degraded-path census | A |
| AUDIT-FAT.md | ffd9dc84 2026-10-23 audit(parity): six deep-parity audits + PARITY-REFACTOR-PLAN ga… | AUDIT-FAT.md — Refactor-D: FAT12/16/32 + exFAT deep-parity audit (read-only) | A |
| AUDIT-LINUX-FS.md | 5de2e2b0 2026-10-29 fix(hfsplus): M-142 — the VH validator's fork clamps replaced w… | AUDIT-LINUX-FS — ext4 / HFS+ / Btrfs / ZFS / ReFS deep-parity audit | A |
| AUDIT-NTFS.md | c741160f 2026-10-25 fix(N-045): scan-cache budget+algorithm registered as PARITY-DE… | AUDIT-NTFS.md — Refactor-C: NTFS deep-parity audit (read-only) | A |
| AUDIT-PIPELINE.md | a4bd4052 2026-10-28 docs(truth): N-180 - the doc-truth staleness batch re-truthed (… | AUDIT-PIPELINE.md — Refactor-F pipeline-parity audit (carving, lost partitions, sessions,… | A |
| AUDIT.md | 148cf8bf 2026-10-23 docs(audit): session-5 take-over verification record - 4 rounds… | AUDIT.md — Session 5 Verification & Reconciliation Audit | A |
| CONTRIBUTING.md | 8943993c 2026-10-27 ci(rebuild)!: the prebuilt-artifact CI lands - benchmark lanes … | Contributing to core_recovery | A |
| README.md | 41d88297 2026-10-28 fix(M-088): one FloorVersion floor + the 4-lane CI-shape doc tr… | core_recovery | A |
| RELEASE-NOTES.md | 0f5d57fb 2026-10-28 trail(M-117): the renumber record + the rebase SHA repairs - th… | core_recovery release notes | A |
| SECURITY.md | 22e96576 2026-10-26 docs(v0.4.1-parity): the OQ-S4-3 transport verdict transcribed … | Security policy | A |
| worklog.md | e8dcaba8 2026-10-29 worklog(session): Session-84 opened — the post-remediation cohe… | ## Session-79 S79 lane — the S-068 seat conceded (the FOURTH independent implementation; … | A |
| bench/README.md | 68aa5b78 2026-10-29 fix(docs): N-150 - the API doc-truth batch landed (counts re-de… | Comparative Benchmark Harness (ours vs og EaseUS DRW free) | B |
| diff-to-fix/API-DELTA.md | f392d27d 2026-10-29 repair(trail): the FOURTH born-collided D-seat repair per SA-9 … | API INVENTORY / DELTA — core_recovery vs the research corpus (API scope) | B |
| diff-to-fix/CORPUS-ERRATA.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | CORPUS-ERRATA.md — corpus errata candidates surfaced by diff-to-fix audit | B |
| diff-to-fix/CROSS-REF-MATRIX.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | CROSS-REF-MATRIX.md — module × parameter verdict matrix (the S83 fix lane on top — the M-… | A |
| diff-to-fix/FIXED-LEDGER.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | FIXED-LEDGER.md — the master index of resolved findings (264 finding rows + 20 resolved O… | A |
| diff-to-fix/PARAMETERS.md | 167dbe3e 2026-10-28 reconcile(session-9): the push-time merge with the parallel ses… | PARAMETERS.md — the parity-audit parameter taxonomy | B |
| diff-to-fix/README.md | 04fb5e88 2026-10-29 audit(S69): the ninth continuation lane's filings — 5 new + 2 a… | diff-to-fix/ — parity audit roadmap (core_recovery vs easeus-drw-research) | A |
| diff-to-fix/SCAN-ENGINE-DELTA.md | 89405af1 2026-10-29 docs(carve): N-204 - the engine-layer carve geometry registered… | SCAN-ENGINE-DELTA.md — the Scan Engine Inventory / Delta Report (Agent 2, scan-engine sco… | B |
| diff-to-fix/SEVERITY-SCALE.md | 439bb802 2026-10-25 diff-audit: T3+T4 — PARAMETERS/SEVERITY-SCALE/README/SUMMARY/CR… | SEVERITY-SCALE.md — verdict definitions used by this audit | B |
| diff-to-fix/SUMMARY.md | 0cf6142d 2026-10-29 fix(carve): M-107's CRW member — the CIFF heap size walk replac… | SUMMARY.md — executive summary (the S79 MAJOR-first fix lane on top — the M-073 seat conc… | A |
| diff-to-fix/SURFACE-INVENTORY.md | ed14732f 2026-10-29 docs(register)+test: S-050 - the file-repair line's formal scop… | SURFACE-INVENTORY.md — the corpus-derived surface inventory (the audit's scope) | A |
| docs/ABI-CONTRACT.md | 0ba5f14e 2026-10-29 fix(orchestration): M-073 — the scan-time folder filter lands (… | ABI-CONTRACT — the `cr_ui` C ABI: ownership, threading, errors, events, stability | A |
| docs/API-SURFACE-AUDIT.md | 497b1277 2026-10-29 repair(docs): the A-ABI-3 count class recurred TWICE in one win… | API-SURFACE-AUDIT — the reference's API operations vs. the core's C ABI | A |
| docs/API.md | 497b1277 2026-10-29 repair(docs): the A-ABI-3 count class recurred TWICE in one win… | API Reference — core_recovery | A |
| docs/BENCHMARK-REPORT.md | 545c838b 2026-10-23 build(release)!: the complete v0.3 SDK bundle workflow + releas… | BENCHMARK-REPORT — core_recovery vs the og reference | B |
| docs/BUILD-SUPPRESSIONS.md | 05e67863 2026-10-28 feat(preview): the cr_thumbnail engine - bounded still-image th… | BUILD-SUPPRESSIONS — every warning/error suppression, with rationale and scope | A |
| docs/CAPABILITY-MATRIX.md | ed14732f 2026-10-29 docs(register)+test: S-050 - the file-repair line's formal scop… | CAPABILITY-MATRIX — core_recovery per-capability status vs the reference | A |
| docs/CI-CURRENT.md | 41d88297 2026-10-28 fix(M-088): one FloorVersion floor + the 4-lane CI-shape doc tr… | CI-CURRENT — the 4-lane Windows-only CI (the 2026-10-28 consolidation + the S-038 install… | A |
| docs/CI-LOCAL.md | 41d88297 2026-10-28 fix(M-088): one FloorVersion floor + the 4-lane CI-shape doc tr… | CI-LOCAL — the local validation entry point and its CI mirror | A |
| docs/CI-SCENARIOS.md | 13fe7258 2026-10-29 fix(imaging): S-040 - the disk-image CREATE producer landed (th… | CI-SCENARIOS — the staged-scenario truth matrix | B |
| docs/CONNECTION-GUIDE.md | 22e96576 2026-10-26 docs(v0.4.1-parity): the OQ-S4-3 transport verdict transcribed … | Connection Guide — binding, driving, and connecting devices | B |
| docs/CORE-API-CURRENT.md | 497b1277 2026-10-29 repair(docs): the A-ABI-3 count class recurred TWICE in one win… | CORE-API-CURRENT — the current public API surface (v0.4.1-parity tree, HEAD 22e9657) | A |
| docs/CORE-DISK-ACCESS.md | c169332e 2026-10-28 fix(abi-docs): the driver-era API-contract drift batch + 4 doc-… | CORE-DISK-ACCESS — the current disk access path (v0.4.1-parity tree, HEAD 22e9657) | B |
| docs/CORE-GAP-LIST.md | f4e500de 2026-10-28 fix(recovery): S-052 - the LDM span recovery basis lands: the t… | CORE-GAP-LIST — final consolidation register (production-complete gate) | A |
| docs/DRIVER-CONTRACT.md | 0ff6cc92 2026-10-28 reconcile(S42-main/v3): the INFO-tier product re-landed over th… | RDA Driver — User-Mode Contract (DRIVER-CONTRACT) | B |
| docs/DRIVER-RELEASE.md | 41d88297 2026-10-28 fix(M-088): one FloorVersion floor + the 4-lane CI-shape doc tr… | RDA Driver — Release Process & Prebuilt Artifact Shape (DRIVER-RELEASE) | B |
| docs/DRIVER-REQUIRED.md | 13fe7258 2026-10-29 fix(imaging): S-040 - the disk-image CREATE producer landed (th… | DRIVER-REQUIRED — the driver-only architecture contract | B |
| docs/DRIVER-UTILIZATION.md | dbbcc6bc 2026-10-28 docs(transport): N-096 - the identity-IOCTL non-issuance regist… | DRIVER-UTILIZATION — the reference's driver usage vs. the core's, per interaction | B |
| docs/FORMATS.md | 1f9f41cf 2026-10-28 merge(hot-window)+converge(N-158)+repair(N-186 NEW): the M-119/… | Formats Guide — what core_recovery reads and writes | A |
| docs/PACKAGING.md | 53eb9fff 2026-10-28 test(S38-M071-M079-converge): the M-071 Windows-CI execution pr… | PACKAGING — the app-installer contract (the packaging milestone's design input) | B |
| docs/PARAMETERS.md | 995ba5dc 2026-10-28 docs(truth): N-095 + N-108 + N-119 - the parity/register doc-tr… | Parameters Reference — every tunable, with defaults and bounds | A |
| docs/PARITY-AUDIT.md | 722cb177 2026-10-23 docs(parity): audit status header — fix waves 1-2 landed (1ecf4… | PARITY-AUDIT.md — independent parity audit (gate document) | B |
| docs/PARITY-DELTAS.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | PARITY-DELTAS — deliberate-deviations registry (AUTHORITATIVE) | A |
| docs/PARITY-OG-CORE-INVENTORY.md | 59a431d3 2026-10-29 fix(docs): N-215 — the R-20 'Unimportant' mechanism attribution… | PARITY-OG-CORE-INVENTORY — og-codebase → core_recovery classification (Agent 3) | B |
| docs/PARITY-REFACTOR-PLAN.md | d8f68083 2026-10-25 fix(N-041): dead-surface deletion — RdaClient + structured-open… | PARITY-REFACTOR-PLAN — deep architecture parity & refactor session | B |
| docs/PERFORMANCE-BASELINE.md | 41d88297 2026-10-28 fix(M-088): one FloorVersion floor + the 4-lane CI-shape doc tr… | PERFORMANCE-BASELINE — measured numbers at session start | B |
| docs/PERFORMANCE-PROFILE.md | d0a102ce 2026-10-28 fix(bench+docs): GAP-S6-5's physical-ceiling scorer action + th… | PERFORMANCE-PROFILE — baseline measurements at HEAD `064fcf4` (session 6, 2026-10-25) | B |
| docs/PLAN-API-EXPANSION.md | e4f3184e 2026-10-28 docs(truth): N-117 + N-118 + N-103 - the W1-landing doc drift b… | PLAN-API-EXPANSION — close the reference-documented API gaps | B |
| docs/PLAN-CI-REBUILD.md | 41d88297 2026-10-28 fix(M-088): one FloorVersion floor + the 4-lane CI-shape doc tr… | PLAN-CI-REBUILD — prebuilt-artifact CI, combined release, local-first validation | B |
| docs/PLAN-DRIVER-ONLY.md | c169332e 2026-10-28 fix(abi-docs): the driver-era API-contract drift batch + 4 doc-… | PLAN-DRIVER-ONLY — remove the direct-access fallback; the driver becomes a hard prerequis… | B |
| docs/PREBUILT-ARTIFACTS.md | 0ff6cc92 2026-10-28 reconcile(S42-main/v3): the INFO-tier product re-landed over th… | PREBUILT-ARTIFACTS — the deterministic latest-release convention | B |
| docs/REFERENCE-API-GAP.md | 532c6811 2026-10-29 fix(reghive): M-113 — the recovered-OS boot-environment write-s… | REFERENCE-API-GAP — the og engine→UI operation inventory vs. our core | B |
| docs/REFERENCE-DRIVER-BEHAVIOR.md | 995ba5dc 2026-10-28 docs(truth): N-095 + N-108 + N-119 - the parity/register doc-tr… | REFERENCE DRIVER BEHAVIOR — EaseUS Data Recovery Wizard (og corpus) | B |
| docs/RELEASE.md | 8943993c 2026-10-27 ci(rebuild)!: the prebuilt-artifact CI lands - benchmark lanes … | RELEASE — the combined release convention and cadence | B |
| docs/UI-READINESS-PLAN.md | 1f08c20d 2026-10-25 feat(abi): carve-format registry (ops #7-#8) + versioning polic… | UI-Readiness Plan — closing G-01…G-13 (issues #1–#10) | A |
| docs/architecture.md | e58bde10 2026-10-27 feat(driver-only)!: the direct-access fallback is REMOVED - the… | Architecture | B |
| docs/benchmarking.md | 41d88297 2026-10-28 fix(M-088): one FloorVersion floor + the 4-lane CI-shape doc tr… | Benchmarking (v0.2 harness) | B |
| docs/build.md | 444d7ba8 2026-10-26 fix: the CI-restoration tail - the fuzz build break (convergent… | Build | B |
| docs/crsf-format.md | 8b76017c 2026-10-28 fix(session): N-090 - the session-list scope + row shape landed… | CRSF - Core Recovery Session Format (v1) | B |
| docs/error-fix-registry.md | 046889d7 2026-10-23 docs(w5): PARITY-DELTAS D-36..D-44 + closing-action refresh + C… | Error-Fix Registry — reference design defects corrected in this rewrite | A |
| docs/ground-truth-contracts.md | e58bde10 2026-10-27 feat(driver-only)!: the direct-access fallback is REMOVED - the… | Ground-Truth Contracts (anti-hallucination anchor) | A |
| docs/recovery-report-schema.md | adb26797 2026-10-29 fix(carve): N-140 — the per-extension carve-result aggregation … | Recovery Report JSON Schema (v1) | B |
| docs/requirements.md | 0db53c88 2026-10-22 docs(scope)!: correct the scope language - full LDM + full ReFS… | Requirements — core_recovery v0.1 (locked) | B |
| docs/roadmap.md | 2dd6e011 2026-10-23 docs(gap): close the W5/UI10 remainder - contracts S11, README … | Roadmap | A |
| docs/testing.md | 68aa5b78 2026-10-29 fix(docs): N-150 - the API doc-truth batch landed (counts re-de… | Testing | B |
| installer/README.md | 624db00f 2026-10-28 fix(installer lane): use the runner image's preinstalled Inno S… | installer/ — the app-installer surface (S-038) | B |
| worklog/DEV-SIGN.md | fcb4d424 2026-10-27 audit(SA-1): Phase 1 dev-sign validation closed - run 405496600… | DEV-SIGN — development-signing validation of the merged product's driver | B |
| worklog/session-05-prompt.md | 027b9db7 2026-10-23 fix(ci): clang-format drift on the orchestrator/telemetry heade… | Session 05 Prompt — Verification & Reconciliation Session | B |
| worklog/session-10-deep-audit-prompt.md | c75755df 2026-10-28 audit(session-10/wave-1): the board-truth sweep — every active … | Session 10 — the continuation deep-audit prompt (saved per SOP, token redacted, read twic… | B |
| worklog/session-10-lane-B-parity-audit-prompt.md | 0e279e43 2026-10-28 reconcile(S10-B): the push-time reconciliation with the four co… | session-10-lane-B-parity-audit-prompt.md (token redacted) | B |
| worklog/session-10-parity-audit-prompt.md | b478a4b1 2026-10-28 worklog(session-10/T1): the baseline — continuation mandate ope… | Session 10 — the function-level completeness pass + board-truth (session prompt, token re… | B |
| worklog/session-11-continuation-audit-prompt.md | 35bee726 2026-10-28 audit(session-11/T1): setup — baselines recorded (core 7990a86d… | Session 11 — continuation audit prompt (token REDACTED) | B |
| worklog/session-11-diff-audit-prompt.md | ac39c321 2026-10-28 worklog(session-11/open): the continuation lane opens at core 7… | Session-11 diff-audit prompt (saved per the SOP; token redacted) | B |
| worklog/session-11-diff-fix-prompt.md | b1473173 2026-10-28 worklog(session-11C/T1): the fix lane opens — baselines recorde… | Session-11 diff-fix prompt (redacted) | B |
| worklog/session-12-continuation-audit-prompt.md | 4ce97779 2026-10-28 worklog(S12/T1): session-12 continuation lane open — core c7fa5… | Session-12 continuation audit prompt (token-redacted) | B |
| worklog/session-12-continuation-audit-prompt.s12b-lane.md | 4ce97779 2026-10-28 worklog(S12/T1): session-12 continuation lane open — core c7fa5… | Session-12 continuation audit prompt — the S12B lane's copy (token-redacted) | B |
| worklog/session-12-diff-audit-prompt.md | 8d0d88b8 2026-10-28 worklog(S12): session-2 continuation audit prompt saved (token … | Session 12 (session-2 continuation audit) — the user's directive (token redacted) | B |
| worklog/session-2-diff-audit-prompt.md | db7324a9 2026-10-25 diff-audit S2 wave 1+2: 17 new findings, 5 fixed moved to FIXED/ | Session 2 — diff-to-fix continuation audit prompt (redacted) | B |
| worklog/session-3-diff-audit-prompt.md | d5282eb9 2026-10-25 diff-audit-s3: T1 bootstrap — state reconciled at 34b3441 (S-01… | Session 3 — diff-audit continuation prompt (token redacted) | B |
| worklog/session-33-diff-fix-prompt.md | a3ae3e7b 2026-10-26 worklog(DFX33): session-33 bootstrap — prompt saved (token reda… | Session 33 (DFX33) — diff-to-fix closing prompt (token redacted) | B |
| worklog/session-4-diff-audit-prompt.md | 63a91679 2026-10-25 worklog(s4): session-4 diff-audit prompt saved token-redacted; … | Session 4 — diff-audit continuation prompt (token redacted) | B |
| worklog/session-40-diff-fix-prompt.md | 6ecd6bb4 2026-10-28 fix(orchestrator): S-051 - the whole-disk target set lands: the… | Session 40 — diff-to-fix execution prompt (saved per SOP; token redacted) | B |
| worklog/session-41-continuation-audit-prompt.md | 9cc6a03e 2026-10-28 audit(S41/final-reconciled): the continuation lane's product re… | Session-41 continuation-audit prompt (saved per the mandate; token redacted; IM gateway m… | B |
| worklog/session-42-continuation-audit-prompt.addendum-focus-battery.md | 88a7928d 2026-10-28 worklog(S42/W2-rescope): the focus-battery addendum lands — the… | Session-42 continuation-audit prompt — ADDENDUM: the focus battery (saved per the mandate… | B |
| worklog/session-42-continuation-audit-prompt.md | 468cd777 2026-10-28 merge(S42-D): the third continuation lane absorbed over the hot… | worklog/session-42-continuation-audit-prompt.md — TWO parallel S42 lanes saved prompts un… | B |
| worklog/session-42-diff-fix-prompt.md | ee1c6760 2026-10-28 fix(carve): S-049 - the og's universal carved-file naming contr… | session-42-diff-fix-prompt.md — the mandate for this fix session (token redacted) | B |
| worklog/session-42c-continuation-audit-prompt.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | Session-42-C continuation audit prompt (token redacted) | B |
| worklog/session-5-diff-audit-prompt.md | 2001f7cc 2026-10-25 worklog(s5): session-5 diff-audit bootstrap — citation-gap inve… | Session 5 (diff-audit continuation) Prompt — token redacted | B |
| worklog/session-50-continuation-audit-prompt.md | 93461483 2026-10-28 worklog(S50): renumbered S49->S50 per SA-9 (the remote's S49 pa… | Session-49 continuation audit prompt (token redacted) | B |
| worklog/session-52b-diff-fix-prompt.md | 612577cb 2026-10-29 repair(window): the 625a1e59 committed-red main closed - the 4t… | Session prompt — the diff-to-fix execution mandate (S52-B lane; renamed at the rebase per… | B |
| worklog/session-54-continuation-audit-prompt.md | 2794da8b 2026-10-28 worklog(S54): the session-54 continuation audit lane opens (ren… | Session-54 continuation audit prompt (renumbered at the rebase per SA-9 — the remote S52/… | B |
| worklog/session-55-continuation-audit-prompt.md | 92fe573d 2026-10-28 worklog(S55): the mandate file aligned to the S55 lane number (… | Session-52 continuation audit mandate (token-redacted) | B |
| worklog/session-58-diff-fix-prompt.md | c5491c96 2026-10-29 merge(hot-window): the racing S60/S61 window absorbed at the re… | Session prompt — diff-to-fix EXECUTION lane (2026-10-29) | B |
| worklog/session-5b-diff-audit-prompt.md | 10d4d9f7 2026-10-25 diff-audit-s5b: the never-dispositioned MOM/product-capabilitie… | Session-5 diff-audit prompt (token redacted) | B |
| worklog/session-6-deep-audit-prompt.md | 022c81d5 2026-10-25 worklog(s6): deep-continuation bootstrap — pulled at bf7df39/03… | Session: diff-audit deep continuation — missed surfaces + fix verification (2026-10-26) | B |
| worklog/session-6-diff-fix-prompt.md | 16fea631 2026-10-25 worklog(s6): diff-fix session bootstrap — prompt saved (token r… | Session: diff-to-fix full-roadmap execution (session 6) | B |
| worklog/session-60-diff-fix-prompt.md | 6d487cfd 2026-10-29 fix(recognition): N-199 - the ReFS classifier's second signatur… | Session-60 diff-to-fix execution prompt (MINOR-first closing lane) | B |
| worklog/session-60-diff-to-fix-prompt.md | ccada380 2026-10-29 worklog(s60): the diff-to-fix execution lane open — the mandate… | Session-60 diff-to-fix execution lane — the saved mandate (2026-10-29) | B |
| worklog/session-61-continuation-audit-prompt.md | 237f54d0 2026-10-29 audit(s61): lane open — the seventh continuation audit lane (pr… | Session-61 continuation audit prompt (the SEVENTH continuation lane of the user's second-… | B |
| worklog/session-68-continuation-audit-prompt.md | f9f8c6e4 2026-10-29 worklog(S68): the eighth continuation audit lane opens - prompt… | Session-68 continuation audit prompt (the EIGHTH continuation lane of the user's second-s… | B |
| worklog/session-69-continuation-audit-prompt.md | 3799dee2 2026-10-29 worklog(S69): the ninth continuation audit lane opens - prompt … | Session-69 continuation audit prompt (the NINTH continuation lane of the user's second-se… | B |
| worklog/session-70-diff-fix-prompt.md | 1ec36d6c 2026-10-29 rebase-continue: N-200 over the racing S71/S72/S73 windows (N-2… | Session-70 S70 lane — the diff-to-fix EXECUTION lane (2026-10-29) | B |
| worklog/session-8-parity-audit-prompt.md | ff879f94 2026-10-28 worklog(session-8): the T1 baseline — repos cloned (core 35adcb… | Session 8 — full parity audit prompt (token REDACTED) | B |
| worklog/session-9-parity-audit-prompt.md | 6e9b6dd0 2026-10-28 worklog(session-9/T1): the baseline — repos cloned (core 005827… | Session 9 — full parity audit prompt (token redacted) | B |
| worklog/session-audit-prompt.md | dd82db9d 2026-10-27 worklog: session open - merged repo confirmed (core_recovery @3… | Session 7 — full parity audit prompt (token REDACTED) | B |
| worklog/session-coherence-prompt.md | e8dcaba8 2026-10-29 worklog(session): Session-84 opened — the post-remediation cohe… | Session-84 — post-remediation coherence session prompt | B |
| worklog/session-core-api-prompt.md | e6d5b01d 2026-10-25 worklog(api-session): bootstrap — baselines recorded (core 7d0f… | Session: core-api — API surface + kernel-driver utilization production-complete | B |
| worklog/session-core-final-prompt.md | 1671b2ad 2026-10-25 docs: session-6 gate documents - gap register, ABI contract, pe… | Session: core-final (production-complete push) | B |
| worklog/session-devsign-audit-prompt.md | dd82db9d 2026-10-27 worklog: session open - merged repo confirmed (core_recovery @3… | NOTE: this file is the SA (dev-sign + parity re-audit) lane's prompt. | B |
| worklog/session-diff-audit-prompt.md | 4723442f 2026-10-25 diff-audit: T1 — session scaffold, redacted prompt, worklog ent… | Session prompt — diff-to-fix parity audit (token redacted) | B |
| worklog/session-diff-audit-s70-prompt.md | c736e9b3 2026-10-29 worklog(S70): the tenth continuation audit lane opens - prompt … | Session-70 diff-audit prompt (saved token-redacted per the standing rule; read twice befo… | B |
| worklog/session-diff-fix-10-prompt.md | 4c35a320 2026-10-25 worklog(DFX10): session-10 bootstrap — baseline 8e79960 green 5… | Session 10 — diff-to-fix execution prompt (token redacted) | B |
| worklog/session-diff-fix-12-minors-prompt.md | 7444fd67 2026-10-25 worklog(DFX12): session-12 minors-first bootstrap — prompt save… | Session prompt — diff-to-fix minors-first closing session (2026-10-26) | B |
| worklog/session-diff-fix-13-prompt.md | 6eecdc3d 2026-10-25 worklog(DFX11): session-11 bootstrap — baseline 534/534 at a5bb… | Session: diff-to-fix remediation — MINOR-first close-out wave (session 11) | B |
| worklog/session-diff-fix-14-prompt.md | c4e37d13 2026-10-25 merge: union with 2678fc7 + repair the 5aaf044 'style' clobber … | Session diff-fix-14 prompt (token redacted) | B |
| worklog/session-diff-fix-15-prompt.md | 00ebed8d 2026-10-25 fix(M-054 NEW-FINDING): exFAT allocation bitmap bit i vs cluste… | Session prompt — diff-to-fix closing session (token redacted) | B |
| worklog/session-diff-fix-16-prompt.md | d565caad 2026-10-25 worklog(DFX16-T1 rebase): union with parallel lane entries | Session 16 — diff-to-fix remediation prompt (token redacted) | B |
| worklog/session-diff-fix-17-minors-prompt.md | 3edaa83e 2026-10-25 worklog(DFX17): session bootstrap — baseline 562+21 green, MINO… | Session DFX17 (diff-to-fix session 17) — MINOR-first close-out wave | B |
| worklog/session-diff-fix-18-prompt.md | e21de3e4 2026-10-25 worklog(DFX18): session-18 prompt saved (token redacted) — mino… | Session 18 prompt (saved at session start; token redacted) | B |
| worklog/session-diff-fix-19-prompt.md | b7dd8745 2026-10-25 worklog(DFX21): tail-half MAJOR lane claim (M-044 first) — queu… | diff-to-fix session 17 prompt (saved at bootstrap; token REDACTED) | B |
| worklog/session-diff-fix-20-prompt.md | d5aee931 2026-10-25 merge: union with origin/main (6785b41) — M-005/M-007/M-054/M-0… | Session DFX17 — diff-to-fix roadmap execution (closing the parity loop) | B |
| worklog/session-diff-fix-21-prompt.md | b7dd8745 2026-10-25 worklog(DFX21): tail-half MAJOR lane claim (M-044 first) — queu… | diff-to-fix session 21 prompt (saved at bootstrap; token REDACTED) | B |
| worklog/session-diff-fix-22-prompt.md | 8a1e8d78 2026-10-25 fix(M-055 NEW-FINDING): win layer UNCOMPILABLE — four structura… | Session 22 prompt (lane re-stamped from 19: IDs 19-21 were squatted by parallel legs in t… | B |
| worklog/session-diff-fix-23-prompt.md | 03608d8b 2026-10-26 diff-fix(DFX25-T3): INFO tier closed into FIXED/ — 22/22 acknow… | Session DFX23 — diff-to-fix closing session (INFORMATIONAL + tracked-open focus) | B |
| worklog/session-diff-fix-26-prompt.md | 75ad6f45 2026-10-26 reconcile(N-064): convergent two-lane landing unified — DFX31 c… | Session 26 prompt (saved 2026-10-26, token redacted; re-stamped past two same-window squa… | B |
| worklog/session-diff-fix-27-prompt.md | db3de2c5 2026-10-26 worklog(DFX27): session-27 bootstrap — disjoint MAJOR-head fron… | Session DFX26 — diff-to-fix execution prompt (token redacted) | B |
| worklog/session-diff-fix-28-prompt.md | 75ad6f45 2026-10-26 reconcile(N-064): convergent two-lane landing unified — DFX31 c… | Session 28 prompt (saved 2026-10-26, token redacted; re-stamped past three same-window sq… | B |
| worklog/session-diff-fix-29-prompt.md | 14b09800 2026-10-26 merge: union with origin/main (2adfb51) — the window's parallel… | Session 29 prompt (token redacted) — diff-to-fix fix session, DFX29 (re-stamped from 25: … | B |
| worklog/session-diff-fix-30-prompt.md | cfa4c758 2026-10-26 repair(N-064 second corruption wave): rrecov.cpp reconstructed … | Session 30 prompt (saved token-redacted at bootstrap; re-stamped 25->30 past | B |
| worklog/session-diff-fix-32-prompt.parallel-DFX31-32.md | 9d6f0370 2026-10-26 merge: union with origin/main (372bcf1) — the parallel DFX32 mi… | Session 31 prompt (DFX31 — saved token-redacted; re-stamped 25→30→31 | B |
| worklog/session-diff-fix-34-prompt.md | 715b4d70 2026-10-26 worklog(DFX34): session-34 bootstrap — MINOR-tail lane claim (b… | Session 34 prompt (saved at bootstrap — token redacted; lane re-stamped | B |
| worklog/session-diff-fix-35-prompt.md | 16b311fd 2026-10-26 merge: union with origin/main (273dde4) — the N-064 repair + N-… | Session DFX35 — diff-to-fix closing session (INFORMATIONAL + tracked-open focus) | B |
| worklog/session-diff-fix-36-prompt.md | 93ef82ca 2026-10-26 repair(union regression): the 7ec5446 union re-dropped the M-05… | Session 36 prompt — diff-to-fix closing wave (S-034, M-030, M-040, N-072, OQ residuals) | B |
| worklog/session-diff-fix-37-prompt.md | e27daf12 2026-10-26 worklog(DFX37-T0): session-37 bootstrap — the third-merge-storm… | Session 37 (DFX37) — the SEVERE/M-040/N-072 close-out lane | B |
| worklog/session-diff-fix-37-prompt.parallel-lane.md | 5c1d63f8 2026-10-26 merge: the second S-034 convergent lane absorbed (session-ID 37… | Session prompt — diff-to-fix execution (DFX-37; the DFX36 name collided with a parallel l… | B |
| worklog/session-diff-fix-38-prompt.md | 53eb9fff 2026-10-28 test(S38-M071-M079-converge): the M-071 Windows-CI execution pr… | Session 38 — diff-to-fix execution prompt (token REDACTED per SOP) | B |
| worklog/session-diff-fix-39-minors-prompt.md | 2cf3f4d0 2026-10-28 trail(session-39B/T1): the phantom-SHA row sweep (19 landing-ci… | Session prompt — the minors-first parity-fix session (2026-10-29) | B |
| worklog/session-diff-fix-39-prompt.md | bfdc1b28 2026-10-28 worklog(session-39/T1): the fix session opens — baselines, prom… | Session 39 — the diff-to-fix parity-fix session (user prompt, token redacted) | B |
| worklog/session-diff-fix-4-prompt.md | c69a7908 2026-10-25 fix: S-012 - NTFS two-pass architecture: pass-1 membership rest… | Session prompt — diff-to-fix session 4 (diff-fix-4) | B |
| worklog/session-diff-fix-40-convergence.md | 538d529b 2026-10-28 worklog(S40-open): the fix-all-minors-first mandate opens - the… | Session-40 N-111 convergence record (2026-10-29) | B |
| worklog/session-diff-fix-40-minors-prompt.md | ca202914 2026-10-28 worklog(S40-open): the session-40 minors continuation lane open… | Session-40 diff-fix prompt (token redacted, saved per mandate) | B |
| worklog/session-diff-fix-40-prompt.md | 865f8b43 2026-10-28 worklog(S40-open): the session-40 lane opens - baselines record… | Session-40 diff-fix prompt (saved per the mandate's T1; token redacted; IM gateway metada… | B |
| worklog/session-diff-fix-40-prompt.parallel-main-lane.md | ff8898b5 2026-10-28 repair(ci)+worklog(S40-ML/open): the push-window format dirt re… | Session 40 — the diff-fix closing mandate (token redacted) | B |
| worklog/session-diff-fix-40-severe-prompt.md | d4b487c8 2026-10-28 fix(carve): S-049 - the og's universal carved-file naming contr… | Session 40 — the SEVERE-first diff-to-fix execution prompt (token redacted) | B |
| worklog/session-diff-fix-42-main-prompt.md | 0ff6cc92 2026-10-28 reconcile(S42-main/v3): the INFO-tier product re-landed over th… | Session fix roadmap prompt (2026-10-29, session-42) — token redacted | B |
| worklog/session-diff-fix-42-prompt.md | b26017a7 2026-10-28 hygiene(tests)+worklog: session-42 opens - the N-115 OoxmlRelsP… | Session-42 diff-fix mandate (2026-10-29) — token redacted | B |
| worklog/session-diff-fix-43-minors-prompt.md | db228b75 2026-10-28 worklog(43-minors/open): the lane's T1 record at 632518b7 - twi… | Session 43 — the minor-first diff-to-fix execution prompt (token redacted) | B |
| worklog/session-diff-fix-44-minors-prompt.md | f644f746 2026-10-28 fix(table): N-161 - the GPT entry-span underflow gated (first<=… | Session prompt — the diff-to-fix execution mandate (Session-44 minors lane) | B |
| worklog/session-diff-fix-44-prompt.md | 93e62dcf 2026-10-28 worklog(S44): the MAJOR-tier lane opens - the S-048 convergence… | # # Session-44 fix-lane prompt (2026-10-29, saved at T1 — token redacted per the mandate) | B |
| worklog/session-diff-fix-45-prompt.md | 862d79ef 2026-10-28 fix(fat): A-014 - the time-word validation lands (hour/min/2s r… | Session 45 — diff-fix execution prompt (token redacted) | B |
| worklog/session-diff-fix-47-prompt.md | 925f4c2a 2026-10-28 merge(hot-window): the S45..S51 parallel window absorbed - A-01… | Session-47 mandate — the MAJOR-first parity-fix lane (saved token-redacted per the standi… | B |
| worklog/session-diff-fix-49-prompt.md | 7e931b1e 2026-10-28 worklog(S49): the session-49 parity-fix lane opens - territory … | Session-49 fix mandate (2026-10-29) — saved token-redacted per house convention | B |
| worklog/session-diff-fix-5-prompt.md | e680c73b 2026-10-25 worklog(s7): closing-session bootstrap — pulled at b65f1e6/03ad… | Session prompt — diff-to-fix closing session (token redacted) | B |
| worklog/session-diff-fix-56-prompt.md | a99d69fd 2026-10-28 worklog(S56): the session-56 ACUTE-first parity-fix lane opens … | Session-56 — the diff-to-fix parity-fix mandate (ACUTE-first lane) | B |
| worklog/session-diff-fix-57-prompt.md | b3c8694b 2026-10-28 merge(hot-window): the S44..S56 parallel window absorbed (30 co… | Session-57 diff-to-fix prompt (token redacted) | B |
| worklog/session-diff-fix-59-prompt.md | c481b812 2026-10-29 repair(trail+tests): N-190 - the hot-window's red main repaired… | Session-59 diff-to-fix prompt (this lane rebranded from the 45 draft at the hot-window me… | B |
| worklog/session-diff-fix-60-prompt.md | 78be139e 2026-10-29 fix(ntfs): M-147 convergence corroboration (the S60 MAJOR-first… | Session prompt — the diff-to-fix execution mandate (token redacted) | B |
| worklog/session-diff-fix-7-prompt.md | 4702ec89 2026-10-25 fix(audit-trail): resolve committed conflict markers in CROSS-R… | Session diff-fix-7 prompt (token redacted) | B |
| worklog/session-diff-fix-9-prompt.md | 797af183 2026-10-25 fix(M-053): the M-015 landing missed the C-consumer fixture — u… | Session 9 — diff-to-fix execution prompt (token redacted) | B |
| worklog/session-diff-fix-closing-prompt.md | 0df9702b 2026-10-28 worklog(DFX-38/T1): session open — baseline 84a1832, board 75 a… | Session prompt — diff-to-fix closing session (2026-10-29) — token redacted | B |
| worklog/session-diff-fix-final-prompt.md | ce2405a9 2026-10-28 fix(M-071): the M-018 regression closed - both rrecov.cpp junk … | The diff-to-fix closing session — the fix mandate (token redacted) | B |
| worklog/session-diff-fix-m117-prompt.md | 1d4c96b6 2026-10-28 fix(carve): M-117 - the Outlook Express Dbx analyser lands (the… | Session: the M-first diff-to-fix execution lane (2026-10-29) | B |
| worklog/session-diff-fix-minors-prompt.md | ab097dbf 2026-10-25 worklog: diff-fix minors-focus session start — baseline recorde… | Session prompt — diff-to-fix close-out (focus: 33 MINORs), 2026-10-26 | B |
| worklog/session-diff-fix-parity-close-prompt.md | f6e303e1 2026-10-28 fix(S-042): WMF placeable+standard validators re-derived from t… | Session prompt — the parity close (diff-to-fix execution, all tiers) | B |
| worklog/session-diff-fix-prompt.md | a67e1f62 2026-10-29 repair(trail): N-217 — the ed501caa conflict-marker recurrence … | Session fix mandate — prompt record (token redacted) | B |
| worklog/session-diff-fix-s46-prompt.md | fa654673 2026-10-28 worklog(S46): the session-46 MINOR-first parity-fix lane opens … | Session-46 diff-to-fix prompt (token redacted) | B |
| worklog/session-diff-fix-s47-prompt.md | 7ea0b6a9 2026-10-29 worklog(S58): the S-064 landing recorded | Session 47 (S47) — the diff-to-fix parity-fix lane (SEVERE-first) | B |
| worklog/session-diff-fix-s48-prompt.md | 0d7b1d9d 2026-10-28 worklog(S48): the session-48 INFO-first parity-fix lane opens -… | Session-48 diff-to-fix prompt (token redacted) | B |
| worklog/session-diff-fix-s51-prompt.md | bbbe78b2 2026-10-29 repair(trail): N-210 - the M-120 class recurred: the f3da99f1 m… | Session-51 diff-to-fix prompt (the severe-first lane; the S47 seat was taken by the remot… | B |
| worklog/session-diff-fix-s53-prompt.md | e0f15e05 2026-10-28 merge(hot-window): the S52-claim + I-035/I-040 window absorbed … | Session-53 diff-to-fix prompt (renumbered from the S47 draft at the hot-window merge per … | B |
| worklog/session-diff-fix-s56-prompt.md | 840b06d6 2026-10-28 worklog(S56): the session-56 MINOR-first parity-fix lane opens … | Session-56 diff-to-fix prompt (token redacted; seat renumbered S49→S56 across three hot-w… | B |
| worklog/session-diff-fix-s57-prompt.md | 132563d9 2026-10-28 fix(fat): A-014 - the FAT time-word gate lands (the full DosDat… | Session-47 diff-to-fix prompt (token redacted) | B |
| worklog/session-diff-fix-s60-prompt.md | 4d859e41 2026-10-29 fix(tree): N-192 - the clone carries the 8.3 short_name (the og… | Session-60 (S60) — the MINOR-first diff-to-fix execution lane | B |
| worklog/session-diff-fix-s63-prompt.md | 592f4ae5 2026-10-29 worklog(S63): the diff-to-fix execution lane opens - prompt sav… | Session prompt — diff-to-fix execution lane S63 (2026-10-29) | B |
| worklog/session-diff-fix-s64-prompt.md | cb6d5bbe 2026-10-29 fix(query): N-194 - the og's two byte-size display formatters l… | Session S64 — diff-to-fix execution lane (the MINOR-first continuation) | B |
| worklog/session-diff-fix-s66-prompt.md | cb6d5bbe 2026-10-29 fix(query): N-194 - the og's two byte-size display formatters l… | Session S66 — diff-to-fix execution lane (the MINOR-first continuation; third lane of the… | B |
| worklog/session-diff-fix-s67-prompt.md | e7917402 2026-10-29 worklog(S67): the diff-to-fix execution lane opens - prompt sav… | Session-67 (S67) — diff-to-fix execution lane prompt (saved token-redacted per protocol) | B |
| worklog/session-diff-fix-s71-prompt.md | bbbe78b2 2026-10-29 repair(trail): N-210 - the M-120 class recurred: the f3da99f1 m… | Session-71 S71 — the diff-to-fix execution lane prompt (2026-10-29) | B |
| worklog/session-diff-fix-s72-prompt.md | 449fc594 2026-10-29 worklog(S72): the twelfth continuation fix lane opens - prompt … | Session-72 S72 fix-lane prompt (token redacted) | B |
| worklog/session-diff-fix-s73-prompt.md | ee19145c 2026-10-29 fix(ext4): M-144 — the 8 MiB GDT single-shot read cap replaced … | Session S73 — diff-to-fix continuation lane (parity close) | B |
| worklog/session-diff-fix-s75-prompt.md | 9bbfd78c 2026-10-29 repair(trail): the N-210 class recurred in the S75 prompt file … | Session-75 (S75) — diff-to-fix execution lane prompt (re-homed s70->s75 per SA-9: the rac… | B |
| worklog/session-diff-fix-s76-prompt.md | 88b1e8d4 2026-10-29 worklog(S76): the MINOR-first fix mandate lane opens - prompt s… | Session-76 (S76) — the MINOR-first diff-to-fix execution lane (prompt, token redacted) | B |
| worklog/session-diff-fix-s77-prompt.md | 31c18153 2026-10-29 worklog(S76): the fourteenth continuation fix lane opens - prom… | Session-76 fix-lane prompt (token redacted) | B |
| worklog/session-diff-fix-s79-prompt.md | 0ba5f14e 2026-10-29 fix(orchestration): M-073 — the scan-time folder filter lands (… | Session-79 (S79) fix-lane prompt — token redacted | B |
| worklog/session-diff-fix-s80-prompt.md | 6d74ec2c 2026-10-29 fix(carve): M-155 — the red-main repair: the ed501caa push's co… | Session prompt — the S80 diff-to-fix mandate (token redacted) | B |
| worklog/session-diff-fix-s82-prompt.md | 42228a11 2026-10-29 converge(S82): the X3F arbitration conceded AFTER first-hand di… | Session 82 — the S82 fix-lane prompt (the user's mandate, token redacted) | B |
| worklog/session-diff-fix-s83-prompt.md | 0eaded3d 2026-10-29 fix(carve): M-156 — the X3F surface's fifth touch: the ASCII ve… | Session-83 fix-lane prompt (token redacted) | B |
| worklog/session-parity-prompt.md | d42bb003 2026-10-23 parity-audit(T1): session bootstrap — mandate saved (token reda… | Session prompt — PARITY AUDITOR (2026-10-23) | B |
| worklog/session-parity-refactor-prompt.md | 08b44ac2 2026-10-23 worklog: session prompt — deep architecture parity & refactor (… | Session Prompt — Deep Architecture Parity & Refactoring (verbatim, token redacted) | B |
| worklog/session-review-validate-prompt.md | 41a2c568 2026-10-26 docs(the overhaul): the API/formats/parameters/connection refer… | Session: review-and-validate (post-fix audit of the fix sessions) | B |
| worklog/session2-diff-audit-prompt.md | 4f0c841e 2026-10-25 diff-audit-s2: T1+T2 — bootstrap (baseline 2545b83 vs corpus 03… | Session 2 — Diff-Audit Continuation Prompt (token redacted) | B |
| diff-to-fix/evidence/s10-field-parity-table.md | 0e279e43 2026-10-28 reconcile(S10-B): the push-time reconciliation with the four co… | s10-field-parity-table.md — the ifileproperties ↔ cr_records per-field parity table (N-13… | B |
| diff-to-fix/evidence/s10-landing-parity-verification.md | 15279503 2026-10-28 repair(trail/T1d): the N-129 ID collision resolved — the wave-3… | (Renumber note, session-11B: this file's "N-128" filing was renumbered N-129 at the wave-… | B |
| diff-to-fix/evidence/s10-reverification-sweep.md | c75755df 2026-10-28 audit(session-10/wave-1): the board-truth sweep — every active … | s10-reverification-sweep.md — the session-10 board-truth pass (every active finding re-ve… | B |
| diff-to-fix/evidence/s10-verification-pass-b.md | 3d886577 2026-10-28 fix(abi+cli): M-116 - the archive inventory's C-ABI half lands … | s10-verification-pass-b.md — the SECOND verification lane (convergent with s10-reverifica… | B |
| diff-to-fix/evidence/s11-board-truth.md | f166440b 2026-10-28 audit(session-11/wave-1): remove-the-fixed + board truth — M-10… | S11 board-truth evidence — the remove-the-fixed verification + the bookkeeping repair rec… | B |
| diff-to-fix/evidence/s11-lead-deep-lanes.md | aab031ac 2026-10-28 audit(session-11-lead/waves-2-3): the five go-deeper lanes — 12… | S11-lead deep-lanes evidence — the five go-deeper waves (Lanes A-E) | B |
| diff-to-fix/evidence/s11-triage-sweep.md | d50a54dc 2026-10-28 audit(S11/close): the second continuation lane's filings — S-05… | S11/L4 — the active-board liveness sweep (TRIAGE) at HEAD | B |
| diff-to-fix/evidence/s11r-api-currency-pass.md | 0f9e4345 2026-10-28 audit(session-11/S11-R): the reconciliation lane — the 63-commi… | S11 / Agent G — API-DELTA currency + R-row truth pass (Task S11-W3-G) | B |
| diff-to-fix/evidence/s12-core-deepread.md | 1bfeda19 2026-10-28 audit(S12): the second-session continuation lane's filings — S-… | S12/B — CORE DEEP-READ parity-disposition report (audit sub-agent B) | B |
| diff-to-fix/evidence/s12-corpus-rewalk.md | 1bfeda19 2026-10-28 audit(S12): the second-session continuation lane's filings — S-… | S12/C — Corpus behavioral surface re-walk (CORPUS BEHAVIORAL RE-WALK) | B |
| diff-to-fix/evidence/s12-liveness-sweep.md | 1bfeda19 2026-10-28 audit(S12): the second-session continuation lane's filings — S-… | Liveness sweep — 55 active findings at core HEAD (sub-agent S12/A) | B |
| diff-to-fix/evidence/s42-board-verification.md | db7332e6 2026-10-28 audit(S42/wave2): the HFS+ residual resolutions — N-038's field… | S42 Wave-1 — the board-truth verification at the post-window HEAD (the S12 sweep's fix-wi… | B |
| diff-to-fix/evidence/s42-liveness-sweep.md | 27a70db0 2026-10-28 audit(S42)+reconcile: the focus-battery filings REBASED over th… | s42 evidence — the W1 remove-the-fixed liveness discharge + the focus batteries' residue … | B |
| diff-to-fix/evidence/s42c-asset-families-worklog.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | W3b — og-codebase asset-family disposition sweep + the corpus worklog cross-check | B |
| diff-to-fix/evidence/s42c-citation-gap-liveness.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | S42/W4 — core-side citation-gap sweep + active-board liveness re-triage | B |
| diff-to-fix/evidence/s42c-diskhealth-walk.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | W3a — og-codebase/diskhealth/ deep walk (S42 continuation lane) | B |
| diff-to-fix/evidence/s42c-landing-parity.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | W2 — Landing-Parity Verification (23 landings, commit window c7fa527c..869da779) | B |
| diff-to-fix/evidence/s50-ifileproperties-part1-census.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | S50 evidence — the ifileproperties-part1 full census (the under-cited flag discharged) | B |
| diff-to-fix/evidence/s50-liveness-battery.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | S50 evidence — the 74-finding liveness battery (the remove-the-fixed ask) | B |
| diff-to-fix/evidence/s54-deepdive-ext4-hfs-btrfs-zfs-refs-raw.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | Deep dive E — ext4/HFS+/Btrfs/ZFS/ReFS/RAW: math, scan, validators, hang-safety (pin 2794… | B |
| diff-to-fix/evidence/s54-deepdive-ntfs-fat-exfat-partition.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | Deep dive D — NTFS/FAT/exFAT/partition: sector math, scan strategy, validators (pin 2794d… | B |
| diff-to-fix/evidence/s54-deepdive-orchestration.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | Deep dive F — orchestration/session/UI: state, persistence, math, hang policy (pin 2794da… | B |
| diff-to-fix/evidence/s54-liveness-battery-major.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | Liveness battery — MAJOR tier (pin 2794da8b, agent B) | B |
| diff-to-fix/evidence/s54-liveness-battery-minor.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | Liveness battery — MINOR tier (pin 2794da8b, agent C) | B |
| diff-to-fix/evidence/s54-liveness-battery-severe.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | Liveness battery — SEVERE tier + tracked-open (pin 2794da8b, agent A) | B |
| diff-to-fix/evidence/s54-liveness-reconciliation.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | S54 liveness battery — the reconciliation record (pin drift documented) | B |
| diff-to-fix/evidence/s55-liveness.md | b2546602 2026-10-29 audit(S55): the sixth continuation lane's filings - 5 lead-veri… | S55 liveness battery — the fifth continuation lane's independent verification (convergent) | B |
| diff-to-fix/evidence/s8-verification-records.md | 370d6595 2026-10-28 audit(session-8): the second-session deep audit — 25 items file… | Session-8 verification records (2026-10-28, audited `35adcb9`; corpus `be0cfe9`) | B |
| diff-to-fix/evidence/sa8-context-sheets.md | 20255547 2026-10-28 audit(SA-8/lead-lane): the FINAL three-window reconciliation — … | SA-8 evidence — the session-8 lead-lane context-sheet consolidation (2026-10-28) | B |
| diff-to-fix/modules/abi.md | f392d27d 2026-10-29 repair(trail): the FOURTH born-collided D-seat repair per SA-9 … | modules/abi.md — cr_ui C ABI audit verdicts | A |
| diff-to-fix/modules/apply.md | d7856484 2026-10-29 fix(apply): N-145 — the cross-run in-flight journal (the og's p… | modules/apply.md — the consent-gated lost-partition apply module audit verdicts (the tabl… | A |
| diff-to-fix/modules/boot-repair.md | 532c6811 2026-10-29 fix(reghive): M-113 — the recovered-OS boot-environment write-s… | boot-repair — the post-recovery boot-chain family (M-040; the finding's new module row) | A |
| diff-to-fix/modules/btrfs.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | modules/btrfs.md — Btrfs engine audit verdicts | A |
| diff-to-fix/modules/carve.md | 0cf6142d 2026-10-29 fix(carve): M-107's CRW member — the CIFF heap size walk replac… | modules/carve.md — carve module audit verdicts | A |
| diff-to-fix/modules/check-phase.md | 370d6595 2026-10-28 audit(session-8): the second-session deep audit — 25 items file… | modules/check-phase.md — the damage-audit check phase (M-041) | A |
| diff-to-fix/modules/cli.md | 842a587e 2026-10-29 fix(cli): N-178 — the pause surface's CLI legs land (--pause-st… | modules/cli.md — rrecov CLI audit verdicts | A |
| diff-to-fix/modules/driver-install.md | bb2cd8a0 2026-10-28 audit(SA-10/close): the lead lane's filings + the five-lane rec… | modules/driver-install.md — the packaging/installer surface (created by the dev-sign sess… | A |
| diff-to-fix/modules/err.md | d122d5d5 2026-10-28 fix(trail): N-154 - the bookkeeping-truth batch #2 landed (the … | modules/err.md — the error-code registry + the logging seam audit verdicts | A |
| diff-to-fix/modules/exfat.md | 2d614cf2 2026-10-29 fix(exfat): S-067 correction — the convergence arbitration: the… | modules/exfat.md — exFAT engine audit verdicts | A |
| diff-to-fix/modules/ext4.md | 67d2bf25 2026-10-29 repair(trail): the born-collided D-150 seat — THREE rows claime… | modules/ext4.md — ext4 engine audit verdicts | A |
| diff-to-fix/modules/fat.md | 9b4a9302 2026-10-29 trail(S79): the S-068 seat conceded to the racing window — this… | modules/fat.md — FAT engine audit verdicts | A |
| diff-to-fix/modules/health.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | modules/health.md — disk health audit verdicts | A |
| diff-to-fix/modules/hfsplus.md | 5de2e2b0 2026-10-29 fix(hfsplus): M-142 — the VH validator's fork clamps replaced w… | modules/hfsplus.md — HFS+ engine audit verdicts | A |
| diff-to-fix/modules/imaging.md | f392d27d 2026-10-29 repair(trail): the FOURTH born-collided D-seat repair per SA-9 … | modules/imaging.md — disk-image scan targets (S-030; new module row, session 8) | A |
| diff-to-fix/modules/ldm.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | modules/ldm.md — partition table + LDM module audit verdicts (MBR/GPT/EBR covered in this… | A |
| diff-to-fix/modules/lost-partition.md | 1ecd0875 2026-10-29 fix(lost): M-149 — the ghost-score two-pass form restored: the … | modules/lost-partition.md — lost-partition module audit verdicts | A |
| diff-to-fix/modules/mom-capabilities.md | 4f44db49 2026-10-28 audit(session-10/S10-F/wave-2): the tracked-open resolutions — … | modules/mom-capabilities.md — the never-audited MOM/product-capability surfaces (session … | A |
| diff-to-fix/modules/ntfs.md | 5e1dca99 2026-10-29 converge(S82): M-151 conceded to the racing 27bda40e — the four… | modules/ntfs.md — NTFS engine audit verdicts | A |
| diff-to-fix/modules/orchestration.md | 033c5d7c 2026-10-29 fix(orchestrator): N-181 — the RAW-first start order lands (the… | modules/orchestration.md — scan orchestrator audit verdicts | A |
| diff-to-fix/modules/orchestrator.md | 92ae836c 2026-10-29 fix(orchestrator): N-216 — the MaxFileSize cap: the consumer de… | ## Session-81 addendum — N-216: the MaxFileSize cap decoded and landed (2026-10-29) | A |
| diff-to-fix/modules/preview.md | 4298a48d 2026-10-29 fix(preview): N-144 — the TxtViewFiles text-snapshot thumbnails… | modules/preview.md — preview module audit verdicts (NEW in session 4) | A |
| diff-to-fix/modules/query.md | 59a431d3 2026-10-29 fix(docs): N-215 — the R-20 'Unimportant' mechanism attribution… | ## S12 addendum (2026-10-29 — the session-2 deep-dive) | A |
| diff-to-fix/modules/rawfixer.md | cf137fc6 2026-10-25 fix(M-046): 4Kn LDM GPT-path LBAs + exFAT repair plan sector si… | modules/rawfixer.md — RawFixer audit verdicts | A |
| diff-to-fix/modules/refs.md | 36c34557 2026-10-29 fix(refs): N-199 - the recognition gate carries the og's SECOND… | modules/refs.md — ReFS engine audit verdicts | A |
| diff-to-fix/modules/reghive.md | 602dd170 2026-10-28 resolve(I-030) + modules(S11B/T9): the module map completed — r… | modules/reghive.md — the offline registry-hive module audit verdicts (the regf parser + t… | A |
| diff-to-fix/modules/session.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | modules/session.md — session module audit verdicts (CRSF vs .rsf) | A |
| diff-to-fix/modules/storage.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | modules/storage.md — storage/slice/device layer audit verdicts (session 3) | A |
| diff-to-fix/modules/transport.md | 995ba5dc 2026-10-28 docs(truth): N-095 + N-108 + N-119 - the parity/register doc-tr… | modules/transport.md — Windows transport / driver-client layer audit verdicts (NEW in ses… | A |
| diff-to-fix/modules/tree.md | f392d27d 2026-10-29 repair(trail): the FOURTH born-collided D-seat repair per SA-9 … | modules/tree.md — result-tree core audit verdicts | A |
| diff-to-fix/modules/visualization.md | c55bd459 2026-10-25 fix: S-009 - the disk-map free overlay: engine bitmap truth pro… | modules/visualization.md — query/preview/disk-map audit verdicts | A |
| diff-to-fix/modules/writer.md | bf097882 2026-10-29 fix(writer): M-135 — the og's error-9 family: hard read failure… | modules/writer.md — recovery writer + report audit verdicts | A |
| diff-to-fix/modules/zfs.md | aab031ac 2026-10-28 audit(session-11-lead/waves-2-3): the five go-deeper lanes — 12… | modules/zfs.md — ZFS engine audit verdicts | A |
| diff-to-fix/tracked-open/OQ-S10-1-thumbnail-codec-bound-formats.md | 8608b1f4 2026-10-28 decode(OQ-S10-1): pushed forward with new first-hand evidence -… | OQ-S10-1 — The codec-bound thumbnail halves (HEIC/WebP/PDF/video key-frames): vendoring p… | B |
| diff-to-fix/tracked-open/OQ-S10-3-driver-repo-resident-fixes.md | dce44e4d 2026-10-28 trail(OQ-S10-3): the session-48 boundary disposition - the driv… | OQ-S10-3 — The driver-repo-resident fixes pushed forward (renumbered from the OQ-S10-2 dr… | B |
| diff-to-fix/tracked-open/OQ-S4-3-chunking-relative-perf.md | 128e3071 2026-10-28 trail(OQ): S4-3 - the TTFR dispatch-ride state recorded (run 40… | OQ-S4-3 — Relative performance of the core's 64 KiB direct-I/O envelope vs the og's 4/7 M… | B |
| diff-to-fix/tracked-open/OQ-S79-1-ntfs-logfile-replay-result-contribution.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | OQ-S79-1 — the og's $LogFile transaction-journal machinery (load + RCDR walk at volume op… | B |
| docs/adr/ADR-UI-SNAPSHOT.md | fc656d2d 2026-10-23 feat(g02)!: mid-scan immutable snapshot + snapshot-based recove… | ADR-UI-SNAPSHOT - mid-scan result access via immutable value snapshots | A |
| docs/parity/PARITY-ARCH.md | 995ba5dc 2026-10-28 docs(truth): N-095 + N-108 + N-119 - the parity/register doc-tr… | PARITY-ARCH — Architecture & Structure Parity Audit (Task 2-d) | A |
| docs/parity/PARITY-CORE-DOM.md | 0f88fd92 2026-10-23 fix(w1): fallback-truth wave - remove invented degradation, wir… | PARITY-CORE-DOM — Durable Sessions (CRSF vs .rsf), Recovery Writer, Health/SMART, RawFixer | A |
| docs/parity/PARITY-CORE-FS1.md | 1d5cd00e 2026-10-23 parity-audit(T2/T3): full parity audit consolidated — gate docu… | PARITY-CORE-FS1 — NTFS / FAT(12/16/32) / exFAT engine audit (Task 2-a1) | A |
| docs/parity/PARITY-CORE-FS2.md | d08b6d2c 2026-10-26 diff-fix(DFX30-T3): OQ-Z2 resolved -> NEW-FINDING S-034 (SEVERE… | PARITY-CORE-FS2 — Filesystem Engines Audit (ReFS / ZFS / Btrfs / ext4 / HFS+) | A |
| docs/parity/PARITY-CORE-REC.md | 995ba5dc 2026-10-28 docs(truth): N-095 + N-108 + N-119 - the parity/register doc-tr… | PARITY-CORE-REC — Parity-A3 Audit: Raw Carving / Lost Partitions / LDM / Partition Tables | A |
| docs/parity/PARITY-DELTAS-draft.md | 1d5cd00e 2026-10-23 parity-audit(T2/T3): full parity audit consolidated — gate docu… | PARITY-DELTAS — Deliberate-Deviations Registry (DRAFT v0.1) | A |
| docs/parity/PARITY-ORCH.md | 6b75b40e 2026-10-28 fix(carve+orchestrator): S-048 - the Reconstructed per-volume c… | PARITY-ORCH — Parity-B: scan orchestration, staged model, progress, cancellation, session… | A |
| docs/parity/PARITY-UI.md | 995ba5dc 2026-10-28 docs(truth): N-095 + N-108 + N-119 - the parity/register doc-tr… | PARITY-UI — Pages / State / Wiring Parity Audit (Task 2-c) | A |
| diff-to-fix/evidence/decompiled-quotes/btrfs-zfs-refs-session2.md | 2ce2e6ba 2026-10-25 diff-audit s2 third leg (reconciled): decompile-exhaustive pass… | Evidence — Btrfs/ZFS/ReFS session-2 quotes (OQ-T6 resolution + profile/gate deltas) | B |
| diff-to-fix/evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md | 14f3a299 2026-10-25 diff-audit-s3: M-009 verified FIXED (7fd00e0 aligned_read_at ro… | Evidence — carve dispatch, signatures, RawFixer repair set, BitLocker (session 2) | B |
| diff-to-fix/evidence/decompiled-quotes/exfat-dentry-validation.md | 9b7212de 2026-10-25 diff-audit: T6 FAT+exFAT — I-002/N-002/N-003 + OQ-T2/T5; exFAT … | evidence/decompiled-quotes/exfat-dentry-validation.md | B |
| diff-to-fix/evidence/decompiled-quotes/ext4-hfsplus-session2.md | 2ce2e6ba 2026-10-25 diff-audit s2 third leg (reconciled): decompile-exhaustive pass… | Evidence — ext4/HFS+ session-2 quotes (journal, decmpfs, gates) | B |
| diff-to-fix/evidence/decompiled-quotes/fat-exfat-session2.md | 2ce2e6ba 2026-10-25 diff-audit s2 third leg (reconciled): decompile-exhaustive pass… | Evidence — FAT/exFAT session-2 quotes (mirror, windows, validators, gates) | B |
| diff-to-fix/evidence/decompiled-quotes/ldm-raid5-and-scoring.md | 2ce2e6ba 2026-10-25 diff-audit s2 third leg (reconciled): decompile-exhaustive pass… | Evidence — LDM RAID-5 rotation + lost-partition scoring (OQ-T7/T9 closures) | B |
| diff-to-fix/evidence/decompiled-quotes/ntfs-pass2-guid-dump.md | bf7df392 2026-10-25 verify: S-012 convergence — hostile-input smoke (400 iters ASan… | Evidence — NTFS pass-2 / GUID-dump architecture (session 2) | B |
| diff-to-fix/evidence/decompiled-quotes/ntfs-viability-and-filters.md | 90e9aba0 2026-10-25 diff-audit: T5 NTFS — S-001/M-001/N-001/I-001 + OQ-T1, evidence… | evidence/decompiled-quotes/ntfs-viability-and-filters.md | B |
| diff-to-fix/evidence/decompiled-quotes/s10-config-layer-absence-greps.md | 0e279e43 2026-10-28 reconcile(S10-B): the push-time reconciliation with the four co… | s10-config-layer-absence-greps.md — the config-layer triplet claimed "landed" but absent … | B |
| diff-to-fix/evidence/decompiled-quotes/s10-refs-population-site.md | 0e279e43 2026-10-28 reconcile(S10-B): the push-time reconciliation with the four co… | s10-refs-population-site.md — first-hand read: the ReFS population site contradicts part2… | B |
| diff-to-fix/evidence/decompiled-quotes/s10f-btrfs-eadir-suppression.md | deea0241 2026-10-28 audit(session-10/S10-F): the function-level completeness filing… | Evidence — S10-F/T3b: the og's Btrfs "@eaDir" subtree suppression (first-hand decompile) | B |
| diff-to-fix/evidence/decompiled-quotes/s10f-cwindowsfs-consumer-trace.md | 4f44db49 2026-10-28 audit(session-10/S10-F/wave-2): the tracked-open resolutions — … | Evidence — S10-F/T11a: the CWindowsFS consumer trace (OQ-S8-1 RESOLVED) | B |
| diff-to-fix/evidence/decompiled-quotes/s10f-engine-volume-labels.md | 8365956e 2026-10-28 audit(session-10/S10-F/T9): the mid-session M-079 landing absor… | Evidence — S10-F: the og's engine-side volume-label decode vs the core's absent surface | B |
| diff-to-fix/evidence/decompiled-quotes/s10f-product-frame-register.md | deea0241 2026-10-28 audit(session-10/S10-F): the function-level completeness filing… | Evidence — S10-F/T4: the never-dispositioned product-frame binaries (the register-complet… | B |
| diff-to-fix/evidence/decompiled-quotes/s11-oq-traces.md | d50a54dc 2026-10-28 audit(S11/close): the second continuation lane's filings — S-05… | S11/L3 — OQ traces + payload probe (Lane L3, OQTRACE) | B |
| diff-to-fix/evidence/decompiled-quotes/s11-rawsearchdll-infra-walk.md | d50a54dc 2026-10-28 audit(S11/close): the second continuation lane's filings — S-05… | S11/L1 — RAWSearchDLL INFRASTRUCTURE function-level walk (RAWINFRA lane) | B |
| diff-to-fix/evidence/decompiled-quotes/s11-sfc-payload-decode.md | d50a54dc 2026-10-28 audit(S11/close): the second continuation lane's filings — S-05… | s11-sfc-payload-decode.md — the OQ-S10-2 "dead payload" resolution REFUTED: the VideoView… | B |
| diff-to-fix/evidence/decompiled-quotes/s11-zfsrefs-function-walk.md | d50a54dc 2026-10-28 audit(S11/close): the second continuation lane's filings — S-05… | S11/L2 — ZFS/ReFS function-level walk (ZfsSearchDLL + ReFSSearchDLL) | B |
| diff-to-fix/evidence/decompiled-quotes/s12-destarming-regression.md | 1bfeda19 2026-10-28 audit(S12): the second-session continuation lane's filings — S-… | Evidence — S-054: the N-013 destination-arming regression (git-proven) | B |
| diff-to-fix/evidence/decompiled-quotes/s12-lostpart-register-channel.md | 25e408b8 2026-10-28 reconcile(S12/post-rebase): the hottest-window absorption — fou… | Evidence — M-118: the three-source lost-partition candidate assembly (mbr / filesystem / … | B |
| diff-to-fix/evidence/decompiled-quotes/s12-partition-read-clamp.md | 25e408b8 2026-10-28 reconcile(S12/post-rebase): the hottest-window absorption — fou… | Evidence — M-119: the og's partition-boundary read semantics (clamp + zero-fill + success) | B |
| diff-to-fix/evidence/decompiled-quotes/s12-rsf-fifth-record.md | 1bfeda19 2026-10-28 audit(S12): the second-session continuation lane's filings — S-… | Evidence — N-142: the .rsf variant-save device-descriptor record (the 5th GUID) | B |
| diff-to-fix/evidence/decompiled-quotes/s2-exfat-checksumdbr.md | f620234b 2026-10-25 diff-audit-s2: T4 FAT/exFAT deep pass — OQ-T2 RESOLVED (CDouble… | s2-exfat-checksumdbr — ExFATFixer::ChecksumDBR skip-set decoded (OQ-T5) | B |
| diff-to-fix/evidence/decompiled-quotes/s2-fat-double-fattable.md | f620234b 2026-10-25 diff-audit-s2: T4 FAT/exFAT deep pass — OQ-T2 RESOLVED (CDouble… | s2-fat-double-fattable — CDoubleFATTable: vtables (RTTI-extracted), mirror consult, const… | B |
| diff-to-fix/evidence/decompiled-quotes/s2-ldm-gpt-gate.md | 23adad1b 2026-10-25 diff-audit-s2: T7 deep pass (rebased onto close-out leg) — A-00… | s2-ldm-gpt-gate — the og's GPT-LDM discovery path (decompiled quotes) | B |
| diff-to-fix/evidence/decompiled-quotes/s2-ldm-raid5-rotation.md | 23adad1b 2026-10-25 diff-audit-s2: T7 deep pass (rebased onto close-out leg) — A-00… | s2-ldm-raid5-rotation — Raid5VolumeStorage read/write/reconstruct (verbatim quotes) | B |
| diff-to-fix/evidence/decompiled-quotes/s2-zfs-utf8-conversion.md | 23adad1b 2026-10-25 diff-audit-s2: T7 deep pass (rebased onto close-out leg) — A-00… | s2-zfs-utf8-conversion — ZfsSearchDLL's CP_UTF8 name conversion (verbatim quote) | B |
| diff-to-fix/evidence/decompiled-quotes/s25-oq-s5-1-checkversion-dead-shipped.md | 78371635 2026-10-26 diff-fix(DFX25-T4f): OQ-S5-1 RESOLVED — CheckVersion.mo is dead… | s25 — OQ-S5-1 resolution: CheckVersion.mo is dead-shipped (MATCH-by-omission) | B |
| diff-to-fix/evidence/decompiled-quotes/s25-oq-s6-1-generator-sets-and-dat-files.md | be3a8a97 2026-10-26 diff-fix(DFX25-T4b): OQ-S6-1 RESOLVED — PartitionRecoveryD gene… | s25 — OQ-S6-1 decode: PartitionRecoveryD generator-set semantics + lostpart.dat / partinf… | B |
| diff-to-fix/evidence/decompiled-quotes/s25-oq-s6-2-btrfs-rc4-false-positive.md | cbe295b1 2026-10-26 diff-fix(DFX25-T4c/T4d): OQ-S6-3 + OQ-S6-2 RESOLVED — the CRAW … | s25 — OQ-S6-2 decode: the "RC4 PRGA" in BtrfsSearchDLL is a capa FALSE POSITIVE | B |
| diff-to-fix/evidence/decompiled-quotes/s25-oq-s6-3-craw-reader-decode.md | 90f1adc1 2026-10-26 diff-fix(DFX25-T4d): OQ-S6-3 RESOLVED — the CRAW reader decode … | s25 — OQ-S6-3 decode: the CRAW reader's +0xF4 / +0x94 / +0x56 consumption (DRWUI) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-calcpartition-scoring.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | S3-LOST evidence — OQ-T9 decode: the CalcPartition family, the real candidate scoring, an… | B |
| diff-to-fix/evidence/decompiled-quotes/s3-ext4-inventory.md | 142f57a9 2026-10-25 diff-audit-s3: wave 2 — engine remainders closed: S-021 HFS+ jo… | s3-ext4-inventory — EXT4SearchDLL function/feature inventory (S3-EHB, session 3) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-filefilter-health.md | 85cc08ce 2026-10-25 diff-audit-s3 (storage/census parallel leg, integrated): 19 ren… | S3 evidence — FileFilter.xml engine-side consumption + DiskHealth grading decode | B |
| diff-to-fix/evidence/decompiled-quotes/s3-hfsplus-inventory.md | b28701fb 2026-10-25 reconcile: S-021 convergence - one journal harvest (s5 pass + D… | s3-hfsplus-inventory — HFSPlusSearchDLL function/feature inventory (S3-EHB, session 3) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-jpeg-eoi-encoder-correction.md | 2adfa29e 2026-10-25 audit-trail: S-018 INVALID — the JPEG EOI premise is refuted by… | s3-jpeg-eoi-encoder-correction — S-018 premise refuted (first-hand, diff-fix session 3) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-ntfs-indx-second-source.md | 142f57a9 2026-10-25 diff-audit-s3: wave 2 — engine remainders closed: S-021 HFS+ jo… | s3-ntfs-indx-second-source — og INDX/$INDEX_ROOT directory-entry harvest (first-hand deco… | B |
| diff-to-fix/evidence/decompiled-quotes/s3-ntfs-usn-gate.md | 142f57a9 2026-10-25 diff-audit-s3: wave 2 — engine remainders closed: S-021 HFS+ jo… | s3-ntfs-usn-gate — og $UsnJrnl harvest reason-gate + bounds (first-hand decompile read, S… | B |
| diff-to-fix/evidence/decompiled-quotes/s3-rawsearch-inventory.md | 3ac4b99c 2026-10-28 fix(carve): M-118 - the Outlook Express Dbx analyser lands (ren… | S3 evidence — RAWSearchDLL analyser/format inventory + scan-dispatch decode (OQ-T8 resolv… | B |
| diff-to-fix/evidence/decompiled-quotes/s3-rawsigns-consumer-decode.md | 85cc08ce 2026-10-25 diff-audit-s3 (storage/census parallel leg, integrated): 19 ren… | S3 evidence — raw_signs.json loader + matcher + consumer decode (RAWSearchDLL) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md | bdcb77ed 2026-10-25 diff-audit-s3: raw-signs deep decode — S-012 SEVERE (min/max Ki… | Evidence — RAWSearchDLL raw_signs.json semantics (session 3 decode) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-refs-engine-inventory.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | S3-REFS evidence — ReFSSearchDLL full-capability inventory (function-by-function map) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-refs-file-pipeline.md | 805eebd2 2026-10-25 diff-audit-s3: ReFS first-class deep pass — M-022 MAJOR (file-v… | Evidence — ReFSSearchDLL file pipeline decode (session 3) | B |
| diff-to-fix/evidence/decompiled-quotes/s3-virtual-volume-device.md | 85cc08ce 2026-10-25 diff-audit-s3 (storage/census parallel leg, integrated): 19 ren… | S3 evidence — CPartitionVirtualVolumeDevice decode (DRWUI + HFSPlusSearchDLL) | B |
| diff-to-fix/evidence/decompiled-quotes/s30-oq-flt1-extension-table-distribution.md | 67cd6ec5 2026-10-26 diff-fix(DFX30-T1): OQ-FLT-1 resolved — extension table is a 4-… | OQ-FLT-1 resolution evidence — the 169-entry extension table is a 4-engine shared vocabul… | B |
| diff-to-fix/evidence/decompiled-quotes/s30-oq-z5-zfs-quick-vs-complete.md | 04c9b9c6 2026-10-26 diff-fix(DFX30-T2): OQ-Z5 resolved — ZFS quick/complete = the I… | OQ-Z5 resolution evidence — CZfsSearchController vs CZfsQuickSearch: the IComplete/IQuick… | B |
| diff-to-fix/evidence/decompiled-quotes/s31-oq-rs-refs-residuals.md | 6a63bca3 2026-10-26 fix(OQ-RS-7): the ReFS system-table set — {0x500, 0x600, 0x701}… | s31 — OQ-RS resolved: the ReFS residual probes (RS-1..RS-8) | B |
| diff-to-fix/evidence/decompiled-quotes/s31-oq-s20-1-bitlocker-derivation-scope.md | 8bd4f6d2 2026-10-26 diff-fix(DFX31-T4): OQ-S20-1 RESOLVED (premise-corrected) — the… | s31 — OQ-S20-1 resolved: the BitLocker derivation-interop premise corrected — the og dele… | B |
| diff-to-fix/evidence/decompiled-quotes/s31-oq-s2m-2-bonus-setter-zfs-name-btrfs-anchor.md | 21fff0b8 2026-10-26 diff-fix(DFX31-T1): OQ-S2M-2 RESOLVED — the +0x68 bonus-setter … | s31 — OQ-S2M-2 resolution quotes: the +0x68 bonus setter family, the ZFS "pool name", the… | B |
| diff-to-fix/evidence/decompiled-quotes/s31-oq-s3-jpeg-fallback-decode.md | ca8cc963 2026-10-26 fix(S-018 re-filed): the truncated-JPEG salvage — validate_jpeg… | s31 — OQ-S3-JPEG resolved: the legacy fallback tier ACCEPTS no-EOI JPEGs at its scan limi… | B |
| diff-to-fix/evidence/decompiled-quotes/s4-crecoverability-grading-engine.md | 3a6257cc 2026-10-25 diff-audit-s4 wave 1 (rebased+renumbered): first-time module au… | Decompiled evidence — session 4: the og's CRecoverability grading engine + RecoveryAssist… | B |
| diff-to-fix/evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md | 3a6257cc 2026-10-25 diff-audit-s4 wave 1 (rebased+renumbered): first-time module au… | Decompiled evidence — session 4: driver client pattern + engine factory contract (M-030 /… | B |
| diff-to-fix/evidence/decompiled-quotes/s41-registry-key-residue.md | c9b9d094 2026-10-28 merge(S42-B/hot-window): the continuation audit lane absorbed o… | S41/T9 evidence — the registry-key residue sweep (the never-filed N-026 territory) | B |
| diff-to-fix/evidence/decompiled-quotes/s42-config-layer-dispositions.md | 271112a0 2026-10-28 audit(S42/wave3): I-036 FILED (INFO - the config-layer disposit… | S42 — the config-layer disposition register (the four un-dispositioned surfaces + the I-0… | B |
| diff-to-fix/evidence/decompiled-quotes/s42-defaultlog-ldq-decode.md | 43437fe3 2026-10-28 audit(S42/wave5): the DefaultLog.txt real-run decode — M-119's … | S42 — the DefaultLog.txt real-run decode: the Ldq read-parameter layer (M-119's machinery… | B |
| diff-to-fix/evidence/decompiled-quotes/s42-grading-gate-polarity.md | 27a70db0 2026-10-28 audit(S42)+reconcile: the focus-battery filings REBASED over th… | s42 evidence — the grading gate polarity decode (S-058 / S-061 / M-133 / M-134 / N-183's … | B |
| diff-to-fix/evidence/decompiled-quotes/s42-hfs-forkfield-resolution.md | db7332e6 2026-10-28 audit(S42/wave2): the HFS+ residual resolutions — N-038's field… | S42 — the HFS+ fork-selection field-name resolution (N-038's UNVERIFIED residual CLOSED) … | B |
| diff-to-fix/evidence/decompiled-quotes/s42-lostpart-surface-batch.md | 27a70db0 2026-10-28 audit(S42)+reconcile: the focus-battery filings REBASED over th… | s42 evidence — the lost-partition surfaced-shape batch (M-137's worksheet) | B |
| diff-to-fix/evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md | c9b9d094 2026-10-28 merge(S42-B/hot-window): the continuation audit lane absorbed o… | S42 evidence — the continuation-audit lane's first-hand decode pack | B |
| diff-to-fix/evidence/decompiled-quotes/s42-reconstruction-export-chain.md | 27a70db0 2026-10-28 audit(S42)+reconcile: the focus-battery filings REBASED over th… | s42 evidence — the reconstruction/export chain (S-059 / S-060 / M-135 / M-136's shared wo… | B |
| diff-to-fix/evidence/decompiled-quotes/s42-scanmode-state-machine.md | 27a70db0 2026-10-28 audit(S42)+reconcile: the focus-battery filings REBASED over th… | s42 evidence — the scan-mode state-machine decode (I-044 / N-181 / N-182's worksheet) | B |
| diff-to-fix/evidence/decompiled-quotes/s5-grading-extension-table-dump.md | 994e0bb0 2026-10-28 merge(hot-window-2): the S57/S55-window absorbed - the remote's… | Context — OQ-S4-1: the og's 169-entry grading extension table (VA 0x141bb0500) | B |
| diff-to-fix/evidence/decompiled-quotes/s5-image-fscheck-persistence.md | 10d4d9f7 2026-10-25 diff-audit-s5b: the never-dispositioned MOM/product-capabilitie… | Evidence — Session-5 disk-image scan + FS-analyser/check-phase + lostpart persistence + F… | B |
| diff-to-fix/evidence/decompiled-quotes/s5-mom-capabilities.md | 10d4d9f7 2026-10-25 diff-audit-s5b: the never-dispositioned MOM/product-capabilitie… | Evidence — Session-5 MOM/product-capability decode (decompiled quotes) | B |
| diff-to-fix/evidence/decompiled-quotes/s5-oq-s4-1-extension-table-dump.md | 10d4d9f7 2026-10-25 diff-audit-s5b: the never-dispositioned MOM/product-capabilitie… | Evidence — OQ-S4-1 RESOLVED: the CRecoverability 169-entry extension table dumped | B |
| diff-to-fix/evidence/decompiled-quotes/s5-oq-s4-2-raw-naming-hook.md | 10d4d9f7 2026-10-25 diff-audit-s5b: the never-dispositioned MOM/product-capabilitie… | Evidence — OQ-S4-2 RESOLVED: the RAW naming hook's engine-side consumption decoded | B |
| diff-to-fix/evidence/decompiled-quotes/s5-session5-corpus-quotes.md | d7e05c92 2026-10-25 diff-audit-s5 (2nd renumber: past the parallel closing-session'… | Session-5 corpus quotes — the never-cited artifacts read in full (evidence pack) | B |
| diff-to-fix/evidence/decompiled-quotes/s50-grading-deep-read-subgates.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | S50 evidence — the grading deep-read sub-gates decoded (OQ-S42-3 closed) | B |
| diff-to-fix/evidence/decompiled-quotes/s50-landing-divergences.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | S50 evidence — the three landing divergences, both sides quoted (S-062 / M-138 / M-139) | B |
| diff-to-fix/evidence/decompiled-quotes/s50-ntfs-btrfs-time-precedence-decodes.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | S50 evidence — the tracked-open time-precedence probes decoded (OQ-S42-1, OQ-S42-2) | B |
| diff-to-fix/evidence/decompiled-quotes/s54-finding-quote-pack.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | S54 decompiled-quote pack — the lead-verified evidence for the session-54 filings | B |
| diff-to-fix/evidence/decompiled-quotes/s6-carve-family-layer.md | 517c56ca 2026-10-25 diff-audit-s6 recon: multi-leg ID reconciliation — 3 convergent… | Evidence — session 6: the RAWSearchDLL family layer (never-cited artifacts, first consump… | B |
| diff-to-fix/evidence/decompiled-quotes/s6-health-grading-decode.md | 5879adeb 2026-10-25 fix(S-024): decoded-truth upgrade — the health grading engine n… | s6-health-grading-decode — the DiskHealth.exe grading engine, decoded first-hand (S-024 u… | B |
| diff-to-fix/evidence/decompiled-quotes/s6-mom-winpe-config-wiring.md | 517c56ca 2026-10-25 diff-audit-s6 recon: multi-leg ID reconciliation — 3 convergent… | Evidence — session 6: MOM/WinPE/config/UIUX wiring layer (16 never-cited artifacts, first… | B |
| diff-to-fix/evidence/decompiled-quotes/s6-session-format-record4-and-ifileproperties.md | 517c56ca 2026-10-25 diff-audit-s6 recon: multi-leg ID reconciliation — 3 convergent… | Evidence — session 6: session-format record-4 grammar + IFileProperties payloads (first c… | B |
| diff-to-fix/evidence/decompiled-quotes/s6-writer-collision-and-mkdir.md | 517c56ca 2026-10-25 diff-audit-s6 recon: multi-leg ID reconciliation — 3 convergent… | Evidence — session 6: writer collision naming + mkdir acceptance gates (DRWUI decompile, … | B |
| diff-to-fix/evidence/decompiled-quotes/s8-autosave-and-stamps.md | 24e93ad3 2026-10-28 audit(session-8/wave-2): the continuation lane — 16 findings + … | S8 evidence — the og's auto-save trigger gates + config model | B |
| diff-to-fix/evidence/decompiled-quotes/s8-batch2-absence-sweep.md | 8ac5211f 2026-10-28 audit(S-040): WMF placeable carve validator contradicts the og … | Evidence — S-041 batch-2 decode resolutions + absence sweep (session 8) | B |
| diff-to-fix/evidence/decompiled-quotes/s8-batch2-formats.md | 24e93ad3 2026-10-28 audit(session-8/wave-2): the continuation lane — 16 findings + … | S8 evidence — the og's carve batch-2 format grammars (decode sheet) | B |
| diff-to-fix/evidence/decompiled-quotes/s8-driver-lane-audit.md | 7922b5e1 2026-10-28 audit(session-8/third-lane): the driver-lane findings + repairs… | Evidence — the driver-lane audit (session 8, M-086/M-087/M-089/N-107) | B |
| diff-to-fix/evidence/decompiled-quotes/s8-thumbnail-pipeline.md | 24e93ad3 2026-10-28 audit(session-8/wave-2): the continuation lane — 16 findings + … | S8 evidence — the og's thumbnail pipeline (first-hand reads) | B |
| diff-to-fix/evidence/decompiled-quotes/s8-wmf-placeable-grammar.md | 24e93ad3 2026-10-28 audit(session-8/wave-2): the continuation lane — 16 findings + … | S8 evidence — the og's WMF placeable grammar (first-hand read) | B |
| diff-to-fix/evidence/decompiled-quotes/s8-wmf-placeable-validator.md | 8ac5211f 2026-10-28 audit(S-040): WMF placeable carve validator contradicts the og … | Evidence — S-040 WMF placeable validator (both decompilers + core, first-hand) | B |
| diff-to-fix/evidence/decompiled-quotes/s9-batch2-registrations.md | df0d5c0f 2026-10-28 worklog(session-9/close): the T19+T20 record — the push-time re… | Evidence: the batch-2 format registrations + gates (filed under the S-041 parent + the pe… | B |
| diff-to-fix/evidence/decompiled-quotes/s9-chm-gate-asm.md | ea591285 2026-10-28 audit(session-9): the wave-2 findings — S-040 WMF placeable lay… | Evidence: the CHM gate — the deferred compare-width puzzle, resolved at instruction level | B |
| diff-to-fix/evidence/decompiled-quotes/s9-crw-mrw-registrations.md | df0d5c0f 2026-10-28 worklog(session-9/close): the T19+T20 record — the push-time re… | Evidence: CRW vs MRW key attribution (M-081, FIXED at 9e5dedc — the SA-9 lane's convergen… | B |
| diff-to-fix/evidence/decompiled-quotes/s9-mid-gz-bounds.md | df0d5c0f 2026-10-28 worklog(session-9/close): the T19+T20 record — the push-time re… | Evidence: MID/GZ og ctor bounds vs our registry caps (N-093 — merged at the push-time rec… | B |
| diff-to-fix/evidence/decompiled-quotes/s9-wmf-placeable-gate.md | df0d5c0f 2026-10-28 worklog(session-9/close): the T19+T20 record — the push-time re… | Evidence: the og's placeable-WMF gate vs our validator (filed as S-042 after the push-tim… | B |
| diff-to-fix/evidence/executions/a007-extents-off-by-two.md | 3b81dbc7 2026-10-25 merge: converge with the 5b0bc7b arc (S-025/S-028/api-close) — … | A-006 fail-before / pass-after execution record | B |
| diff-to-fix/evidence/executions/build-suite-session2.md | 2ce2e6ba 2026-10-25 diff-audit s2 third leg (reconciled): decompile-exhaustive pass… | Execution — Linux build + full test suite at the session-2 audited commit (2026-10-25) | B |
| diff-to-fix/evidence/executions/gpt-guid-probe.md | abe26b8e 2026-10-25 fix: A-002 reconciliation + cross-encoding guard (parallel fix … | gpt-guid-probe — executed evidence (session, main auditor) | B |
| diff-to-fix/evidence/executions/s003-s021-verification-corrections.md | 6c8d6a64 2026-10-25 fix(S-003/S-021 verification): JIB layout corrected (no magic f… | S-003/S-021 verification-correction fail-before / pass-after record | B |
| diff-to-fix/evidence/executions/s021-failbefore-journal.md | c75f646f 2026-10-25 fix: S-021 + S-003 - HFS+ journal differential harvest (the og'… | S-021 / S-003 fail-before / pass-after execution log (2026-10-26) | B |
| diff-to-fix/evidence/executions/s10-preview-text-eligibility.md | 0e279e43 2026-10-28 reconcile(S10-B): the push-time reconciliation with the four co… | s10-preview-text-eligibility.md — EXECUTED proof: the text-preview eligibility model (M-1… | B |
| diff-to-fix/evidence/executions/s10-rsf-ground-truth.md | 0e279e43 2026-10-28 reconcile(S10-B): the push-time reconciliation with the four co… | s10-rsf-ground-truth.md — EXECUTED: first-hand parse of the og's three real SaveScan .rsf… | B |
| diff-to-fix/evidence/executions/s10-wholedisk-scan-targets.md | f6c00177 2026-10-28 audit(session-10/wave-3): the fix-wave landing-parity verificat… | Executed evidence — s10-wholedisk-scan-targets (the whole-disk scan flow: the og reads th… | B |
| diff-to-fix/evidence/executions/s10b-real-rsf-session-mining.md | e27e7c16 2026-10-28 audit(SA-10B/L-B): the real-rsf session mining — 3 findings + t… | Executed evidence — s10b-real-rsf-session-mining (the first mining of the corpus's three … | B |
| diff-to-fix/evidence/executions/s12-build-execution.md | 1bfeda19 2026-10-28 audit(S12): the second-session continuation lane's filings — S-… | S12/D — BUILD + EXECUTION VERIFICATION LOG | B |
| diff-to-fix/evidence/executions/s2-ldm-guid-constants.md | 23adad1b 2026-10-25 diff-audit-s2: T7 deep pass (rebased onto close-out leg) — A-00… | s2-ldm-guid-constants — extracted LDM GUID constants from og binaries + core comparison | B |
| diff-to-fix/evidence/executions/s2-ldm-real-guid-proof.md | 23adad1b 2026-10-25 diff-audit-s2: T7 deep pass (rebased onto close-out leg) — A-00… | s2-ldm-real-guid-proof — executed proof of the A-002 regression | B |
| diff-to-fix/evidence/executions/s27-m058-readpipe-uaf-asan.md | 75d29b6d 2026-10-26 fix(M-058): the read-pipe cache-failure fallback use-after-free… | M-058 execution evidence — the pre-fix use-after-free, reproduced under ASan | B |
| diff-to-fix/evidence/executions/s4-preview-caps-bypass.md | a6962302 2026-10-25 diff-audit-s4 evidence: embed the preview-caps harness source v… | Executed evidence — session 4: PreviewReader Caps.max_read bypass on the zip/docx path (M… | B |
| diff-to-fix/evidence/executions/s4-suite-green-and-abi-check.md | 3a6257cc 2026-10-25 diff-audit-s4 wave 1 (rebased+renumbered): first-time module au… | Executed evidence — session 4: full suite + C-ABI check green at the audited commit | B |
| diff-to-fix/evidence/executions/s42-build-at-head.md | 30e191ca 2026-10-28 trail(S42/M-120): CONVERGENT-RESOLVED intra-session — the paral… | S42 — the from-scratch build at HEAD (the W4 executed verification): cr_core/cr_ui clean,… | B |
| diff-to-fix/evidence/executions/s42-build-suite-execution.md | 27a70db0 2026-10-28 audit(S42)+reconcile: the focus-battery filings REBASED over th… | s42 execution log — the W4 build + full-suite run at the audited HEAD `b26017a7` | B |
| diff-to-fix/evidence/executions/s42-suite-red-at-head.md | 73a4693a 2026-10-28 audit(S42): the continuation lane — the 49-finding liveness swe… | S42 execution evidence — the suite is RED at HEAD `869da779` (Linux release build) | B |
| diff-to-fix/evidence/executions/s42c-red-main-reproduction.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | S42-B executed evidence — the red-main reproduction (the corroboration of N-157) | B |
| diff-to-fix/evidence/executions/s5-baseline-suite-green.md | eb08d955 2026-10-25 diff-audit-s5 rebased (renumbered past the parallel legs; confl… | Session-5 baseline — full suite green at the audited commit | B |
| diff-to-fix/evidence/executions/s50-landing-parity-sweep.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | S50 evidence — the 25-landing corpus-parity sweep (the regression watch) | B |
| diff-to-fix/evidence/executions/s9-carve-registry-inventory.md | df0d5c0f 2026-10-28 worklog(session-9/close): the T19+T20 record — the push-time re… | Evidence: the carve-registry inventory (product-absence proof at 35adcb9 — pre-b92ebef ba… | B |
| diff-to-fix/evidence/executions/s9-m053-abi-fixture-regression.md | 797af183 2026-10-25 fix(M-053): the M-015 landing missed the C-consumer fixture — u… | Executed evidence — session 9: M-053 baseline-regression proof (before/after) | B |
| diff-to-fix/evidence/s12-lane-sheets/lane-A-sector-math.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | LANE A — SECTOR NUMBERS / LBA MATH — deep-dive audit (read-only) | B |
| diff-to-fix/evidence/s12-lane-sheets/lane-B-scan-progress.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | LANE B — SCAN STRATEGY & PROGRESS MATHEMATICS & NO-STUCK/FREEZE (read-only audit) | B |
| diff-to-fix/evidence/s12-lane-sheets/lane-C-localdb.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | LANE C — LOCAL DATABASE / EUUserData FAMILY / USER-SIGNATURE SYSTEM / PER-SCAN STATISTICS | B |
| diff-to-fix/evidence/s12-lane-sheets/lane-D-statemgmt.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | LANE D — STATE MANAGEMENT (read-only audit): scan lifecycle, cancel/pause/resume, checkpo… | B |
| diff-to-fix/evidence/s12-lane-sheets/lane-E-detection-calc.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | Lane E — DETECTION ACCURACY & CALCULATION MATHEMATICS (deep-dive) | B |
| diff-to-fix/evidence/s12-liveness/liveness-1.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | Liveness battery — batch 1 (14 ACTIVE findings) | B |
| diff-to-fix/evidence/s12-liveness/liveness-2.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | Liveness battery batch 2 (Task L2) | B |
| diff-to-fix/evidence/s12-liveness/liveness-3.md | f85def0f 2026-10-28 audit(S12/findings-wave-1): 39 new findings filed (RENUMBERED p… | Liveness battery — batch 3 (L3) | B |
| diff-to-fix/evidence/s3-agent-logs/PROTOCOL.md | d7e05c92 2026-10-25 diff-audit-s5 (2nd renumber: past the parallel closing-session'… | S3 PROTOCOL — shared rules for session-3 deep-pass agents (READ FIRST, THEN YOUR TASK PRO… | B |
| diff-to-fix/evidence/s3-agent-logs/S3-CARVE.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | S3-CARVE scratch report (consolidated by the session-3 lead) | B |
| diff-to-fix/evidence/s3-agent-logs/S3-LOST.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | S3-LOST scratch report — OQ-T9 CalcPartition scoring decode + lost-partition deep pass | B |
| diff-to-fix/evidence/s3-agent-logs/S3-NTFS2.md | 142f57a9 2026-10-25 diff-audit-s3: wave 2 — engine remainders closed: S-021 HFS+ jo… | S3-NTFS2 — NTFS remainder deep pass (agent scratch report) | B |
| diff-to-fix/evidence/s3-agent-logs/S3-REFS.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | S3-REFS scratch report — ReFS deep pass + ADS gap filing (session 3) | B |
| diff-to-fix/evidence/s55-lane-sheets/S55-D1-localdb.md | b2546602 2026-10-29 audit(S55): the sixth continuation lane's filings - 5 lead-veri… | S55-D1 — local-database family deep-dive (lostpart.dat / partinfo.dat / EUUserData) | B |
| diff-to-fix/evidence/s55-lane-sheets/S55-D2-scanmath.md | b2546602 2026-10-29 audit(S55): the sixth continuation lane's filings - 5 lead-veri… | S55-D2 — SCAN STRATEGY + SECTOR/LBA MATH DEEP-DIVE (read-only context sheet) | B |
| diff-to-fix/evidence/s55-lane-sheets/S55-D3-grading.md | b2546602 2026-10-29 audit(S55): the sixth continuation lane's filings - 5 lead-veri… | S55-D3 context sheet — the grading deep-read subgates (FUN_1401bc5a0 / FUN_14038c470) + t… | B |
| diff-to-fix/evidence/s55-lane-sheets/S55-L1-liveness-tier1.md | b2546602 2026-10-29 audit(S55): the sixth continuation lane's filings - 5 lead-veri… | S55-L1 — Liveness battery, tier 1 (ACTIVE findings vs core HEAD) | B |
| diff-to-fix/evidence/s55-lane-sheets/S55-L2-liveness-tier2.md | b2546602 2026-10-29 audit(S55): the sixth continuation lane's filings - 5 lead-veri… | S55-L2 — Liveness battery, tier 2 (context sheet) | B |
| diff-to-fix/evidence/s68-lane-sheets/S68-L1-registry-initializer-decode.md | 07674b4e 2026-10-29 audit(S68) L3: the raw_signs content verification - all 199 hea… | S68 lane sheet L1 — the RAWSearchDLL static registry initializer, fully decoded (66 keys) | B |
| diff-to-fix/evidence/s69-lane-sheets/S69-A-localdb.md | 04fb5e88 2026-10-29 audit(S69): the ninth continuation lane's filings — 5 new + 2 a… | S69-A context sheet — local database & persisted state | B |
| diff-to-fix/evidence/s69-lane-sheets/S69-B-scanstatemachine.md | 04fb5e88 2026-10-29 audit(S69): the ninth continuation lane's filings — 5 new + 2 a… | S69-B context sheet — scan state machine & the N-182 resolver decode | B |
| diff-to-fix/evidence/s69-lane-sheets/S69-C-sectormath.md | 04fb5e88 2026-10-29 audit(S69): the ninth continuation lane's filings — 5 new + 2 a… | S69-C context sheet — sector/LBA/calculation arithmetic fresh eyes | B |
| diff-to-fix/evidence/s69-lane-sheets/liveness-battery.md | 04fb5e88 2026-10-29 audit(S69): the ninth continuation lane's filings — 5 new + 2 a… | S69 lane — liveness battery of the active board (first-hand, at core `3799dee2`) | B |
| diff-to-fix/evidence/s79-lane-sheets/agentA-rawsearch-pipeline.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | Agent A — RAWSearchDLL raw-carving pipeline census (function-level) | B |
| diff-to-fix/evidence/s79-lane-sheets/agentB-ntfs-pipeline.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | Agent B — NTFSSearchDLL function-level pipeline census (parity ground truth) | B |
| diff-to-fix/evidence/s79-lane-sheets/agentC-fs-pipelines.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | Agent C — FS Engines Function-Level Pipeline Census (FAT / exFAT / ext4 / HFS+ / Btrfs) | B |
| diff-to-fix/evidence/s79-lane-sheets/agentD-core-map.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | Agent D — core_recovery STAGE-AND-BOUNDS MAP (scan orchestration / state / session persis… | B |
| diff-to-fix/evidence/s79-lane-sheets/agentE-math-census.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | Agent E — NUMERIC-CONSTANT CENSUS: og calculation / grading / scoring math vs core_recove… | B |
| diff-to-fix/evidence/s79-lane-sheets/s79-lead-verification.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | s70-lead-verification.md — the S79 lane's lead-verification sheet (every filed candidate,… | B |
| diff-to-fix/evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md | 0eaded3d 2026-10-29 fix(carve): M-156 — the X3F surface's fifth touch: the ASCII ve… | S83-L1 — the X3F two-generation chain + the dispatcher-mechanism refutation, decoded firs… | B |
| diff-to-fix/evidence/s83-lane-sheets/s83-l2-crw-ciff-walk-decode.md | 0cf6142d 2026-10-29 fix(carve): M-107's CRW member — the CIFF heap size walk replac… | S83-L2 — the CRW CIFF heap size walk, decoded first-hand from the raw asm (M-107's CRW me… | B |
| diff-to-fix/findings/FIXED/A-012-carve-registry-lost-fmt-svg-row-in-rebase-sweep.md | ad0535d6 2026-10-28 fix(S-051+M-104): the whole-disk partition-list targets + the p… | ACUTE A-012 — the carve registry lost its FMT_SVG row in the 81ee9775 rebase-window confl… | B |
| diff-to-fix/findings/FIXED/A-013-cli-resume-without-out-truncates-session-before-read.md | 4f0fc160 2026-10-28 fix(cli): A-013 - the resume-without-out flow never truncates t… | ACUTE A-013 — the CLI's `--resume`-without-`--out` flow truncates the session file BEFORE… | B |
| diff-to-fix/findings/FIXED/A-014-fat-time-word-validation-fabricates-shifted-timestamps.md | 612577cb 2026-10-29 repair(window): the 625a1e59 committed-red main closed - the 4t… | ACUTE A-014 — FAT time-word validation: the core accepts hour≥24 / minute≥60 / 2s-field≥3… | B |
| diff-to-fix/findings/FIXED/A-016-ntfs-root-dot-node-prunes-whole-volume-under-folder-filter.md | e2ea4d50 2026-10-29 fix(orchestrator/tree): A-016 + N-218 — the convergent-verifica… | ACUTE A-016 — the folder filter's path walk treats the NTFS volume-root "." node as a pat… | B |
| diff-to-fix/findings/FIXED/I-026-devsign-unsigned-by-design-posture.md | 0ff6cc92 2026-10-28 reconcile(S42-main/v3): the INFO-tier product re-landed over th… | INFORMATIONAL I-026 — the driver ships unsigned-by-design with CI/dev test-signing (vs th… | B |
| diff-to-fix/findings/FIXED/I-027-og-codebase-data-asset-register.md | 0ff6cc92 2026-10-28 reconcile(S42-main/v3): the INFO-tier product re-landed over th… | INFORMATIONAL I-027 — og-codebase data-asset ground-truth register (session-8 sweep): the… | B |
| diff-to-fix/findings/FIXED/I-028-thumbnail-superset-register.md | 468cd777 2026-10-28 merge(S42-D): the third continuation lane absorbed over the hot… | INFORMATIONAL I-028 — Thumbnail engine superset register: deliberate core improvements ov… | B |
| diff-to-fix/findings/FIXED/I-029-driver-contract-code-count-corpus-grounding.md | 0ff6cc92 2026-10-28 reconcile(S42-main/v3): the INFO-tier product re-landed over th… | INFORMATIONAL I-029 — DRIVER-CONTRACT's "EUDCPEPM contract — 19 recognized codes, byte-fo… | B |
| diff-to-fix/findings/FIXED/I-031-rawsearchdll-infra-register.md | 0ff6cc92 2026-10-28 reconcile(S42-main/v3): the INFO-tier product re-landed over th… | INFORMATIONAL I-031 — the RAWSearchDLL infrastructure register: the function-level walk's… | B |
| diff-to-fix/findings/FIXED/I-032-zfsrefs-function-walk-register.md | 468cd777 2026-10-28 merge(S42-D): the third continuation lane absorbed over the hot… | INFORMATIONAL I-032 — the ZFS/ReFS function-level walk register: OQ-T6 resolved first-han… | B |
| diff-to-fix/findings/FIXED/I-033-config-data-asset-register-extension-batch.md | bbbe78b2 2026-10-29 repair(trail): N-210 - the M-120 class recurred: the f3da99f1 m… | INFORMATIONAL I-033 — the config/data-asset register-extension batch: every engine-behavi… | B |
| diff-to-fix/findings/FIXED/I-034-never-cited-artifact-register.md | ddd7c5dc 2026-10-28 fix(trail): N-160 - the register-truth batch landed (I-034 cens… | INFORMATIONAL I-034 — The never-cited corpus-artifact register: 23 artifacts with zero tr… | B |
| diff-to-fix/findings/FIXED/I-035-chk-repair-chain-decode-addendum-for-s050.md | a0533cea 2026-10-28 trail(I-035)+ack: the CHK repair-chain decode register ACKNOWLE… | INFORMATIONAL I-035 — the CHK repair chain decode addendum: rename-to-identified-ext sema… | B |
| diff-to-fix/findings/FIXED/I-036-user-sign-default-load-cli-only-posture.md | 55e88af0 2026-10-28 ack(trail): I-036 - the user-sign default-load posture register… | INFORMATIONAL I-036 — the user-sign "default-on" posture is CLI-app-level only: the engin… | B |
| diff-to-fix/findings/FIXED/I-037-s42-never-walked-decode-register.md | c9b9d094 2026-10-28 merge(S42-B/hot-window): the continuation audit lane absorbed o… | INFORMATIONAL I-037 — The S42 never-walked decode register: SearchResult.txt (the QA-gate… | B |
| diff-to-fix/findings/FIXED/I-038-diskhealth-companion-tree-register.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | INFORMATIONAL I-038 — the og-codebase/diskhealth/ companion-tree surface register (818 fi… | B |
| diff-to-fix/findings/FIXED/I-039-corpus-worklog-crosscheck-register.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | INFORMATIONAL I-039 — the corpus-worklog cross-check register (the first audit lane to re… | B |
| diff-to-fix/findings/FIXED/I-040-og-defect-non-adoption-register-batch-2.md | 1fccb0a4 2026-10-28 ack(trail): I-040 CONVERGENT - the og-defect non-adoption regis… | INFORMATIONAL I-040 — og-defect non-adoption register batch #2: the ext4 i_size_high lega… | B |
| diff-to-fix/findings/FIXED/I-041-stopped-vs-finished-heuristic-vs-stage-mask-register.md | 94ef9d37 2026-10-28 docs(register)+trail: I-041 ACKNOWLEDGED - the stopped-vs-finis… | INFORMATIONAL I-041 — stopped-vs-finished classification: the og infers "stopped" from pr… | B |
| diff-to-fix/findings/FIXED/I-042-paused-state-observability-telemetry-only-register.md | c4b57ff1 2026-10-28 docs(register)+trail: I-042 ACKNOWLEDGED - the paused-state obs… | INFORMATIONAL I-042 — paused-state observability: the og surfaces pause in the master sta… | B |
| diff-to-fix/findings/FIXED/I-043-state-ownership-single-owner-vs-unlocked-enum-register.md | d5eb7e53 2026-10-28 docs(register)+trail: I-043 ACKNOWLEDGED - the state-ownership … | INFORMATIONAL I-043 — state ownership: the og's master scan-state enum sits UNLOCKED on t… | B |
| diff-to-fix/findings/FIXED/I-044-scanmode-decode-register-batch.md | 95541b2b 2026-10-28 trail(I-044): retired to findings/FIXED/ - the scan-mode decode… | INFORMATIONAL I-044 — the scan-mode decode register: the auto-advance state machine, the … | B |
| diff-to-fix/findings/FIXED/I-045-ext4-unwritten-ee-len-non-adoption-register.md | ab6e39c1 2026-10-29 fix(ext4): I-045 - the unwritten-extent ee_len non-adoption reg… | INFORMATIONAL I-045 — Og-defect non-adoption register, batch #3: the ext4 unwritten-exten… | B |
| diff-to-fix/findings/FIXED/I-046-lostpart-partinfo-lifecycle-register-per-process-cache-delete-latch-marker-restore.md | ed033c0e 2026-10-29 fix(registers): I-045+I-046 - the INFORMATIONAL pair closed (th… | INFORMATIONAL I-046 — the og's lostpart.dat/partinfo.dat lifecycle decoded: NOT a cross-s… | B |
| diff-to-fix/findings/FIXED/I-047-ifileproperties-0x80-format-icon-id-constant-family-register.md | 86dadf2b 2026-10-28 audit(S50): the continuation audit lane's product - the 74-find… | INFORMATIONAL I-047 — The IFileProperties +0x80 per-class format/icon-ID constant family … | B |
| diff-to-fix/findings/FIXED/I-048-three-registers-selected-dead-sumary-remote-ext-size-32bit.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | INFORMATIONAL I-048 — three deliberate-difference registers from the S69 local-DB and sec… | A |
| diff-to-fix/findings/FIXED/M-073-scan-time-folder-filter-absent.md | 793c9c4b 2026-10-29 trail(S79): the M-073 seat concession — this lane's independent… | MAJOR M-073 — Scan-time folder filter absent (PushFolderFilter: the og constrains tree in… | B |
| diff-to-fix/findings/FIXED/M-076-trim-facet-enum-rows.md | 4bd5ee7a 2026-10-28 fix(abi+cli): M-076 - the per-row TRIM chip lands - the opt-in … | MAJOR M-076 — TRIM facet absent from enumeration rows (the og's per-row "TRIM ON/OFF" chi… | B |
| diff-to-fix/findings/FIXED/M-098-hik-hikvision-carve-absent-ps-private-stream-discriminator-decoded.md | ddd7c5dc 2026-10-28 fix(trail): N-160 - the register-truth batch landed (I-034 cens… | MAJOR M-098 — the HIK (Hikvision surveillance) carve format is absent: the og discriminat… | B |
| diff-to-fix/findings/FIXED/M-099-aep-after-effects-carve-absent-rifx-probe-xmp-end-decoded.md | 43c618a3 2026-10-28 fix(carve): M-099 - the AEP After-Effects project analyser land… | MAJOR M-099 — the AEP (After Effects project) carve format is absent: the og probes bare … | B |
| diff-to-fix/findings/FIXED/M-103-lostdir-parent-record-grouping-absent-flat-lost-root.md | 3cc22c52 2026-10-28 fix(ntfs+orchestrator): M-103 - the LostDir per-parent grouping… | MAJOR M-103 — The lost-files branch loses the og's per-parent-record grouping: no "File P… | B |
| diff-to-fix/findings/FIXED/M-104-wholedisk-volume-roots-anonymous-candidates.md | 3f7c190e 2026-10-28 merge(S-051 DU): the convergent S-051 union — the S40 lane's 6e… | MAJOR M-104 — table-matched known volumes surface as anonymous "Candidate @ <LBA>" roots:… | B |
| diff-to-fix/findings/FIXED/M-106-one-3ds-size-walks-decoded-but-standing-in.md | c98eb7a7 2026-10-28 fix(carve): M-106 - the ONE chunk-chain and 3DS section size wa… | MAJOR M-106 — ONE and 3DS size-walks are decoded but still standing in: the school-1 land… | B |
| diff-to-fix/findings/FIXED/M-111-explorer-partition-browse-capability-absent.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | MAJOR M-111 — the og's in-app partition file-browse capability (ExplorerPartition → CWind… | B |
| diff-to-fix/findings/FIXED/M-112-csl-sizefield-0x44c-branch-unported-unowned.md | a8c03cc2 2026-10-29 fix(carve): M-140 — the CSL/CD01 decline fall-through removed (… | MAJOR M-112 — the CSL size branch (dword@0x44c + 0x480) is decoded-but-unported and carri… | B |
| diff-to-fix/findings/FIXED/M-113-recovered-os-boot-surgery-absent.md | 532c6811 2026-10-29 fix(reghive): M-113 — the recovered-OS boot-environment write-s… | MAJOR M-113 — the recovered-OS boot-environment write-side surgery is absent and unregist… | B |
| diff-to-fix/findings/FIXED/M-114-system-restore-purge-absent-undecided.md | b2bc1970 2026-10-29 fix(session)+fix(docs): the S82 registration batch + the M-154 … | MAJOR M-114 — MainModule's silent System-Restore purge is absent and undecided: the og de… | B |
| diff-to-fix/findings/FIXED/M-115-thumbnail-grade-promotion-read-gated-not-decode-gated.md | cc81f2b6 2026-10-28 converge(M-115)+repair(gate): the read/decode promotion gate co… | MAJOR M-115 — the thumbnail grade-promotion trigger is mis-modeled: the og promotes on cl… | B |
| diff-to-fix/findings/FIXED/M-116-archive-inventory-unreachable-from-abi-m100-reopen.md | 3d886577 2026-10-28 fix(abi+cli): M-116 - the archive inventory's C-ABI half lands … | MAJOR M-116 — the archive browse inventory (M-066's engine) is unreachable from every shi… | B |
| diff-to-fix/findings/FIXED/M-117-outlook-express-dbx-analyser-absent.md | 80cc8bdc 2026-10-28 merge(S42-B/hot-window-2): the DBX renumber chain adopted whole… | MAJOR M-117 → M-121 — the ID-collision stub (Outlook Express Dbx carve analyser) | B |
| diff-to-fix/findings/FIXED/M-118-lostpart-three-source-assembly-register-channel-absent.md | 86dad938 2026-10-29 fix(lost): M-118 — the register channel lands (the og's third c… | MAJOR M-118 — The og's lost-partition candidate assembly consults THREE sources (mbr / fi… | B |
| diff-to-fix/findings/FIXED/M-119-partition-boundary-read-semantics-clamp-vs-refuse.md | a61ec2a2 2026-10-28 trail(M-119): the landing's bookkeeping - Resolution + ledger r… | MAJOR M-119 — Partition-boundary read semantics: the og's partition device CLAMPS out-of-… | B |
| diff-to-fix/findings/FIXED/M-120-committed-conflict-markers-red-main.md | 09c412b7 2026-10-28 trail(M-120): main red closed - the acceptance executed at 72a1… | MAJOR M-120 — main is RED: committed git conflict markers in tests/unit/test_carve.cpp (t… | A |
| diff-to-fix/findings/FIXED/M-121-outlook-express-dbx-analyser-absent.md | 5a84b63c 2026-10-28 renumber(carve): the Dbx finding M-118 -> M-121 per SA-9 (the p… | MAJOR M-121 — Outlook Express Dbx carve analyser entirely absent (the renumber chain: the… | B |
| diff-to-fix/findings/FIXED/M-122-stbio-read-cb-single-shot-breaks-large-png-idats.md | c7312269 2026-10-28 audit(S42-C/close): the third continuation lane's reconciled pr… | MAJOR M-122 — The StbioSource single-shot read broke every photo-scale PNG decode (the re… | B |
| diff-to-fix/findings/FIXED/M-123-gpt-header-validation-laxity-no-position-reserved-usable-range-gates.md | a127d538 2026-10-28 trail(M-123): the landing's bookkeeping - Resolution + FIXED-LE… | MAJOR M-123 — GPT header validation laxity: the core accepts headers the og's analyzer re… | B |
| diff-to-fix/findings/FIXED/M-124-ebr-link-type-0x85-linux-extended-not-a-container.md | 1cb665aa 2026-10-28 trail(M-124): the Resolution's fix-commit SHA recorded (dcb123d… | MAJOR M-124 — EBR link type 0x85 (Linux extended) is not a container in the core: a whole… | B |
| diff-to-fix/findings/FIXED/M-125-progress-display-math-piecewise-remap-99-cap-decoded-unregistered.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | MAJOR M-125 — progress display mathematics: the og's piecewise UX remap + 99.99% stall ca… | B |
| diff-to-fix/findings/FIXED/M-126-eta-model-absent-og-rate-ema-decoded.md | db82b5e6 2026-10-29 merge(hot-window): absorbed with both sides' records kept | MAJOR M-126 — the og's ETA model (rate-EMA remaining time + time-budget variant + "3m00s"… | B |
| diff-to-fix/findings/FIXED/M-127-fat-exfat-chain-cycle-guard-o-n-squared-vs-og-range-tree.md | e01255f0 2026-10-29 trail(M-127): the landing's bookkeeping - Resolution + ledger r… | MAJOR M-127 — FAT/exFAT chain cycle-guard complexity: the core's O(n²) visited-vector sca… | B |
| diff-to-fix/findings/FIXED/M-128-user-sign-hex-case-semantics-12-og-dead-signs-live-in-core.md | 13e07257 2026-10-28 trail(M-128+M-129): the landings' bookkeeping - Resolutions + l… | MAJOR M-128 — user-sign hex-case semantics: the og matches signs as UPPERCASE-hex text me… | B |
| diff-to-fix/findings/FIXED/M-129-euuserdata-parser-all-or-nothing-vs-og-tolerant-skip.md | 13e07257 2026-10-28 trail(M-128+M-129): the landings' bookkeeping - Resolutions + l… | MAJOR M-129 — EUUserData parser strictness: the og tolerates unknown members, case-insens… | B |
| diff-to-fix/findings/FIXED/M-130-fat-dentry-shape-validator-family-unported.md | 080a34d0 2026-10-28 fix(fat): M-130 - the dentry shape-validator family lands (the … | MAJOR M-130 — the FAT dentry shape-validator family (byte-histogram gate, dir-size gate, … | B |
| diff-to-fix/findings/FIXED/M-131-cli-checkpoint-truncate-in-place-torn-write-destroys-previous-good.md | 22d8f59d 2026-10-28 trail(M-131): FIXED-ALREADY - the parallel A-013 landing (4f0fc… | MAJOR M-131 — the CLI's in-run checkpoint sink is the truncate-in-place shape N-014 fixed… | B |
| diff-to-fix/findings/FIXED/M-132-gpt-backup-probe-position-dead-lba1-asymmetry.md | 4d4ba101 2026-10-28 trail(M-132): the landing's bookkeeping - Resolution + ledger r… | MAJOR M-132 — GPT backup-header probe position + dead-LBA1 asymmetry: the og always probe… | B |
| diff-to-fix/findings/FIXED/M-133-suite-red-at-head-convergent-n157-m120.md | c466d732 2026-10-28 merge(S42-D/v2): the S12 findings-wave absorbed (39 findings; b… | MAJOR M-133 (renumbered TWICE per earlier-push-wins: filed M-118 [ID consumed by the S12 … | B |
| diff-to-fix/findings/FIXED/M-134-grading-deep-read-validation-absent.md | 943c5809 2026-10-29 merge(hot-window) + repair(trail): the window's N-202/M-147 lan… | MAJOR M-134 — the og's deep-read grade validation (sector-content grading) and full-tree … | B |
| diff-to-fix/findings/FIXED/M-135-recovery-read-failure-semantics-batch.md | 15ebcb12 2026-10-29 fix(writer): M-135's S71 convergent rider over the S72 seat - t… | MAJOR M-135 — recovery read-failure semantics: the og truncates + numbers + logs each fai… | B |
| diff-to-fix/findings/FIXED/M-136-recovery-reads-bypass-retry-cache-pipe.md | d28244df 2026-10-29 fix(writer): M-136 - the recovery reads ride the scan's retry+c… | MAJOR M-136 — recovery-time reads bypass the retry/cache pipeline the scan built: one tra… | B |
| diff-to-fix/findings/FIXED/M-137-lostpart-surfaced-shape-batch.md | 0945581e 2026-10-28 trail(M-137): retired to findings/FIXED/ - the surfaced-shape b… | MAJOR M-137 — the lost-partition surfaced shape: naming template, relabel, presentation m… | B |
| diff-to-fix/findings/FIXED/M-138-twin-key-device-family-set-wrong.md | ddd7c5dc 2026-10-28 fix(trail): N-160 - the register-truth batch landed (I-034 cens… | MAJOR M-138 — the twin-key device-family offset: the og adjusts kinds {FAT16, EXT2-family… | B |
| diff-to-fix/findings/FIXED/M-139-hik-discriminator-bc-not-bd.md | 5561aebe 2026-10-29 fix(carve): M-139 - the HIK discriminator start-code byte right… | MAJOR M-139 — The HIK discriminator start-code byte is wrong: the og probes 00 00 01 BC (… | B |
| diff-to-fix/findings/FIXED/M-140-csl-cd01-latch-decline-drops-not-endtag-retry.md | a8c03cc2 2026-10-29 fix(carve): M-140 — the CSL/CD01 decline fall-through removed (… | MAJOR M-140 — The CSL/CD01 walker: the og's per-candidate latch permanently binds CD01 ca… | B |
| diff-to-fix/findings/FIXED/M-141-ntfs-lcn-bounds-core-only-mirror-bound.md | b09b46c8 2026-10-29 fix(ntfs): M-141+M-145+M-147 — the open-ladder acceptance-band … | MAJOR M-141 — Core-only mft_lcn/mftmirr_lcn < total_clusters refusals: the og gates only … | B |
| diff-to-fix/findings/FIXED/M-142-hfsplus-vh-validator-wrong-fields-fork-clamps.md | 5de2e2b0 2026-10-29 fix(hfsplus): M-142 — the VH validator's fork clamps replaced w… | MAJOR M-142 — HFS+ volume-header validator gates the WRONG FIELDS: the og gates freeBlock… | B |
| diff-to-fix/findings/FIXED/M-143-ext4-journal-inode-image-harvest-absent.md | 84784c63 2026-10-29 fix(ext4): M-143 — the journal inode-image harvest surface land… | MAJOR M-143 — ext4 journal inode-image harvest branch absent: the og's jbd2 pass treats e… | B |
| diff-to-fix/findings/FIXED/M-144-ext4-large-volume-caps-refuse-og-scannable.md | ee19145c 2026-10-29 fix(ext4): M-144 — the 8 MiB GDT single-shot read cap replaced … | MAJOR M-144 — ext4 large-volume refusal caps the og does not have: the 8 MiB GDT read cap… | B |
| diff-to-fix/findings/FIXED/M-145-ntfs-open-55aa-gate-core-only.md | b09b46c8 2026-10-29 fix(ntfs): M-141+M-145+M-147 — the open-ladder acceptance-band … | MAJOR M-145 — Core-only 0x55AA signature gate at NTFS engine open: the og's NTFS path nev… | B |
| diff-to-fix/findings/FIXED/M-146-probe-family-ntfs-gate-family-mismatch.md | 9c3cf3f1 2026-10-29 fix(orchestrator): M-146 — probe_family's NTFS gate family repl… | MAJOR M-146 — probe_family's NTFS gate family is not the og analyser's: the core demands … | B |
| diff-to-fix/findings/FIXED/M-147-ntfs-cpr-cib-zero-defaults-vs-reject-d52-correction.md | 78be139e 2026-10-29 fix(ntfs): M-147 convergence corroboration (the S60 MAJOR-first… | MAJOR M-147 — NTFS cpr==0/cib==0: the og's scan path DEFAULTS (record 0x400 / index block… | B |
| diff-to-fix/findings/FIXED/M-148-deep-read-address-is-twin-key-not-extent-start.md | 047d8a96 2026-10-29 trail(collision-rider-4): the doc_truth_gate's FIRST REMOTE CI … | MAJOR M-148 — the deep-read grade's read address is the extent START; the og reads at the… | B |
| diff-to-fix/findings/FIXED/M-149-lostpart-scoring-denominator-bonus-fold.md | d1730da3 2026-10-29 trail(S80): the M-149 convergence record — the seat conceded to… | MAJOR M-149 — lost-partition scoring math: the +500 table-match bonus is folded into the … | B |
| diff-to-fix/findings/FIXED/M-150-session-count-prealloc-memory-bomb.md | d558e6a4 2026-10-29 converge(S82): M-150 conceded to the racing f7432fb4 — the thir… | MAJOR M-150 — session load: the tree-record count pre-allocates `8 × count` bytes from an… | B |
| diff-to-fix/findings/FIXED/M-151-ntfs-truncated-mft-run-growth-absent.md | 5e1dca99 2026-10-29 converge(S82): M-151 conceded to the racing 27bda40e — the four… | MAJOR M-151 — NTFS: the truncated-$MFT run-growth probe (the og extends each run past its… | B |
| diff-to-fix/findings/FIXED/M-152-ext4-sweep-orphan-reserved-skip-unregistered.md | 92bb8ead 2026-10-29 trail(ext4): the M-152 convergence record — the third independe… | MAJOR M-152 — ext4 deleted-inode sweep: the core's `dtime==0 && links!=0` skip drops the … | B |
| diff-to-fix/findings/FIXED/M-153-ldm-crossdisk-member-reads-bypass-scan-pipe.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | MAJOR M-153 — cross-disk LDM member reads bypass the retry/cache substrate during scans: … | B |
| diff-to-fix/findings/FIXED/M-154-autosave-retention-and-lastscan-pointer-absent.md | cac42582 2026-10-29 trail(session): the M-154 convergence record — the seat concede… | MAJOR M-154 — the auto-session lifecycle: the og keeps exactly ONE auto session (pointer-… | B |
| diff-to-fix/findings/FIXED/M-155-red-main-committed-conflict-markers-ed501caa.md | 9508b90b 2026-10-29 audit(S81): the M-155 convergence record — the parallel lane's … | MAJOR M-155 — red main: the ed501caa push committed unresolved merge conflict markers in … | B |
| diff-to-fix/findings/FIXED/M-156-x3f-ascii-gate-rejects-real-files-chain-router.md | 0eaded3d 2026-10-29 fix(carve): M-156 — the X3F surface's fifth touch: the ASCII ve… | MAJOR M-156 — the landed X3F surface rejects every real Sigma file: the ASCII version gat… | B |
| diff-to-fix/findings/FIXED/N-090-session-list-scope-row-shape.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | MINOR N-090 — Session-list scope and row shape drift from the og's summary record (no `au… | B |
| diff-to-fix/findings/FIXED/N-096-identity-ioctl-non-issuance.md | dbbcc6bc 2026-10-28 docs(transport): N-096 - the identity-IOCTL non-issuance regist… | MINOR N-096 — Identity IOCTL non-issuance: the og client queries GET_DISK_CONTEXT_MODE pe… | B |
| diff-to-fix/findings/FIXED/N-115-officetagfromxml-partial-content-types-gate-only.md | 67de3276 2026-10-28 fix(carve): N-115 - the OfficeTagFromXML typing pass lands + N-… | MINOR N-115 — the og's OfficeTagFromXML ZIP-inner classifier is only partially mirrored: … | B |
| diff-to-fix/findings/FIXED/N-122-nvram-create-delete-exclusion-unregistered.md | ef74eb83 2026-10-28 docs(boot): N-122 - the NVRAM Boot#### create/delete exclusion … | MINOR N-122 — the NVRAM Boot#### create/delete exclusion lives only inside M-040's resolu… | B |
| diff-to-fix/findings/FIXED/N-125-drive-disconnect-states-undispositioned.md | ab00be9f 2026-10-28 fix(transport): N-125 - the drive-disconnect classification lan… | MINOR N-125 — the drive-disconnect mid-scan states are undispositioned: the og detects ta… | B |
| diff-to-fix/findings/FIXED/N-126-unsaved-file-category-absent.md | ae6ce2bf 2026-10-28 fix(query): N-126 - the Unsaved File category lands (Category::… | MINOR N-126 — the "Unsaved File" category is absent: the og's search/type scope lists it … | B |
| diff-to-fix/findings/FIXED/N-128-capability-matrix-ci-row-truth.md | 21a3bd2a 2026-10-28 docs(matrix): N-128 - the CAPABILITY-MATRIX prebuilt-CI row's d… | MINOR N-128 — CAPABILITY-MATRIX's prebuilt-CI row is doubly untrue: it cites four workflo… | B |
| diff-to-fix/findings/FIXED/N-129-parity-og-core-inventory-numeric-defects.md | ddd7c5dc 2026-10-28 fix(trail): N-160 - the register-truth batch landed (I-034 cens… | MINOR N-129 — PARITY-OG-CORE-INVENTORY numeric and citation defects: api-ms count 44→40, … | B |
| diff-to-fix/findings/FIXED/N-130-videofilerepairer-undispositioned.md | b3e6c287 2026-10-28 docs(register): N-130 + N-138 - the VideoFileRepairer.exe dispo… | MINOR N-130 — VideoViewer/VideoFileRepairer.exe has no disposition row: the Fixo video-re… | B |
| diff-to-fix/findings/FIXED/N-131-fs-native-parent-anchors-not-persisted.md | 174d3b5f 2026-10-29 fix(session): N-131 — the FS-native parent anchors persist (ntf… | MINOR N-131 — the og's FS-native parent anchors are not persisted: NTFS parent-reference … | B |
| diff-to-fix/findings/FIXED/N-132-api-delta-false-landed-claims-config-triplet.md | 4e74ee2f 2026-10-28 fix(query+docs): N-132 - the display-config data layers land + … | MINOR N-132 — API-DELTA rows 44-45 claim "landed" for three config surfaces that are abse… | B |
| diff-to-fix/findings/FIXED/N-133-engine-side-volume-labels-never-surfaced.md | 838fe939 2026-10-28 fix(orchestrator+engines): N-133 - the engine-side on-disk volu… | MINOR N-133 — engine-side on-disk volume labels never decoded/surfaced: the ext4 half-dec… | B |
| diff-to-fix/findings/FIXED/N-135-fix-wave-bookkeeping-truth-batch.md | bbbe78b2 2026-10-29 repair(trail): N-210 - the M-120 class recurred: the f3da99f1 m… | MINOR N-135 — the fix-wave bookkeeping truth batch: one cited doc site missed while its R… | B |
| diff-to-fix/findings/FIXED/N-137-thumbnail-abi-header-contradiction.md | 896c8895 2026-10-28 fix(abi): N-137 - the thumbnail header's two contract-truth def… | MINOR N-137 — the thumbnail ABI header contradicts its own implementation: "ERR_TRUNCATED… | B |
| diff-to-fix/findings/FIXED/N-138-sfc-dead-payload-resolution-refuted.md | b3e6c287 2026-10-28 docs(register): N-130 + N-138 - the VideoFileRepairer.exe dispo… | MINOR N-138 — the OQ-S10-2 "dead payload" resolution is REFUTED: VideoViewer/sfc is Video… | B |
| diff-to-fix/findings/FIXED/N-139-carve-oom-policy-silent-and-inconsistent.md | 133cced9 2026-10-28 fix(carve): N-139 - the OOM policy lands - the graceful code on… | MINOR N-139 — carve-engine OOM policy: the og logs a 5-line memory diagnostic then dies b… | B |
| diff-to-fix/findings/FIXED/N-140-carve-per-extension-stats-surface-absent.md | adb26797 2026-10-29 fix(carve): N-140 — the per-extension carve-result aggregation … | MINOR N-140 — the per-extension carve-result aggregation surface (the og's GetRawUserInfo… | B |
| diff-to-fix/findings/FIXED/N-143-ispecial-two-step-dispatch-absent.md | cb93adf9 2026-10-28 docs(registry)+test: N-143 - the ISpecial two-step dispatch's i… | MINOR N-143 — the og's two-step analyser validation (ISpecialRawFileAnalyser full-validat… | B |
| diff-to-fix/findings/FIXED/N-144-txtview-text-snapshot-thumbnails-absent.md | 4298a48d 2026-10-29 fix(preview): N-144 — the TxtViewFiles text-snapshot thumbnails… | MINOR N-144 — TxtView text-snapshot thumbnails absent: the og renders the first ≤100 byte… | B |
| diff-to-fix/findings/FIXED/N-145-pending-marked-disk-marker-absent.md | d7856484 2026-10-29 fix(apply): N-145 — the cross-run in-flight journal (the og's p… | MINOR N-145 — the cross-run apply-interruption marker absent: the og marks the target dis… | B |
| diff-to-fix/findings/FIXED/N-146-fh11-zero-end-standin-unowned.md | 2c129efc 2026-10-28 fix(carve): N-146 - the FH11 zero-end stand-in re-homed into M-… | MINOR N-146 — the FH11 "zero-end family" extent stand-in is unowned: the og's FreeHand-11… | B |
| diff-to-fix/findings/FIXED/N-148-comment-truth-batch.md | 78e92955 2026-10-28 docs(truth): N-148 - the comment-truth batch, all seven items | MINOR N-148 — the comment-truth batch: cr_ldm.h still claims the RAID-5 rotation is "UNDE… | B |
| diff-to-fix/findings/FIXED/N-149-carve-filter-exempt-set-absent.md | 996b8fd7 2026-10-28 fix(carve): N-149 - the filter's always-on exempt set lands (th… | MINOR N-149 — the carve filter's hard-wired always-on set: the og's per-search filter reb… | B |
| diff-to-fix/findings/FIXED/N-150-api-doc-truth-batch-s11r.md | 68aa5b78 2026-10-29 fix(docs): N-150 - the API doc-truth batch landed (counts re-de… | MINOR N-150 — the API doc-truth batch (S11-R): API.md's 81-export headline (the truth is … | B |
| diff-to-fix/findings/FIXED/N-151-svg-xml-declared-document-double-carves.md | 6f7d33e8 2026-10-28 trail(N-151): the renumber record - this lane's N-150 (the nest… | MINOR N-151 — the XML/SVG family double-carves XML-declared documents: the declaration-st… | B |
| diff-to-fix/findings/FIXED/N-152-dxf-999-comment-entry-key-absent.md | 4f8fa661 2026-10-28 fix(carve): N-152 - the DXF 999-comment entry key lands (FMT_DX… | MINOR N-152 — The DXF "999" comment-header entry key absent: the og's u16 key 0x3939 → CD… | B |
| diff-to-fix/findings/FIXED/N-153-zip-inner-lfh-double-carves.md | 869da779 2026-10-28 trail(N-115+N-153): the retirement bookkeeping the broken chain… | MINOR N-153 — the ZIP family double-carves multi-entry archives: every inner local-file-h… | B |
| diff-to-fix/findings/FIXED/N-154-bookkeeping-truth-batch-2.md | d122d5d5 2026-10-28 fix(trail): N-154 - the bookkeeping-truth batch #2 landed (the … | MINOR N-154 — Bookkeeping-truth batch #2: the N-079 ID collision (an in-source comment ci… | B |
| diff-to-fix/findings/FIXED/N-155-hfs-zero-logical-size-blocks-fallback-absent.md | 09cf54bd 2026-10-28 fix(hfsplus): N-155 - the zero-logical-size totalBlocks fallbac… | MINOR N-155 — The og's zero-logical-size fallback (dataFork.totalBlocks × 4096) is absent… | B |
| diff-to-fix/findings/FIXED/N-156-rrwr-direction-naming-inverted.md | 8aafefbb 2026-10-28 trail(collision-rider): the doc_truth_gate hardening + two more… | MINOR N-156 — the RRWR pair's direction naming inverted in DRIVER-CONTRACT §4.4 (0x800C2B… | B |
| diff-to-fix/findings/FIXED/N-157-ooxml-rels-extension-case-push-window.md | 39004bcc 2026-10-28 docs(records): N-185 - the b26017a7 record-truth erratum lands … | MINOR N-157 (renumbered twice at the parallel-window reconciliations — the remote's bookk… | B |
| diff-to-fix/findings/FIXED/N-158-officetagfromxml-five-branch-typing-gap.md | 1f9f41cf 2026-10-28 merge(hot-window)+converge(N-158)+repair(N-186 NEW): the M-119/… | MINOR N-158 — OfficeTagFromXML's five remaining ZIP-content typing branches (XMIND / MMAP… | B |
| diff-to-fix/findings/FIXED/N-159-failed-files-html-report-decode-classification-axis.md | 035508fa 2026-10-28 fix(report): N-159 - the failed-files report's tree-classificat… | MINOR N-159 — The failed-files recovery log is an HTML report with a per-file tree-classi… | B |
| diff-to-fix/findings/FIXED/N-160-register-truth-batch-s42.md | ddd7c5dc 2026-10-28 fix(trail): N-160 - the register-truth batch landed (I-034 cens… | MINOR N-160 — Register-truth batch: I-034's census scope label + phantom-script citations… | B |
| diff-to-fix/findings/FIXED/N-161-gpt-entry-span-underflow-no-first-le-last-gate.md | 34d4990d 2026-10-28 fix(table)+test: N-161 - the GPT entry-span underflow gated (la… | MINOR N-161 — GPT entry span underflow: `sector_count = last_lba − first_lba + 1` with no… | B |
| diff-to-fix/findings/FIXED/N-162-fat-cluster-to-sector-32bit-wrap-og-core-u64-unregistered.md | 2d41f755 2026-10-28 docs(registry)+test: N-162 - the FAT cluster->sector u64 width … | MINOR N-162 — FAT cluster→sector 32-bit multiply wrap in the og (4 verified sites): the c… | B |
| diff-to-fix/findings/FIXED/N-163-ntfs-runlist-malformed-input-tolerance-truncate-and-keep.md | 142a7f40 2026-10-28 fix(ntfs): N-163 - the runlist malformed-input truncate-and-kee… | MINOR N-163 — NTFS runlist malformed-input tolerance: the og truncates-and-keeps (and its… | B |
| diff-to-fix/findings/FIXED/N-164-gpt-deliberate-strictness-entry-size-cap-unregistered.md | 39e81d5f 2026-10-28 docs(registry): N-164 - the GPT deliberate strictness registere… | MINOR N-164 — the core's deliberate GPT strictness (entry_size==128 exact + 1 MiB entries… | B |
| diff-to-fix/findings/FIXED/N-165-fat-out-of-volume-runlist-refusal-absent.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | MINOR N-165 — FAT "File overflows partition!" run-end refusal absent: the og destroys out… | B |
| diff-to-fix/findings/FIXED/N-166-live-file-count-telemetry-stage-boundary-only.md | 1f9b2849 2026-10-29 fix(orchestration): N-166 - the live file-count telemetry lands… | MINOR N-166 — live file-count telemetry is stage-boundary-only: the og's counters grow li… | B |
| diff-to-fix/findings/FIXED/N-167-no-total-found-size-aggregate-in-scanstats.md | f65cf79c 2026-10-29 fix(stats): N-167 - the found-size aggregate lands (ScanStats::… | MINOR N-167 — the og's scan-completion headline reports found-count + total found SIZE ("… | B |
| diff-to-fix/findings/FIXED/N-168-og-user-sign-bounds-guard-2x-overread-core-correct-unregistered.md | 30bc8b79 2026-10-28 docs(registry)+test: N-168 - the user-sign availability bound's… | MINOR N-168 — the og's user-sign bounds guard demands offset + hex-STRING length (2× the … | B |
| diff-to-fix/findings/FIXED/N-169-euuserdata-duplicate-asf-block-merge-semantics-unresolved.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | MINOR N-169 — duplicate-extension-key semantics in EUUserData: the shipped file carries a… | B |
| diff-to-fix/findings/FIXED/N-170-stale-pre-erratum-euuserdata-claims-batch.md | e9cc5a22 2026-10-28 docs(truth): N-170 - the pre-erratum EUUserData/GetRawUserInfoM… | MINOR N-170 — stale pre-erratum claims batch: 8 core-tree comments/docs still carry the r… | B |
| diff-to-fix/findings/FIXED/N-171-user-sign-files-invisible-in-carved-found.md | 612577cb 2026-10-29 repair(window): the 625a1e59 committed-red main closed - the 4t… | MINOR N-171 — user-sign emissions never tick `validated`/`carved_found`: the headline car… | B |
| diff-to-fix/findings/FIXED/N-172-fat-deleted-first-char-hash-vs-underscore-plus-unregistered-recovery.md | afa2adff 2026-10-28 fix(fat): N-172 - the deleted-name presentation aligned to the … | MINOR N-172 — FAT deleted-entry first character: the og substitutes `'#'`; the core uses … | B |
| diff-to-fix/findings/FIXED/N-173-fat-2107-date-u32-clamp-zero-filetime.md | 98079347 2026-10-28 fix(fat): N-173 - the 2106/2107 date collapse repaired (the u32… | MINOR N-173 — FAT 2106-02-07+/2107 dates collapse to the zero FileTime (u32 clamp) while … | B |
| diff-to-fix/findings/FIXED/N-174-exfat-appledouble-prefix-sink-filter-absent.md | 3139a7fd 2026-10-28 merge(hot-window)+converge: the S46/W1 N-174 landing absorbed (… | MINOR N-174 — exFAT `._`-prefix sink filter absent: the og drops macOS AppleDouble sideca… | B |
| diff-to-fix/findings/FIXED/N-175-fat-ntres-case-folding-absent.md | b1b5eba0 2026-10-28 fix(fat): N-175 - the NTRes 8.3 display-case fold lands (the og… | MINOR N-175 — FAT 8.3 NTRes case-folding absent: the og applies the NT reserved byte's ca… | B |
| diff-to-fix/findings/FIXED/N-176-fat-lfn-illegal-char-fallback-absent.md | ff5b96ad 2026-10-28 fix(fat): N-176 - the LFN illegal-char fallback lands (the og's… | MINOR N-176 — FAT LFN illegal-character fallback absent: the og discards an assembled LFN… | B |
| diff-to-fix/findings/FIXED/N-177-ext4-size-exceeds-blocks-damaged-flag-not-carried.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | MINOR N-177 — ext4 "size exceeds blocks" damaged flag (props FLAGS bit 0x200) is computed… | B |
| diff-to-fix/findings/FIXED/N-178-cli-pause-surface-legs-verb-and-engine-notification.md | 67d2bf25 2026-10-29 repair(trail): the born-collided D-150 seat — THREE rows claime… | MINOR N-178 — the pause surface's CLI legs: no pause verb in the CLI (the og ships a pixe… | B |
| diff-to-fix/findings/FIXED/N-179-usersigns-ext-length-comment-false.md | 40e46f86 2026-10-28 fix(carve): N-179 - the S-039 cap-annotation set's false ext-le… | MINOR N-179 (renumbered TWICE per earlier-push-wins: filed N-154 [consumed by the S42-mai… | B |
| diff-to-fix/findings/FIXED/N-180-doc-truth-staleness-batch.md | a4bd4052 2026-10-28 docs(truth): N-180 - the doc-truth staleness batch re-truthed (… | MINOR N-180 — the doc-truth staleness batch: five shipped-defaults/API claims contradicte… | B |
| diff-to-fix/findings/FIXED/N-181-raw-first-start-ordering.md | 033c5d7c 2026-10-29 fix(orchestrator): N-181 — the RAW-first start order lands (the… | MINOR N-181 — the og starts the RAW carve controllers before the FS deep engines; the cor… | B |
| diff-to-fix/findings/FIXED/N-182-deep-range-derivation-absent.md | 868d1e85 2026-10-29 fix(orchestration/docs): N-182 — the deep-range resolver closes… | MINOR N-182 — the og's deep-scan range resolver absent: the deep pass can run a derived s… | B |
| diff-to-fix/findings/FIXED/N-183-gauge-population-score-formula-qsde-batch.md | fbc94447 2026-10-29 fix(grading): N-183 - the recoverability aggregate lands (the 2… | MINOR N-183 — the gauge population + score formula + QSDE counter mapping: the og's recov… | B |
| diff-to-fix/findings/FIXED/N-184-preview-media-metadata-families-absent.md | 68f39b4a 2026-10-29 fix(preview): N-184 - the media metadata families land (AVI avi… | MINOR N-184 — preview metadata covers MP4/WAV only: the og's media metadata families (AVI… | B |
| diff-to-fix/findings/FIXED/N-185-b26017a7-repin-self-defeating-n157-addendum.md | 39004bcc 2026-10-28 docs(records): N-185 - the b26017a7 record-truth erratum lands … | MINOR N-185 — the b26017a7 "repair" of the OoxmlRelsPathSplit red was itself self-defeati… | B |
| diff-to-fix/findings/FIXED/N-186-rsf-variant-device-descriptor-record-field-gap.md | d0ba3eb2 2026-10-28 audit(S54): the continuation audit lane's filings — 20 new find… | MINOR N-186 — The og's .rsf variant-save device-descriptor record (GUID 76a672b0…, [552-B… | B |
| diff-to-fix/findings/FIXED/N-187-mpegps-lengths-read-little-endian.md | ddd7c5dc 2026-10-28 fix(trail): N-160 - the register-truth batch landed (I-034 cens… | MINOR N-187 — validate_mpegps reads the MPEG-PS system-header and PES length fields as LI… | B |
| diff-to-fix/findings/FIXED/N-188-m119-window-committed-red-app-parse.md | c63b56b9 2026-10-28 merge(hot-window)+converge(N-166): the remote window's N-171-do… | MINOR N-188 (renumbered from the N-186 draft at the N-160 window merge per SA-9 — the rem… | B |
| diff-to-fix/findings/FIXED/N-189-registry-sentinel-walk-asan-red.md | c481b812 2026-10-29 repair(trail+tests): N-190 - the hot-window's red main repaired… | MINOR N-189 — (discovered in fix session S59) two registry walks in test_carve.cpp use a … | B |
| diff-to-fix/findings/FIXED/N-190-hot-window-merge-red-build-gate.md | c481b812 2026-10-29 repair(trail+tests): N-190 - the hot-window's red main repaired… | MINOR N-190 — (discovered in fix session S59) the hot-window merge pushed a red tree: a d… | B |
| diff-to-fix/findings/FIXED/N-191-parity-deltas-d118-numbering-collision.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | MINOR N-191 — PARITY-DELTAS carries two rows numbered D-118 (N-143's id-8-demote row and … | B |
| diff-to-fix/findings/FIXED/N-192-clone-drops-short-name-on-fanout.md | 4d859e41 2026-10-29 fix(tree): N-192 - the clone carries the 8.3 short_name (the og… | MINOR N-192 — FileProperties::clone() drops `short_name` — the hard-link fan-out's cloned… | B |
| diff-to-fix/findings/FIXED/N-193-stock-raw-sign-never-ticks-validated.md | 8aafefbb 2026-10-28 trail(collision-rider): the doc_truth_gate hardening + two more… | MINOR N-193 — the stock raw-sign layer never ticks `validated`: carved_found excludes the… | B |
| diff-to-fix/findings/FIXED/N-194-size-display-formatters-absent.md | cb6d5bbe 2026-10-29 fix(query): N-194 - the og's two byte-size display formatters l… | MINOR N-194 — The og's two byte-size display formatters (binary /1024 tiers — log-side KB… | B |
| diff-to-fix/findings/FIXED/N-195-bench-verify-driver-phantom-zero-ioctls.md | 1b4b2aa5 2026-10-29 fix(bench): N-195 - the driver-verification bench's phantom-zer… | MINOR N-195 — bench/ours/verify-driver.ps1 carries the phantom-zero extended-IOCTL codes … | B |
| diff-to-fix/findings/FIXED/N-196-ntfs-walk-entry-geometry-gate-absent.md | 1110a3d9 2026-10-29 fix(ntfs): N-196 - the walk-entry record-geometry stance regist… | MINOR N-196 — NTFS walk-entry record-geometry gate absent in the core (odd or >16-device-… | B |
| diff-to-fix/findings/FIXED/N-197-ntfs-spc-shift-threshold-og-internal-inconsistency.md | 0892f799 2026-10-29 fix(ntfs): N-197 - the spc shift-decode threshold registered + … | MINOR N-197 — spc shift-form decode threshold is og-internally inconsistent (NTFSSearchDL… | B |
| diff-to-fix/findings/FIXED/N-198-exfat-core-only-numeric-bounds-unregistered.md | 9b92e3d9 2026-10-29 fix(exfat): N-198 - the three core-only boot-gate bounds regist… | MINOR N-198 — exFAT boot-gate strictness beyond the og: core-only `volume_length >= 2048`… | B |
| diff-to-fix/findings/FIXED/N-199-refs-second-signature-fsrs-absent.md | 6d487cfd 2026-10-29 fix(recognition): N-199 - the ReFS classifier's second signatur… | MINOR N-199 — ReFS recognition lacks the og's second signature ("FSRS" @+0x10) and the cl… | B |
| diff-to-fix/findings/FIXED/N-200-lostpart-ntfs-probe-acceptance-weaker-no-mirror-retry.md | 1ec36d6c 2026-10-29 rebase-continue: N-200 over the racing S71/S72/S73 windows (N-2… | MINOR N-200 — Lost-scan NTFS candidate acceptance is weaker in the core: the og requires … | B |
| diff-to-fix/findings/FIXED/N-201-fat-media-word-flavor-refinement-absent.md | dc0d11c9 2026-10-29 rebase-continue: N-201 over the racing S76 window's N-131 landi… | MINOR N-201 — FAT quick-open media-word flavor refinement absent: the og reads FAT1/FAT2 … | B |
| diff-to-fix/findings/FIXED/N-202-ext4-superblock-band-deltas-consolidated.md | 3e50f03c 2026-10-29 fix(ext4): N-202 - the four SB acceptance-band deltas consolida… | MINOR N-202 — ext4 superblock validator acceptance-band deltas (consolidated): inode_size… | B |
| diff-to-fix/findings/FIXED/N-203-resume-roundtrip-drops-candidate-data-area-extent.md | f3da99f1 2026-10-29 fix(session): N-203 - the resume round-trip keeps the candidate… | MINOR N-203 — the resume round-trip drops the candidate's data-area extent: resumed lost-… | B |
| diff-to-fix/findings/FIXED/N-204-og-engine-layer-carve-buffer-geometry-never-decoded-f6-og-side-empty.md | 89405af1 2026-10-29 docs(carve): N-204 - the engine-layer carve geometry registered… | MINOR N-204 — the og's engine-layer carve buffer geometry (4 MiB block / 8 MiB double buf… | B |
| diff-to-fix/findings/FIXED/N-205-rawcount-population-semantics-og-validated-vs-core-carved-plus-phantom-raw-tail-zero.md | f392d27d 2026-10-29 repair(trail): the FOURTH born-collided D-seat repair per SA-9 … | MINOR N-205 — the recoverability rawCount counts different populations: the og's forced d… | B |
| diff-to-fix/findings/FIXED/N-206-disk-sector-count-rounding-og-ceil-vs-core-floor.md | 104631d2 2026-10-29 fix(storage): N-206 - the sector-count CEIL lands (the og's geo… | MINOR N-206 — disk sector-count rounding: the og CEILS the last partial sector (GET_DRIVE… | B |
| diff-to-fix/findings/FIXED/N-207-parity-deltas-born-collided-seat-batch.md | 1a0cb035 2026-10-29 fix(registry): N-191+N-207 - the PARITY-DELTAS duplicate-D renu… | MINOR N-207 — the PARITY-DELTAS born-collided-seat batch: NINE additional duplicate D-num… | B |
| diff-to-fix/findings/FIXED/N-208-dxf-canonical-gate-8-byte-vs-og-16-byte.md | 9e0f3f2d 2026-10-29 repair(trail): the CI doc-truth red closed - the born-collided … | MINOR N-208 — The DXF canonical acceptance gate (renumbered from its born-collided N-189 … | B |
| diff-to-fix/findings/FIXED/N-209-jpeg-exif-prefix-39-char-cap-absent.md | 9e0f3f2d 2026-10-29 repair(trail): the CI doc-truth red closed - the born-collided … | MINOR N-209 — The JPEG carve-name prefix path accepts EXIF Model/Software strings up to 2… | B |
| diff-to-fix/findings/FIXED/N-210-committed-conflict-markers-recurred-f3da99f1.md | 11a01a53 2026-10-29 audit(S79): the eleventh continuation lane's filings + the two … | MINOR N-210 — the M-120 class recurred: four audit-trail files on main carry live committ… | B |
| diff-to-fix/findings/FIXED/N-211-cr-scan-h-autosave-contract-text-stale.md | b051fe45 2026-10-29 rider(orchestrator): N-211's third site — rrecov.cpp's completi… | MINOR N-211 — the cr_scan.h autosave_dir contract comment still says "a cancelled run nev… | B |
| diff-to-fix/findings/FIXED/N-212-asf-acceptance-band-and-extent-source.md | e17a4e6d 2026-10-29 fix(carve): N-212 — the ASF acceptance band matches the og's ti… | MINOR N-212 — ASF acceptance gates a torn count/size field the og never reads, and takes … | B |
| diff-to-fix/findings/FIXED/N-213-ntfs-record-header-pregate-band-absent.md | e54762dd 2026-10-29 fix(ntfs): N-213 — the record-header pre-gate band, family-keye… | MINOR N-213 — NTFS record-header pre-gate band: the og's `usa_offset ∈ [0x2A, 0x100]`, `a… | B |
| diff-to-fix/findings/FIXED/N-214-ext-probe-journal-indicator-gate-absent.md | 55f6ecc1 2026-10-29 fix(lost): N-214 — the ext probe's journal-indicator consistenc… | MINOR N-214 — the lost-partition ext probe's journal-indicator consistency gate absent: t… | B |
| diff-to-fix/findings/FIXED/N-215-r20-unimportant-mechanism-misattributed.md | 59a431d3 2026-10-29 fix(docs): N-215 — the R-20 'Unimportant' mechanism attribution… | MINOR N-215 — R-20's mechanism attribution is wrong: the og's "Unimportant files" bucket … | B |
| diff-to-fix/findings/FIXED/N-216-version-ini-maxfilesize-surface-unregistered.md | 92ae836c 2026-10-29 fix(orchestrator): N-216 — the MaxFileSize cap: the consumer de… | MINOR N-216 — config/version.ini's `MaxFileSize ` per-file cap surface (default 1 TiB, Gi… | B |
| diff-to-fix/findings/FIXED/N-217-ed501caa-committed-conflict-markers-red-main.md | 42228a11 2026-10-29 converge(S82): the X3F arbitration conceded AFTER first-hand di… | MINOR N-217 — the M-120/N-210 class recurred a third time: `ed501caa` pushed with live co… | B |
| diff-to-fix/findings/FIXED/N-218-detach-live-counter-over-subtracts-full-nodes.md | e2ea4d50 2026-10-29 fix(orchestrator/tree): A-016 + N-218 — the convergent-verifica… | MINOR N-218 — the detach paths subtract FullNode intermediates from live_files_: the unsi… | B |
| diff-to-fix/findings/FIXED/N-219-27bda40e-committed-conflict-markers-red-main-4th-recurrence.md | f18ba226 2026-10-29 repair(ntfs): N-219 — the fourth red-main conflict-marker repai… | MINOR N-219 — the M-120/N-210/N-217 class's FOURTH recurrence: `27bda40e` (the M-151 land… | B |
| diff-to-fix/findings/FIXED/OQ-S42-1-ntfs-si-fn-time-precedence.md | 448a037e 2026-10-28 resolve(OQ): S42-1 - the NTFS $SI/$FN time precedence closed RE… | OQ-S42-1 — NTFS $STANDARD_INFORMATION vs $FILE_NAME time precedence: which attribute's ti… | B |
| diff-to-fix/findings/FIXED/OQ-S42-2-btrfs-nsec-treatment.md | 1b1fbdc9 2026-10-28 resolve(OQ): S42-2 CONVERGENT-MATCH - the Btrfs nsec question c… | OQ-S42-2 — Btrfs nsec handling in the og: does the engine fold sub-second precision or ca… | B |
| diff-to-fix/findings/FIXED/OQ-S42-3-grading-deep-read-subgates.md | b2546602 2026-10-29 audit(S55): the sixth continuation lane's filings - 5 lead-veri… | OQ-S42-3 — the grading deep-read sub-gates: FUN_1401bc5a0's predicate, the +0x72-bit-7 / … | B |
| diff-to-fix/findings/FIXED/OQ-S52-1-ext23-vs-ext4-kind-classification.md | 982efeb5 2026-10-29 fix(ext4): OQ-S52-1 resolved - the ext-family kind stamps per t… | OQ-S52-1 — the og's ext2/3-vs-ext4 volume classification rule (which volumes report kind … | B |
| diff-to-fix/findings/FIXED/S-040-image-create-absent-r93.md | f392d27d 2026-10-29 repair(trail): the FOURTH born-collided D-seat repair per SA-9 … | SEVERE S-040 — Disk-image creation wholly absent (the og's "Create Image" home-page flow;… | B |
| diff-to-fix/findings/FIXED/S-048-carve-branch-no-volume-grouping-reconstructed-absent.md | 6b75b40e 2026-10-28 fix(carve+orchestrator): S-048 - the Reconstructed per-volume c… | SEVERE S-048 — The carve branch has no per-volume grouping: the og's `Reconstructed/<volu… | B |
| diff-to-fix/findings/FIXED/S-049-carved-file-naming-contract-ext-counter-content-prefixes-absent.md | 9941208b 2026-10-28 merge(S-049 convergence): the two-lane union - the executed cou… | SEVERE S-049 — The og's universal carved-file naming contract (`<PREFIX>_%06d.EXT` with c… | B |
| diff-to-fix/findings/FIXED/S-050-file-repair-line-absent-and-unregistered.md | ed14732f 2026-10-29 docs(register)+test: S-050 - the file-repair line's formal scop… | SEVERE S-050 — The og's file-repair line (photo/video/office/archive/HWP/raw repair — 9 p… | B |
| diff-to-fix/findings/FIXED/S-051-wholedisk-scan-targets-absent-silent-empty.md | 3f7c190e 2026-10-28 merge(S-051 DU): the convergent S-051 union — the S40 lane's 6e… | SEVERE S-051 — the whole-disk scan target never consults the partition list: `cr_ui_open_… | B |
| diff-to-fix/findings/FIXED/S-052-ldm-span-recovery-misbases-layout-relative-extents.md | f4e500de 2026-10-28 fix(recovery): S-052 - the LDM span recovery basis lands: the t… | SEVERE S-052 — LDM span recovery misbases: dynamic-volume nodes keep layout-relative exte… | B |
| diff-to-fix/findings/FIXED/S-054-n013-regression-multi-extent-dest-arming-clobbered.md | 632518b7 2026-10-28 trail(S-054): the SEVERE retirement completed - the N-013-regre… | SEVERE S-054 — N-013 regression: the multi-extent destination arming was clobbered back t… | B |
| diff-to-fix/findings/FIXED/S-055-user-sign-layer-inverts-builtin-first-collision-rule.md | a5fe33a5 2026-10-28 fix(carve): N-171 + the S-055/S-056 convergence supplements + t… | SEVERE S-055 — the user-sign layer probes BEFORE the built-in raw-sign table: the M-048/D… | B |
| diff-to-fix/findings/FIXED/S-056-disabled-formats-filter-does-not-gate-user-sign-layer.md | a5fe33a5 2026-10-28 fix(carve): N-171 + the S-055/S-056 convergence supplements + t… | SEVERE S-056 — the disabled-formats filter does not gate the user-sign layer: turning a f… | B |
| diff-to-fix/findings/FIXED/S-057-og-auto-saves-stopped-scans-core-never-saves-cancelled.md | e8a441d6 2026-10-28 fix(session+cli): S-057 - the stop-window auto-save lands (the … | SEVERE S-057 — the og auto-saves STOPPED scans' partial results; the core never auto-save… | B |
| diff-to-fix/findings/FIXED/S-058-grading-exttable-gate-polarity-inverted.md | f854ae70 2026-10-29 fix(grading): S-058 - the extension-table gate polarity righted… | SEVERE S-058 — the grading extension-table gate is INVERTED: the core twin-flags the wron… | B |
| diff-to-fix/findings/FIXED/S-059-sparse-hole-export-leftshift-or-refusal.md | 6f5b29f8 2026-10-29 fix(ntfs+writer): S-059 - the sparse-hole export repaired (the … | SEVERE S-059 — sparse/hole files: the writer left-shifts content over holes or refuses th… | B |
| diff-to-fix/findings/FIXED/S-060-recovered-file-timestamps-attributes-not-restored.md | fa07b5a2 2026-10-28 fix(writer): S-060 - the metadata restoration lands (the og's p… | SEVERE S-060 — recovered files lose their timestamps and DOS attributes: the og restores … | B |
| diff-to-fix/findings/FIXED/S-061-refs-btrfs-insert-time-grade-absent.md | ea405392 2026-10-28 fix(grading): S-058 + S-061 - the extension-table gate polarity… | SEVERE S-061 — ReFS and Btrfs files never receive the og's insert-time High grade: the wh… | B |
| diff-to-fix/findings/FIXED/S-062-one-chunk-walk-size-gate-exits-publish-2x-device-not-cursor.md | 31506f31 2026-10-29 fix(carve): S-062 - the ONE chunk-walk's size-gate exits publis… | SEVERE S-062 — The ONE chunk-chain walk publishes the last-valid-chunk end on the size-ga… | B |
| diff-to-fix/findings/FIXED/S-063-ntfs-mft-bootstrap-mftmirr-fallback-absent.md | f8244aba 2026-10-29 fix(ntfs): S-063 - the torn-$MFT bootstrap falls back to the $M… | SEVERE S-063 — NTFS torn-$MFT bootstrap: the og falls back to reading record 0 at the $MF… | B |
| diff-to-fix/findings/FIXED/S-064-fat-flavor-selection-string-driven-vs-bracket.md | 127077bd 2026-10-29 fix(fat): S-064 - the engine-open flavor selection is STRING-FI… | SEVERE S-064 — FAT engine flavor selection is string-driven in the og (FUN_18002f970) but… | B |
| diff-to-fix/findings/FIXED/S-065-ext4-indirect-ladder-unbounded-freeze-oom.md | ca14c872 2026-10-29 fix(ext4): S-065 - the indirect-block ladder bounded by the fil… | SEVERE S-065 — ext4/ext2/ext3 indirect-block ladder is unbounded: a single stale-pointer … | B |
| diff-to-fix/findings/FIXED/S-066-exfat-lost-anchor-base-12-correction-absent.md | ea196958 2026-10-29 fix(exfat): S-066 - the lost-anchor base-12 re-base lands (FUN_… | SEVERE S-066 — exFAT lost-anchor recognition: the og re-bases a volume whose scan anchor … | B |
| diff-to-fix/findings/FIXED/S-067-exfat-out-of-range-fat-entry-clamp-absent.md | 2d614cf2 2026-10-29 fix(exfat): S-067 correction — the convergence arbitration: the… | SEVERE S-067 — exFAT chain walker: the og's out-of-range FAT-entry clamp (entry ≥ cluster… | B |
| diff-to-fix/findings/FIXED/S-068-fat32-deleted-high16-rebase-absent.md | 9b4a9302 2026-10-29 trail(S79): the S-068 seat conceded to the racing window — this… | SEVERE S-068 — FAT32 deleted files: the og's first-cluster high-16 re-base heuristic (the… | B |
| diff-to-fix/findings/MAJOR/M-075-chkdsk-launch-absent.md | 370d6595 2026-10-28 audit(session-8): the second-session deep audit — 25 items file… | MAJOR M-075 — chkdsk launch absent: the og's check→WinChkdsk escalation endpoint has no c… | B |
| diff-to-fix/findings/MAJOR/M-078-bitlocker-turn-off-absent.md | 370d6595 2026-10-28 audit(session-8): the second-session deep audit — 25 items file… | MAJOR M-078 — BitLocker turn-off absent: the og's CBitlockerTurnOffOperation step cannot … | B |
| diff-to-fix/findings/MAJOR/M-086-driver-fileversion-pin-defeats-versioned-layout.md | ddd7c5dc 2026-10-28 fix(trail): N-160 - the register-truth batch landed (I-034 cens… | MAJOR M-086 — rda-driver FileVersion pinned 1.0.0.0 defeats the -Layout Versioned upgrade… | B |
| diff-to-fix/findings/MAJOR/M-087-regdrv-rda-reg-upperfilters-invalid.md | f166440b 2026-10-28 audit(session-11/wave-1): remove-the-fixed + board truth — M-10… | MAJOR M-087 — regdrv-rda.reg UpperFilters is an invalid hex(7) value: the WinPE disk-clas… | B |
| diff-to-fix/findings/MAJOR/M-089-rdabench-phantom-zero-extended-ioctl-fuzz-gap.md | f166440b 2026-10-28 audit(session-11/wave-1): remove-the-fixed + board truth — M-10… | MAJOR M-089 — rdabench's fuzz-hammer probes the phantom-zero extended IOCTLs: the live 0x… | B |
| diff-to-fix/findings/MAJOR/M-101-results-tree-branches-missing-versions-node-tags-extracted.md | 20255547 2026-10-28 audit(SA-8/lead-lane): the FINAL three-window reconciliation — … | MAJOR M-101 — the results tree lacks two og branches: the "Versions" node (NTFS multi-ver… | B |
| diff-to-fix/findings/MAJOR/M-105-drwservice-install-lifecycle-unaudited-absent.md | bb2cd8a0 2026-10-28 audit(SA-10/close): the lead lane's filings + the five-lane rec… | MAJOR M-105 — DRWService install lifecycle absent and never scored: the og ships a transi… | B |
| diff-to-fix/findings/MAJOR/M-107-batch1-size-walk-stand-in-residue-unowned.md | 0cf6142d 2026-10-29 fix(carve): M-107's CRW member — the CIFF heap size walk replac… | MAJOR M-107 — the batch-1 size-walk stand-in residue is unowned: eleven formats still car… | B |
| diff-to-fix/findings/MINOR/N-107-driver-stale-command-count-prose.md | 7922b5e1 2026-10-28 audit(session-8/third-lane): the driver-lane findings + repairs… | MINOR N-107 — Driver-repo command-count prose is stale at 14 current-tense sites: "15/18 … | B |
| diff-to-fix/findings/MINOR/N-127-driver-doc-vs-code-truthfulness.md | 167dbe3e 2026-10-28 reconcile(session-9): the push-time merge with the parallel ses… | MINOR N-127 — rda-driver doc-vs-code truthfulness defects: a claimed admin gate with no c… | B |
| tests/data/images/README.md | 93d7fa62 2026-10-25 fix(S-030): disk-image scan targets — flat/VMDK/VHD/VHDX as fir… | tests/data/images — real-world image fixtures (S-030) | B |
| diff-to-fix/evidence/executions/devsign-run-40549660074/README.md | fcb4d424 2026-10-27 audit(SA-1): Phase 1 dev-sign validation closed - run 405496600… | devsign-run-40549660074 — the dev-sign + install + uninstall execution evidence | B |
| diff-to-fix/evidence/executions/s3-calcpartition-vtables/README.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | S3-LOST execution evidence — ICalcPartition vtable extraction (RE probe) | B |
| diff-to-fix/evidence/executions/s3-ehb-ext4-dirwalk/README.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | s3-ehb-ext4-dirwalk — executed comparison (S3-EHB, session 3) | B |
| diff-to-fix/evidence/executions/s3-ftyp-signature-deadness/README.md | c27d8cf8 2026-10-25 diff-audit-s3: wave-1 reconciliation onto main — parallel-leg m… | S3-CARVE executed probe — the ftyp-family signatures are unreachable (S-012 evidence) | B |
## Doc count summary

Totals per directory (sub-directory granularity; A = deep-read, B = skimmed):

| Directory | Total md | A | B |
|---|---|---|---|
| diff-to-fix/findings/FIXED | 197 | 2 | 195 |
| worklog | 125 | 0 | 125 |
| diff-to-fix/evidence/decompiled-quotes | 88 | 0 | 88 |
| docs | 44 | 16 | 28 |
| diff-to-fix/modules | 33 | 33 | 0 |
| diff-to-fix/evidence | 29 | 0 | 29 |
| diff-to-fix/evidence/executions | 23 | 0 | 23 |
| (root) | 12 | 12 | 0 |
| diff-to-fix | 10 | 5 | 5 |
| diff-to-fix/findings/MAJOR | 8 | 0 | 8 |
| docs/parity | 8 | 8 | 0 |
| diff-to-fix/evidence/s79-lane-sheets | 6 | 0 | 6 |
| diff-to-fix/evidence/s12-lane-sheets | 5 | 0 | 5 |
| diff-to-fix/evidence/s3-agent-logs | 5 | 0 | 5 |
| diff-to-fix/evidence/s55-lane-sheets | 5 | 0 | 5 |
| diff-to-fix/evidence/s69-lane-sheets | 4 | 0 | 4 |
| diff-to-fix/tracked-open | 4 | 0 | 4 |
| diff-to-fix/evidence/s12-liveness | 3 | 0 | 3 |
| diff-to-fix/evidence/s83-lane-sheets | 2 | 0 | 2 |
| diff-to-fix/findings/MINOR | 2 | 0 | 2 |
| bench | 1 | 0 | 1 |
| diff-to-fix/evidence/executions/devsign-run-40549660074 | 1 | 0 | 1 |
| diff-to-fix/evidence/executions/s3-calcpartition-vtables | 1 | 0 | 1 |
| diff-to-fix/evidence/executions/s3-ehb-ext4-dirwalk | 1 | 0 | 1 |
| diff-to-fix/evidence/executions/s3-ftyp-signature-deadness | 1 | 0 | 1 |
| diff-to-fix/evidence/s68-lane-sheets | 1 | 0 | 1 |
| docs/adr | 1 | 1 | 0 |
| installer | 1 | 0 | 1 |
| tests/data/images | 1 | 0 | 1 |
Top-level rollup: diff-to-fix 459 · worklog 125 · docs 53 · (root) 12 · tests/data 1 · installer 1 · bench 1 — **622 total**.

## First-pass findings

### F-D1 (CONFLICT-CORRUPTED, highest severity) — the S82 batch `b2bc1970` committed git conflict markers into FIVE documentation files

`git grep -E '^(<{7}|={7}|>{7})( |$)' -- '*.md'` at HEAD `e8dcaba8`:

| File | Marker lines | Content stranded in the conflict |
|---|---|---|
| docs/PARITY-DELTAS.md | 4677, 4678, 4682, 4707, 4741, 4742, 4743, 4863 (nested, ~187 lines) | the entire S82 register tail: D-155 (I-048 combined), D-160/D-161/D-162 (renumbered I-048 registers), D-158 (M-111 scope decision), D-159 (M-114 posture decision) |
| diff-to-fix/CROSS-REF-MATRIX.md | 39-46 | the M-154 row in BOTH verdict forms (DIVERGENCE-MAJOR open vs MATCH/FIXED) |
| diff-to-fix/FIXED-LEDGER.md | 656-669 (triple-nested: two `>>>>>>>` closes) | I-048/M-152/N-145 vs I-048/M-111/M-114 ledger rows |
| diff-to-fix/findings/FIXED/I-048-three-registers-selected-dead-sumary-remote-ext-size-32bit.md | 52-62 | the finding's Resolution block |
| worklog.md (root) | 12069-12168 | the Session-82 worklog entries |

- The racing blocks also DISAGREE with each other on register numbering: PARITY-DELTAS' block says D-156/D-157 for the remote-tunable/ext-size registers; CROSS-REF-MATRIX's and worklog.md's blocks say D-161/D-162 (the renumbered seats that the HEAD-side D-160/D-161/D-162 rows use). Both resolutions of the same merge are committed simultaneously.
- This is a RECURRENCE of the exact class M-120/M-155 documented and repaired earlier (M-120: markers in test_carve.cpp; M-155: markers in validate_x3f + the M-107 finding file, repaired by the S80 lane). The doc_truth_gate does not currently scan for conflict markers — a marker-lint (the N-210-class lint the matrix mentions) would catch this class at push time.
- `diff-to-fix/findings/FIXED/M-120-…red-main.md` contains marker-looking lines but INSIDE a ```cpp fence (deliberate evidence quote) — not corruption.
- Note: PARITY-DELTAS also carries a pre-existing duplicate register seat `### D-52` (twice) despite the gate's duplicate-number invariant claim in its header comment.

### F-D2 (STALE, self-contradiction) — BUILD-SUPPRESSIONS.md "ZERO entries" banner vs its own S-1 row; two cross-doc repeats

- docs/BUILD-SUPPRESSIONS.md:7 "This file intentionally carries ZERO entries" vs the S-1 row in the same file (landed with `05e67863` 2026-10-28, the cr_thumbnail engine). The S-1 row itself matches code (src/core/preview/cr_thumbnail.cpp:34-45).
- CONTRIBUTING.md:75 "currently zero entries" — same stale claim.
- docs/CI-LOCAL.md:46 "carries zero rows" — same stale claim.

### F-D3 (STALE) — docs/PARAMETERS.md documents removed/incomplete CLI surface

- Line 51 lists `bench … [--no-rda]` — the flag is REMOVED (src/app/rrecov.cpp:2932, 3517; docs/API.md:53 documents the removal; the driver-only lint forbids its return).
- Missing live flags/verbs: `--filter-folder` (M-073), `--autosave-dir`, `--pause-stdin`, `rrecov driver` (+ `--service`), `rrecov archive-list`, `rrecov image` (all present in src/app/rrecov.cpp:3848-3914 flag/verb dispatch).
- Doc's own title claims "every tunable … at the current tree".

### F-D4 (SUSPECT) — docs/API.md CLI grammar missing 2 verbs; API.md/API-SURFACE-AUDIT.md "92 CR_UI_API lines" arithmetic stale

- API.md §1 lists 11 verbs; the CLI has 13 (rrecov.cpp:3904 `archive-list`, :3914 `image`).
- Both API.md:96 and API-SURFACE-AUDIT.md:32-33 say "92 `CR_UI_API` lines minus 3 `#define` macros" → the header actually has 94 CR_UI_API-containing lines (91 declarations after the 3 #defines). CORE-API-CURRENT.md:14 states the correct 94−3=91. The headline "91 exported functions" is correct everywhere and doc_truth_gate-enforced (I re-derived: exports=91, error codes=45, event types=12 — all match).

### F-D5 (SUSPECT) — docs/CORE-API-CURRENT.md "82 C++ public headers" is wrong at HEAD

- Claim at line 13: "C++ public headers 82 under include/cr/**"; actual `git ls-files 'include/cr/**/*.h'` = **91** (was 79 at the doc's original `22e9657` pin; 91 already at the doc's last commit `497b1277`). The count is NOT covered by doc_truth_gate's invariants (only exports/error-codes/events are), so it drifted silently. Appendix directory census also stale (diff-to-fix 344→459 tracked files; worklog 47→125) — bannered as historical.

### F-D6 (STALE) — docs/error-fix-registry.md F13 "no decryption code at all"

- F13 (last commit 2026-10-23) says the BitLocker fix is "detection + legitimate Windows unlock only (R8); no decryption code at all". M-033 later landed user-FVEK in-stack XTS-AES decryption (`DecryptingDevice` — 5 src files; `--bitlocker-fvek` CLI; `cr_ui_open_volume_with_key` ABI; documented in README:47-49, API.md:117, CAPABILITY-MATRIX §3). The row was never annotated/updated — a reader consulting the registry would wrongly conclude the decrypt path doesn't exist.

### F-D7 (SUSPECT) — docs/CI-CURRENT.md cites a nonexistent tool filename

- CI-CURRENT.md "Files" section cites `tools/gen_fat16_junk_fixture.py`; the tracked file is `tools/gen_junk_gate_fixture.py` (checks.yml:318 cites the correct name). One stale filename in an otherwise verified doc (4 lanes, FloorVersion pins, all scripts/assets exist).

### F-D8 (doc-vs-code contradiction, new at HEAD) — "pure core, no OS headers" vs cr_lastscan.cpp's windows.h

- docs/architecture.md §2: "Everything under `src/core` compiles identically on gcc … and MSVC (/W4 /WX) - **no OS headers**."
- src/core/session/cr_lastscan.cpp:12 includes `<windows.h>` (under `#ifdef _WIN32`; `#else <unistd.h>`), landed with M-154 in `b2bc1970` 2026-10-29 — the first src/core file to break the rule. AUDIT-ARCH.md Dimension-2's "PASS (0)" layering table is likewise outdated on this axis. Either the file should move its platform shims behind a seam, or architecture.md (+ the layering lint, if any) needs a registered exception.

### F-D9 (STALE, mild) — docs/roadmap.md cites removed CI workflows

- roadmap.md v0.2 rows cite `benchmark.yml` and `comparison.yml` CI lanes (removed by the 2026-10-28 windows-only consolidation; CI-CURRENT.md records the deletion). Historical framing but no pointer to the consolidation; doc last touched 2026-10-23.

### F-D10 (STALE, mild) — README.md "worklog.md was retired 2026-10-28" vs reality

- worklog.md received the HEAD commit itself (`e8dcaba8`, "Session-84 opened") and carries ~100 lines of committed conflict markers. If it is retired, the retirement didn't take; if it isn't, README's Documentation table misstates it.

### F-D11 (STALE, in part) — docs/UI-READINESS-PLAN.md unbanned baseline line

- "rrecov scan passes CancelToken* = nullptr and never saves on cancel" (Design baseline section) — superseded by S-057/N-211 (the CLI auto-saves the stop window; doc_truth_gate's check_header_contract_phrases enforces the retired-phrase class in headers + rrecov.cpp). The plan doc carries no supersession banner on that line.

### F-D12 (editorial) — CAPABILITY-MATRIX.md duplicated row; modules duplication fallout

- CAPABILITY-MATRIX.md §2: the "Lost-partition discovery" row appears twice verbatim (adjacent duplicate).
- diff-to-fix/modules/session.md carries TWO "M-154 FIXED" addenda (S80 at :495, Session-82 at :523) from the racing lanes.
- diff-to-fix/modules/orchestrator.md is a 27-line no-H1 addendum file beside modules/orchestration.md (503 lines) — a confusing near-duplicate name; modules/query.md likewise starts with an addendum heading and no H1.
- CROSS-REF-MATRIX.md rows immediately after its conflict block are duplicated verbatim (M-151/N-182 rows).

### F-D13 (cross-doc tension) — GAP-P1-1 id reuse: "open" vs "CLOSED"

- README.md:118-120 and CAPABILITY-MATRIX row 33 present GAP-P1-1 as the live NTFS-compression gap ("the og is LZNT1-only too"); docs/CORE-GAP-LIST.md:42 marks GAP-P1-1 **CLOSED (464d8e5)** — verified: LZNT1 landed (`lznt1_payload_decode`, `expand_compressed_data`, `NtfsVolume.Lznt1MsXcaDocumentedVector`). The residual XPRESS/LZX limitation is real (no xpress codec in src/core/ntfs) but now has no open gap id of its own — the register docs disagree on what "GAP-P1-1" currently means.

### F-D14 (observation) — SUMMARY board counts vs on-disk finding files

- diff-to-fix/SUMMARY.md latest board: "29 active / 176 FIXED / 4 tracked-open"; on-disk: 10 active finding files (8 MAJOR + 2 MINOR), 197 files in findings/FIXED/, 4 in tracked-open/. The deltas are explainable (combined findings, purge convention, board = census-of-record) but a board↔filesystem reconciliation pass would close the ambiguity.

### Verified-clean highlights (no action)

- The doc_truth_gate-guarded counts all match at HEAD: 91 exports / 45+OK error codes / 12 event types / 13 verbs / 13 errmod modules (re-derived with the gate's own regexes).
- All 4 CI workflows, FloorVersion v0.2.4 at exactly two live sites, .github/scripts/* and bench/ assets cited by CI-CURRENT.md exist.
- 10/10 sampled CAPABILITY-MATRIX symbols and 9/9 cited test files exist; AUDIT.md's 9 cited commits all resolve; RdaClient is deleted (comment-only mentions); the removed transport primitives (`rda_available` etc.) exist only in comments; ADR's cr_snapshot.h exists.
- No committed conflict markers in any of the 33 modules/*.md, the 8 docs/parity/*.md, or any findings/evidence file other than the I-048 finding (and M-120's fenced quote).
