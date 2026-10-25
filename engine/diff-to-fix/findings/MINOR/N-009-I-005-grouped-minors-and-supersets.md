# MINOR N-009 — Grouped minors: health percentage_used width, writer done-bytes over-report, CLI filter ASCII mangling, ACL non-preservation
# INFORMATIONAL I-005 — Grouped deliberate supersets/hardenings (register in PARITY-DELTAS)

---

## N-009 — Grouped minors

### N-009a — NVMe percentage_used read as u16 (spec: u8)
**Module:** health · **Parameters:** B9-adjacent (field semantics), C20
**Core references:** `src/core/health/cr_nvme.cpp` (agent-D observation: the
percentage_used field is read as u16 where the NVMe Identify Controller/Health log
defines u8; the following byte belongs to the next field — values >100 impossible per
spec, so practical impact is nil, but the parse is spec-wrong).
**Fix:** one-line width change + test `Nvme_PercentageUsed_U8_Width` (pin 100% and
overflow-adjacent values).

### N-009b — Writer counts error-item bytes as done
**Module:** writer · **Parameters:** O38-adjacent (progress honesty), D13
**Core references:** `src/core/recovery/cr_safewriter.cpp:446` (agent-D observation:
`process()` adds to done_bytes even for items ending in Error — progress can over-report
on error-heavy runs; the og's ExportThread counts written bytes).
**Fix:** accumulate done_bytes only on Done/Salvaged/Resumed outcomes; add
`Writer_Progress_Excludes_Error_Items`.

### N-009c — CLI `--filter` ASCII-only widen/narrow mangles non-ASCII
**Module:** cli · **Parameters:** B10
**Core references:** `src/app/rrecov.cpp` (agent-D observation: the widen/narrow
helpers are ASCII-only; a non-ASCII `--filter` argument is mangled before matching).
The og has no CLI (GUI app) — this is a core-internal defect, not an og divergence.
**Fix:** UTF-8 decode the filter once (reuse the S-005 decoder) and compare against the
decoded name; test `Cli_Filter_NonAscii_Matches`.

### N-009d — ACLs/security descriptors not preserved on recovery
**Module:** writer · **Parameters:** F64 (SDDL/access control — same or stronger)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/pending-operation-engine.md`
(epmpublic: CAccessControlList/CAccessEntry/CFileDaclProc/CFileSecurityInfo — ACLs
preserved on copy; capa IOCTL_DISK_DELETE_DRIVE_LAYOUT).
**Core references:** no ACL handling in `cr_safewriter.cpp` / `cr_destio.cpp` (grep).
Recovered files inherit destination defaults. For a recovery product this is defensible
(recovered files are new files), but the reference preserves ACLs on its copy paths.
**Fix (optional):** add a WriterConfig flag `preserve_dacl` that applies the source
stream's security descriptor via SetSecurityInfo when the source FS exposes one
(NTFS $SDH/$SII); register the default-OFF decision in PARITY-DELTAS.

---

## I-005 — Grouped deliberate supersets/hardenings (all reviewed, all accepted; register)

1. **RAR5 carving** — og rejects RAR5; core implements the vint walk (clean-room,
   documented `cr_analysers.cpp:955–1018`). Recovery-positive superset.
2. **PST hardening** — core adds !BDN magic + wMagicClient "MS" + AMap probe beyond the
   og's version/size/floor checks (CARVE-4-aligned polarity, documented).
3. **ZFS uberblock shift-probing 10..13** — supports ashift-13 pools the og's fixed 1 KiB
   slots cannot read; BE pools refused loudly (G-18).
4. **64 KiB direct-I/O envelope (GAP-S6-13, concurrent `7777e1c`)** — the og's EPMNtDrv
   driver has a hard 512-byte sector contract (`len<<9`, decompile-verified); the core's
   Windows transport now issues 64 KiB-aligned reads with a zero-copy fast path — a
   throughput superset (~3.5× on the slow-path shapes per the fix's measurements).
5. **Driver-absent fallback** — og requires EPMNtDrv for raw access; core falls back to
   direct volume handles with an alignment shim (FB-T06/T07 documented).
6. **Btrfs deep scan + mirror probing** (I-003), **ReFS version superset** (I-004),
   **ext4/FAT/Btrfs hardening bounds** (N-005/N-006), **session torn-tail/bomb-cap
   hardening** (F10/P1-7) — previously filed.
7. **NTFS metafile filter** (I-001), **FAT12 BAD canonicalization** (I-002) — previously
   filed.

**Action:** ensure each has a PARITY-DELTAS row (several already do); the audit verified
the code comments document every one of them — no undocumented superset was found in the
engine layer.

---

**Related findings:** S-005 (the mojibake fix lands in the same decoder N-009c reuses).
**Open questions:** none.
