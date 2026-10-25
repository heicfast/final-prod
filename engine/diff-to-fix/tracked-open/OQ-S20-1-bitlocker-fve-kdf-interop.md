# OQ-S20-1 — BitLocker FVE-key derivation interop (recovery password / BEK → VMK → FVEK)

Filed by: DFX20 (M-033's Resolution; extends the finding's own open question)
Date: 2026-10-26 · Corpus: `03ad46c` (unmoved)

## The question

The M-033 fix landed the in-stack XTS-AES decrypting device with the
FVEK-input surface (the og's `GetFullVolumeEncryptionKey` output shape).
What remains out-of-corpus is the **derivation interop**: turning a
48-digit recovery password or a .BEK file into that FVEK inside our stack,
which requires parsing the FVE metadata (volume boot sector info-block
offsets → metadata header/entries → key-protector datums) and the
AES-CCM unwrap chain (stretched key → VMK → FVEK).

## Why it is out-of-corpus (verified first-hand this session)

- `artifacts/PartitionRecovery/bitlockerlib--bitlocker-implementation.md`
  pins the derivation's EXISTENCE and shape ("stretch/derive (custom
  rounds) → VMK → FVEK"; `SetProcessProtectorKey` /
  `SetProcessProtectorKeyFromBEK` / `GetFullVolumeEncryptionKey`) but
  explicitly labels the stretch's KDF position "inferred from structure".
- The decompile's "custom stretch" (`BitLockerLib_decompiled.c:111-470`)
  is plain SHA-256 (Ch/Maj/ROTR(2,13,22)/σ(6,11,25) verified — the
  function at :111a0 is SHA256_Transform; :1da0/:1ea0 are
  Update/Final). Good: our cr_sha256 already covers it.
- The FVE metadata parse is `ParserBitLocker` (:27633) — but the datum
  walk sits behind opaque C++ object graphs (`FUN_180042350`
  "metadata valid" / `FUN_180044e90` per-copy validate, vtable-mediated
  unique_ptr chains), not a byte-level parse legible enough to
  re-implement from without an RE pass of its own.

## Precise next step

A focused decompile pass over the `bitlocker::` namespace in
`BitLockerLib_decompiled.c` (the RTTI names are recovered): decode
`ParserBitLocker`'s metadata-object constructors (the classes behind
the vtables at +0x10/+0x18/+0x40) to extract the on-disk FVE metadata
grammar — header layout, entry/datum headers, the VMK (type 2) and FVEK
(datum 5) records, the AES-CCM wrapper header, and the stretch datum's
salt/MAC fields. Deliverable: an `evidence/decompiled-quotes/`
artifact with the byte layout, then a `cr_fve_metadata.cpp` parser +
the recovery-password KDF (SHA-256 stretch chain per the decoded round
count) + `SetProcessProtectorKeyFromBEK`-equivalent, gated by the same
S61 key-required posture. Until then the FVEK-input surface is the
honest, documented scope (PARITY-DELTAS D-75).

## Consumers that would change

- `cr_ui_open_volume_with_key` (accept `recovery-password:`/`bek:` forms
  in addition to raw FVEK hex)
- CLI `--bitlocker-fvek` grammar may gain `--bitlocker-key <type:value>`
  (breaking change deferred; additive alternative preferred per the ABI
  policy)
