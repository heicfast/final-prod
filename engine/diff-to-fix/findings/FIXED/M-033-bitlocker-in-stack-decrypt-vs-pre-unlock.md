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

## Resolution

**Status:** FIXED
**Fixed in commit:** <this push — DFX20 session, M-033 landing>
**Verified by:** 13 tests in tests/unit/test_bitlocker_xts.cpp (AesXts.Aes128Fips197KnownAnswer, AesXts.Aes256Fips197KnownAnswer, AesXts.Xts128CrossImplementationVector, AesXts.Xts256CrossImplementationVector, AesXts.Xts256WholeSectorVector, AesXts.WeakKeyHalvesRejected, AesXts.RoundTripVariedLengths, AesXts.DistinctTweaksGiveDistinctCiphertexts, DecryptingDevice.ReadsPlaintextThroughWrapper, DecryptingDevice.UnalignedAndCrossSectorReads, DecryptingDevice.InnerErrorsPropagateVerbatim, DecryptingDevice.BadKeyMaterialCreateFails, BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice); suite 608/608 linux-gcc (RelWithDebInfo + ASan). Fail-before is intrinsic (the DecryptingDevice/Aes/AesXts types did not exist on the audited tree; the "with key" scan had no code path). The refusal contract (locked without key) stays pinned by the existing gates (cr_bitlocker status/auto-unlock/refusal, FB-B01) plus the new LockedVolumeWithoutKeyRefusal-family coverage on the Windows lane test_bitlocker_win.cpp surface (unchanged default).
**Resolution notes:** CONFIRMED-DIVERGENCE, fixed to the corpus-pinned architecture. Verified first-hand against the corpus: architecture/05-scan-engine-architecture.md section 8 places the BitLocker XTS-AES decrypt in-stack below the engines; artifacts/PartitionRecovery/bitlockerlib--bitlocker-implementation.md pins the cipher (OpenSSL EVP aes_{128,256}_xts), the key surface (GetFullVolumeEncryptionKey/SetProcessProtectorKey/SetProcessProtectorKeyFromBEK — VMK/FVEK material, BEK file), and the recovery flow (user key entry -> derive -> engines see plaintext); BitLockerLib_decompiled.c:27075-27107 confirms GetFullVolumeEncryptionKey's >=0x40-byte output shape; the decompile's "custom stretch" at :111-470 is plain SHA-256 (Ch/Maj/ROTR structure verified — the artifact's own assessment already labeled the stretch's KDF position as inferred). The fix: (1) cr/crypto/cr_aes.h + src/core/crypto/cr_aes.cpp — clean-room AES-128/256 (FIPS-197; KATs pinned) + XTS (IEEE 1619-2007 / SP 800-38E, whole-data-unit discipline — BitLocker sector units are 512/4096-byte multiples; no CTS partial units exist in the reference's sector path), verified byte-exact against vectors generated independently by pycryptodome AND OpenSSL EVP 3.5 (the EVP interface the og itself links); XTS weak-key halves rejected as OpenSSL does. (2) cr/storage/cr_decrypt_device.h + src/core/storage/cr_decrypt_device.cpp — DecryptingDevice : IStorageDevice, the in-stack layer wrapping the volume slice (physical -> slice -> [XTS] -> engine, the og's order); per-sector data-unit tweak = LE sector index (BitLocker convention); unaligned reads decrypt the spanned sectors sector-by-sector; transport errors propagate verbatim (error-map honesty preserved). (3) Key surface matching the og's API shapes: the user supplies the FVEK (64/128 hex chars — the GetFullVolumeEncryptionKey output bytes; `--bitlocker-fvek` CLI pre-pass + `cr_ui_open_volume_with_key` additive ABI export, no version bump per the documented policy); locked/Unknown + key -> in-stack decrypt; auto-unlock-able volumes still take the OS path; locked WITHOUT key keeps the ERR_FVE_LOCKED refusal (S61/F13 legitimate-unlock-only posture preserved — nothing is bypassed). Scope note, per the finding's own open question (confirmed first-hand this session: the FVE metadata parse in ParserBitLocker sits behind opaque C++ object graphs — FUN_180042350/FUN_180044e90 — and the corpus pins the derivation's EXISTENCE but not its byte interop): in-STACK decryption of recovery-password/BEK-derived FVEKs requires the FVE metadata unwrap chain (stretch -> VMK -> FVEK via AES-CCM datums), which is out-of-corpus; filed as tracked-open OQ-S20-1 with the exact decompile entry points. The landed surface accepts the FVEK directly (the og's own GetFullVolumeEncryptionKey consumer shape); a key that does not match the volume yields honest detection failure, never wrong-file results. Safety hardenings preserved: FB-B01 never-scan-blind (key required to leave the refusal), no crypto bypass of any kind, SDDL gating untouched. Docs updated: CAPABILITY-MATRIX.md (BitLocker row), architecture.md volume-IO layer, ABI-CONTRACT.md (new export), PARITY-DELTAS.md D-75 (the FVEK-input surface scope).
