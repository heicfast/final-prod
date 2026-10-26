# core_recovery v0.4.1-parity

**Scope of this release:** the warnings-hygiene + docs truth pass on top
of `v0.4.0-parity` (whose release notes — the parity-audit-close
headline set — remain at the `v0.4.0-parity` tag and in git history).
Zero behavior change: every engine, CLI, ABI, and test assertion in
this release is bit-identical to v0.4.0-parity; what changed is the
warning surface, the CI gate strength, and the documentation truth.

## New since v0.4.0-parity

- **The zero-warning fuzz lane** (commit `095aab5`): the clang
  libFuzzer lane's entire 48-warning backlog is zeroed and the lane
  now compiles with `-Werror` — the same zero-warning discipline the
  gcc and MSVC lanes already enforced. The backlog was fixed at its
  root, never suppressed: 44 wire-format constants (zfs/hfsplus/
  btrfs/ext4/refs) are `[[maybe_unused]]`-annotated documentation
  (zero codegen change), the refs Walker initializer is explicit at
  its existing effective values (construction byte-identical, verified
  against the struct's own defaults), the structurally-unreachable
  `> 4 GiB` RIFF-size ceiling in `validate_avi` is dropped with its
  contract recorded (a u32 can never reach it), the btrfs
  `subvol_index` dead store and `cr_fscheck`'s caller-less `rd64` are
  deleted, and the S-023 merge-storm duplicate of the
  analysis-adoption block in `cr_ui.cpp` is removed (the live twin
  executes first; the second guard could never fire).
- **OQ-S4-3's transport half is now a measured verdict** (not an
  argument): with the windows link fixes green again, the alignbench
  harness executed on the live Windows lane (run `40315255368`,
  win22, `\\.\C:`, 512 MiB span, the og's own qd=1, RDA service
  RUNNING). The core's 64 KiB direct-I/O envelope **meets/exceeds**
  the og's 4/7 MiB buffered-synchronous chunking (203.98 vs
  197.32/196.15 MiB/s — the og trails 3.3–4.0%) at comparable CPU, and
  the 256 KiB cache block **exceeds** it by 18–19% at lower CPU.
  Transcribed in the OQ file's closing addendum + M-030's Resolution
  measurement addendum + `SUMMARY.md` (Session-38 delta). Remaining
  open: only the acceptance's scan-level time-to-first-result half
  (a comparative-bench dispatch; no engineering prerequisite).
- **Driver pin**: the API-benchmark lane now fetches `rda-driver`
  **v0.2.2** (was v0.2.1) — the driver's current release with the
  `sc.exe` installer-script fix; `>= v0.2.1` remains the functional
  floor (the bare-open error-87 fix). Pinning addendum in
  `docs/DRIVER-UTILIZATION.md`.

## Documentation truth pass

- **New** `CONTRIBUTING.md` (build lanes, gate inventory, commit
  style, the zero-behavior-change discipline) and `SECURITY.md`
  (BitLocker **legitimate-unlock-only** policy with no FVEK
  extraction, the default-deny consent model, the Win7 import floor,
  the driver-side SDDL boundary).
- `docs/CONNECTION-GUIDE.md` §8: known host projects — the UI repo
  currently consumes nothing (MockConnector + the typed TS connector
  contract mirroring the `cr_ui` ABI, binding session pending); the
  rda-driver relationship (runtime `\\.\RDA` opens, bench pin).
- `diff-to-fix/SUMMARY.md`: Session-38 delta appended (the convention:
  history is never rewritten).
- Test-count truth: **926 tests** measured (`ctest --preset linux-gcc`,
  ASan, at this tree) — `docs/testing.md` already matches; the 924 in
  the v0.4.0-parity notes described the pre-wave tree and stays in
  that tag's history.

## Validation at this tag

- **926/926 tests** green on Linux (plain + **ASan**; the ubsan/tsan
  CI lanes and the Windows MSVC `/W4 /WX` lane green at the parent
  commits; this release adds documentation + warning-surface changes
  only on top).
- clang-format-18 gate clean; the win-layer parse gate
  (`win_syntax_check`) green; the fuzz lane green under its new
  `-Werror` gate; **API-Benchmark (alignbench) green** — the
  `alignbench.json` artifact and the OQ-S4-3 measurements above come
  from that run.
- **Win7 import gate green on every shipped PE** (allowlist extended
  at v0.4.0-parity for the S-014/S-010/M-033 imports and the N-062
  `K32GetProcessMemoryInfo` binding — recorded in `tools/verify_pe_um.py`).

## Known honest gaps (registered, not hidden)

NTFS XPRESS/LZX compressed $DATA (GAP-P1-1 — no corpus surface exists;
the og is LZNT1-only too), ReFS container-map translation (N-036), ZFS
multi-vdev topologies (single-vdev only), HFS+ attributes items the
corpus itself leaves UNVERIFIED, and OQ-S4-3's **scan-level
time-to-first-result half** (the transport half is now measured — see
above). Nothing silently degrades: unimplemented surfaces return
explicit errors.
