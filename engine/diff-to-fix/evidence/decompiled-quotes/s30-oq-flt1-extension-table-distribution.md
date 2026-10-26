# OQ-FLT-1 resolution evidence — the 169-entry extension table is a 4-engine shared vocabulary; the M-035 scorer is exFAT-only

**Session:** DFX30 (tracked-open tail lane), 2026-10-26
**Method:** first-hand PE section mapping + byte-pattern search + decompile reference scan on the corpus binaries (`og-codebase/*SearchDLL*.dll`, `DRWUI.exe`) and decompiles (`re-workspace/reversed/ghidra-searchdll/*_decompiled.c`). Probe scripts: `oq_flt1_probe.py`, `oq_flt1_consumers.py` (session tooling, not committed — commands reproduced below).

## The table

`exFATSearchDLL.dll` VA `0x18005a090` (.data, file offset 0x58690): 170 slots ×
char[9]; slots 0..168 populated, slot 169 all-zero sentinel; every flag byte 0;
first key "Raw", then "3DS".."COMMON". 169 populated keys — **set-identical to
DRWUI.exe's grading table at VA 0x141bb0500** (169 keys, `only-in-exFAT: [],
only-in-DRWUI: [], identical sets: True` from the set diff).

## Engine-wide distribution (byte-exact 1530-byte block search)

| Engine | copies | location(s) | code refs (decompile DAT_ scan + binary pointer scan) |
|---|---|---|---|
| NTFSSearchDLL.dll | 1 | .rdata 0x18006db20 | **0 refs** (DAT_ scan 0; full-VA 8-byte and RVA 4-byte pointer scans: 0 hits) — dead data |
| FATSearchDLL.dll | 3 | .rdata 0x18004dc60 (0 refs, dead) + .data 0x1800630d0 (5 refs) + .data 0x180067720 (2 refs) | live |
| exFATSearchDLL.dll | 2 | .rdata 0x180045b00 (0 refs, dead) + .data 0x18005a090 (4 refs) | live (M-035's own cites) |
| RAWSearchDLL.dll | 1 | .data 0x18008f070 (1 ref) | live |
| EXT4 / Btrfs / ReFS / HFSPlus / Zfs SearchDLL | 0 | — | — |

## What the live consumers do (read first-hand)

- **exFAT** `FUN_180024fe0` (:30785-30960): the M-035 classifier (index gate
  `< 0xAA`, `_strupr(table + idx*9)`, strstr against the filter string) and
  `FUN_180025390` (:30973-31156): the M-035 validation scorer with the reject
  rule `(considered>>1 <= failed) && ratio < 0.3 && failed != 0`.
- **FAT** `FUN_180024a10` (:29075+): the same index-gate `< 0xAA` +
  `_strupr(table + idx*9)` + `strstr(param_1+0x128, ext)` filter-check shape
  (the FileFilter "filter by extension index" path, file record `+0x18` = the
  extension index). `FUN_180017b90` (:17526+): iterates the whole table
  building strings matched against a list at `param_1+0x1d8` (category-set
  construction). `FUN_180002ae0` (:1378) / `FUN_180013ed0` (:14523): the same
  iterate-and-build shape.
- **RAW** `FUN_18000b7a0` (:8599+): extension-name accessor — reads the record
  field `+0x18` (u32 extension index), builds the name string from
  `table + idx*9` (no gate visible at this site; the caller family is the
  properties/file-info accessor set FUN_18000b760..FUN_18000b8b0).

## The scorer blast-radius answer (M-035's open question)

`grep -c "<= 0\.3\|0\.3 &&\|0x3e99999"` over every engine decompile:
**0 hits in FAT, RAW, NTFS, EXT4, Btrfs, ReFS, HFSPlus, Zfs** — the
majority-mismatch + ratio≤0.3 ghost-rejection scorer exists ONLY in
exFATSearchDLL. M-035's fix scope is exFAT-only.

The table itself, however, is the product-wide extension vocabulary: the
extension INDEX carried in engine record fields (`+0x18`, gate `< 0xAA`) is
positionally meaningful across FAT/exFAT/RAW (and the same 169-key set feeds
DRWUI's deep-validation grading gate). Relevant to the N-049/N-050
lost-record field-richness family and any filter-layer parity work.

## Reproduction

```sh
python3 oq_flt1_probe.py        # PE mapping, table extraction, engine byte-search, DRWUI set diff
python3 oq_flt1_consumers.py    # file-offset -> VA mapping + decompile DAT_ reference scan
grep -n "DAT_1800630d0\|DAT_180067720" FATSearchDLL_decompiled.c      # 5 + 2 sites
grep -n "DAT_18008f070" RAWSearchDLL_decompiled.c                     # 1 site
grep -c "<= 0\.3" <each engine>_decompiled.c                          # 0 everywhere except exFAT
```
