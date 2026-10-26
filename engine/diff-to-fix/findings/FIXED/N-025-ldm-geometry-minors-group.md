# MINOR N-025 — LDM geometry minors (session-2 group)

**Severity:** MINOR (grouped) · **Module:** ldm · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** B1, A27, X49
**Audited commit:** `e93decf` · corpus `03ad46c`

**N-012a — Config-size gate.** og rejects LDM config regions ≥ 0x801 sectors (1 MiB): `LdmManager_decompiled.c:59750-59759` `"ConfigSize[%I64d] larger than 1m ?."` → rc 0xfffffff8. Core `cr_ldm.cpp:1106-1107` clamps to 8192 sectors (4 MiB) and continues. A >1 MiB (nonstandard) config region aborts og parsing but parses in the core. Fix: reject above the og's bound or register the clamp as a reviewed superset. Test: `Ldm.OversizeConfigRejected`.

**N-012b — RAID-5 zero-interleave default.** og: interleave 0 → 64 KiB default (`:47665`, `:68632-6841`; interleave sectors → 0x80). Core: `cr_ldm.cpp:1344 if (chunk == 0 …) return ERR_UNSUPPORTED;` — refuses. An LDM component VBLK with chunk_size 0 yields a readable default-geometry volume in the og and an error in the core. Fix: default 64 KiB (128 sectors) instead of refusing. Test: `Ldm.ZeroInterleaveDefaultsTo64K`.

**Related:** M-016 (TOCBLOCK candidates) · OQ-T7 resolution (the rotation itself is og-MATCH; these are the surrounding geometry edges)

**Open questions:** None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — N-012a gate + N-012b default, one finding one commit)
**Verified by:** `Ldm.OversizeConfigRejected` (fail-before RED on the stash-isolated pre-fix tree: the oversized disk parsed OK, 4-MiB clamp path), `Ldm.ConfigSizeExactlyOneMebibyteAccepted` (the `< 0x801` boundary pin — 2048 sectors keep parsing), `Ldm.ZeroInterleaveDefaultsTo64K` (fail-before RED: layout incomplete on chunk==0; after: complete, chunk_sectors==128, volume reads resolve column 0 row 0 / column 1 on the second 64 KiB chunk). Suite 740/740 linux-release.
**Resolution notes:** N-012a: the og's 1 MiB config-region gate is now first-class in `parse_ldm_database` — `config_size >= 0x801` returns ERR_BAD_CHECKSUM with the corpus cite (LdmManager_decompiled.c:59748-59759, gate re-verified first-hand this session: `if (config_size < 0x801) ok; else "ConfigSize larger than 1m ?." -> rc 0xfffffff8`); the old 8192-sector read clamp is removed as unreachable (the gate fires first — spec fixes the region at 1 MiB, so a larger declaration is header corruption the og refuses; the previously unregistered parse-anyway superset is closed). N-012b: both the striped and RAID-5 layout paths default a zero interleave to the og's 64 KiB (0x10000 bytes / 512 = 0x80 sectors; ctor default :47665, use-site re-default :68631-68637, sector fallback 0x80 :68638-68641 — all four sites re-verified first-hand); the old behavior lost the volume entirely (zero-capacity layout). The EOD-backup PRIVHEAD fallback and the forgiving TOCBLOCK walk (M-016's registered supersets) are untouched — verified by the full Ldm suite staying green.
