# MINOR N-025 — LDM geometry minors (session-2 group)

**Severity:** MINOR (grouped) · **Module:** ldm · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** B1, A27, X49
**Audited commit:** `e93decf` · corpus `03ad46c`

**N-012a — Config-size gate.** og rejects LDM config regions ≥ 0x801 sectors (1 MiB): `LdmManager_decompiled.c:59750-59759` `"ConfigSize[%I64d] larger than 1m ?."` → rc 0xfffffff8. Core `cr_ldm.cpp:1106-1107` clamps to 8192 sectors (4 MiB) and continues. A >1 MiB (nonstandard) config region aborts og parsing but parses in the core. Fix: reject above the og's bound or register the clamp as a reviewed superset. Test: `Ldm.OversizeConfigRejected`.

**N-012b — RAID-5 zero-interleave default.** og: interleave 0 → 64 KiB default (`:47665`, `:68632-6841`; interleave sectors → 0x80). Core: `cr_ldm.cpp:1344 if (chunk == 0 …) return ERR_UNSUPPORTED;` — refuses. An LDM component VBLK with chunk_size 0 yields a readable default-geometry volume in the og and an error in the core. Fix: default 64 KiB (128 sectors) instead of refusing. Test: `Ldm.ZeroInterleaveDefaultsTo64K`.

**Related:** M-016 (TOCBLOCK candidates) · OQ-T7 resolution (the rotation itself is og-MATCH; these are the surrounding geometry edges)

**Open questions:** None.
