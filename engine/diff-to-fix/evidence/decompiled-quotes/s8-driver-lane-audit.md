# Evidence — the driver-lane audit (session 8, M-086/M-087/M-089/N-107)

Driver repo `323e75a` (v0.2.4, main) — read first-hand this session. Core `35adcb9`.
All quotes captured directly from the trees.

## 1. M-086 — the FileVersion pin chain

`src/driver/rda_version.h` (whole file):

```c
/* Generated version header - single source of truth for rda.rc.
 * Keep in sync with rda_ioctl.h RDA_VERSION_*. */
#define RDA_VERSION_MAJOR       1
#define RDA_VERSION_MINOR       0
#define RDA_VERSION_FILE_STRING "1.0.0.0"
```

`scripts/install.ps1:99-115` (the tag derivation):

```powershell
# Versioned: copy to "<ServiceName><major><minor>.sys" (e.g. eprdtdrv10.sys) ...
$tag = "$($vi.FileMajorPart)$($vi.FileMinorPart)"
...
$destPath = Join-Path $driverDir "$ServiceName$tag.sys"
```

`scripts/install.ps1:118-135` (the upgrade test):

```powershell
$isUpgrade = $imagePath -ne $destPath   # :130-131 (same file -> classic exit 2)
...
if (-not $isUpgrade) { ...; exit 2 }     # :132-135 (the refuse)
```

`.github/workflows/release.yml:23-24` — plain msbuild invocation, no version
stamping step (read in full). Git: `rda_version.h` unchanged across all v0.2.x
tags. Consequence chain: FileVersion always 1.0.0.0 → tag always "10" → dest
always `<ServiceName>10.sys` → `$isUpgrade = $false` between releases → exit 2.

## 2. M-087 — the regdrv-rda.reg byte decode

`scripts/winpe/regdrv-rda.reg:28-31`:

```
; Disk-class upper filter (GUID_DEVCLASS_DISKDRIVE) - the documented
; mechanism replacing EUDCPEPM's AddDevice patching (fix F6).
[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}]
"UpperFilters"=hex(7):r,d,a,\,0
```

Token bytes (od decode): `r`=0x72, `d`=0x64, `a`=0x61, `\`=0x5C, `0`=0x30 →
5 bytes `72 64 61 5C 30`. Violations vs the REG_MULTI_SZ grammar: single bytes
(not UTF-16LE units), odd count, no per-string `00 00` terminator, no final
extra `00 00`. The correct 10-byte form: `72,00,64,00,61,00,00,00,00,00`.

The og's correct form — `og-codebase/BUILDPE/x64/regdrv.reg:35-36`:

```
"UpperFilters"=hex(7):45,00,55,00,44,00,43,00,50,00,45,00,50,00,4d,00,00,00,00,00
```

(= "EUDCPEPM" UTF-16LE + string NUL + list NUL.)

## 3. M-089 — the phantom-zero probe list

`src/bench/rdabench.c:390` — the fuzz-hammer's IOCTL candidate list carries
`0x60002440 / 0x60002444 / 0x60002448` (one extra zero). The real codes —
`src/shared/inc/rda_ioctl.h:75-79`:

```c
#define IOCTL_RDA_ACQUIRE_BUFFER_CONTEXT  CTL_CODE_RDA(0x910)  /* 0x6002440 */
#define IOCTL_RDA_RELEASE_BUFFER_CONTEXT  CTL_CODE_RDA(0x911)  /* 0x6002444 */
#define IOCTL_RDA_GET_BUFFER_CONTEXT      CTL_CODE_RDA(0x912)  /* 0x6002448 */
```

dispatched at `src/driver/rda_ioctl.c:276-280`. No hammer iteration can reach
them through the phantom values (they exercise only the unknown-IOCTL path).
The driver lane's own wire-truth commits (`bd79633`, `163e3cc`) corrected this
class in prose twice — the bench source was never swept.

## 4. N-107 — the 14 stale count sites (each read; exact text in the finding)

Authoritative 19-count, verified three ways in source: 19 `#define`s
(`rda_ioctl.h:183-201`), 19 `case` labels (`rda_ioctl.c:47-66`), 19 values in
the verifier tuple (`verify_reference_binaries.py:214-232`); plus the
raw-binary dword probe: all 19 session-code dwords present in the reference
`EUDCPEPM.sys` (1-2 sites each, incl. 0x800C2AC0 and 0x800C2B3E/B40/B42).
Zero driver-repo sites state 19. The 14 stale sites are enumerated in
N-107's Core references with their exact current text.

## 5. The wire contract re-verification (context for the above)

Identity IOCTLs raw-value-exact vs the corpus
(`eprdtdrv-sys--driver-internals.md:81-83`, `epmntdrv-internals-v2…md:44,60,76`):
`0x60002400/0x60002404/0x60002408` (`rda_ioctl.h:63-65`, dispatched
`rda_ioctl.c:272-274`) — match. Information×512 (`RDA_LEGACY_INFORMATION_SCALE`,
`rda_ioctl.h:146`, applied `rda_readwrite.c:269`) vs the corpus's
"Irp->IoStatus.Information = Length << 9" (internals:68) — match. Session
LBA×probed-sector-size (`rda_session.c:1122`, `rda_disk.c:191`,
`rda_filter.c:295`) — match. The extended family is RDA-native
(non-colliding by design, `rda_ioctl.h:72-74`). NAS/Cloud exclusion holds
(case-insensitive sweep — only exclusion declarations in docs).
