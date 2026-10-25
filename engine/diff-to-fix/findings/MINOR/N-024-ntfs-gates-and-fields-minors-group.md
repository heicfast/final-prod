# MINOR N-024 — NTFS gates/fields minors (session-2 group)

**Severity:** MINOR (grouped) · **Module:** ntfs · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** B10, C19, C20, D11, D12, B7, C23
**Audited commit:** `e93decf` · corpus `03ad46c`

Five small divergences from the session-2 NTFS re-read (agents S2-A2/S2-G1/S2-F; quotes in `evidence/decompiled-quotes/ntfs-pass2-guid-dump.md`):

**N-011a — 8.3 short-name node field absent (D11/D12).** The og's node properties carry a separate short-name (vtable `GetShortNameLen/SetShortName/GetShortNamePtr`, `raw-evidence/props-vtables.txt`; persisted per `artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md:19-21`). Core has a single `name` (`cr_node.h:68`); DOS-namespace fallback in `cr_mftwalk.cpp:44-51` preserves recovery but not the field. Fix: add a `short_name` node field (nullable), persist in `.crsf` as a versioned optional field. Test: `Ntfs.ShortNameFieldRoundtrips`.

**N-011b — Boot-gate two-sided minors.** og NTFSLib `CNtfsVolume::LoadNtfsVolume :19292-19344`: **no 0xAA55 check**, bps multiple-of-512 only, spc < 0x81 direct else `1 << (256-cpr)`, **cpr == 0 or 0-result → REJECT** ("It is not a NTFS volume!"), index-block byte 0 → reject, **no pow2 checks, no 0x400 clamp**. Core `cr_bootstrap.cpp:30-56` + `cr_ntfs_wire.h:208-217`: pow2 required, `< 0x400 → clamp 0x400`, `%bps → ERR`, device-size sanity the og lacks. Two-sided: volumes the og rejects (cpr==0) the core clamps-and-continues; geometries the core rejects (non-pow2 spc) the og accepts. Fix: align the ladder with the og (reject cpr==0; drop the 0x400 clamp; keep core's stricter device sanity registered as safety). Tests: `Ntfs.BootRecordSizeZeroRejected`, `Ntfs.BootNoPow2ClusterAccepted`.

**N-011c — Pass-2 record-stride quirk (512-byte-record MFTs lose half).** og pass 2 probes at 0, then +0x400 after a successful parse, +512 after a hole (`:21158/:21258/LAB_18001b0cb`) — on 512-byte-record MFT dumps, every other record is skipped. Core walks any record size uniformly (superset). Informational; do not replicate the quirk — register it so dump-pass comparisons (S-012) don't count it as core divergence.

**N-011d — Namespace ranking is a core superset.** og `FUN_1800448b0` picks one $FILE_NAME with no namespace keep/skip logic; core ranks POSIX > WIN32_AND_DOS > WIN32 > DOS (`cr_mftwalk.cpp:44-51`). Better behavior, deliberate — register informational.

**N-011e — Error-code families unmapped.** og uses `0x2980005/0x2a30006/0x2a90006/0x3a90001/0x17c0004` facility codes (`:18908-27`, `:18966`, `:17897`) plus HFS+ family `0x50LLxxxx00000nnn`. Core `cr::Err` enum carries different classes. A consumer-visible mapping table belongs in `docs/ABI-CONTRACT.md` (registration item, not an engine change).

**Related:** S-012 (the pass architecture these minors came from) · M-001 (record-size ladder — its session-1 quote was re-attributed this session; see that file's update) · I-001 (metafile filter refinement)

**Open questions:** None.
