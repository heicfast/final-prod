# S50 evidence — the 25-landing corpus-parity sweep (the regression watch)

**Lane:** S50 continuation audit lane (2026-10-29). **Window:** every fix
landing in `869da779..0d7b1d9d` (the 92-commit window since the last
battery pin — the S42-C sweep's pin predates all 25). **Pins:** core
`93461483` (code-identical to `0d7b1d9d`), corpus `be0cfe9`.
**Method:** per landing — the Resolution's claims re-read, the landed core
code re-read at HEAD, the cited og decompile re-read first-hand, verdict
PARITY / DIVERGENCE / LIGHT-CHECK-OK. The three DIVERGENCE sites were
additionally re-verified by the lead line-by-line on both sides (see the
sibling sheet `s50-landing-divergences.md` and the S-062/M-138/M-139
finding files).

## Verdict table

| # | ID | Landing (commit) | Verdict | One-line evidence |
|---|---|---|---|---|
| 1 | A-013 | resume never truncates the source (4f0fc160) | PARITY | CliCkSink stages `<path>.tmp` + rename; the final/resume path never opened for truncation before the read (rrecov.cpp:999-1035/:1651/:1677) |
| 2 | S-048 | Reconstructed per-volume carve tree (6b75b40e) | PARITY | root "Reconstructed" + the 7 categories in the og's id order 5..11 (DRWUI:635216-635271 `for (local_1c8 = 5; local_1c8 < 0xc; ...)`) — EXACT match incl. order |
| 3 | S-049 | universal carve naming (ee1c6760) | PARITY | the %06d/%d crossover, all 8 content prefixes, the 254 cap = og 0xff (DRWUI FUN_140c44770 :1878330-81) — one residual edge filed N-187 (the 39-char EXIF cap) |
| 4 | S-054 | destination-overlap restore (a3741812) | PARITY | both production sites feed ALL extents (rrecov.cpp:2334-2358, cr_ui.cpp:4145-4157); fence tests present |
| 5 | M-076 | TRIM chip (4bd5ee7a) | PARITY | opt-in _ex flag + per-row chip semantics = the UIUX/07 decode |
| 6 | M-098+N-156 | HIK discriminator + PS BE lengths (c0efacc3) | **DIVERGENCE** | the og's discriminator constant is -0x43ff0000 = **00 00 01 BC** (both probe sites :30438/:30452); the core probes **0xBD** — filed **M-138**. The N-156 half (BE lengths) is PARITY |
| 7 | M-099 | AEP analyser (43c618a3) | PARITY | bare-RIFX weak grammar, 0x1e00000 cap, XMP-end walk, 0x200 stride, "AEP" prefix byte-dumped from the og binary — all match (:11688+) |
| 8 | M-103 | LostDir grouping (3cc22c52) | PARITY | LostDir%llu per-parent + "File Path Lost" (NTFSSearchDLL:48915-48924, DRWUI:320998-321005) |
| 9 | M-106 | ONE/3DS walks (c98eb7a7) | **DIVERGENCE** | on the two size-gate failure exits the og publishes 2x-device flat (uVar5=uVar1 clobber, :52316-52358, Ghidra+retdec agree); the core keeps the last-valid cursor (0 on first chunk) — filed **S-062** |
| 10 | M-112 | CSL CD01 size branch (73a134eb) | **DIVERGENCE** | the branch itself is parity, but the og's per-candidate latch (+0x120) kills the end-tag branch for latched CD01 candidates and a declined 0x44c read leaves them unsized (dropped); the core falls back to the </svg> walk and carves — the landed "retries other sizing" claim is contradicted by the decompile — filed **M-139** |
| 11 | M-116 | archive C-ABI (3d886577) | PARITY | two-call pattern + archive-list verb + the 512-entry hostile bound |
| 12 | M-121 | Dbx analyser (3ac4b99c) | PARITY | JMF6 key, 4 OE store GUIDs, marked 16-byte pattern, bisection re-traced by hand, empty name, 3 registry rows (:17509-17690/:2192-2232) |
| 13 | M-125 | progress display math (f368f81c) | PARITY | the piecewise remap is a verbatim port of FUN_1401af820 (:314929-59) incl. the 99.99 dead-cap; engine permille untouched. Registry defect found: the D-118 numbering collision — filed **N-188** |
| 14 | N-139 | carve OOM policy (133cced9) | PARITY | graceful codes on every allocation path; the og kills the process (RaiseException 0xe0000001 :979) — the deliberate divergence is registered (D-115) |
| 15 | N-143 | id-8 demote declined (cb93adf9) | LIGHT-CHECK-OK | D-118 row exists at PARITY-DELTAS.md:3602; pin test Carve.SpecialRejectPlainAcceptRefusedByName present; og cite spot-verified (:9877-9883/:10020-10039) |
| 16 | N-146 | FH11 re-home (2c129efc) | PARITY | predicate exact vs FUN_180029f30 :32612-32633 (stamps 0x1d, no size computation) |
| 17 | N-149 | always-on exempt set (996b8fd7) | PARITY | mask 0x1000801 over ids 0x30/0x3b/0x48 (:4412-4418) — exact |
| 18 | N-152 | DXF 999 entry key (4f8fa661) | PARITY | the 999 branch exact vs FUN_1800376f0 :42258-42293. Footnote: the CANONICAL branch is looser than the og's full 16-byte gate — filed **N-186** |
| 19 | N-155 | HFS+ totalBlocks fallback (09cf54bd) | PARITY | data-fork-only 4096-shift fallback vs FUN_180009940 :7665-7672 exact |
| 20 | N-162 | FAT u64 width registered (2d41f755) | LIGHT-CHECK-OK | D-119 row exists (:3623); pin test present; og casts spot-verified (:13194/:11239) |
| 21 | N-164 | GPT strictness registered (39e81d5f) | LIGHT-CHECK-OK | D-116 row exists (:3565); code matches the stance; og cite relocated first-hand (ghidra-mo/Partition_decompiled.c:4530-4541/:4703) |
| 22 | N-168 | user-sign bound registered (30bc8b79) | LIGHT-CHECK-OK | D-117 row exists (:3585); the 2x-guard og cite spot-verified (DRWUI:1003371) |
| 23 | N-172 | '#' placeholder (afa2adff) | PARITY | 0xE5 -> '#' (0x23) exact vs FATSearchDLL:44166-44172; the LFN-recovery superset registered D-120 |
| 24 | N-173 | 2107 date repair (98079347) | PARITY | DosDateTimeToFileTime zero-on-fail semantics preserved (FATSearchDLL:32135/:32144/:32151); u64 end-to-end |
| 25 | N-179 | ext-length re-truth (40e46f86) | LIGHT-CHECK-OK | re-measured over og-codebase/EUUserData first-hand: 111 keys, longest 8 ("manifest") — the comment is TRUE |

## Summary

**17 PARITY · 3 DIVERGENCE (M-098's HIK byte -> filed M-138; M-106's ONE
exits -> filed S-062; M-112's CSL latch -> filed M-139) · 5 LIGHT-CHECK-OK
· 0 NOT-VERIFIABLE.** Secondary defects surfaced and filed: the DXF canonical
gate looseness (N-186), the EXIF 39-char cap residual (N-187), the
PARITY-DELTAS D-118 numbering collision (N-188). Cosmetic residue (not
filed, recorded here for the N-180 doc-truth batch): `validate_mpegps`'s
BE-length comments cite "N-155" where the landed finding is N-156
(cr_analysers.cpp, the N-156 half of c0efacc3).

The 3 divergences are all in the fix lanes' own newest code — the regression
watch's exact predicted class ("new code is the newest divergence source"):
each was a MAJOR-tier stand-in repair whose og decode was mistranscribed at
one constant (M-098), one exit-value family (M-106), or one control-flow
structure (M-112).
