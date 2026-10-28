# INFORMATIONAL I-039 — the corpus-worklog cross-check register (the first audit lane to read easeus-drw-research/worklog.md in full): 3 identity/asset datums registered (the FoxitRestore codename, the NAS LM-hash auth material, the shipped-build identity), 3 corpus-side count/prose errata filed, ~45 worklog-claimed RE surfaces verified dispositioned, and the core-side never-cited navigability set (RENUMBERED TWICE at the hot-window reconciliations: this lane's draft I-036 collided with the remote S42 lane's own config-layer register [ACKNOWLEDGED-retired at 0bd04cf9 — its shellexe/TreeType/themdark/DRWLog subjects are a complement, not an overlap, of this register's families]; then the draft I-037/I-038 pair was displaced by the remote second-continuation lane's I-037 decode register and this lane's own diskhealth register taking I-038; earlier-push-wins — the final seat is I-039)

**Severity:** INFORMATIONAL (a register: identity datums + errata filings + a cross-check verdict; no behavioral gap)
**Module:** cross-cutting (the corpus's own record-keeping vs the audit trail) — README.md's corpus section
**Parameters affected:** D46 (documentation accuracy), P69 (the register's completeness — the mark-all-remaining duty)
**Corpus references:** `easeus-drw-research/worklog.md` (62 KB, 62,396 B — read in full, S42/W3b; line cites below); `re-workspace/artifacts/UIUX/02-string-inventory.md:15` (the Mainland_Foxit.qm corroboration); `og-codebase/language/` (37 .qm — the on-disk ground truth for the Languages count); `docs/PARITY-OG-CORE-INVENTORY.md:157` (the RawFixer 31-vs-33 drift's trail-side instance — docs/, queued not edited); `docs/PARITY-DELTAS.md` (the retention-nag row)
**Core references (at the audited commit `869da779`):** the trail-negative greps (each datum: `rg` zero-hit across core_recovery for the datum string); the W4 citation census (232 core files: 170 cited, 62 exact-name-uncited — the navigability set below)
**Status:** ACKNOWLEDGED-RETIRED per the trail's current INFO convention (the e41cbb7e class: filed, rationale verified first-hand, retired to findings/FIXED/ with the ledger row) — the datums' dispositions and the errata rows stand; the errata rows land in CORPUS-ERRATA.md's Session-42-B block (this lane's trail-side surface); no fix applied

## Summary

No prior audit lane had ever read the corpus's own `worklog.md` (62 KB —
grep-verified: zero citations of it anywhere in diff-to-fix/ history). The
S42/W3b pass read it in full and cross-checked every worklog-claimed RE
surface against the audit trail: **~45 claimed surfaces, zero lacking a
disposition, zero material contradictions** — three near-misses cleared
(DRWService's pipe direction, the RegLib hbin-writer phrasing, EUinApp's
role), three numeric drifts filed (two corpus-side errata + one trail-side
docs row queued). The worklog carries three product-identity/asset datums
the trail had never registered; this file registers them. It also records
the W4 citation census's verdict: the core tree has **no behavior-level
never-dispositioned surface** — the never-cited residue is navigability,
not coverage.

## What the reference does (the corpus worklog's content — the datums)

- **C1. The "FoxitRestore" internal codename** (corpus worklog :87 — the
  registry root `SOFTWARE\Foxit Software\FoxitRestore` + the agent URL;
  corroborated by UIUX/02-string-inventory.md:15 — `Mainland_Foxit.qm`, 19
  messages, "the 'FoxitRestore'-branded variant"). The og product is the
  re-branded FoxitRestore line for the Mainland OEM channel. `rg` over
  core_recovery → 0 hits: the trail never recorded the identity datum.
- **C2. The LM-hash DES "KGS!@#$%" NAS-auth material** (worklog :142 —
  DRWUI's crypto map carries the magic constant of the
  LM-authentication-key derivation, used by the NAS auth flow). `rg` → 0
  hits; maps to the EXCLUDED-NAS registration (SURFACE-INVENTORY §5/§6) —
  the auth-side datum belongs on that row.
- **C3. The shipped-build identity** (worklog :482-483 — the installed
  og-codebase's git commit `40504ccb1923…` and build time 2026-10-13
  13:33:43). The trail carries SetupBuildTime but never the source commit —
  a versioning datum for any future "which build was the corpus" question.

## What our core does

Nothing — by design, in all three cases: the core's ABI has no Mainland-OEM
rebrand surface (the C1 datum maps to the UI/OOS inventory line); the core
implements no NAS auth (C2, the EXCLUDED-NAS posture); the core's own build
identity is its own (C3 is a corpus-forensics datum). The register's purpose
is that a future parity session asking "what is FoxitRestore / why does
DRWUI carry an LM-hash constant" finds the answer on the trail without
re-reading the corpus worklog.

## Gap → Register rows + errata

**Registered (this file):** C1/C2/C3 as above, each with its disposition:
C1 → the OOS-UI/rebrand datum row; C2 → the EXCLUDED-NAS auth-datum row; C3
→ the corpus-forensics datum row.

**Corpus-side errata (filed in CORPUS-ERRATA.md's Session-42 block):**
- **E-A. The Languages census count defect** — the corpus claims "8
  qtbase" .qm files where the on-disk tree has **11** (the "25+4+8=37"
  arithmetic holds only by coincidence; actual: 22 product + 4 Mainland
  OEM + 11 qtbase = 37, English.qm a 16-B header-only). Corpus cite: the
  worklog's language-census lines + UIUX/02-string-inventory.md:16's "13
  languages" qtbase claim (both stale vs disk).
- **E-B. The retention-nag period drift** — corpus worklog :785 says
  "~10-60 s" where the trail's PARITY-DELTAS row carries "30–60 s" (the
  decoded CChronoSnap retry band). Cosmetic; recorded so the two records
  converge.

**Trail-side docs drift (queued for the next doc lane, the I-033
convention — docs/ is outside this lane's writable surface):**
- **E-C. docs/PARITY-OG-CORE-INVENTORY.md:157** — the RawFixer row says
  "31-method inventory"; the truth is **33** (modules/rawfixer.md:4,108 —
  `rawfixer-v2--method-inventory.md` "33 methods"; the corpus itself
  drifted 31→33 across worklog :163→:699; S-008's evidence upgrade names
  "the full 33-method og table"). One-word repair.

## The W4 citation census (the core-side mark-all-remaining verdict)

232 src/include/tests files at `869da779`: **170 cited by exact name**,
62 exact-name-uncited (11 src + 24 include + 27 tests), 35 uncited in any
stem form. Every uncited file maps to an existing module-doc/register/ledger
disposition — **zero behavior-level gaps**. The navigability set (the
stems a future session greps for and finds nothing): `cr_family.cpp` (0
hits anywhere, but health.md:108 + S-024's ledger row disposition it
exactly); `cr_bits.h` (the one file with neither citation nor diff-to-fix
disposition — its dedup note lives in core-side AUDIT-LINUX-FS DC-12; pure
byte-order utility); `cr_dict.cpp`/`cr_report.cpp` (stem hits land on
siblings; the surfaces are owned by N-064/API-DELTA #43 and writer.md D13);
the header twin set (cr_exfat_engine/wire.h, cr_fat_engine.h, ntfs
attrs/fixup/runlist.h, cr_bootfix/destio/rawfixer/nvramfix/vollabel.h) —
all directory-cited or module-doc-covered; 27 uncited test files pin
covered surfaces (only `test_tmpdir.hpp` is pure test-infra). **The
verdict: cite-density is a navigability nicety, not an audit gap — this
register is the index that closes the residue.**

## Reproduction / verification

Every claim cites a file read first-hand: the corpus worklog in full (line
numbers above); the 37-.qm on-disk recount (find + per-file header check);
the trail-negative `rg` runs (FoxitRestore, KGS, 40504ccb, per-datum); the
W4 census (the full 232-row table in
/home/z/my-project/s42-scratch/w4-citation-gap-liveness.md — scratch, not
committed; the load-bearing rows transcribed here); the ~45-surface
worklog→trail cross-check (each surface greped against diff-to-fix/ —
DRWCfg.bin AES key → R-57/D-60f/I-019; VAPreview ports 20171/2 →
modules/preview.md; EUEDKEPM → DRIVER-UTILIZATION; connect.exe → row 83;
FS-type codes → R-10; settings+0x60 → SCAN-ENGINE-DELTA A1/A5;
RecoveryAssistant IDs → R-27/C4/S-023; PST/ZIP caps → PARITY-CORE-REC/W2;
ChronoSnap → PARITY-DELTAS P02e/P40; CrystalDiskInfo → R-60/FB-H01; the
QA-night StartSector 705794048 forensic → s3-calcpartition-scoring.md; …).

## Fix plan (for the engineering team)

1. **Trail-side (DONE this wave):** this register; the CORPUS-ERRATA
   Session-42 rows (E-A/E-B).
2. **Doc-side (queued, the I-033 convention):** the E-C one-word repair
   (PARITY-OG-CORE-INVENTORY.md:157 "31" → "33"); the EXCLUDED-NAS row's
   auth-datum clause (C2); the README corpus-section's identity-datum
   pointers (C1/C3) if the maintainers want them on the trail's front
   page.
3. **Corpus-side (for the corpus maintainers — NOT applied, read-only):**
   the E-A/E-B rows fold into the corpus's own
   `re-workspace/artifacts/CORRECTIONS.md` (the running list, 9 rows at
   audit time).
4. **Acceptance criteria:** a future session greps "FoxitRestore" /
   "KGS!@#$%" / "40504ccb" and finds this register; the docs row reads 33;
   no other change.

## Tests to add

None — register-only. (The navigability set's coverage is pinned by the
existing per-module tests; a citation is not a testable behavior.)

## Related findings

**I-034** (the never-cited-artifact register — the corpus-side sibling of
this census; this file completes the core-side half); **I-027/I-033** (the
asset/config registers whose doc-application queues E-C joins); **I-035**
(this lane's companion-tree register — the same walk's other product);
**N-135** (the truth-batch class E-C's count-drift family); **N-129** (the
register numeric-defect precedent for E-A/E-C).

## Open questions

None material. The three near-misses cleared during the cross-check are
recorded in the scratch sheet for provenance only (no trail change
needed); U-class datums live on I-035, not here.
