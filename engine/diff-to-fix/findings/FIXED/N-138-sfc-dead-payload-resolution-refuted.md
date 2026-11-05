# MINOR N-138 — the OQ-S10-2 "dead payload" resolution is REFUTED: VideoViewer/sfc is VideoFileRepairer.exe's LIVE encrypted engine-init database (magic + XOR key + custom LZ, all decoded; the "decisive probe" could not match DAT_ symbols or hex constants)

**Severity:** MINOR (audit-trail truth: a false-negative resolution on the board; no product behavior affected — the payload is repair-family either way — but the recorded reason is wrong and would mislead every future sweep)
**Module:** audit-trail / og-tree disposition register (OQ-S10-2, N-134's addendum)
**Parameters affected:** A68 (documentation accuracy — the resolution record), A69-adjacent (register truth), P54-adjacent (the disposition class itself: dead-shipped → live-OOS-machinery)
**Corpus references:** `re-workspace/reversed/ghidra-wave2/VideoFileRepairer_decompiled.c:248131` (loader `FUN_00514f80`: builds `<dir>\sfc` from `DAT_0059d24c`="sfc" at :248187-248208), `:248309` (magic check `*piVar1 == 0x73666320`), `:248313` (decrypt `FUN_00517640(buf+4, size-4)`), `:248357-248364` (version-nibble dispatch → v2), `:246026-246300` (the v2 custom-LZ parser `FUN_00512fa0`), `:249149`+`:249399` (builder `FUN_00516270`, "wb", magic write), `:249499` (record serializer `FUN_00516a70`), `:245536`+`:246921-246931` (the repair-engine ctor `FUN_005126f0`: load at startup, rebuild on failure), `:249963-249966` (the XOR key dwords `0x9760584c/0x2e4e4146/0x938ce698/0x8b2a8111`); `og-codebase/VideoViewer/sfc` (211,614 bytes; head `20 63 66 73 …`); objdump on the exe: the key as immediates at VMA `0x517658-0x51766d`
**Core references:** audited `46b81573` — the closed OQ record `diff-to-fix/findings/FIXED/OQ-S10-2-videoviewer-sfc-unknown-payload.md` (its RESOLUTION section + its "Status: CLOSED" line carry the false verdict, commit `4f44db49`); `diff-to-fix/findings/MINOR/N-134-product-frame-register-completion.md` (its S10-F ADDENDUM registers the sfc as "dead-shipped installer-chain payload")
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The S10-F wave-2 lane closed OQ-S10-2 with "NO consumer exists anywhere in the
reversed corpus; the file is dead payload", based on `grep -l "sfc\|cfs"` over
the decompiles returning zero hits. That probe structurally cannot find this
consumer: the filename appears in the decompile only as the `DAT_0059d24c`
symbol (the string lives in .data), and the magic appears only as the hex
constant `0x73666320` — neither contains the ASCII substring "sfc"/"cfs". The
consumer is VideoFileRepairer.exe itself: its repair-engine ctor loads
`<dir>\sfc` at startup, decrypts it with a 16-byte XOR key, decompresses a
custom-LZ stream, and rebuilds the file when the load fails. The OQ's
hypothesis 1 (a live data asset of the video-repair line) was right; the
resolution recorded hypothesis 3 (dead payload) — a false negative.

## What the reference does

The full decoded chain (every cite first-hand, byte-verified):
- Container: magic `20 63 66 73` (" cfs") + payload XOR-obfuscated with
  `4C 58 60 97 46 41 4E 2E 98 E6 8C 93 11 81 2A 8B` (decompile
  `:249963-249966`; objdump immediates). The "FAN." motif the OQ noted is the
  key showing through zero-plaintext runs (the file carries `46 41 4E 2E`
  verbatim at offset 8-11 because those plaintext bytes are zero).
- Decoded header: u32 LE uncompressed size `0x000CA061` = 827,489 bytes
  (3.91× ratio), then a version byte `0x25` whose high nibble selects the v2
  parser (`:248357-248364`).
- v2 parser `FUN_00512fa0`: a custom LZ decompressor (varint literal
  lengths, 64-byte blocks, match tokens, `:246026-246300`).
- Consumer: the repair-engine ctor `FUN_005126f0` (`:245536`) calls the
  loader at startup and REBUILDS the database via the builder pair
  (`:246921-246931`) when loading fails — live machinery, self-healing.

## What our core does

n/a (core-side product code is not implicated — this is an audit-record
correction). The board record: the OQ file's RESOLUTION section claims the
zero-hit grep proves no consumer; N-134's addendum registers the payload as
"dead-shipped installer-chain payload (the PE_Tools/dlconfig class)".

## Gap

Falsifiable: `grep -n "0x73666320\|9760584c" ghidra-wave2/VideoFileRepairer_decompiled.c`
returns `:248309`, `:249399`, `:249963` — the consumer chain quoted above. The
closed OQ's central claim ("NO consumer exists anywhere in the reversed
corpus") is contradicted by the corpus's own decompile. The "no clean u32
length framing" note is also refuted: the framing is clean once decrypted
(`61 a0 0c 00` = 827,489 LE).

## Impact

Roadmap trust: the dead-payload verdict would be cited by every future
register sweep ("sfc = dead, stop looking"). The truth (a live, encrypted,
self-rebuilding engine-init database of the OOS repair line) changes nothing
in core scope — the registration class stays OOS-repair (I-010/D-26) — but
the recorded reason and the decode are materially different, and the probe
methodology lesson (DAT_-symbol and hex-constant blindness in string greps)
applies to every future "no consumer" claim on this board.

## Reproduction / verification

Executed + decompiled (strength 1 for the decode, §1-2 of the evidence):
`evidence/decompiled-quotes/s11-sfc-payload-decode.md` — the byte-level
container decode (od/python XOR with the decompile's key), the methodology
autopsy of the zero-hit grep, and the full consumer chain with every address.

## Fix plan (for the engineering team)

1. `diff-to-fix/findings/FIXED/OQ-S10-2-...md`: append a REFUTATION addendum
   (this finding's ID + the evidence file) marking the "dead payload" verdict
   superseded — the question's correct answer is hypothesis 1, live repair-line
   database, fully decoded. (Led by the audit lane; recorded here as the plan
   so the trail owns it.)
2. `N-134`'s S10-F addendum: correct the disposition line to "live encrypted
   engine-init database of the OOS repair line (VideoFileRepairer.exe ctor
   load + rebuild-on-failure), decoded — see N-138's evidence"; the register
   row's class (repair-family OOS) is unchanged.
3. docs/PARITY-OG-CORE-INVENTORY.md's sfc row (when the fix lane lands it):
   carry the corrected wording, not "dead".
4. Methodology note for future probes (CORPUS-ERRATA or the sweep SOP):
   consumer searches must grep the MAGIC AS HEX (`0x73666320`), the import
   names, AND run a strings pass on the binary itself — never ASCII-only over
   decompiled text.
5. Acceptance: the OQ file, N-134, and the register all agree on the decoded
   truth; no future sweep re-derives "dead payload".

## Tests to add

None (no product behavior change; the acceptance is documentation truth).

## Related findings

- N-130 (VideoFileRepairer.exe's own disposition — the consumer this decode
  names; its "opaque payload" open question closes with this decode)
- N-134 (the register batch carrying the wrong disposition)
- N-135 (the resolution-record truth class this belongs to — the N-103
  precedent: a resolution that claims more than it verified)

## Open questions

The database's record semantics (what the 827,489 bytes contain, beyond the
container grammar) would need the ~275-line LZ decompressor ported to
enumerate — registered as the non-blocking residual in the evidence file;
nothing in core scope depends on it.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-130 + N-138 landing — recorded post-push in the ledger row)
**Verified by:** the three fix-plan sites all current: (1) the register's new VideoFileRepairer.exe row names the sfc as the binary's LIVE encrypted engine-init database (magic " cfs" + the 16-byte XOR key + custom-LZ, the N-138 cites); (2) the FIXED-LEDGER's OQ-S10-2 row carries the SUPERSEDED verdict with the decode summary; (3) the CROSS-REF-MATRIX's refutation row (:724) and the README note (:15) — landed by the audit lane — re-verified standing.
**Resolution notes:** The false-negative resolution is corrected in every live trail site. The refuted verdict's root cause (the grep probe structurally cannot match DAT_ symbols or hex constants) is preserved as the methodology note the matrix row already carries. The disposition class flips dead-shipped → live-OOS-machinery (the repair line stays OOS either way — no product behavior affected). The retired OQ-S10-2 file itself stays in history as the dated record; the ledger row is the navigable truth and now points at the refutation.
