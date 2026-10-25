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
