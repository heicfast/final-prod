# MAJOR M-033 — BitLocker: og decrypts in-stack (FVE keys → XTS-AES inside the device stack); core requires pre-unlock

**Severity:** MAJOR (user-observable capability gap on locked volumes)
**Module:** storage, bitlocker
**Parameters affected:** S61 (BitLocker handling — legitimate unlock only), A8 (multi-volume behavior), Q46 (contract documentation)
**Corpus references:** `architecture/05-scan-engine-architecture.md:116-125` (§8 Volume IO layers): "BitLocker: FVE keys (DRWService pipe) → XTS-AES decrypt (BitLockerLib/OpenSSL)" — decryption happens INSIDE the volume-IO stack, below the engines; engines are BitLocker-agnostic.
**Core references:** `src/app/rrecov.cpp:271-293` (pre-unlock gate: `bitlocker::status`/`try_auto_unlock`, else refuse with unlock guidance), `src/ui/cr_ui.cpp:916-928` (same gate). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-017` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og scans locked BitLocker volumes transparently: the user enters the recovery key in-product, DRWService fetches FVE keys, and an in-stack XTS-AES decrypting device wraps the volume — every engine sees plaintext. Our core refuses locked volumes unless the OS volume was already unlocked externally; there is no in-stack decrypting device.

## What the reference does

The architecture doc (§8, cited) places the BitLocker decrypt layer in the volume-IO stack: physical → cache → [BitLocker XTS-AES] → virtual volume device → engine. Key acquisition is via the DRWService pipe (FVE keys).

## What our core does

A pre-unlock gate: `bitlocker::status` → if locked, `try_auto_unlock` (existing protectors only); on failure the scan is refused with explicit guidance to unlock externally (`Unlock-BitLocker`). No decrypting device exists in include/cr/storage/ (5 headers, all local/plain).

## Gap

A locked volume the og scans after in-product key entry cannot be scanned by core without an external unlock step. Falsifiable: locked BitLocker fixture + recovery key → og product flow scans it; core CLI refuses.

## Impact

User-observable capability gap on the locked-volume scenario (a common data-recovery case: the dead machine's disk moved to a host, volume locked). Legitimacy posture is unchanged on both sides (recovery key required — no bypass, per parameter S61).

## Reproduction / verification

Corpus-derived (architecture doc §8; the decrypt layer's code lives in DRWService/BitLockerLib, outside the SearchDLLs — the doc is the corpus's own verified artifact). Core: code read. UNVERIFIED-BY-EXECUTION (needs a Windows host + locked fixture).

## Fix plan

1. Add a `DecryptingDevice : IStorageDevice` wrapper (XTS-AES; the crypto layer already exists in include/cr/crypto for BitLocker detection).
2. Key acquisition: accept a recovery key / passphrase via CLI/UI, derive FVE, construct the wrapper between cache and slice (the og's layer order).
3. Keep the pre-unlock gate as the default; the wrapper is the opt-in path when the user supplies a key.
4. Document the contract: no bypass, key-required (S61 posture).

## Tests to add

- `BitLocker.InStackDecryptDeviceReads` — synthetic AES-XTS fixture with a known key; assert plaintext reads through the wrapper.
- `BitLocker.LockedVolumeScanWithKey` — end-to-end: locked fixture + key → scan succeeds (fails today: refusal).
- `BitLocker.LockedVolumeWithoutKeyRefused` — pin the refusal (legitimacy posture).

## Related findings

M-032 (same layer family), I-017 (slice supersets), M-034 (NAS — the other missing device sibling).

## Open questions

The FVE-key derivation path (DRWService pipe protocol) is out-of-corpus for the SearchDLLs; a DRWService decompile pass would be needed for exact interop. The XTS-AES layer itself is standard (BitLockerLib/OpenSSL per the doc).
