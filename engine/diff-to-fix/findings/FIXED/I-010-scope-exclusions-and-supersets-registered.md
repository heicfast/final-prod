# INFORMATIONAL I-010 — Scope-exclusion confirmations and engine supersets registered by the coverage sweep

**Severity:** INFORMATIONAL (grouped) · **Modules:** cross-cutting · **Parameters:** B1, C19, Q68
**Audited commit:** `e93decf` · corpus `03ad46c` · **Status:** READ-ONLY — registrations

**Scope exclusions confirmed** (each with its excluding doc line — audit the claim, not the wish):
- Cloud recovery (Cloud/, cloud_session.dll, EuICloudRecovery.dll): `docs/CAPABILITY-MATRIX.md` §3 / PARITY-DELTAS D-26.
- NAS/remote (EuShare, EuMrx, libssh2): same lines.
- File repair chain (EUImgRepair/Office/Archive/Video/PDF/HWP): `docs/parity/PARITY-CORE-REC.md:164` "out of scope (repair chain, not carving)" — note M-021 covers the *carve-pipeline hook*, which is in scope.
- WinPE media builder, Installer, Licensing/quota, Languages/i18n, DRWReport crash reporter, runtime-families (Qt/OpenSSL/UCRT/codecs): D-26 family — no recovery semantics claimed.

**Engine supersets re-confirmed as deliberate (register in PARITY-DELTAS):**
- **ZFS label fields:** core reads name/ashift/asize/offset/guid/type from the label nvlist (`cr_zfs_engine.cpp:1014-1056`); the og matches only "vdev_tree" and "id" (`ZfsSearchDLL_decompiled.c:21089-90`, `:21114-17`) and never reads the pool name from the label. Core superset (M-019 is the divergence that matters; this is informational).
- **ext4 primary-SB-only is og parity, not an accident:** og `EXT4SearchDLL_decompiled.c:3303` reads a single superblock at base+0x400 — no backup probes. The core matches (`cr_ext4_engine.cpp`); the earlier artifact errata claiming overreach is settled (CORRECTIONS row 4 folded).
- **ext4 BIGALLOC:** og has no gate in detection; core refuses (`cr_ext4_engine.cpp:432-33`) — a registered safety choice (og behavior NOT-FOUND in this pass; keep the refusal until decoded).
- **ReFS deleted-grading:** core's medium-allocator deleted grading (`cr_refs_engine.cpp:312-350`) has NO SearchDLL counterpart (class census clean) — core-only feature, og-boundary question stays as session-1 noted.

**Coverage admission (session-1 gap, closed by session 2):** F61 (BitLocker) was never scored by session 1 and had no module file coverage for the win transport; the substantive result of closing it is S-014. F59 (driver-absent) re-verified MATCH (probe `\\.\EPRDTDRV\…` → direct `\\.\PhysicalDrive%d` fallback, `Device_decompiled.c:7793-7822` ↔ `cr_transport.h:23-37`); PID-token handshake, 200-disk cap, IOCTL table (0x60002400/04/08) all re-confirmed byte-exact.

**Action:** register rows in `docs/parity/PARITY-DELTAS.md`; no engine changes.

**Session-3 row (2026-10-26, S3-LOST agent):**
- **Lost-partition probe surface (post-S-006) is a deliberate superset.** Once
  S-006's missing probe families land, the core's probe set exceeds the og's
  (the og probes FAT/exFAT/NTFS/ext/HFS+/ReFS/Btrfs/ZFS/BitLocker-FVE + raw
  signature classes; the core adds probe families the og does not carry — see
  S-006's fix plan + S-019's BitLocker family). Register the superset in
  PARITY-DELTAS and pin it so a future "og-parity only" toggle does not remove
  finds the og cannot make. (Renumbered into this register from the parallel
  leg's stand-alone I-010.)

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** the cited doc lines re-checked this session (CAPABILITY-MATRIX scope rows, PARITY-CORE-REC:164 repair-chain exclusion, PARITY-DELTAS D-26 family).
**Resolution notes:** Scope-exclusion registrations confirmed: cloud/NAS/repair-chain/WinPE-builder/installer/licensing/i18n/crash-reporter/runtime-families stay excluded with documented rationale; the engine supersets stay registered in PARITY-DELTAS. M-021 (carve-side video-repair HOOK) remains in scope as the finding itself notes - it is a pipeline hook, not the repair chain.

**Status:** ACKNOWLEDGED (registration group — all rows verified or re-registered)
**Verified by:** spot-checks this session: ZFS label fields — core reads
name/ashift/asize/offset/guid (`cr_zfs_engine.cpp:1014-1040`) vs og
vdev_tree/id only; ext4 BIGALLOC refusal stands (`cr_ext4_engine.cpp:45,
:513`); ReFS single-record shape — current `emit_file` (post-S-020) still
emits ONE FileNode per file row with ONE extents list (no stream/ADS
children; grep "stream" = zero hits in the refs engine), matching the
og's structural no-ADS shape (I-012). Scope-exclusion rows re-confirmed
(D-26 family).
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59e**. The S-006-fix probe superset row (session-3 addition) is
already registered via D-50.
