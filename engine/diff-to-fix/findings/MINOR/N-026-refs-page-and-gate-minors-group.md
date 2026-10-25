# MINOR N-026 — ReFS page/gate minors (session-2 group)

**Severity:** MINOR (grouped) · **Module:** refs · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** B1, C19, C20, A25
**Audited commit:** `e93decf` · corpus `03ad46c`

From the session-2 ReFS decompile read (agent S2-C2; quotes in `evidence/decompiled-quotes/btrfs-zfs-refs-session2.md`):

**N-013a — Checkpoint body offset hardcoded vs dynamic.** og reads the body offset dynamically (`ReFSSearchDLL_decompiled.c:13743-13756`: `uVar14 = *(uint*)(pagehdr+4)` = 0x20 + ref-parsed extension, then version gate `body+4==3 && 3<*(ushort*)(body+6)`, body size `0x58 if minor>=0xE else 0x44`). Core hardcodes body at page+0x50 (`cr_refs_engine.cpp:546-547`, list at `:567` assumes body=0x50). Identical on every observed volume (extension is 0x30 → 0x50); a page with a different declared extension diverges. Fix: read the dynamic offset (`*(u32*)(page+4)`), keep 0x50 as the fast-path default. Test: `Refs.CheckpointDynamicBodyOffset` (synthetic page with extension 0x40).

**N-013b — Boot gates, two-sided.** og DBR gate `:13511-13566`: bps ∈ {256,512,1024,2048,4096}; **cluster = spc×bps ∈ {4 KiB, 64 KiB} ONLY** (`:13544-46`); major==3 && minor>3 (**3.4+ only**). Core `cr_refs_engine.cpp:369-374`: bps ∈ {512,4096} only (og broader), spc pow2 ≤ 128 with arbitrary pow2 cluster (og stricter: two cluster sizes only). Two-sided divergence; fix to the og's sets (accept 256/1024; require 4K/64K clusters). Tests: `Refs.Cluster64KAccepted`, `Refs.Cluster16KRejected`, `Refs.Bps256RejectedByCore` (currently accepted by og-side parity the core lacks — pick per og).

**N-013c — Second-copy location differs.** og: metadata base = `spc×30` sectors + second copy via the mid-volume formula `((pos/unit) >> 1) - 2` (`FUN_180011e90 :13244`, scan helper `:2360`); core probes cluster 30 → n-3 → n-2 (`cr_refs_engine.cpp:461-479`). Both work on their benches; a torn-volume case where only one copy survives will diverge. Fix: probe the og's mid-volume location too (superset order: start, mid, tail). Test: `Refs.SecondCopyMidVolumeProbed`.

**N-013d — Volume-open read size.** og reads 4 KiB at 0 (`:2425`); core reads 512 B (`cr_refs_engine.cpp:424`). Harmless (gate needs ≥ 0x200) — informational.

**Related:** N-007/I-004 (detection strictness — N-007's first-DWORD claim re-confirmed verbatim at `:11701`) · GAP-S6-3 (the fixed minor gate — verified og-parity at `:13744-56` this session)

**Open questions:** None.
