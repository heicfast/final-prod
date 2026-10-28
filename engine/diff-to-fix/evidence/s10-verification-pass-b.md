# s10-verification-pass-b.md — the SECOND verification lane (convergent with s10-reverification-sweep.md; the unique verdicts + the reconciliation record)

**Session 10, lane B (2026-10-29). Core `84a1832`. Corpus `be0cfe9`.**
**Convergence note (push-time, the no-duplication policy):** the parallel session-10
lane filed `evidence/s10-reverification-sweep.md` FIRST with the same mandate (the
board-truth pass at `84a1832`, four agents). This lane ran five agents over the same
board independently; the two lanes' 69 STILL-ACTIVE verdicts are **CONVERGENT**. This
file keeps only lane B's UNIQUE content: (1) the **M-100 NOW-FIXED discovery the first
lane missed** (their board kept M-100 active — the resolution record below); (2) the
S-041 remaining-format precision list; (3) the four partial-residue verdicts
(N-101/N-106/N-108/N-120); (4) the M-071/M-050/N-108 historical records (those
findings were retired to history by the parallel purge mid-flight — lane B's addenda
on them are preserved here); (5) the 13-entry line-drift register; (6) the push-time
reconciliation against the concurrent fix lanes + the purge convention.

**Method (lane B):** five parallel verification agents (SEVERE / MAJOR M-071..M-089 /
MAJOR M-090..M-102 + MINOR N-090..N-108 / MINOR N-111..N-127 + INFO + the two OQs),
every verdict file:line-backed at `84a1832`; the lead re-verified the load-bearing
verdicts first-hand (M-100's landed capability in-source; M-071's gate census; S-041's
remaining-format greps; the config-layer absences; the parent-anchor greps).

## The unique board move: M-100

**M-100 (archive browse-inside absent) → NOW-FIXED — closed-as-covered.** The
capability predates the finding: commit `6d0f16c` ("fix(preview): M-066 + N-088 — the
archive-content inventory") is an ancestor of BOTH of M-100's own audited baselines
(`35adcb9` and `ff879f9`; `git merge-base --is-ancestor` verified). In-source at
`84a1832`: `include/cr/preview/cr_preview.h:114-149` — `ArchiveEntry` (:119-124:
name/size/compressed/crc_present), `kMaxPreviewArchiveEntries = 512` (:130),
`Metadata.entries` (:144-149, "for ANY archive whose central directory is walkable at
preview time, carved or FS-origin alike"); `src/core/preview/cr_preview.cpp:694-737` —
the CD walk with the damaged-zip partial-CD salvage flagged truncated (:690-693,
:733-734) and the hostile cap (:718-719). The finding's central "grep for
archive-entry enumeration: absent" claim was FALSE at its own baseline (the landing
used `Metadata.entries`, not an `archive_list` symbol — the filing lane's grep missed
it). **Residue (not a divergence):** the fix-plan nicety of a dedicated
`cr_ui_archive_list` two-call ABI — reachable today through `cr_ui_preview_metadata`;
an API-ergonomics plan item (REFERENCE-API-GAP lane). The RAR/7z browse-scope open
question stands as recorded. **Disposition:** FIXED-LEDGER row + the MAJOR/ file
retired (the SA-10B convention), this record + the SUMMARY session block carrying the
narrative.

## Partial-residue verdicts (originally addenda; the findings were retired by the concurrent lanes at the SECOND push — the content is preserved HERE)

- **N-101:** the session-9 currency-pass halves stand; residue = the per-binary row
  "113/114" (honest: 112/114 — M-064 cites deepening-batch1 ×4) + the unstruck "78
  cited" at :16.
- **N-106:** the disposition stands; residue = the claimed "sixth-source note in
  N-071's row" never existed (grep: 0 hits).
- **N-108:** residue verified **FIXED by the remote doc-truth batch** (`e1e5124` —
  `docs/PARAMETERS.md:78-79` now carries 1 MiB/4 KiB + 12/48/1024/80; checked
  post-rebase). Lane B's addendum is moot; the finding's retirement by that batch is
  correct.
- **N-120:** residue = I-010's D-26 over-claim (runtime-families not in D-26's list),
  PARITY-CORE-REC `:164` off-by-one (seven trail sites cite the blank line), the
  FileRepair wrong-register pointer.

## S-041's remaining list (the precision pass)

LANDED (7): CHM, ONE, MNY, EMF, SWF (FWS+CWS), 3DS (MM + NCSD) — school 1 `b92ebef`;
CRW — `9e5dedc`. CLOSED-AS-COVERED (1): FCPEvt (FMT_SQLITE + the N-089 subtype
relabel). LANDED BY THE PARALLEL LANE (1): MPEG-TS/M2TS — S-047 `a3abfbe`
(FMT_TS/FMT_M2TS verified in-tree post-rebase at cr_signatures.h:202-203). **REMAINING
(13):** JP2→N-111, MOBI→M-096, HTML→M-092, SVG/XML→M-093, LZH→N-112, VCF/GPX/OSP/POS/
CHFP→N-113, HIK→M-098, AEP→M-099, OfficeTagFromXML→N-115. The registry carries 111 FMT_
entries at the post-rebase tree (109 at `84a1832` + FMT_TS + FMT_M2TS).

## The historical addenda preserved here (findings retired to history by the purge before this lane's push)

- **M-071 narrative completion:** the junk gates were transiently RE-ADDED by the
  union merges `53b7d68`/`6f0052b`/`001ac04` (10-26 06:41-06:48) then dropped a SECOND
  time at `b95de7d` — the filing's 4-point chain (69507cb → cfa4c75 → c228d2d → HEAD)
  skipped the restoration window. The remote fix lane (`ce2405a`/`dd0ffd5`) closes the
  regression; the git-archaeology record stands here for the history file.
- **M-050 completeness record:** the deliberate not-persisted wrapper-scalar set's
  full enumeration — `f_48`(−1), `flag_91_bit0`, `flag_72` bits 0/1/7, `flag_73_bit1`,
  `f_1c8/f_1d0/f_1d4`, `f_1e4/f_1e8/f_1ec/f_1f0`, `f_200/f_208`, `f_288/f_289`,
  CGeneric +0x3A (const 0) — all the undecoded-semantics class, gated on corpus
  decodes ("carrying opaque u16s for undecoded semantics would be inventing
  structure"). Also recorded in `s10-field-parity-table.md` (the surviving record).

## The line-drift register (verdicts unchanged; cites refreshed at `84a1832`)

M-071 (plan lambda :1961-1988) · M-079 (ABI copy :1181; naming reads :2026/:1714;
fill_identity_card :1846-1862) · M-090 (registry :712-721) · M-091 (validator
:3543-3557; registry :883-893) · M-093 (+14 rows) · N-093 (MID :705; GZ :802; PSP
:782-791; SIT :792-801) · N-094 (usage :2997-3034; dispatch :3039-3060) · N-095
(§7f/§7g ~:881-895/:897-916) · N-098 (enumerate :1649-1679) · N-105 (rrecov :1714) ·
N-116 (CLI sort :960) · N-123 (dispatch :3039-3060). Cause: the
`b92ebef`/`9e5dedc`/`dd225da`/`095a329` landings above the cited lines.

## PUSH-TIME RECONCILIATION (the moving-target SOP executed — this lane's post-rebase record)

Four concurrent fix lanes + the SA-10B purge-convention lane landed on origin/main
while this verification ran. **This lane's verdicts stand as the record at `84a1832`**
(the tree they were verified at); the landings below SUPERSEDE still-active verdicts
at the new HEAD — each fix lane's own RED-test reproduction corroborates this lane's
verdict (the defect was real at the audited tree):

| This lane's verdict at `84a1832` | Superseded by (remote) | Landing |
|---|---|---|
| S-042 STILL-ACTIVE | FIXED | `f6e303e` (WMF placeable grammar) |
| S-043 STILL-ACTIVE | FIXED | `b3938c8`/`d0787e9` (EXIF orientation, the 8-case map) |
| S-044 STILL-ACTIVE | FIXED | `9496f3c` |
| S-045 STILL-ACTIVE | PARTIAL-FIXED | `3aba383` (the TIFF-RAW half; the codec-bound halves → tracked-open OQ-S10-1, the remote's numbering) |
| S-047 STILL-ACTIVE | FIXED | `a3abfbe` (FMT_TS/FMT_M2TS) |
| M-071 STILL-ACTIVE | FIXED | `ce2405a`/`dd0ffd5` |
| M-080 STILL-ACTIVE | FIXED | `5ccc0da` (max_read 4→32 MiB + the byte-oversize embedded fast path) |
| M-082 STILL-ACTIVE | FIXED | `1f71488` (the DeviceManager offline-OS hive walk) |
| M-084/M-091 STILL-ACTIVE | FIXED | `16475b8` (the MDB family pair) |
| M-085 STILL-ACTIVE | FIXED | `edee513`/`fb7cf30` (native TIFF LZW, the early-change width) |
| M-088 STILL-ACTIVE (core half) | FIXED | DFX-38 `41d8829` |
| M-090 STILL-ACTIVE | FIXED | `771e2ec`/`9daa787` |
| N-094 STILL-ACTIVE | FIXED | `26da616` |
| N-095/N-108/N-119 STILL-ACTIVE | FIXED | `e1e5124` (the doc-truth batch) |
| N-103/N-116/N-117/N-118 STILL-ACTIVE | FIXED | `954c386`/`632d21e`/`9445627` |
| M-092/M-093-class residuals | STILL-ACTIVE | (the MDB landing covers that family only — HTML/XML/MOBI/HIK/AEP/N-111..N-113 unchanged) |

**The remote board's own ID collision, repaired by this lane at push:** two S-050
files had landed concurrently (`S-050-file-repair-line-absent-and-unregistered.md` —
the SA-10B lane, first; `S-050-wholedisk-scan-targets-absent-silent-empty.md` — the
session-10 wave-2 lane, second). Per the session-9 collision convention (the earlier
remote numbering wins), the second file is renumbered **S-051** (references updated;
the renumber note lives in the file).

## The read-only guarantee (lane B)

This lane's commits touch NOTHING outside `diff-to-fix/` + `worklog*`. The probe
binaries/scripts live outside the repo (`/home/z/my-project/scripts/`); the two
compiled probe runs (`s10-preview-probe`, `rsf-probe.py`) read repo sources without
writing them.


---

## SECOND PUSH-TIME RECONCILIATION (the SA-10 lead close + the truth batches + the M-092 fix landed between this lane's first and second pushes)

- **Renumbered again** (the collision convention): this lane's M-105 → **M-108** (the
  SA-10 lead close consumed M-105 DRWService-install-lifecycle, M-106 ONE/3DS,
  M-107 batch-1-stand-in) and N-128 → **N-132** (their N-128 capability-matrix-ci-row).
  N-129/N-130/N-131, OQ-S10-2 and E69 were free — kept.
- **CONVERGENT repair confirmed:** the remote SA-10 close (`bb2cd8a`) performed the
  same S-050→S-051 collision repair this lane made — both lanes, one conclusion.
- **The retired-addenda dispositions (their resolutions landed with the concurrent
  lanes):** S-041 closed-as-umbrella (`33dce10`/`8d4ebb4` — the children own the
  remainders; after the M-092 HTML carve fix (`eb1f116`), **8 child findings remain
  active**: M-093/M-096/M-098/M-099/N-111/N-112/N-113/N-115); N-101 FIXED
  (`33dce10` — every step verified executed in-tree); N-106 FIXED-ALREADY (the
  batch-C truth commit, with reproductions); N-120's live residues recorded by the
  batch-C truth commit (the I-010/D-26 pointer + the PARITY-CORE-REC :164 off-by-one
  + the FileRepair-register pointer — still open trail debt, now owned by that
  commit's record).
- **Board arithmetic at this lane's final push:** 54 active (6 SEVERE · 24 MAJOR ·
  19 MINOR · 5 INFO) · 279 resolved (the FIXED-LEDGER, +M-100 this push) ·
  4 tracked-open.
