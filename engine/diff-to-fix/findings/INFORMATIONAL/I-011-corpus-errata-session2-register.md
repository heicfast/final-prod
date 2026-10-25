# INFORMATIONAL I-011 — Corpus errata register (session-2): eight artifact claims corrected against the decompile

**Severity:** INFORMATIONAL (corpus-side doc drift; feeds `re-workspace/artifacts/CORRECTIONS.md`)
**Modules:** cross-cutting (evidence quality) · **Parameters:** Q68 (documentation accuracy — of the corpus this time)
**Audited commit:** `e93decf` · corpus `03ad46c` · **Status:** READ-ONLY — errata candidates for the research repo

Session-2 exhaustive decompile reads contradicted eight corpus artifact claims. Recommended as CORRECTIONS.md rows (or artifact edits) so future sessions don't re-import the errors:

1. **NTFS flags 0x10 semantics** — `ntfs-engine--deep-dive.md:77-80` says `(flags & 0x10) == 0` = "not in-use"; `ntfs-engine-algorithms-v2--deleted-viability.md:41` says "not a folder". The decompile settles it: **0x10 = $FILE_NAME DIRECTORY bit** (`FUN_180043700 :53481`: `*(u32*)(fnobj+0x50) & 0x10000000`; set at props build `:5329-30`). algorithms-v2 is right; deep-dive is wrong. (This contradiction fed S-001's original mis-scoping.)
2. **NTFS pass structure** — both NTFS artifacts describe `FUN_18001acc0`'s destroy/viability loop as "the deleted-file pipeline" without noting it parses **GUID-dump contents**, not the live MFT (see S-012). The "record_in_use" claim in algorithms-v2 §1 is also wrong (base-record gate = extension-record skip, CORRECTIONS row 2 already covers the symbol; extend the row).
3. **`size ≤ 0x3ff` semantics** — artifacts call it a "fast-path keep" for small files; the decompile shows small records **fall into the destroy label** (`:21208`/`:21256`) — dropped, not kept.
4. **HFS+ journal magic** — session-1's S-003 guessed `0x6a6e6c78`; the og compares raw **`0x4a4e4c78`** with endian marker `0x12345678` (`:13542-43`) — little-endian journals only.
5. **ZFS label geometry** — `zfs-engine--deep-dive.md:23` "4 labels × 128KB" is inferred and wrong: the og reads only the front region (112 KiB label + 128 KiB ring; zero L1-L3 constants). The artifact's "0x4000 @ 0x1C000" also swaps the two arguments of `FUN_180018220(param_1, &buf, size=0x4000, len=0x1C000)`… i.e. the read is 0x1C000 bytes at offset 0x4000.
6. **ZIP version gate** — the carve artifact's "version_needed ≤ 10" is only the first-record LFH gate (`FUN_1800176e0 :18389-99`); the quick gate is `≤ 0x13 || == 0x62` (`:18288-96`). See N-014d.
7. **Carve signature census** — three mislabels in the dual-registry artifact: `0x4D4D "MM"` is **3DS** (`File3DSAnalyser`, `:2885-88`), not TIFF-BE; `0x4449 "DI"` is the **MP3 analyzer** (`:2556`), not AIFF/IANA; `0xd302020` is **DXF** `"  0\r"` (`:1936`), not an MPEG frame sync.
8. **IOCTL 0x60002400** — `eprdtdrv-sys--driver-internals.md` §4 calls it "sets/resolves the global target"; the decompile dispatch shows **GetProcName** (`eprdtdrv_decompiled.c:496`; traces `:410/:431`; the resolver sighted at `:701` is a 0x60002404 call site). The core already matches the corrected reading (`cr_rda.h:41`).

**CORRECTIONS.md fold-in status (session-1's 8 rows):** rows 2/3/4/6/7/8 folded into verdicts; row 1 (MSVC string-dtor artifact) moot — no verdict depends on it and the core has no 0xFFF resident gate; row 5 partially folded — `modules/visualization.md:16/:33` still quotes the stale "og 8/1489" figure (fix those two lines when the module file is next touched).

**Action:** file as an errata PR against the research corpus (out of this repo's write scope); meanwhile every session-2 finding cites the decompile, not the corrected artifacts.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** the eight corrections cross-checked against diff-to-fix/CORPUS-ERRATA.md (rows present; the session-2 register mirrors the corpus-side CORRECTIONS.md the read-only repo carries).
**Resolution notes:** Corpus-side errata register, acknowledged as evidence-quality maintenance: future sessions must cite the CORRECTED facts (flags 0x10 = DIRECTORY; pass 2 parses GUID-dump contents not the live MFT; small records are DESTROYED not kept; HFS+ journal magic 0x4a4e4c78 little-endian). Items 1-3 fed S-001/S-012 and are now embodied in those findings' resolutions; no core change.
