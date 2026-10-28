# RDA Driver — Release Process & Prebuilt Artifact Shape (DRIVER-RELEASE)

> Evidence base: rda-driver repo @ `da7efb9e0c94046893c62b25695bb95edf519f8b` (branch `main`, tag `v0.2.2`), local git metadata (43 commits, 4 tags; `git ls-remote` NOT used — no network). All `file:line` citations are relative to the rda-driver repository root. Consumer of this document: core_recovery CI, which must fetch the driver's PREBUILT releases.
>
> Repository: `https://github.com/heicfast/rda-driver` (local `git remote -v`: `origin https://github.com/heicfast/rda-driver.git`). Release owner: `heicfast`.

---

## 1. Release workflow (`.github/workflows/release.yml`) — job by job

**One job**: `build-release`, name `"Build release artifacts (WDK 26100.6584)"`, runner `windows-2022` (`release.yml:13-15`).

**Triggers** (`release.yml:3-7`):
- `on: push: tags: - 'v*'` — any tag starting with `v`.
- `workflow_dispatch` (manual).
- Permissions: `contents: write` (`release.yml:9-10`).

**Steps** (in order):

1. `actions/checkout@v4` (`release.yml:17`).
2. `nuget/setup-nuget@v2` + `nuget restore .\packages.config -PackagesDirectory .\packages\` — pinned WDK/SDK `10.0.26100.6584` (`release.yml:19-20`; pins at `packages.config:6-8`).
3. `microsoft/setup-msbuild@v2` (`release.yml:21`).
4. **Build from source**: `msbuild .\rda.sln -m -v:minimal -nologo -p:Configuration=Release -p:Platform=x64` (`release.yml:23-24`). Produces `bin\x64\Release\rda.sys` + `rdacli.exe` (+ `rdabench.exe`) — `docs/build.md:22-26`.
5. **Win7 loader stamp**: `python tools\stamp_osversion.py bin\x64\Release\rda.sys` — stamps PE OS 6.1 / NATIVE 6.01 + recomputes the checksum (`release.yml:26-28`; tool contract `tools/stamp_osversion.py:7-17`).
6. **PE gate**: `python tools\verify_pe.py bin\x64\Release\rda.sys` — AMD64 + NATIVE + OS 6.1 + import allowlist (reference-union 75 functions + audited additions); any stray import fails the release (`release.yml:30-32`; `tools/verify_pe.py:7-20,34-104`).
7. **Assert unsigned**: parses the PE certificate-table directory RVA and fails if nonzero ("unexpected signature") — the release artifact must be unsigned (`release.yml:34-41`).
8. **Stage release bundle** into `dist\` (`release.yml:43-57`), exactly:
   ```
   dist\rda.sys
   dist\rdacli.exe
   dist\rda.inf
   dist\scripts\install.ps1
   dist\scripts\uninstall.ps1
   dist\scripts\run-integration.ps1
   dist\LICENSE
   dist\README.md
   dist\docs\signing.md
   dist\docs\testing.md
   dist\docs\win7-compat.md
   ```
   (No `.pdb`, no `.cat`, no zip at this stage.)
9. **Checksums**: `Get-FileHash -Algorithm SHA256` over every file in `dist` **recursively**, written to `dist\SHA256SUMS` in `<UPPERCASE-SHA256>␣␣<dist-relative-path>` lines (e.g. `…  rda.sys`, `…  scripts/install.ps1`, `…  docs/signing.md`) — `release.yml:59-67`. Note: SHA256SUMS is generated *before it exists*, so it does **not** hash itself; it covers all 11 staged files (7 of which are attached to the Release — see below).
10. **Upload workflow-run artifact** `rda-release-${{ github.ref_name }}` = the whole `dist` tree (`release.yml:69-74`).
11. **Attach to GitHub Release** — only when the ref is a tag (`if: startsWith(github.ref, 'refs/tags/v')`, `release.yml:76-77`) — via `softprops/action-gh-release@v2` with `generate_release_notes: true` and the file set (`release.yml:78-86`):
   ```
   dist/rda.sys
   dist/rdacli.exe
   dist/rda.inf
   dist/SHA256SUMS
   dist/scripts/*
   ```
   The release body (`release.yml:87-95`) states verbatim: *"RDA Raw Disk Access Driver - ${{ github.ref_name }}"* / *"Unsigned build. Windows kernel-mode code signing applies before production deployment - see docs/signing.md (the OSSign pipeline is prepared but DISABLED until certificate onboarding completes)."* / *"Win7 SP1 -> Win11 x64, single binary. Verified by the CI PE gate (import allowlist + OS 6.1 header contract)."*

**Asset names on the GitHub Release — EXACT.** There is **no zip** (nothing like `rda-driver-vX.Y.Z.zip` exists anywhere in the workflow). The Release carries 7 individual files. Empirically (core_recovery's working fetcher, `.github/workflows/api-benchmark.yml` "Fetch RDA driver release" step, which downloads every `$rel.assets` and then runs `driver\install.ps1` flat), the attached asset **names are flat basenames**:
`rda.sys`, `rdacli.exe`, `rda.inf`, `SHA256SUMS`, `install.ps1`, `uninstall.ps1`, `run-integration.ps1`.

**Checksum caveat for consumers**: the `SHA256SUMS` lines use *dist-relative* paths (`rda.sys`, `rdacli.exe`, `rda.inf`, `scripts/install.ps1`, `scripts/uninstall.ps1`, `scripts/run-integration.ps1`, `LICENSE`, `README.md`, `docs/signing.md`, `docs/testing.md`, `docs/win7-compat.md`), while only 7 files are attached (LICENSE/README/docs are not attached). So 6 of the 11 lines are verifiable against downloaded assets, and the three script lines carry a `scripts/` prefix that does not match the flattened asset names — a fetcher must map `scripts/install.ps1` → downloaded `install.ps1` (or skip with an explicit allow-list of unattached files, which is what core's current fetcher does). A deterministic verifier should:
1. download all assets,
2. parse SHA256SUMS, keep lines whose second field is one of `rda.sys`, `rdacli.exe`, `rda.inf`, `scripts/install.ps1`, `scripts/uninstall.ps1`, `scripts/run-integration.ps1`,
3. strip any leading `scripts/` from the expected name,
4. compare SHA-256.

**Signing steps — status: NONE in the release workflow.** The release deliberately ships **unsigned** (assert step 7; `SignMode=Off` at `src/driver/rda.vcxproj:48-58`). The OSSign pipeline exists but is **DISABLED**:
- `signing/ossign/sign.yml` — "Sign release": server-dispatched (`workflow_dispatch` with `ref`/`sha`/`release_name`/`dispatch_id`), checks out the **pinned SHA**, rebuilds with the same WDK NuGet + `verify_pe.py` gate, signs `rda.sys` and `rdacli.exe` through `ossign/ossign@1.3.1` (fileType `pecoff`, tokenType `ossignServer`), verifies with `ossign verify`, fetches the review report, publishes a release — but "Disabled until OSSign onboarding: requires repo variables SOURCE_REPO / OSSIGN_SERVER / OSSIGN_KEY and the protected environment 'signing' to exist" (`signing/ossign/sign.yml:9-11,41-127`).
- `signing/ossign/review.yml` — "Review": scanner workflow (gitleaks 8.21.2 + osv-scanner 1.9.1, pinned) uploading reports to the OSSign server via OIDC (`signing/ossign/review.yml:14-79`). Hosted in the OSSign **mirror** repo once onboarding is approved (`signing/ossign/review.yml:9-12`).
- Scope honesty (`docs/signing.md:62-75`): an OSSign signature is standard Authenticode, **not** a Microsoft kernel-mode signature — on Win10/11 retail the driver still needs test-signing or Hardware Dev Center attestation; on Win7/8/8.1 an OSSign Authenticode chain loads like the reference cross-signed files; WinPE is relaxed. Test-signed posture: see §7.

---

## 2. Tag convention

- Trigger filter: `'v*'` (`release.yml:6`) — any tag whose name starts with `v`.
- In practice the project uses **semver `vX.Y.Z`** — local tags (`git tag -l`, no network): **`v0.1.0`, `v0.2.0`, `v0.2.1`, `v0.2.2`**.
- Tag → commit map (from local `git show -s` / `git log --decorate`):
  - `v0.1.0` → `1b69dafb2e43cb3d0438e6817a79a5c096556ec1` (2026-10-22 11:09:43 +0000) — "first green build".
  - `v0.2.0` → `2b1db5f2d50322635f80934f413aed978543d357` (2026-10-22 15:56:32 +0000) — performance parity with reference drivers; tag message: **"This tag provides the stable release artifact consumed by core_recovery CI."**
  - `v0.2.1` → `7e88e38c585604c077517027a4cce30863369c6c` (2026-10-23 07:59:27 +0000) — classifier error-87 fix (bare `\\.\RDA` opens previously failed `STATUS_INVALID_PARAMETER`, forcing the user-mode stack onto the direct path).
  - `v0.2.2` → `da7efb9e0c94046893c62b25695bb95edf519f8b` (2026-10-26 08:53:40 +0000) — installer-script repair (`sc.exe.exe` → `sc.exe`, 4 sites) + docs truth pass; tag message: "No binary change from v0.2.1."
  - `v0.2.3` → `6b50bb8` (2026-10-28) — the deployer parity batch: elevation guard fixed (S-036), fatal start-failure (M-062), verified teardown (M-061), Parameters write+verify (M-064 script half); no binary change.
- Release name on GitHub: the tag name (`softprops/action-gh-release` default), body as in §1.

---

## 3. Deterministically fetching the LATEST prebuilt release

The driver repo has exactly one release-producing workflow (§1), semver `v*` tags, and flat asset names. Recommended deterministic procedure (matches the already-working pattern in core's `api-benchmark.yml` / `comparative-bench.yml`):

**Pinned (recommended for CI):** query the release by tag:
```
GET https://api.github.com/repos/heicfast/rda-driver/releases/tags/<TAG>
```
then download every asset via each `assets[].browser_download_url` (or `https://github.com/heicfast/rda-driver/releases/download/<TAG>/<ASSET-NAME>`).

**Latest (rolling):**
```
GET https://api.github.com/repos/heicfast/rda-driver/releases/latest
```
`tag_name` → newest non-prerelease `v*` tag; `assets[]` → the 7 flat-name assets. (Caveat: "latest" excludes releases marked prerelease/draft; the driver workflow never sets `prerelease`/`draft`, so every `v*` tag lands as a full release — `release.yml:78-86` passes neither flag.)

**Asset selection** — match by exact name from: `rda.sys`, `rdacli.exe`, `rda.inf`, `SHA256SUMS`, `install.ps1`, `uninstall.ps1`, `run-integration.ps1`. There is no version-suffixed asset name and no zip; the tag name is the only version key.

**Checksum verification** — SHA-256, procedure in §1 (note the `scripts/` prefix mapping). Generation side: `Get-FileHash -Algorithm SHA256` (`release.yml:62-66`). The repo's own verification tooling patterns: the release workflow's PE-level assertions (`release.yml:34-41`) and `tools/verify_pe.py` (import allowlist + header contract) can additionally be run on the fetched `rda.sys` for a stronger gate — `scripts/verify/verify_reference_binaries.py` is a Capstone/pefile disassembly verifier for the *reference* binaries, not a release-asset verifier (`scripts/verify/verify_reference_binaries.py:2-34`), but it demonstrates the project's SHA-256-logging discipline (`benchmark.yml` records reference-binary hashes the same way — `.github/workflows/benchmark.yml:65-69`).

**Current consumer pins in core_recovery** (for cross-reference; read from core repo):
- `api-benchmark.yml`: `RDA_RELEASE_TAG: v0.2.2` with the comment "v0.2.2 = the driver's current release (the sc.exe installer fix). Functional floor is >= v0.2.1 (the error-87 fix); pinned to the newest." (`.github/workflows/api-benchmark.yml` env block).
- `comparative-bench.yml`: `RDA_RELEASE_TAG: 'v0.2.1'` (older lane).

**Functional floor for consumers: ≥ v0.2.1** (the bare-open error-87 fix, tag `v0.2.1` message; regression locked by `tests/unit/test_compat.c:147-165`). v0.2.2 adds only script/doc fixes on top of the identical binary (tag message: "No binary change from v0.2.1").

---

## 4. Install / uninstall scripts

### `scripts/install.ps1` (also shipped as release asset `install.ps1`)

Parameters: `-ServiceName` (mandatory, `^[A-Za-z0-9_]+$`), `-Source` (mandatory, path to a `rda.sys`), `-IoctlBase` (default 0), `-MaxDisks` (default 200), `-DebugLevel` (default 1), `-NoStart` switch (`install.ps1:19-33`).

Sequence:
1. **Elevation guard** — refuses to run non-admin (`install.ps1:37-41`).
2. **Preflight** — fails if the service name already exists (`install.ps1:43-47`); copies the source binary to `%SystemRoot%\System32\drivers\<ServiceName>.sys` (`install.ps1:49-54`). Note: the service name, not `rda.sys`, names the on-disk file.
3. **Service creation**: `sc.exe create <ServiceName> binPath= "<full drivers path>" type= kernel start= demand error= normal group= "Boot File System"` (`install.ps1:57`) — i.e. `SERVICE_KERNEL_DRIVER`, `SERVICE_DEMAND_START`, `SERVICE_ERROR_NORMAL`.
4. **Parameters written** under `HKLM:\SYSTEM\CurrentControlSet\Services\<name>\Parameters`: `IoctlBase`, `MaxDisks`, `DebugLevel`, `SessionsEnabled=1` (`install.ps1:60-65`).
5. **Start**: `sc.exe start <ServiceName>` unless `-NoStart` (`install.ps1:67-76`). **Reported states**: success → `"Service '<name>' started."`; failure → `Write-Warning "StartService failed ($LASTEXITCODE). Check code-signing policy (testsigning), driver signature, and the System event log."` — the script continues (non-terminating warning) (`install.ps1:69-75`).
6. **Post-install guidance printed** (this is the driver-status surface the installer reports): `Expected control device: \Device\<SERVICE-NAME-UPPER>`, `User-mode path: \\.\<NAME>\<pid-token>\<disk>\<part>`, and a pointer to `rdacli.exe grammar 0` (`install.ps1:78-84`).

Canonical usage from the README: `.\scripts\install.ps1 -ServiceName eprdtdrv -Source .\bin\x64\Release\rda.sys` (installs → `\Device\EPRDTDRV`) (`README.md:76-85`). The driver then derives its device identity from the service name at load time (`src/driver/rda_control.c:4-10`).

### `scripts/uninstall.ps1`

Elevation guard (`uninstall.ps1:15-17`); stops the service if running (`sc.exe stop` + 1 s wait, `uninstall.ps1:22-25`); `sc.exe delete` (`:26`); removes `System32\drivers\<ServiceName>.sys` (`:32-36`). Warns if the service was absent.

### Test-signing requirement

The shipped binary is unsigned (§1), so loading it requires one of:
- **Test signing** on the target OS: `bcdedit /set testsigning on` + reboot (`README.md:83-85`, `docs/testing.md:32-34`); or
- a provisioned test certificate for the driver (`install.ps1:11-12`); or
- signing it yourself (Authenticode) — which covers Win7/8/8.1-class chains and WinPE, but NOT Win10/11 retail kernel loading (Microsoft attestation/HLK required — `docs/signing.md:5-9,62-75`).

Hosted CI runners cannot reboot into test-signing; the established CI pattern (driver `benchmark.yml` and core's `api-benchmark.yml`) is a **job-local self-signed code-signing certificate** imported into `LocalMachine\Root` + `TrustedPublisher`, then `signtool sign /fd SHA256` the fetched `rda.sys` for the session — valid on hosted runners because SecureBoot is off there (`.github/workflows/benchmark.yml:71-90` with the rationale at `:12-15`; `docs/benchmarking.md:19-22`).

### WinPE deployment profile (no installer)

`scripts/winpe/regdrv-rda.reg` registers service `rda` (Type=1 kernel, Start=3 demand, Group="Boot File System", ImagePath `\SystemRoot\System32\drivers\rda.sys`) with `Parameters`: `IoctlBase=0x10000`, `MaxDisks=0xC8`, `DebugLevel=1`, `SessionsEnabled=1`, `FilterMode=1`, `VolSnapAttach=0`, plus the disk-drive class `UpperFilters` REG_MULTI_SZ `rda` on `{4d36e967-e325-11ce-bfc1-08002be10318}` — `scripts/winpe/regdrv-rda.reg:13-31`. This activates the EUDCPEPM-style copy-filter role; in WinPE signature enforcement is relaxed (`docs/signing.md:72-73`).

---

## 5. The driver's own CI (`.github/workflows/ci.yml`) — what a consumer can rely on

Triggers: push/PR to `main` + manual; concurrency-cancel per ref; `contents: read` (`ci.yml:4-15`).

| Job | Runner | What it does |
|---|---|---|
| `lint` | ubuntu-latest | clang-format check over all `src/**/*.{c,h}`; markdown trailing-newline sanity; portable unit tests via `scripts/run-unit-tests.sh` (gcc C11 `-Wall -Wextra -Werror`) — `ci.yml:18-46` |
| `fuzz` | ubuntu-latest | libFuzzer round (60 s, ASan/UBSan) over the exact CREATE-path logic; invariant selftest (4019 adversarial inputs); unit tests under ASan/UBSan — `ci.yml:48-72` |
| `unit` | windows-2022 | MSVC `/W4 /WX` compile + run of `tests\unit\test_compat.c` (the wire-table freeze) — `ci.yml:74-101` |
| `build` | windows-2022, matrix Debug+Release, WDK `26100.6584` | NuGet-pinned WDK restore → `msbuild rda.sln` → `stamp_osversion.py` → `verify_pe.py` gate → assert-unsigned → upload artifacts `rda-wdk26100-<Configuration>` (`rda.sys`, `rdacli.exe`, `rda.inf`) — `ci.yml:103-172` |
| `codeql` | windows-2022 | CodeQL C analysis on the Release build (same pinned toolchain; windows-2022 required because the WDK NuGet build-task DLL matches MSBuild 17) — `ci.yml:174-195` |

**Reliability takeaway for consumers**: every pushed `main` commit is compiled by the exact toolchain the release uses (VS2022 v143 + WDK NuGet 26100.6584, `ci.yml:110-117` vs `release.yml:14-24`), machine-gated for Win7-loadability (AMD64/NATIVE/OS 6.1 + import allowlist — `tools/verify_pe.py:106-110`), asserted unsigned, and contract-frozen by the unit/fuzz layer. The release workflow adds only staging/checksumming/attaching. Note CI **never loads** the driver (hosted runners can't test-sign without reboot — `docs/signing.md:24-27`); runtime validation happens in the benchmark workflow and the documented VM matrix (`docs/testing.md:55-66`).

## 6. Benchmark workflow (`.github/workflows/benchmark.yml`) — context

Triggers: `workflow_dispatch` + push to `main` touching `src/**` or the workflow itself (`benchmark.yml:17-24`). One job, `windows-2022`, 75-minute timeout (`:29-31`). Pipeline (`:37-259`, narrative in `docs/benchmarking.md:9-35`):

1. Release build + Win7 stamp + PE gate (identical discipline to CI).
2. Fetch the three ORIGINAL reference binaries at run time from the research corpus `https://raw.githubusercontent.com/heicfast/easeus-drw-research/main` (never committed; SHA-256 logged) (`:57-69`).
3. **Test-sign** our `rda.sys` with a job-local self-signed cert (Root + TrustedPublisher import; SecureBoot-off runners → no testsigning reboot) (`:71-90`).
4. Create + seed a 4 GiB fixed GPT VHDX (NTFS + FAT32 volumes; realistic JPEG/PDF/MP4/ZIP/text/1000-small-file corpus) (`:92-158`).
5. Install BOTH drivers side by side under distinct service names — `sc.exe create eprdtdrv/rdatest type= kernel start= demand binPath= "System32\drivers\…" group= "FSFilter Boot File System"` (`:160-180`).
6. Run the `rdabench` matrix per driver (info / open / seq / rand / verify / fuzz; JSONL output) (`:182-201`).
7. Build a markdown comparison + **assert cross-driver SHA-256 byte-equality** (any mismatch fails the job) (`:203-235`).
8. Always: stop/delete both services, detach + delete the VHD, remove certs (`:237-250`); upload `benchmark-results` (30-day retention) (`:252-259`).

Latest results (`docs/benchmarks/2026-10-22-runner-comparison.md:14-25`): sequential throughput at parity (532.9 vs 533.1 MiB/s), random IO −2.9%, open mean +2.2%, hostile-dispatch +52%, **64/64 sampled 1 MiB ranges byte-identical** across drivers. This is the strongest "the prebuilt driver behaves like the reference" evidence available to a consumer.

---

## 7. Test-signing posture (summary)

- **Build never signs**: `SignMode=Off` (`src/driver/rda.vcxproj:48-58`); CI and release both *assert* the artifact carries no signature (`ci.yml:151-162`, `release.yml:34-41`); reasoning at `docs/signing.md:29-37` (reproducible byte-comparable artifacts; signing authority belongs to the release pipeline).
- **GitHub Releases carry UNSIGNED `rda.sys`** (release body text, `release.yml:90-93`). A consumer must test-sign it (job-local cert + `signtool`, §4) or enable OS test-signing before `sc.exe start` will succeed on Win10/11.
- **OSSign signing pipeline prepared but disabled** pending onboarding (repo vars `SOURCE_REPO`/`OSSIGN_SERVER`/`OSSIGN_KEY` + protected `signing` environment) — `signing/ossign/sign.yml:9-11`, `docs/signing.md:39-60`. It would sign `rda.sys` + `rdacli.exe` (pecoff) after a pinned-SHA rebuild and scanner review.
- **OSSign ≠ Microsoft kernel signature** — Win10/11 retail loading still requires Hardware Dev Center attestation/HLK (EV cert); Win7/8/8.1 accept the Authenticode chain; WinPE is relaxed (`docs/signing.md:62-75`).
- In-tree example of the CI test-signing recipe to copy: `benchmark.yml:71-90` (New-SelfSignedCertificate CodeSigningCert → export .cer/.pfx → import Root + TrustedPublisher → signtool sign /fd SHA256 → signtool verify /pa).
- **Timestamps (N-084)**: the copyable recipe intentionally omits `/tr` — the job-local cert outlives the job and nothing needs the signature later. For ANY certificate whose signature must remain verifiable after the cert expires (a real release cert, an OSSign chain), add `/tr <TSA> /td SHA256` to the `signtool sign` line; the install-driver composite accepts `timestamp_server:` for the same purpose.

---

## 8. Compatibility mapping (core release tier → driver release tier)

There is no formal ABI-tier registry; compatibility is expressed through these concrete, checkable surfaces (all details with citations in `docs/DRIVER-CONTRACT.md` §6):

| Contract tier | Driver-side surface | Consumer-side check |
|---|---|---|
| Identity/grammar tier (EPMNtDrv + EUEDKEPM) | Open grammar + PID token; identity IOCTLs `0x60002400/04/08`; byte-length READ/WRITE with ×512 Information; IOCTL pass-through | Open `\\.\RDA`, `IOCTL_RDA_GET_VERSION` == `(1<<32)\|0`; grammar round-trip via `rdacli grammar` |
| Extended tier (RDA-native) | `IOCTL_RDA_GET_INFO` (`0x6002440`) reporting Major/Minor + `Flags` (FILTER_MODE/SESSIONS) + `MaxDisks` + `IoctlBase`; `IOCTL_RDA_OPEN_TARGET` (`0x6002444`) | `GET_INFO`, compare `StructSize`/version/flags |
| Session tier (EUDCPEPM) | 19 IoctlBase-normalized codes; session `GET_VERSION` dword `0x00010100`; `SessionsEnabled` parameter (default on) | Issue session `GET_VERSION`, expect `0x00010100` + product string |
| File/INF identity | PE version resource `1.0.0.0`; INF `DriverVer 01/01/2026,1.0.0.0` | (informational only) |

**Version history → tier support** (from `git log`, tag messages, and the repo worklog):

| Tag | Date | Contract-relevant changes |
|---|---|---|
| `v0.1.0` (`1b69daf`) | 2026-10-22 | First green build: full merged wire contracts, Win7 import gate, `/W4 /WX`, CodeQL, portable contract tests (tag message; worklog Session 1). |
| `v0.2.0` (`2b1db5f`) | 2026-10-22 | Perf parity with reference binaries; full session copy-engine (OPEN_SESSION/StartBackup/GETNEXTBLOCK pump, 18-code set incl. discovery + RRWR); **extended IOCTLs GET_INFO / OPEN_TARGET / GET_VERSION added**; SLIST pool; geometry cache. Tag message: "This tag provides the stable release artifact consumed by core_recovery CI." |
| `v0.2.1` (`7e88e38`) | 2026-10-23 | **Functional floor for consumers**: bare `\\.\RDA` (empty-name) opens previously returned error 87 (`STATUS_INVALID_PARAMETER`) — fixed to GLOBAL classification; without it every user-mode stack silently fell back to the direct path (tag message; regression test `tests/unit/test_compat.c:147-165`; evidence note referencing core_recovery session 6 at `src/shared/inc/rda_compat.h:183-192`). |
| `v0.2.2` (`da7efb9`) | 2026-10-26 | Current HEAD/main. Installer repair (`sc.exe.exe` → `sc.exe` in install/uninstall, 4 sites) + docs truth pass. "No binary change from v0.2.1" (tag message). |
| `v0.2.3` (`6b50bb8`) | 2026-10-28 | **Current HEAD/main; the deployer parity batch (scripts only, no binary change)**: the elevation guard's `InvokeMethodOnNull` chain fixed in both scripts (S-036 — the pre-v0.2.3 installer could never complete on a stock host); install start-failure now FATAL (exit 6 + testsigning guidance, M-062); uninstall polls STOP_PENDING + checks every SCM call + verifies the registration is gone + loud locked-image failure (M-061); the Parameters write+verify read-back (M-064 script half); full exit-code contract (install 0-7, uninstall 0-6). |

**Recommended mapping for core releases**: pin the driver by exact tag (current: `v0.2.3`; functional floor: `v0.2.3` — the CI composite's FloorVersion; `v0.2.1` remains the WIRE-functional floor), verify via SHA256SUMS, probe capabilities at runtime with `GET_INFO`/`GET_VERSION`, and treat any future Major bump of `IOCTL_RDA_GET_VERSION` (currently 1.0) or a `GET_INFO`-`StructSize` change as a contract boundary requiring re-verification (the driver enforces nothing itself — `docs/DRIVER-CONTRACT.md` §6).

**Full commit history** (43 commits, oldest → newest, `git log --oneline | tac`): `ae931da` bootstrap → `c65831e` wire-contract headers + portable grammar + unit tests → `261414a` full kernel driver + client + PE gate + CI → `82e96df`/`5ba6531`/`4cc4d42`/`c7db335`/`538d8ce`/`b8ea2ae`/`7ccb07f`/`707ef57` CI+build bring-up fixes → `726b0dc`/`fb17602`/`572d16d`/`9472385` allowlist + codeql + worklog → `1b69daf` **v0.1.0** → `14d97d6` full decompile re-verification (28 contract gaps + 12 defects) → `6597226`/`59323bd`/`d3c8fb6`/`b5f26e3`/`b7076b4`/`b72f789`/`e662957`/`c565a4f`/`bcd17d4`/`1382b7b` session-2 corrections + bench/fuzz infra → `26d84e2`…`711005a` benchmark pipeline bring-up + optimizations → `95e09d9`/`1aa40dd`/`3c2377e`/`1eda98f` SLIST export fix → `2b1db5f` **v0.2.0** → `7e88e38` **v0.2.1** → `da7efb9` **v0.2.2**.

**Local tags reported**: `v0.1.0`, `v0.2.0`, `v0.2.1`, `v0.2.2`, `v0.2.3` — 5 tags, all annotated, all on `main`'s first-parent line (`v0.2.3` at `6b50bb8`, scripts/docs only; the driver binary is unchanged from v0.2.1).
