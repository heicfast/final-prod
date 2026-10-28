# s11-sfc-payload-decode.md — the OQ-S10-2 "dead payload" resolution REFUTED: the VideoViewer/sfc file is VideoFileRepairer.exe's live encrypted engine-init database

**Session:** S11 (session-11 continuation lane, L3/OQTRACE) · 2026-10-29
**Core audited:** `46b81573` (code tree = `432d15dc`'s; the S11 worklog commit on top) · **Corpus:** `be0cfe9`
**Method:** byte-level container decode (od/python, first-hand) + decompile walk of
`ghidra-wave2/VideoFileRepairer_decompiled.c` (the corpus's own artifact) + objdump
cross-proof on the PE. Every load-bearing cite re-verified by the lead.

---

## 1. Why the S10-F wave-2 "decisive probe" could not find the consumer (methodology autopsy)

The closed OQ's resolution
(`findings/FIXED/OQ-S10-2-videoviewer-sfc-unknown-payload.md`, commit `4f44db49`)
rests on: `grep -l "sfc\|cfs"` over every `*_decompiled.c` → zero hits → "NO consumer
exists anywhere in the reversed corpus … dead payload". That probe is **structurally
incapable** of finding this consumer, for two independent reasons:

1. **The filename never appears as ASCII in the decompile.** The loader builds the
   path from a Ghidra `DAT_` symbol whose target string lives in `.data`
   (`DAT_0059d24c` = `"sfc"` — §2.2). The decompiled C text contains the symbol
   name, not the string. A grep for `sfc` matches nothing.
2. **The magic never appears as "cfs" ASCII either.** The magic check is a hex
   constant compare — `*piVar1 == 0x73666320` at `:248309` (§2.2). The ASCII
   substring "cfs" does not occur in `0x73666320` as text.

The correct probes (executed this session, all positive):
- `grep -n "0x73666320\|9760584c" ghidra-wave2/VideoFileRepairer_decompiled.c` →
  **`:248309`** (magic check), **`:249399`** (the builder's magic write),
  **`:249963-249966`** (the XOR key dwords).
- `objdump -d` on `og-codebase/VideoViewer/VideoFileRepairer.exe` → the key
  materialized as immediates at VMA `0x517658-0x51766d`
  (`movl $0x9760584c … -0x10(%ebp)` …), the decryption routine's stack-key setup.

## 2. The decode (first-hand, byte-verified)

### 2.1 The container grammar — `og-codebase/VideoViewer/sfc` (211,614 bytes)

```
0000000  20 63 66 73 2d f8 6c 97 46 41 4e 2e bd a6 c2 a5   | cfs-.l.FAN.....|
0000010  11 81 2a b2 7e 60 26 d4 04 05 7c 6d dd df c8 aa   ..*.~`&...|m....
```

- **Magic:** bytes 0-3 = `20 63 66 73` = `" cfs"` (LE u32 `0x73666320` — matches
  the decompile's check at `:248309` exactly).
- **Payload:** bytes 4.. = the whole remainder, **XOR-obfuscated with a 16-byte
  key** `4C 58 60 97 46 41 4E 2E 98 E6 8C 93 11 81 2A 8B`, proven twice:
  - decompile: `FUN_00517640` at `VideoFileRepairer_decompiled.c:249963-249966`
    loads `local_14[0]=0x9760584c; [1]=0x2e4e4146; [2]=0x938ce698; [3]=0x8b2a8111`
    (LE byte order = the key above; note `[1]` = `46 41 4E 2E` = **"FAN."**).
  - objdump immediates at VMA `0x517658-0x51766d` (§1).
- **The "FAN. motif" explained:** the OQ's own first-hand note ("30 `FAN.` markers
  at scattered offsets") is the key showing through zero plaintext — where the
  compressed stream contains zero runs, XOR leaves the raw key bytes visible. The
  file at offset 8-11 carries `46 41 4E 2E` verbatim because the plaintext there is
  zero (§2.2's decoded header confirms: payload bytes 4-11 decrypt to all zeros).
- **Decoded header** (payload after XOR, lead-executed):
  `61 a0 0c 00 | 00 00 00 00 | 25 | 40 4e 36 | …`
  - u32 LE at +0 = `0x000CA061` = **827,489 uncompressed bytes** (ratio 3.91× vs
    211,610 compressed) — a **clean u32 length framing**, refuting the OQ
    resolution's "no clean u32 length framing" note (invisible without the key).
  - version byte at buf+0xC (file offset 12) = `0x25` → high nibble 2 → the **v2
    parser** dispatch at `:248357-248364`.

### 2.2 The consumer chain — inside VideoFileRepairer.exe itself

- **Loader `FUN_00514f80`** (`VideoFileRepairer_decompiled.c:248131`): builds
  `<dir>\sfc` (`DAT_0058d4ec` = `"\\"`, `DAT_0059d24c` = `"sfc"`, `:248187-248208`),
  `_wfopen_s(…, "rb")`, magic check `*piVar1 == 0x73666320` (`:248309`), decrypt
  `FUN_00517640(buf+4, size-4)` (`:248313`), version-nibble dispatch at buf+0xC
  (`:248357-248364`) → **v2 parser `FUN_00512fa0`** = a custom LZ decompressor
  (varint literal lengths, 64-byte blocks, match tokens; `:246026-246300`).
- **Builder `FUN_00516270`** (`:249149`, `"wb"`) + record serializer
  `FUN_00516a70` (`:249499`, `fwrite` 4-byte fields + `ostringstream`); the
  builder writes the magic at `:249399`.
- **Caller = the repair-engine ctor `FUN_005126f0`** (`:245536`): loads the
  database at startup and **rebuilds it on load failure** (`:246921-246931`).

So the file is **live product machinery**: the Fixo-line video-repair engine's
encrypted, LZ-compressed engine-init database, loaded by its ctor, rebuilt when
corrupt. Not dead payload.

## 3. The corrected disposition

- The **OQ's question is now answered correctly**: hypothesis 1 was right in
  substance (a data asset of the video-repair line) but the file is **ALIVE**, not
  dead. The consumer is the repair family (OOS per I-010/D-26 — the repair line is
  scope-excluded), so the **registration outcome does not change** (still a
  registration row, not a capability finding) — but the *recorded reason* changes:
  "dead-shipped installer-chain payload" → "live encrypted engine-init database of
  the OOS repair line, fully decoded".
- **N-134's S10-F addendum carries the wrong disposition** and is corrected by the
  N-138 filing (the false-resolution record).
- Residual (non-blocking, registered): the record semantics inside the database
  would need the ~275-line LZ decompressor ported to enumerate; nothing in core
  scope depends on it.

## 4. Verification log (lead, first-hand)

1. `od -A d -t x1z -N 32 og-codebase/VideoViewer/sfc` — magic + visible key bytes
   (reproduced in §2.1).
2. Python XOR decode with the decompile's key: header `61 a0 0c 00 00 00 00 00 25
   40 4e 36` — size 827,489 LE, version `0x25`.
3. `grep -n "0x73666320\|9760584c" ghidra-wave2/VideoFileRepairer_decompiled.c` →
   `:248309`, `:249399`, `:249963` (all three quoted in §1/§2).
4. `sed -n '249963,249967p'` — the four key dwords verbatim (§2.1).
5. The S10-F resolution's own greps re-run mentally against the above: their
   `grep "sfc\|cfs"` cannot match `DAT_0059d24c` or `0x73666320` — the
   methodology gap is mechanical, not judgment.

## 5. Corroboration note (OQ-S8-1 / M-111)

This lane independently re-derived the CWindowsFS consumer trace convergent with
S10-F wave-2's M-111 (same chain: `GetProcAddress("CreateExplorerPartitionObject")`
DRWUI:438891 → ExplorerPartition::Open's `FUN_1800017f0("FileSystem")` + module
vtable+0x38 factory PartitionRecovery:8481-8491 → the QTableView browse surface,
4-column widths `0x183/0x96/0x50/0x6e` at DRWUI:1412346-1412352, root listing from
`QString "\\"` :1412353-1412358, `ShellExecuteA("open")` vs in-app browse split at
:1421113/:1421118). No discrepancy with their filing; the corroboration strengthens
M-111's evidence class (two independent traces, same cites).
