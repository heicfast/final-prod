# MAJOR M-087 — regdrv-rda.reg UpperFilters is an invalid hex(7) value: the WinPE disk-class filter never attaches

**Severity:** MAJOR
**Module / Surface:** Driver pre-registration (WinPE) — the K78 boot-media deployment parameter family
**Parameters affected:** K78 (boot-media deployment: offline-hive service matrix + UpperFilters), Q68-adjacent (the shipped script's correctness)
**Corpus references:** `og-codebase/BUILDPE/x64/regdrv.reg:35-36` — the og's own correct encoding: `"UpperFilters"=hex(7):45,00,55,00,44,00,43,00,50,00,45,00,50,00,4d,00,00,00,00,00` ("EUDCPEPM" UTF-16LE + string NUL + final NUL); `architecture/03-winpe-boot-environment.md:54` (the class-guid context — note the corpus labels {71A27CDD-…} DISKDRIVE; the actual DiskDrive class is {4d36e967-e325-11ce-bfc1-08002be10318}); `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` (the regdrv matrix: 4 services + disk-class UpperFilters, eprdtdrv absent from PE)
**Core references:** audited driver `323e75a`: `scripts/winpe/regdrv-rda.reg:30-31` — `[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}]` / `"UpperFilters"=hex(7):r,d,a,\,0`
**Status:** PUSHED-FORWARD (driver-repo-resident; this session's two-repo grant excludes heicfast/rda-driver)
**Prior audit check:** "expanded on prior finding" the D4 handoff — recorded in the core trail's SCAN-ENGINE-DELTA driver-lane handoffs ("D4: regdrv-rda.reg hex(7) UpperFilters still invalid") without a finding file; this filing gives it an owner.

## Summary

The driver repo's WinPE pre-registration file writes the DiskDrive class `UpperFilters` value as `hex(7):r,d,a,\,0` — five literal ASCII bytes (`72 64 61 5C 30` = "rda\0" with the backslash-zero written as characters, not a NUL). A REG_MULTI_SZ must be UTF-16LE, each string NUL-terminated, with a final extra NUL: the correct encoding for "rda" is `hex(7):72,00,64,00,61,00,00,00,00,00`. As shipped, `reg import` yields a corrupt multi-string (UTF-16 units 0x6472, 0x305C and a dangling byte); the disk-class UpperFilters would never name the `rda` service, so the WinPE filter role silently never attaches. The og's own `regdrv.reg` demonstrates the correct form for EUDCPEPM.

## What the reference does

`og-codebase/BUILDPE/x64/regdrv.reg:35-36` writes the UpperFilters multi-string in the valid Windows .reg encoding: comma-separated bytes, UTF-16LE code units for "EUDCPEPM" (45 00 55 00 … 4d 00), the string-terminating NUL (00 00), and the list-terminating extra NUL (00 00). The og's PE image thus registers the hook driver as a disk-class upper filter at offline-hive merge time — the documented K78 mechanism (`boot-config--winpeshl-autobat-regdrv.md`).

## What our implementation does

`scripts/winpe/regdrv-rda.reg:31` (audited `323e75a`, byte-level decode first-hand this session with `od -t x1z`):

```
"UpperFilters"=hex(7):r,d,a,\,0
```

Tokens: `r`=0x72, `d`=0x64, `a`=0x61, `\`=0x5C, `0`=0x30 → 5 bytes, odd count, no UTF-16LE structure, no per-string NUL, no final NUL. Four independent validity violations. The `\,0` construct appears to be an attempt to write "rda\0" inside a .reg hex body where `\` is not an escape and `,` separates single bytes.

## Gap

Falsifiable: decode `scripts/winpe/regdrv-rda.reg:31` per the .reg hex(7) grammar — the value is 5 bytes `72 64 61 5C 30`, which is not a valid REG_MULTI_SZ encoding of any string list containing "rda"; the og's `regdrv.reg:35-36` shows the required 10-byte form for an analogous value. An offline merge of the shipped file cannot produce a working UpperFilters entry.

## Impact

The WinPE boot-media deployment path (K78 — the roadmap's boot-media builder design input, and the driver repo's own winpe script set): when the PE media is built with this file, the `rda` driver installs as a service but never attaches as the disk-class upper filter, so raw-disk access in PE depends on the service being opened directly rather than through the filter seat the og's EUDCPEPM occupied. The defect is latent (the merged product's PE builder is a roadmap item) but the script is shipped in the driver repo today and is the exact artifact future sessions will copy.

## Reproduction / verification

Byte-level decode this session (`od -A d -t x1z` over the file; the token bytes quoted above) + the .reg grammar (hex(7) = REG_MULTI_SZ, UTF-16LE, double-NUL list terminator) + the og's own correct form at `BUILDPE/x64/regdrv.reg:35-36` as the corpus contrast. Execution of `reg import` is Windows-bound (this container is Linux) — the encoding violation is provable statically; UNVERIFIED-BY-EXECUTION is marked only for the empirical import transcript.

## Fix plan (for the engineering team)

1. One-line fix in `scripts/winpe/regdrv-rda.reg:31`:
   `"UpperFilters"=hex(7):72,00,64,00,61,00,00,00,00,00`
2. While touching the file, state the deliberate class-GUID divergence in the comment block: the og's regdrv.reg targets the Volume class GUID {71A27CDD-812A-11D0-BEC7-08002BE2092F} (which corpus `architecture/03:54` mislabels DISKDRIVE); rda targets the actual DiskDrive class {4d36e967-…} (the fix-F6 divergence — deliberate, but currently implicit).
3. Add a parse gate to the driver CI (the existing PowerShell AST gate's pattern): a small script that hex-decodes every `hex(7):` value in `scripts/winpe/*.reg` and asserts even byte count, UTF-16LE NUL terminators, and a final empty string — fails on any invalid multi-string. (The v0.2.3 CI lesson — the PS parse gate — extended to the .reg surface.)
4. Tests: a CI step `reg-value-grammar` green; a static assertion that the UpperFilters string list contains exactly "rda".
5. Acceptance: the decoded value is byte-identical to the og's structure (UTF-16LE "rda" + terminator + list NUL); the gate would have failed on the pre-fix file (fail-before: run the gate on the old line).

## Tests to add

- `RegHex7GrammarGate` (the CI decode gate — fails on odd counts / missing terminators)
- `WinpeUpperFiltersNamesRda` (decoded content assertion)

## Related findings

- **N-107** (the driver-repo doc-drift cluster — same lane, same repo)
- **M-086** (the FileVersion pin — the other deferred-execution driver defect)
- **K78 / S-038** (the packaging family whose boot-media input this file feeds)

## Open questions

None. The encoding violation is statically decidable; the corpus shows the correct form.


## SA-9 addendum (2026-10-28 — a parameter this finding did not carry: the Start posture)

The SA-9 lane's read of the og's `regdrv.reg` found a third divergence
beyond the hex(7) encoding and the class GUID: **the Start split**. The
og's PE profile registers its filter service **EUDCPEPM as Start=0
(BOOT_START)** — precisely so the filter attaches when PnP builds the
volume stack — while the data-path raw drivers (epmdkdrv, ebrntdrv) are
Start=3 (demand) and eprdtdrv is absent. Our profile registers the single
`rda` service as Start=3 (correct for the data-path role) while
simultaneously activating the UpperFilters copy-filter role
(Parameters FilterMode=1 + the UpperFilters value) — a demand-start class
filter is not loaded at stack-build time, so **the filter role the file's
own comment promises ("activate the EUDCPEPM-style copy-filter role on
every disk device in the PE") cannot engage at boot** even once the
hex(7) encoding is fixed.

Fix-plan addition: split the postures the way the og does — either
(a) register Start=0 for the PE deployment (matching EUDCPEPM's PE
posture; the og's own file is (a)), or (b) keep Start=3 and drop the
UpperFilters write (the data-path-only PE posture — a documented scope
choice). Decide with the boot-media lane; the execution proof (a PE boot
transcript showing the filter attached) is the boot-media lane's first
dispatch.


## Resolution

**Status:** PUSHED-FORWARD (tracked-open OQ-S10-2)
**Verified by:** bf17e55-lineage tree: the byte-level decode re-verified against the finding's grammar (the shipped scripts/winpe/regdrv-rda.reg is a DRIVER-repo path - heicfast/rda-driver scripts/winpe/regdrv-rda.reg:31; the corpus contrast og-codebase/BUILDPE/x64/regdrv.reg:35-36 carries the correct 10-byte UTF-16LE form).
**Resolution notes:** The one-line fix (hex(7):72,00,64,00,61,00,00,00,00,00) + the .reg parse gate land in heicfast/rda-driver - outside this session's two-repo grant. Pushed forward as the driver-repo lane's next fix with the exact corrected byte sequence + the gate design recorded in the finding body.
