# s31 — OQ-S20-1 resolved: the BitLocker derivation-interop premise corrected — the og delegates user-key unlock to WINDOWS; its in-stack surface is the FVEK-extract + XTS path the core already has

> Session DFX31, corpus `03ad46c` unmoved. All quotes first-hand from
> `re-workspace/reversed/ghidra-partition/BitLockerLib_decompiled.c`
> (83,772 lines, full C++ symbols recovered) +
> `re-workspace/reversed/PartitionRecovery/BitLockerLib.dll/strings_ascii.txt`
> + `artifacts/PartitionRecovery/bitlockerlib--bitlocker-implementation.md`.

## 1. The recovery-password stretch does NOT exist in the og's binary

The standard BitLocker recovery-password stretch (publicly documented,
dislocker/libbde class) iterates SHA-256 0x100000 times. Decisive negative
evidence, re-run this session:

- `grep 0x100000|0xfffff|0xF4240|1048575|1048576` over the decompile: **zero
  iteration-loop hits** (the only 0x100000-shaped hits are byte-swap shifts
  `* 0x1000000` and unrelated KDF arithmetic at :7591-:7615).
- The SHA-256 layer exists and is verified: `FUN_1800011a0` = SHA256_Transform
  (the OQ's ":111a0"; the ROTR/σ structure confirmed), `FUN_180001da0` =
  Update, `FUN_180001ea0` = Final (the 0x38/0x40 pad boundary + the
  big-endian length<<3 stamp at :440-470). Its ONLY in-library consumers:
  the one-shot helper `FUN_180001fb0` (:499, called at :5148/:5151 via the
  hash dispatcher's case 8/9) and the digest helper at :42957. **No loop
  calls it repeatedly.**
- The KDF framework that DOES exist is generic and differently-parameterized:
  the KDF context ctor `FUN_1800064f0` (:2763) defaults to
  `*(undefined4 *)(param_1 + 4) = 10000;` — ten thousand, not 2^20 — and the
  only "BitLocker-specific" derivation is the **"BITLOCKER"-labeled KDF**
  `FUN_18003fd80` (:38474-38497):

```c
  FUN_180005f40(local_358);                                  /* hash ctx: registers the 0x20-byte hash (SHA-256) */
  FUN_1800064f0(local_4b8);                                  /* KDF ctx: default 10000 iterations */
  sVar1 = strlen("BITLOCKER");
  FUN_180006110(local_358,(ulonglong *)"BITLOCKER",sVar1);   /* hash the label */
  FUN_180006f30((undefined1 (*) [16])local_4b8,&LAB_180006240,local_358,
                (undefined8 *)"BITLOCKER",sVar1);            /* KDF setup with the label */
  puVar2 = (undefined8 *)FUN_180039420(param_2);
  FUN_180007200((undefined1 (*) [16])local_4b8,puVar2,param_1);  /* derive */
```

  — a vendor KDF (their own key-wrapping for process/auto-unlock keys), NOT
  the on-disk recovery-password stretch. The corpus artifact's "8-round
  word-mixing chain" description is a misread of SHA-256's round function
  (lines ~181-230 sit INSIDE FUN_1800011a0); the artifact's own confidence
  note already flagged the stretch's position as "inferred from structure" —
  the inference is now corrected by the decode: there is nothing to infer
  because the loop is not there.

## 2. What the og's in-stack surface actually is

- **`SetProcessProtectorKey(protector_t)`** (:28203-28316): the input is
  `param_2` = a 36-char GUID string (`strlen == 0x24`), a type byte at
  `param_2[0x25]`, and the material at `param_2 + 0x26`. The function walks
  the parsed protector list (`stride 0x88`) and stores the material onto the
  type-matching protector: type 2 → raw copy (`FUN_18004c920`), type 3 →
  string builder `FUN_180045ef0`, type 8 → `FUN_180045400`. It sets material;
  it does NOT derive anything here.
- **`GetFullVolumeEncryptionKey(out, &len)`** (:27079-27113): requires
  `*len >= 0x40` (**the 64-byte XTS key pair**), pulls the FVEK from the
  parsed metadata object (`vtable+0x60` on the this+0x38 metadata), copies
  it out, succeeds when ≥ 0x10 bytes. This is the FVEK-extract surface.
- **The unlock flows** (corpus artifact, line-verified strings):
  `PSUnlockBitLocker` builds `Unlock-BitLocker -MountPoint '<drive>'`
  (PowerShell) with a pre-PowerShell `manage-bde -unlock` fallback — **the
  user-supplied password / recovery key / BEK unlock is DELEGATED TO
  WINDOWS**, which performs the standard KDF in its own code. The in-stack
  auto path applies DRWService's FVEAutoUnlock external keys
  (`GetAutoUnlockKey`, the `\\.\Pipe\DRWBitlocker` service).
- **The XTS decrypt**: OpenSSL EVP is statically linked (the full EVP cipher
  name table at strings:1240-1265 incl. AES-128/256-XTS); M-033's landing
  already decoded and implemented the XTS read-through device.

## 3. The verdict on the OQ's premise

The OQ asked for "the derivation interop: turning a 48-digit recovery
password or a .BEK file into that FVEK inside our stack… the AES-CCM unwrap
chain (stretched key → VMK → FVEK)" and assumed the og's binary contains
that chain for us to port. **It does not.** The og's division of labor:

| Capability | og | Where |
|---|---|---|
| FVE metadata parse (info blocks, datums, protector list) | yes | ParserBitLocker + the metadata objects (this+0x38) |
| Protector enumeration + material set | yes | GetProtectors ×type / SetProcessProtectorKey |
| Recovery-password / BEK → VMK (stretch + AES-CCM) | **NO — delegated** | Windows via `Unlock-BitLocker` / `manage-bde` |
| Auto-unlock external keys | yes | DRWService pipe + GetAutoUnlockKey |
| FVEK extract (64-byte XTS pair) | yes | GetFullVolumeEncryptionKey |
| XTS read-through decrypt | yes | OpenSSL EVP (M-033 landed) |
| Whole-volume decrypt/encrypt | yes | BitLockerManager::Encryption/Decryption |

The core's M-033 posture — the in-stack XTS decrypting device with the
FVEK-input surface — is **the og's own in-stack surface**. Implementing the
recovery-password KDF + AES-CCM unwrap in the core would be a
**dislocker-class SUPERSET over the og**, not parity.

## 4. The honest remaining scope (a deliberate-superset decision, registered)

1. **The FVE metadata parser** (info-block geometry → datum walk → protector
   enumeration): the og has it because its Windows-delegation flow needs the
   protector list; a Windows-free core would need it only to enumerate
   protectors for diagnostics or to feed its own unwrap. Publicly documented
   format; corpus side needs no further decode (the objects are
   vtable-mediated; the datum grammar was never the blocker).
2. **The standard KDF + AES-CCM VMK unwrap + BEK file parse**: the og itself
   lacks the stretch; porting it means implementing the public spec, not the
   og. Register as a scope decision: a Windows-free unlock capability
   (valuable in WinPE/broken-OS scenarios where even the og's delegation
   fails — but that is an IMPROVEMENT register, not a parity fix).
3. **A Windows-side unlock shim** (the og's actual parity answer for user
   keys): the core COULD replicate `Unlock-BitLocker -MountPoint` delegation
   on Windows for the same end-to-end capability. Product-scope call.

The PARITY-DELTAS D-75 registration (the FVEK-input surface as the honest
scope) is CONFIRMED by this decode — the OQ's residual is closed with the
premise corrected rather than a non-parity implementation shipped as a fix.
