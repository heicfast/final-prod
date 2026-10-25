# MINOR N-023 — FAT/exFAT gate and semantics minors (session-2 group)

**Severity:** MINOR (grouped) · **Modules:** fat, exfat · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** B1, B9, B3, C19, C20, A27
**Audited commit:** `e93decf` · corpus `03ad46c`

Six small divergences from the session-2 decompile read (agent S2-B; quotes in `evidence/decompiled-quotes/fat-exfat-session2.md`):

**N-010a — FAT flavor boundary off-by-one at 65524 data clusters.** og `FUN_180007e50 :5219-5231`: FAT16 range `[0xff5, 0xfff4)` **upper exclusive**, FAT32 lower bound **inclusive** (`0xfff4` → 65524 → FAT32). Core `cr_chain.cpp:14-18` uses `< 65525` → 65524 becomes FAT16. A 65524-cluster volume reads different chain bytes on each side. Fix: make the boundary inclusive for FAT32 at exactly 65524. Test: `Fat.FlavorBoundary65524`.

**N-010b — BPB FS-type string and bps==device gates.** og `FUN_18002f970 :38048/:38055/:38062`: requires `_strnicmp(boot+0x36,"FAT16   ",8)` / `(boot+0x52,"FAT32   ",8)` and `*(ushort*)(boot+0xb) == device_sector_size`; failure → flavor 0xE → engine declines (`:8012`). Also spc ∈ {1,2,4,8,16,32,64,128} bitmask `0x800000008000808b`, FAT16 root_entries ≥ 224. Core `cr_chain.cpp:243-301` checks none of these (any pow2 spc incl. 256+). Fix: add the string + bps gates and the spc set; keep the looser set only if registered. Test: `Fat.BootStringGate`.

**N-010c — FAT dentry date-field sanity gates.** og `FUN_1800365a0 :44298-44315`: create/write/access month 1-12, day≠0 (or whole field 0) else the dentry is rejected. Core emits dentries regardless of date sanity. Fix: add the three-field sanity gate. Test: `Fat.DateSanityRejected`.

**N-010d — exFAT timezone offset applied by core, absent in the og engine.** No `*900`/`*0x384`/`*3600` patterns anywhere in exFATSearchDLL (the +0x16..0x18 tz bytes are never read; the dentry validator only sanity-checks timestamps). Core `cr_exfat_quick.cpp:121-139` `exfat_time_to_unix100(ts, tz)` subtracts quarter-hours. Cross-module caveat: the og may convert in DRWUI — keep the core behavior but register the delta until that side is pinned. Test: n/a (registration item).

**N-010e — exFAT stream reserved-field test: og byte-OR vs core whole-word.** og `FUN_180003090 :1843-1846` tests `param_1[2]==0 && (param_1[6]==0 || param_1[7]==0) && (param_1[0x10]==0 || param_1[0x11]==0 || param_1[0x12]==0 || param_1[0x13]==0)` vs core `cr_exfat_quick.cpp:294-295` word-nonzero tests. Divergence only when a reserved word is partially nonzero; the OR form may be Ghidra's rendering of u16 tests — re-probe once with a raw disassembly before changing anything.

**N-010f — FAT directory mid-chain FREE salvage is map-gated in the og; mid-chain BAD aborts the og's directory read.** og `FUN_18000f950 :11075-11116` continues at `cluster+1` on a FREE answer only when the range is found in the volume-claim/bitmap RB-trees; `FUN_18000fb80 :11231-11243` aborts the whole directory read on canonical BAD (device-size check trips). Core keeps pre-BAD clusters and marks the chain broken (`cr_chain.cpp:159-161`). Informational-grade content-shape difference on damaged volumes — register, do not chase.

**Related:** M-011/M-012/M-014/M-015 (the substantive FAT/exFAT majors) · N-002 (EOC families, mechanism-corrected this session)

**Open questions:** N-010e's raw-disassembly re-probe (one pass, engineering team).
