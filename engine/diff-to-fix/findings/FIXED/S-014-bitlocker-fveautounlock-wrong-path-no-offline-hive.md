# SEVERE S-014 — BitLocker auto-unlock reads the wrong registry hive path; the offline-hive (dead-system) auto-unlock is missing; enumeration is unassociated with the target volume

**Severity:** SEVERE (locked volumes the og unlocks stay locked — FVE safety parameter)
**Module:** win (transport/bitlocker), safety
**Parameters affected:** F61 (BitLocker handling — legitimate unlock only), B1 (detection), F64-adjacent (consistency)
**Corpus references:** `re-workspace/reversed/ghidra-core/DRWService_decompiled.c:104` (`RegOpenKeyExA(HKLM, "SYSTEM\\CurrentControlSet\\Control\\FVEAutoUnlock")`), `re-workspace/reversed/DRWService.exe/strings_ascii.txt:32`, `re-workspace/reversed/ghidra-mo/bitlockerhelp_decompiled.c:3128-3155` (offline hive: `snprintf("%s\\Windows\\System32\\config\\SYSTEM")` → `"\ControlSet001\Control\FVEAutoUnlock\"` → per-subkey `"%s%s\\Data"`), `bitlockerhelp.mo/strings_ascii.txt:454`, artifacts `DRWService-exe--bitlocker-service.md` §2-§3 (`:104-130`, `:1461-1593`), `artifacts/PartitionRecovery/bitlockerlib--bitlocker-implementation.md`
**Core references:** `src/win/cr_bitlocker.cpp:176-236` (`try_auto_unlock` + `enum_binary_blobs`) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og reads BitLocker auto-unlock external keys from `HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock` (and, for dead systems, from the **target volume's own offline registry hive** `…\Windows\System32\config\SYSTEM`). The core opens `HKLM\SYSTEM\CurrentControlSet\Services\FVEAutoUnlock` — the wrong path; the live-registry unlock can never succeed on a machine where the og's path has keys, and the offline-hive path (the og's flagship "OS is broken" scenario) has no core implementation at all. The core also tries **every** ≥32-byte REG_BINARY value in the tree against the target volume with no volume association and no record/size caps, while the og reads per-volume GUID subkeys' `Data` blobs (≤ 256 B, ≤ 100 records).

## What the reference does

**Live path** — `DRWService_decompiled.c:104`:

```c
RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\FVEAutoUnlock", ...)
```

with per-volume subkeys named by volume GUID and a REG_BINARY `"Data"` value holding the BEK-class external-key blob (artifact §2-§3).

**Offline path (dead system)** — `bitlockerhelp_decompiled.c:3128-3155`:

```c
snprintf(path, "%s\\Windows\\System32\\config\\SYSTEM", volume_root);
// hive-type check, then
"\ControlSet001\Control\FVEAutoUnlock\"      // per-subkey walk
"%s%s\\Data"                                 // REG_BINARY read per volume GUID
```

via `RegLib::OpenHiveByVolumeA/W` (`RegLib_decompiled.c:203/275`) — the og mounts the target volume's own SYSTEM hive and reads the same key family, enabling auto-unlock when the OS disk is the one being recovered.

## What our core does

`cr_bitlocker.cpp` (`try_auto_unlock`):

```cpp
if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\FVEAutoUnlock", 0,
                  KEY_READ, &k) != ERROR_SUCCESS) {
    return ERR_FVE_LOCKED; /* no auto keys configured */
}
```

- **Wrong hive path:** `Services\FVEAutoUnlock` vs the og's `Control\FVEAutoUnlock` — on a live system with auto-unlock configured, `RegOpenKeyExW` fails and the function reports "no auto keys configured".
- **No offline-hive reader:** `grep -rn -i "hive|regf|config\\\\SYSTEM" src/ include/` finds no offline-hive implementation anywhere in the core.
- **Unassociated enumeration:** `enum_binary_blobs` recurses over every subkey and tries every REG_BINARY ≥ 32 bytes against the target (no volume-GUID match to the subkey name, no ≤100-record cap, no ≤256 B size cap).

The in-code comment ("FVEAutoUnlock external keys (corpus DRWService path)") claims corpus parity that the decompile contradicts.

## Gap

Falsifiable claims:
1. On a live Windows system with a BitLocker data volume configured for auto-unlock (og-readable at `Control\FVEAutoUnlock`), the core's `try_auto_unlock` returns `ERR_FVE_LOCKED`; the og unlocks and scans.
2. Scanning a dead system's data volume whose auto-unlock key lives in that volume's own `…\System32\config\SYSTEM` hive: og auto-unlocks; core reports the volume locked (no code path exists).
3. A machine whose `Services\FVEAutoUnlock` key exists but contains unrelated binary data (the wrong hive path) can make the core burn unlock attempts on arbitrary blobs the og would never select (no GUID association).

## Impact

- **Safety/parity (F61):** the core's detection-only/legitimate-unlock posture is correct, but the legitimate unlock itself is broken on both live and dead systems — the recovery feature silently degrades to "locked volume, no data" exactly where the og advertises the capability.
- **Dead-system recovery:** scenario 2 is the product's flagship use case; the core cannot serve it via auto-unlock keys at all.
- **Determinism:** blind enumeration adds nondeterministic behavior (up to hundreds of unlock attempts on foreign blobs).

## Reproduction / verification

- Corpus-derived (strength 2/3): decompile quotes + strings file citations above (agent S2-F quote sheet in `evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md`).
- Core side verified first-hand at `e93decf` (`cr_bitlocker.cpp:176-236` read directly; hive grep clean).
- Not executed (no Windows/wine in this environment) — the path mismatch is fully determined by the two quoted strings; execution belongs to the engineering team's Windows lab (see fix-plan acceptance).

## Fix plan (for the engineering team)

1. **Live path:** change the key to `SYSTEM\CurrentControlSet\Control\FVEAutoUnlock` in `cr_bitlocker.cpp:219` (keep `KEY_READ`, the .BEK temp-file flow, and the F13 no-crypto-in-process rule — those are verified-correct policy).
2. **Volume association:** iterate subkeys (volume GUIDs) and prefer the subkey matching the target volume's GUID (available from the volume object); only fall back to trying other subkeys if the match fails, with the og's caps: ≤ 100 records, blob size cap 256 B (artifact §2-§3).
3. **Offline path:** add a minimal hive reader for dead-system scans: mount `<volume>\Windows\System32\config\SYSTEM` read-only (an in-house `regf` parser is acceptable — no OS API on non-Windows; keep it read-only), walk `\ControlSet00X\Control\FVEAutoUnlock` (iterate ControlSet001…NNN per the hive's `Select` key family), and feed the same per-GUID `Data` blobs into the existing .BEK unlock path. Gate it to the WinPE/dead-system modes where the volume's OS is not the running one.
4. **Comment fix:** correct the in-code "corpus DRWService path" comment to the real path.
5. **Docs:** update `docs/CAPABILITY-MATRIX.md` F13/F61 rows to state live+offline auto-unlock support once landed.

## Tests to add

- `Bitlocker.AutoUnlockLivePath` — registry fixture with `Control\FVEAutoUnlock\{guid}\Data`: unlock succeeds (fails before — wrong path).
- `Bitlocker.AutoUnlockOfflineHive` — synthetic `config\SYSTEM` hive fixture with the key family: dead-system scan unlocks the volume (no code path before).
- `Bitlocker.AutoUnlockPrefersMatchingVolumeGuid` — two volume subkeys: only the target GUID's blob is attempted.

## Related findings

- F61 was never scored by session 1 (coverage admission — see `modules/health.md` session-2 note); this finding is the substantive result of closing that hole.

## Open questions

None beyond the engineering team's Windows-lab execution of the acceptance tests.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-014 commit; see git log)
**Verified by:** Linux lane (341/341 ASan, 11 new tests): `Reghive.ParsesValidHiveAndRejectsGarbage` / `Reghive.RejectsCorruptedHeaderChecksum` / `Reghive.WalksControlSetKeyFamilyCaseInsensitive` / `Reghive.EnumeratesSubkeyNames` / `Reghive.ReadsBinaryValueDataPointedAndInline` / `Reghive.UnicodeKeyNamesWalkAsAscii` / `FveAutoUnlock.HarvestsPerGuidDataBlobs` / `FveAutoUnlock.HarvestCapsBlobsAt256Bytes` / `FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs` / `FveAutoUnlock.SelectionPrefersMatchingVolumeGuid` / `Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing` — fail-before proven by construction: the pre-fix tree has NO reghive module, so the suite fails to even compile the tests (`fatal error: cr/reghive/cr_reghive.h: No such file or directory` — reproduced by removing the module from the fixed tree). Windows lane (Windows CI, 5 tests registered as the `bitlocker` ctest target): `Bitlocker.AutoUnlockLivePathMatchesOgDrwService` (path-constant pin — fails against the pre-fix Services\... string by not compiling: the seam did not exist), `Bitlocker.AutoUnlockEnumeratesPerGuidDataValues` (HKCU fixture: per-GUID subkeys + Data blobs, decoys ignored, 256-B cap, _strupr_s shape), `Bitlocker.AutoUnlockPrefersMatchingVolumeGuid` (recorder: only the matching GUID's blob attempted), `Bitlocker.AutoUnlockOfflineHive` (synthetic SYSTEM hive file + recorder; garbage hive refused), `Bitlocker.AutoUnlockSelectionNoGuidFallsBackInOrder`. Fuzz: `fuzz/fuzz_reghive.cpp` added to the fuzz suite (CI lane; the harness parses + walks + harvests + selects arbitrary bytes). Real-BitLocker-volume unlock remains the Windows-lab acceptance the finding itself records (no BitLocker volume exists in CI — same lab boundary class the finding's Reproduction section states).
**Resolution notes:** Three-part fix, all corpus-verified first-hand this session before implementation: (1) LIVE PATH — `try_auto_unlock` now opens `SYSTEM\CurrentControlSet\Control\FVEAutoUnlock` (DRWService_decompiled.c:104, RegOpenKeyExA; the pre-fix `Services\FVEAutoUnlock` appears in no og binary) and enumerates per-volume-GUID subkeys' `Data` REG_BINARY blobs with the og's caps (50-char GUID, 256-byte blob cap = the pipe record's memcpy(Data, min(size,0x100)), 100 records; DRWService :104-140 + artifacts/DRWService-exe--bitlocker-service.md :56-74/:1461-1593). (2) VOLUME ASSOCIATION — the target volume's GUID comes from GetVolumeNameForVolumeMountPointW; the matching record is attempted first, the rest follow in enumeration order (`reghive::select_auto_unlock_attempts`, portable, pinned on Linux). (3) OFFLINE PATH — new portable module `cr/reghive` (read-only regf parser, every structural offset decoded from the og's own RegLib_decompiled.c: XOR header checksum at +0x1FC with the 0xFFFFFFFF/0 canonicalization from FUN_180007670, root cell at header+0x24 via FUN_180007620, bins-size bound at +0x28 and the >=0x20/8-aligned/allocated cell gates from FUN_180007700, the 4/8-byte cell stride switch from FUN_180005ff0, nk flags/count/list/name offsets from the Key accessors and the write path, vk length/type/inline-bit from RegEnumValueA, lf/lh/li/ri list semantics) + `try_auto_unlock_offline(letter, hive_path)` and an automatic probe inside `try_auto_unlock` that walks every other mounted volume's `<X>:\Windows\System32\config\SYSTEM` (the og's bitlockerhelp flow, :3128-3160; running-OS letter and target letter skipped). The harvest walks the og's LITERAL `\ControlSet001\Control\FVEAutoUnlock` (hardcoded in bitlockerhelp :3140-3146 — the og does NOT consult the Select key; the fix plan's "iterate ControlSet001..NNN per Select" was corrected to match the corpus). The old blind `enum_binary_blobs` recursion (every REG_BINARY >= 32B in the whole tree, no GUID association, no caps) is gone. Safety hardenings preserved: detect+legitimate-unlock-only posture (F13), no crypto in-process, .BEK temp-file flow, PowerShell-primary + manage-bde fallback, no new OS crypto surface. Win7-inclusive gate: two conscious allowlist additions with reasons in tools/verify_pe_um.py (RegQueryValueExW - the "Data" value read, NT-era; GetVolumeNameForVolumeMountPointW - the GUID association, Win2000-era; both per the gate's documented process). CLI/ABI surfaces unchanged (try_auto_unlock keeps its signature; try_auto_unlock_offline is additive).
