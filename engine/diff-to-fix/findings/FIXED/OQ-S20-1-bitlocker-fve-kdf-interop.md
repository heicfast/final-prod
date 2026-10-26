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

---

## Resolution (session DFX31 — the premise CORRECTED by the decode; the OQ's deliverable scope re-registered)

**Status:** RESOLVED (PREMISE-CORRECTED; evidence `evidence/decompiled-quotes/s31-oq-s20-1-bitlocker-derivation-scope.md`, all quotes first-hand, corpus `03ad46c`)

**The decode's findings (first-hand):**
1. **The standard recovery-password stretch (0x100000 SHA-256 rounds) does
   NOT exist in BitLockerLib.** No 0x100000/0xFFFFF/0xF4240 loop constant
   anywhere; the verified SHA-256 layer (FUN_1800011a0 Transform / :1da0
   Update / :1ea0 Final) has only 3 non-iterating consumers; the generic KDF
   framework (FUN_1800064f0 :2763) defaults to 10000 iterations and its only
   BitLocker-specific use is the vendor "BITLOCKER"-labeled KDF
   (FUN_18003fd80 :38474-97) — their own process/auto-unlock key wrapping,
   not the on-disk stretch. The corpus artifact's "8-round word-mixing
   chain" was a misread of SHA-256's round function (its own confidence
   note flagged the position as inferred; the inference is now corrected).
2. **The og DELEGATES user-key unlock to WINDOWS**: PSUnlockBitLocker builds
   `Unlock-BitLocker -MountPoint '<drive>'` (line-verified in the corpus
   artifact) with a `manage-bde -unlock` fallback — Windows performs the
   standard KDF in its own code. The in-stack auto path applies DRWService's
   FVEAutoUnlock external keys (GetAutoUnlockKey, the \\.\Pipe\DRWBitlocker
   service).
3. **The og's in-stack surface** (the actual parity target): FVE metadata
   parse (ParserBitLocker + the metadata objects), protector enumeration +
   material set (SetProcessProtectorKey — protector_t = 36-char GUID, type
   byte @+0x25, material @+0x26; the 0x88-stride list), the FVEK extract
   (GetFullVolumeEncryptionKey — 64-byte XTS pair, ≥16 bytes), the auto-
   unlock keys, and the XTS read-through decrypt (M-033's landed surface).

**Verdict:** the core's M-033 posture (the in-stack XTS device with the
FVEK-input surface) **is the og's own in-stack surface** — D-75's
registration is CONFIRMED by the decode. Implementing the recovery-password
KDF + AES-CCM unwrap + BEK parse in the core would be a dislocker-class
SUPERSET over the og, not parity — a deliberate-improvement scope decision,
not a fix owed by the audit.

**The remaining honest scope (registered for the product decision, in
PARITY-DELTAS terms):**
- (a) an in-stack FVE metadata parser (info-block geometry → datum walk →
  protector enumeration) — the og has it for its Windows-delegation flow;
- (b) the standard KDF + AES-CCM VMK unwrap + BEK parse — the og itself
  lacks it; porting means implementing the public spec (an IMPROVEMENT
  register: Windows-free unlock in WinPE/broken-OS scenarios where even the
  og's delegation fails);
- (c) a Windows-side unlock shim (replicating the og's own
  `Unlock-BitLocker` delegation for end-to-end input-form parity).

**Resolution notes:** The OQ closed with the premise corrected rather than a
non-parity implementation shipped as a "fix": the corpus wins — the og's
binary contains no derivation chain to port, and pretending otherwise would
have been hallucinated parity. D-75 stands as the honest scope; the three
superset options are registered with their rationale. No code change.
