# MAJOR M-072 — regdrv-rda.reg (WinPE profile): unparseable hex(7) UpperFilters data, wrong class GUID, and a demand-start filter posture that cannot attach at boot

**Severity:** MAJOR
**Module / Surface:** driver-install (the WinPE pre-registration profile — `scripts/winpe/regdrv-rda.reg`)
**Parameters affected:** P22 (the regdrv.reg UpperFilters write), the Start-split matrix (P21-adjacent), the WinPE deployment posture (P-inst per SURFACE-INVENTORY §6 row 22)
**Corpus references:** `og-codebase/BUILDPE/x64/regdrv.reg` (the og's actual PE pre-registration file — read in full this session); `architecture/03-winpe-boot-environment.md`; `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` (the service matrix + UpperFilters documentation); `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md` (EUDCPEPM's filter role)
**Core references (rda-driver @ 323e75a):** `scripts/winpe/regdrv-rda.reg` (the whole file; the UpperFilters line is its final value entry)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** expanded on the SCAN-2f driver-lane handoff D4 ("regdrv-rda.reg hex(7) UpperFilters still invalid" — recorded not fixed) + the SURFACE-INVENTORY §1 "Driver pre-registration (WinPE) — PARTIAL — no dedicated finding owns the UpperFilters write / Start-split / eprdtdrv-absence parameters". This finding owns those parameters; two ADDITIONAL divergences beyond the recorded D4 are documented here (the class GUID and the Start posture).

## Summary

Our WinPE deployment profile `regdrv-rda.reg` diverges from the og's
`BUILDPE/x64/regdrv.reg` in three ways: (1) the `UpperFilters` value's
`hex(7)` data (`r,d,a,\,0`) is not valid .reg syntax — the tokens are
ASCII characters, not two-digit hex bytes, so a conforming registry editor
rejects the value (or the whole import) where the og's file demonstrates
the correct UTF-16LE encoding; (2) the value is written under the DISKDRIVE
class GUID `{4d36e967-…}` while the og writes it under the VOLUME class
GUID `{71A27CDD-…}`; (3) the profile registers `rda` as Start=3
(DEMAND_START) while simultaneously activating the UpperFilters copy-filter
role — the og's filter driver EUDCPEPM is Start=0 (BOOT_START) precisely so
it attaches when the volume stack builds; a demand-start class filter is
not loaded at that point, so the filter role the file claims to activate
("activate the EUDCPEPM-style copy-filter role on every disk device in the
PE") cannot engage at boot.

## What the reference does

The og's `og-codebase/BUILDPE/x64/regdrv.reg` (UTF-16LE file, read in full):

- `epmdkdrv`: Type=1, **Start=3** (demand), ErrorControl=1, ImagePath
  `\SystemRoot\System32\drivers\epmdkdrv.sys`, DisplayName;
- `EUDCPEPM`: Type=1, **Start=0 (BOOT_START)**, ImagePath
  `system32\drivers\EUDCPEPM.sys`, DisplayName, **DeviceName
  "epmdiskcopydrv"**, **IoctlBase 0x10000**;
- `EUEDKEPM`: Type=1, **Start=1 (SYSTEM_START)**, DeviceName "EUEDKEPM",
  IoctlBase 0x10000;
- `ebrntdrv`: Type=1, **Start=3** (demand), Group "Boot File System";
- `eprdtdrv`: **absent** (the live-OS raw-disk driver is not
  pre-registered in PE);
- the class filter: written under
  `HKLM\PE_Sys\ControlSet001\Control\Class\{71A27CDD-812A-11D0-BEC7-08002BE2092F}`
  (**GUID_DEVCLASS_VOLUME**) as
  `"UpperFilters"=hex(7):45,00,55,00,44,00,43,00,50,00,45,00,50,00,4D,00,00,00,00,00`
  — "EUDCPEPM" in UTF-16LE (16 bytes) + a string terminator NUL (2 bytes)
  + the final empty string of the MULTI_SZ list (2 bytes) = 20 bytes of
  two-digit hex byte values with a comma-separated list.

The Start-split is deliberate: the data-path raw drivers are demand-start
(opened on demand by the recovery host), the **filter** is boot-start
(because PnP builds the volume stack before any user-mode demand could
start it).

## What our implementation does

`scripts/winpe/regdrv-rda.reg` (read in full):

- the `rda` service: Type=1, **Start=3** (demand), ErrorControl=1, Group
  "Boot File System", ImagePath `\SystemRoot\System32\drivers\rda.sys` —
  the demand-start posture is right **for the raw-data role** (matches
  epmdkdrv/ebrntdrv);
- Parameters: IoctlBase=0x10000 (matches the og), MaxDisks=0xC8,
  DebugLevel, SessionsEnabled=1, **FilterMode=1**, VolSnapAttach=0 —
  our own extensions (no og equivalent in regdrv.reg; the og configures
  the filter role by being a boot-start UpperFilter, not by a flag);
- the class filter:
  `[HKLM\...\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}]`
  (**GUID_DEVCLASS_DISKDRIVE**) with
  `"UpperFilters"=hex(7):r,d,a,\,0`.

Three defects in that last entry:
1. **Syntax:** `r,d,a,\,0` are not hex byte values. A .reg `hex(7)` data
   body must be comma-separated two-digit hexadecimal byte values
   (optionally with `\` line continuations — which is likely how the
   author intended the `\` token). `r`, `d`, `a`, a bare `0` are invalid
   tokens; `regedit` import of this file fails on the value (the file's
   other keys may or may not land depending on the editor's error
   handling — the value itself cannot parse).
2. **Encoding:** even charitably read as the bytes `72 64 61 5C 30`
   (ASCII "rda\0"), a REG_MULTI_SZ requires UTF-16LE: the correct value
   is `hex(7):72,00,64,00,61,00,00,00,00,00` ("rda" + terminator + list
   terminator) — the og's own 20-byte entry is the working reference.
3. **Class + posture:** the og's filter rides the VOLUME class; ours
   targets DISKDRIVE. Combined with Start=3: even with a syntactically
   valid value, a demand-start driver registered as a class UpperFilter
   is not loaded when PnP builds the stack at boot, so the filter role
   the file's own comment promises ("activate the EUDCPEPM-style
   copy-filter role on every disk device in the PE") does not engage.

## Gap

Falsiable claims: (a) `regedit /s regdrv-rda.reg` on a stock Windows
cannot materialize the UpperFilters value as written (invalid hex tokens);
(b) the og's file writes UpperFilters under the VOLUME class GUID, ours
under DISKDRIVE — different registry keys; (c) with Start=3 and
FilterMode=1, no code path loads rda.sys at volume-stack build time in PE.

## Impact

- **WinPE deploy path (the boot-media builder lane):** the PE profile is
  broken at the exact value that defines its filter posture. If the
  boot-media lane ships with this file, the PE deployment either fails at
  import or silently skips the filter role — the "copy-filter on every
  disk device" capability the profile documents does not exist.
- **Parity truth:** the og's Start-split (filter boot-start, data drivers
  demand-start) is a documented corpus parameter our profile does not
  reproduce; the DISKDRIVE-vs-VOLUME class choice changes which device
  stack the filter would attach to even if started.
- **Not affected:** the live-OS install path (install.ps1) and the S-038
  app installer — they do not consume this file.

## Reproduction / verification

1. The og's file: `og-codebase/BUILDPE/x64/regdrv.reg` read directly (the
   UTF-16LE content decoded this session; the UpperFilters bytes
   45,00,55,00,… spell "EUDCPEPM" + terminators).
2. Our file: `scripts/winpe/regdrv-rda.reg` read directly (quoted in the
   finding).
3. Syntax check: the hex(7) grammar per the .reg format (two-digit hex
   tokens; `\` only as a line-continuation terminator); `r`, `d`, `a`,
   `0` are not valid tokens; the og's entry demonstrates the correct
   form.
4. GUID identity: {4d36e967-e325-11ce-bfc1-08002be10318} =
   GUID_DEVCLASS_DISKDRIVE; {71A27CDD-812A-11D0-BEC7-08002BE2092F} =
   GUID_DEVCLASS_VOLUME (the og's target). Cross-checked against the
   well-known class GUID list; the corpus file's path is the primary
   evidence.
5. D4's original record: SCAN-ENGINE-DELTA.md:329 ("regdrv-rda.reg
   hex(7) UpperFilters still invalid") — this finding is its expansion.

## Fix plan (for the engineering team)

1. No architectural change — one file rewrite + one posture decision.
2. File to modify: `rda-driver/scripts/winpe/regdrv-rda.reg`.
3. The exact change:
   - UpperFilters value:
     `"UpperFilters"=hex(7):72,00,64,00,61,00,00,00,00,00` ("rda" UTF-16LE
     + string NUL + list terminator).
   - Class key: decide by role. If the filter role is meant to mirror the
     og's EUDCPEPM PE posture, target the **VOLUME** class GUID
     `{71A27CDD-812A-11D0-BEC7-08002BE2092F}` (the og's choice); if the
     DISKDRIVE attach is deliberate (F6's documented mechanism), keep
     {4d36e967-…} but REGISTER that divergence in
     `docs/ground-truth-contracts.md` with the reason (the og's AddDevice
     patching vs our filter — the F6 design note).
   - Start posture: split the roles the way the og does — the filter
     service entry must be Start=0 (BOOT_START) for the filter to attach
     at stack build; the data-path service stays Start=3. With one
     service doing both (FilterMode=1), the PE profile should either
     (a) register Start=0 for the PE deployment (matching EUDCPEPM's PE
     posture) or (b) keep Start=3 and drop the UpperFilters write (the
     data-path-only PE posture — a documented scope choice). Decide (a)
     vs (b) with the boot-media lane; the og's own file is (a).
   - Keep IoctlBase=0x10000 (matches the og's PE deployment).
4. Tests: add the reg file to the repo's doc/lint verification — a
   PowerShell/Python parser gate that (i) parses every hex(7) body as
   two-digit hex tokens, (ii) verifies MULTI_SZ parity (even byte count,
   double-zero terminator), (iii) asserts the class GUID matches the
   documented decision. The v0.2.3 CI lesson (a broken script broke every
   CI install — the AST parse gate) is the precedent for gating this
   file class.
5. Contract/doc updates: `docs/ground-truth-contracts.md` gains the
   corrected PE-profile contract; the core repo's
   `diff-to-fix/modules/driver-install.md` row flips when landed; the
   WinPE lane's roadmap note (roadmap.md:71) references the corrected
   profile.
6. Acceptance: `reg.exe load` + import of the fixed file into a scratch
   hive yields the exact UpperFilters value; a PE boot with the profile
   shows the filter attached (execution proof on the boot-media lane's
   first dispatch — mark the attach proof execution-bound until then).

## Tests to add

- `WinpeReg.ProfileParses` — the parser gate above (fails on the current
  file: the hex(7) body).
- `WinpeReg.MultiSzEncoding` — parity + terminator check.
- `WinpeReg.ClassGuidMatchesDecision` — the GUID constant vs the
  documented role.
- (Execution-bound, boot-media lane) `WinpeBoot.FilterAttachs` — PE boot
  transcript showing rda attached to the volume/disk stack.

## Related findings

- M-071 — the same deploy surface's version-pin defect.
- N-090 / N-091 — the driver repo's doc-truthfulness batch.
- M-064 (FIXED) / M-030 (FIXED) — the deployer contract and the PE
  registration lineage this profile rides on.
- The corpus-errata candidate on the boot-config artifact's class
  labeling (see CORPUS-ERRATA E55 context in the worklog) — separate from
  this product-side defect.

## Open questions

- The (a)-vs-(b) posture decision (boot-start filter vs data-path-only PE
  profile) is an engineering choice to make with the boot-media lane; the
  og's own file is (a). Not an unknown — a decision to register.
