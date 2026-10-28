# MINOR N-097 — auto.txt misclassified in the gate inventory: it is the BitLocker FVE auto-unlock key snapshot DRWUI persists (plaintext at the install root), not a WinPE payload

**Severity:** MINOR
**Module:** health (BitLocker) / docs (inventory)
**Parameters affected:** F61-adjacent (BitLocker handling — the security datum), Q68 (the inventory's truth)
**Corpus references:** `DRWUI_decompiled.c:430715` (first-hand: DRWUI **creates** `\\.\Pipe\DRWBitlocker` via `CreateNamedPipeA(...,3,6,0xff,0xc800,0xc800,0,0)`); `:430781-430798` (reads the 0x138-stride count-prefixed key push, then **deletes and rewrites `auto.txt`** with the received records); adjacent strings `DRWService.exe /install`, `/uninstall`, `VerifyRecovery=%s`, `CBitlockerUnlock::Static_UnlockThread`; the og file itself (`og-codebase/auto.txt`, 316 B: u32 count + one 0x138-byte record with a volume GUID `{429E92F9-5908-42A7-AD61-655349DAE2F1}`, two FILETIME-like stamps, and a key-blob tail)
**Core references:** audited `35adcb9`. The misclassification: `docs/PARITY-OG-CORE-INVENTORY.md:55` files auto.txt as "INSTALLER — WinPE auto.bat payload" — the cited artifact (`re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md`) concerns `BUILDPE/x64/auto.bat`, a different file, and explicitly states nothing in it touches DRW files. Zero findings decode auto.txt (`rg -n 'auto\.txt' diff-to-fix/findings/` → 0 hits). Engine-side FVE harvest is covered (S-014 FIXED → `src/win/cr_bitlocker.cpp`); the core persists no key material to disk (grep: no auto.txt counterpart).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The corpus's `auto.txt` is the on-disk snapshot of BitLocker auto-unlock keys: DRWUI creates the `\\.\Pipe\DRWBitlocker` pipe, receives FVE key records (0x138-stride, count-prefixed, from DRWService's registry harvest), and rewrites auto.txt with them after each fetch. The gate inventory classifies it as a WinPE installer payload via a miscitation. The misclassification hides a security-relevant product behavior the downstream host should know: the og persists FVE auto-unlock key material in a plaintext file at the install root.

## What the reference does

The decompile chain (cited above): pipe creation → 0xC800-buffer read of count + 0x138-stride records → auto.txt delete + rewrite. The service side (S-014's chain) harvests the keys from the registry (`\ControlSet001\Control\FVEAutoUnlock` volumes + the external keys).

## What our core does

The core harvests FVE auto-unlock keys for *unlocking* (in-memory use — S-014, live + offline-hive paths) and never persists key material to disk. The inventory's wrong row is the only trace of auto.txt, and it points at the wrong consumer/semantics.

## Gap

The inventory row is false (wrong file semantics + a miscitation), and the plaintext-persistence behavior — a genuine og product-behavior fact with security implications for the merged product's deployment posture — is recorded nowhere.

## Impact

Documentation/registration only (the core's own behavior is the safer one and needs no change). The datum matters for: host integrators deciding where to run from (the install root gets key material in the og), and parity decisions (if the merged product ever adds an auto-unlock key cache file, it should NOT replicate plaintext-at-install-root without a deliberate decision).

## Reproduction / verification

Corpus first-hand this session: the decompile sites read (pipe create :430715; delete+rewrite :430781-430798); the 316-byte og file's structure decoded (count + GUID + stamps + key tail); the inventory row and its cited artifact read (the artifact disproves the row); the findings grep (zero).

## Fix plan (for the engineering team)

1. Correct `docs/PARITY-OG-CORE-INVENTORY.md:55`: auto.txt = "BitLocker FVE auto-unlock key snapshot (DRWUI's `\\.\Pipe\DRWBitlocker` consumer; plaintext at install root — OOS-UI/DRWService sidecar, security datum for host deployers)". Cite the decompile sites.
2. Optionally add one sentence to `docs/SECURITY.md`'s BitLocker section: the core never persists key material (the positive posture statement) + the og's contrast (the datum).

## Tests to add

- None (documentation). The core's no-persistence posture could get a pinning test — `Bitlocker.NoKeyMaterialWrittenToDisk` (scan a fixture with unlock; assert no key-shaped file appears in the CWD/install dir) — optional hardening.

## Related findings

- S-014 (FIXED — the harvest chain whose og counterpart feeds this file).
- I-027 (this session — the og-codebase data-asset register this correction anchors).

## Open questions

None.
