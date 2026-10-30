# Parameters Reference — every tunable, with defaults and bounds

Runtime-facing parameters of the current tree. On-disk wire constants
and protocol fields live in [ABI-CONTRACT.md](ABI-CONTRACT.md) and
[FORMATS.md](FORMATS.md); the deliberate-divergence register for
policy-laden parameters is [PARITY-DELTAS.md](PARITY-DELTAS.md) (each
row cited below where it governs a parameter).

---

## 1. CLI parameters (`rrecov`)

### scan

| Flag | Default | Meaning |
|---|---|---|
| `--disk N` / `--volume C:` / `--image file` | — (one required) | target: PhysicalDrive N, volume letter, or a disk image (`.img`/`.vmdk`/`.vhd`/`.vhdx`/raw `.dmg`) |
| `--whole-disk` | off | lost-partition discovery over the full disk view (implies the disk target) |
| `--deep` | — | DEPRECATED alias for `--whole-disk` (N-044, rrecov.cpp:1527-1535: sets lost-partition discovery, prints a note, kept one release). NOT the deep-stage selector — the deep engine stage is always on; `--quick-only` is what disables it |
| `--quick-only` | off | quick stage only (deep + carve disabled) |
| `--no-carve` | off | disable the carve stage |
| `--threads N` | 0 = auto | worker threads; 0 = hw−1 clamped 1..8; 1 = single-threaded |
| `--out file.crsf` | — | save the durable session (atomic; also enables auto-checkpoints) |
| `--resume file.crsf` | — | resume the saved session (revalidates device identity) |
| `--autosave-dir D` | — | auto-session directory (retention-one: the pointer-named previous auto session is deleted before every write, M-154) |
| `--label s` | — | the auto-save volume-label override (M-079) |
| `--pause-stdin` | off | N-178's keyboard pause surface ('p'/SPACE pauses, 'r'/Enter resumes; no-op when stdin is redirected) |
| `--filter s` | — | name substring filter (ASCII case-insensitive) |
| `--filter-folder DIR` | — | repeatable scan-time folder filter (M-073: out-of-prefix folders never enter the session; volume-root relative) |
| `--deleted-only` | off | deleted results only (carved files count as deleted, S-031) |
| `--json` / `--csv` / `--log file` | off | machine-readable run summary / flat list / detailed log |
| `--bitlocker-fvek hex` | — | **consent**: open a locked volume with a user-supplied FVEK (in-stack XTS-AES, M-033) |
| `--bitlocker-dict [file]` | — | **consent**: try the password dictionary (the shipped 228-entry corpus-shape table; with a file: shipped table + the user's og-format dictionary, N-064/D-80) |
| `--check` | off | run the read-only damage-audit pre-scan pass (M-041; the report informs, the scan proceeds) |
| `--repair` | off | pre-scan repair: analyze + dry-run only |
| `--repair-apply` | off | **consent**: apply the repair plan (transactional; undo log saved) |
| `--repair-undo file` | — | where to write the repair undo log |
| `--user-signs file` | — | teach the carver new formats (M-048; og EUUserData JSON shape; invalid input aborts honestly) |

### tree / recover / health / fix / bench

- `tree --in S [--filter s] [--deleted-only] [--csv] [--json]` — read a
  saved session.
- `recover --in S --disk N --to DIR [--deleted-only] [--filter s]
  [--report file.json]` — write the recovered set (exit 2 = partial:
  item errors / hash mismatch).
- `health --disk N` — SMART/NVMe grading (exit 3 = every source
  unavailable).
- `fix --disk N [--part P] [--apply] [--undo file] [--undo-apply file]
  [--chkdsk C:]` — RawFixer analyze/dry-run by default; `--apply`
  **consent**; `--undo-apply` restores; `--chkdsk C:` the M-075
  escalation endpoint (launches Windows chkdsk /f on a volume; a
  volume-letter operation, no --disk).
- `apply --disk N --session file.crsf [--candidate N] [--apply]
  [--undo file] [--boot-fix lba] [--boot-fix-bcd file]` — lost-partition
  table apply (**consent**; plan is read-only with refusals listed; the
  undo surface is `--undo` here and `--undo-apply` on fix — there is no
  standalone `undo` verb).
- `bench --disk N [--mode seq|rand|open|read|info] [--mb N]
  [--block-kb N] [--iterations N] [--seed N] [--json]` —
  transport/device microbenchmarks (`--no-rda` is REMOVED — the
  driver-only architecture, PLAN-DRIVER-ONLY R10; the parser rejects it).
- `list`, `list-carve-formats`, `info` (via bench mode) — read-only.
- `sessions [--dir D] [--json]` — the saved-session inventory.
- `archive-list --in file.crsf (--disk N | --image file) --node ID
  [--json]` — the browse-inside archive inventory (M-116).
- `image --create --disk N --out file.img` — the flat-raw disk-image
  producer + `.sha256` sidecar (S-040; VHD/VHDX writers out of scope,
  D-147).
- `driver [--service NAME] [--json]` — the driver-status surface (exit
  0 = ready, 4 = prerequisite not met).

## 2. ScanConfig (the C++ / ABI surface)

`include/cr/orchestrator/cr_scan.h` — `cr_ui_scan_config_t` mirrors
these field-for-field at the ABI.

| Field | Default | Meaning |
|---|---|---|
| `run_quick` / `run_deep` / `run_carve` | true | stage enables |
| `whole_disk` | false | lost-partition discovery |
| `threads` | 0 (auto, 1..8) | worker pool size |
| `quick`/`deep`/`carve` weights | 40/35/25 | monotonic weighted progress model (percent of total per stage) |
| `pre_scan_repair` | false | repair stage before engines; requires the `consent` callback for any write |
| `pre_scan_check` | false | damage-audit stage after repair, before engines (never aborts; M-041) |
| `volume_label` / `volume_model` / `volume_serial` / `volume_image` | empty | informational identity for the session record |

## 3. Engine-side constants (load-bearing ones)

| Constant | Value | Where |
|---|---|---|
| FAT/NTFS/exFAT remnant + walk bounds | per-engine caps (e.g. NTFS remnant 4 MiB blocks; FAT chain caps) | engine headers |
| exFAT structural gates | secondary_count ∈ [2,18]; timestamp sanity; stream non-zero; name flags (M-014) | `cr_exfat_engine.h` |
| ZFS ring walk | every uberblock state, newest-first, identity dedup (S-034); label retry delay-execution (N-072) | `cr_zfs_engine.h` |
| Btrfs re-scan cache | OFF by default; caller-supplied directory (D-89) | `cr_btrfs_cache.h` |
| Carve window | 1 MiB sequential scan windows + 4 KiB overlap (M-025, D-101 stateless validator architecture) | `cr_carve.h`, `cr_carve.cpp:35-36` |
| Raw-sign table | 139 formats + user signs (caps: 256 entries / 12 signs / 48 bytes / offset ≤ 1024 / mime ≤ 80, M-048 + S-039/M-067 v3) | `cr_rawsigns.h`, `cr_usersigns.h:40-47` |
| User-sign tag space | 0xFFFE0000; fixed confidence 600; MIME on carve_note | M-048 |
| Overlap policy | accumulated base+verdict ranking; two-generation fallback REGISTERED D-75 | N-070 |
| Scan cache | 8 MiB / 256 KiB reactive LRU (the og: 4 MiB prefetch read-ahead — REGISTERED D-72, benchmark-gated) | `cr_cached.h` |
| Direct-I/O envelope | 64 KiB multiples on every direct read (Windows slow-path avoidance; D-37/D-38 family) | transport layer |
| Read chunking | 256 KiB blocks through the retry pipe | transport layer |
| Enumerate bound | 200 disks (driver grammar disk# ≤ 199, D-85 scope) | `cr_enumerate.h` |
| Checkpoint cadence | stage boundaries (+ auto-checkpoint with `--out`) | orchestrator |

## 4. Caps and budgets (honest limits)

| Surface | Cap |
|---|---|
| Dictionary loader | 4096 candidates / 32 chars per entry / 256 KiB file (N-064) |
| Preview | budgeted reads with honest `truncated` flags (M-028) |
| Carve max extent | 256 MiB per carved file (M-056 tightened the TIFF basis) |
| Session save | atomic temp-then-rename; ≤ 8 MiB persisted free-map (S-009 downsample) |
| Writer collisions | `%s_%d` ladder, 10,000 cap per name, exhaustion fails the item (N-065) |
| Query paging | server-side sort/filter/page; bucket IDs stable (M-029) |

## 5. Consent-gated operations (the safety architecture)

No write-side or unlock-side operation ever fires implicitly. The
consent surfaces, all documented deliberate safety supersets over the
reference:

1. **RawFixer apply** — explicit `--repair-apply` / consent callback
   (NULL = refuse; zero writes, not even a volume lock).
2. **Lost-partition apply** — plan (read-only) → consent callback →
   execute; single-entry preservation by default (D-98a..d).
3. **BitLocker** — legitimate unlock only: user-supplied FVEK or the
   explicit dictionary aid; locked volumes without consent stay
   `ERR_FVE_LOCKED`. No key derivation in-stack (scope).
4. **Boot-chain repair** — BCD/NVRAM writes are plan/execute/undo with
   default-deny consent (M-040).
5. **Destination protection** — apply refuses overlap with live entries;
   the writer refuses a destination inside the source volume.
