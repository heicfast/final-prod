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


## Resolution (partial — N-009a/N-009b fixed this session; N-009c/N-009d remain)

**Status:** N-009a FIXED · N-009b FIXED · N-009c/N-009d still open (below)
**Fixed in commit:** this session's quick-wins batch (cr_nvme.cpp u8 width; cr_safewriter.cpp done_bytes)
**Verified by:** `NvmeHealth.PercentageUsedIsU8Width` (reserved byte 0xFF at +6 no longer leaks into the value — fails on the pre-batch source, passes after) and `WriterDirs.ProgressExcludesErrorItems` (done reaches only the written items' bytes: 4 of 6 with one poisoned item — fails pre-batch). Both proven in the fail-before worktree run at `6b58b6f`. Suite 397/397.
**Resolution notes:** N-009a: percentage_used is a u8 per the NVMe health-log spec (values > 100 impossible); the u16 read consumed the reserved byte. N-009b: done_bytes accumulates only on non-Error outcomes — the og's ExportThread counts written bytes; the old unconditional add reported "100% done, N errors". N-009c (CLI --filter UTF-8 mangling) and N-009d (ACL preservation flag + registration) remain open in this file.

## I-005 Resolution

**Status:** ACKNOWLEDGED (all 7 grouped supersets — preserved)
**Verified by:** coverage check per the finding's own instruction ("ensure
each has a PARITY-DELTAS row; several already do"): RAR5 + PST hardening =
D-13a/b (pre-existing); ZFS ashift 10..13 = D-04 (pre-existing);
driver-absent fallback = D-37/D-24 (pre-existing); the 64 KiB direct-I/O
envelope had NO row and is registered this session as D-48c; Btrfs/ReFS/
bounds/session supersets are cross-registered (D-48a, D-48b, D-38). The audit
verified every code comment documents its divergence — re-confirmed. No code
change — future work must PRESERVE all of them.

## Resolution (session 23 — N-009c + N-009d; the group is now fully closed)

**Status:** FIXED (N-009c code fix; N-009d resolved by deliberate-divergence registration D-77)
**Fixed in commit:** (this session's N-009/M-057 batch — the commit carrying this file)
**Verified by:** `Text.WideToUtf8EncodesNonAscii` + `Text.WideToUtf8SurrogatePairAndLoneHalf` + `Text.CliFilterUtf8MatchesDecodedNames` (the CLI-seam property: a CJK `--filter` encoded by `wide_to_utf8` is a byte substring of `name_utf8()` output; the old `'?'` mangled form is pinned to never match) — fail-before by construction (the encoder did not exist); the extended `win_syntax_check` gate proves the app wiring parses (8/8 files: 7 win-layer + rrecov.cpp). Suite **740/740** linux-release at the fix commit.
**Resolution notes:** N-009c: `cr::text::wide_to_utf8` (locale-independent wide→UTF-8, lone surrogate halves → one U+FFFD per the shared text policy) added to cr_text.h; all three `--filter` sites (scan print, tree, recover) now encode once through it, and the recover path's match was aligned to the shared ASCII-case-insensitive contract (`name_contains_ci`, factored out of `name_matches` — the query layer's `name_substring` semantics that the tree command already used; the old recover-path match was case-SENSITIVE `find`, an internal inconsistency now removed). N-009d: the capability is deliberately not ported — **D-77 registered** (recovered files inherit destination DACL defaults; re-applying source ACLs can re-create unreadable files and the $SDH/$SII decode surface is a feature-sized port with no current consumer; the finding's own fix text marks the port optional and asks for the registration). The optional `preserve_dacl` flag is recorded as D-77's next step, not silently dropped. BONUS (same batch): the parse-gate extension for the app layer surfaced **M-057** (cmd_apply's undeclared `&e` — the app has been MSVC-uncompilable since `0a68937`); fixed in the same commit with its own finding file.

