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
