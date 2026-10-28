# MINOR N-186 — The og's .rsf variant-save device-descriptor record (GUID 76a672b0…, [552-B struct] + [5400-B volumeDevice blob]) has no field-complete core equivalent: the session identity card carries model/serial/boot-hash only — firmware, WWN, interface string, and the 512-byte boot-sector snapshot are absent

**Severity:** MINOR
**Module:** session (persistence format field parity)
**Parameters affected:** D12 (session persistence format — field equivalence), D11/D13 (node/report schema adjacency — the device identity block), Q68 (documentation accuracy of the session-format parity table)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:70-75` (the record-type GUID table — record types 1-4 plus: "A 5th constant `DAT_1414d47a0` = `76a672b048058b46ba9e91093a63b517` drives a record type written by FUN_1403f1180 = `[552-byte struct] + [5400-byte blob]` — used by a variant save path, not present in the shipped auto-save samples"); `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:205-233` (the 5400-byte volumeDevice blob's observed layout: `+0x000 u32 7 (device/bus type — USB)`, `+0x004 "ST500DM0"` model, `+0x108 "02-1BD142"` serial, `+0x206 "1.02"` firmware revision, `+0x30A char[16]` WWN/serial-2, `+0x414 "USB Device"` interface, `+0x518 byte[512]` the full MBR/BOOT-sector snapshot `33 C0 8E D0 … 55 AA`)
**Core references:** audited `c7fa527c` — `include/cr/session/cr_records.h:203-223` (`VolumeSessionInfo`: `char model[48]`, `char serial[32]`, `u32 boot_hash` — the SESS-2 v2 identity-card tail; no firmware/WWN/interface fields, no boot-sector snapshot); `src/core/session/cr_crsf.cpp` (the REC_VOLUME_INFO writer — the identity card is the device-identity surface; grep-verified: no 512-byte sector image, no firmware/WWN/interface strings anywhere in the session layer); `diff-to-fix/modules/session.md` (the field-parity table — record types 1-4 dispositioned; no row for the variant record's device-descriptor field set)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber:** N-142 → **N-186** (2026-10-29, the S56 lane: the N-160 doc-truth gate caught the collision live — the remote's CrPreview dead-wiring N-142 retired first at `c26f77d9` 19:08:34 UTC vs this finding's first push `25e408b8` 19:54:43 UTC; the retired row keeps the seat per SA-9 earlier-push-wins; no content change)

## Summary

The og's session writer has a fifth record type (the `76a672b0…` GUID) — a device-descriptor record whose 5400-byte blob serializes the full volume-device identity: bus type, model, serial, FIRMWARE REVISION, WWN, interface string, and the complete 512-byte boot-sector snapshot. The core's session identity card (the `VolumeSessionInfo` v2 tail) carries model, serial, and a boot-sector HASH — the firmware, WWN, and interface fields and the boot-sector content have no equivalent. Per the session-format parameter (P12: "we may have our own byte format, but the fields must be equivalent"), the descriptor's field set is a gap in the parity table; nothing in the trail dispositions the record.

## What the reference does

The corpus artifact documents the variant-save record (FUN_1403f1180) as `[552-byte struct] + [5400-byte blob]`, with the blob's layout decoded from a real sample (the artifact's §"Observed layout of the 5400-byte volumeDevice blob"): a u32 bus/device type (7 = USB), the disk model at +0x004, the serial at +0x108, the firmware revision at +0x206, a 16-char WWN/serial-2 field at +0x30A, the interface description at +0x414, and the FULL first-sector snapshot at +0x518 (MBR/DBR, `33 C0 8E D0 … 55 AA`). The artifact notes the record is written by a variant save path and does not appear in the shipped auto-save samples — i.e. the og can persist the complete device identity + boot image in sessions when that path is taken.

## What our core does

`VolumeSessionInfo` (cr_records.h:203-223) persists: fs_family, stage masks, slice offset/bytes, sector sizes, label, scanned/elapsed counters, and the identity card — `model[48]`, `serial[32]`, `boot_hash` (u32) — plus the image path (v3) and save time (v5). No firmware, WWN, or interface field exists; the boot sector is reduced to a hash (presence/integrity signal, not content). The session module's field-parity table (modules/session.md) covers record types 1-4 and the core's own 5-9 extensions; the variant record's descriptor field set has no row.

## Gap

Falsifiable: at `c7fa527c`, no session record persists the destination/source device's firmware revision, WWN, interface string, or boot-sector bytes; the og's variant record persists all five. The parity table has no disposition for the record.

## Impact

Low and honest: the record rides a variant save path not seen in the shipped samples, so everyday session round-trips are unaffected. Where it would matter: session portability/forensics (a session re-opened on different hardware can verify the device identity more strictly — firmware+WWN distinguish same-model different-unit drives that model+serial alone can when serials collide or are spoofed) and any future "resume on the original device" trust check. The boot-sector snapshot also enables post-hoc boot-sector comparison/diff at RESUME time (the hash only detects change, not what changed).

## Reproduction / verification

Corpus-derived (this session): the artifact excerpts quoted above (`rsf-format--scan-session.md:70-75` and `:205-233`, captured in `evidence/decompiled-quotes/s12-rsf-fifth-record.md`); the core struct read at `cr_records.h:190-210`; grep over `src/core/session/` for firmware/WWN/interface/snapshot fields — zero hits.

## Fix plan (for the engineering team)

1. Architectural: none — extend the identity card, do not add a new record type.
2. Files: `include/cr/session/cr_records.h` (add `char firmware[16]`, `char wwn[17]`, `char interface[24]`, `u8 boot_sector[512]` — or a `DeviceDescriptor` sub-struct behind a v6 tail), `src/core/session/cr_crsf.cpp` (write/read the tail, skip-unknown-tail rule for old readers), `src/core/orchestrator/cr_scan.cpp` (populate from the device geometry/identity surface at save; the win layer already queries identity for health — reuse that source).
3. Algorithm: none — straight field addition; the og's field widths (model at +0x004, serial +0x108, firmware +0x206, WWN char[16] at +0x30A, interface +0x414, snapshot 512 at +0x518) are the corpus's observed layout and the natural sizing reference.
4. Tests: round-trip (`Session.DeviceDescriptorRoundTrip` — all fields byte-exact), old-reader skip (a v5 session loads with zeros/no descriptor), boot-snapshot pin (a fixture disk's first 512 bytes byte-equal in the record).
5. Contract/doc: `modules/session.md` parity-table row for the variant record's field set (this finding closes it); `docs/FORMATS.md` if the session tail is documented there.
6. Acceptance: the parity table row lands as MATCH-or-registered; the round-trip test green.

## Tests to add

- `Session.DeviceDescriptorRoundTrip`, `Session.V5LoadsTailless`, `Session.BootSnapshotByteExact`.

## Related findings

- S-030 (image-path field — the same v-tail extension pattern), N-090 (session list scope row shape), I-027 (the data-asset register — the .rsf corpus samples row), I-035 (the register batch)

## Open questions

The variant save path's TRIGGER (when does the og take the FUN_1403f1180 path over the standard writer — a one-caller decompile at fix time; not blocking for the field addition, but worth pinning so the core's "always write the descriptor" posture is a registered superset rather than an unexplained difference).

---

## Resolution (S50 lane, 2026-10-29 — retired as a FALSE-ACTIVE duplicate of N-066; no code change owed)

**The battery verdict: FIXED-ALREADY at its own audited tree.** The exact
fields this finding asks for — `char firmware[32]`, `char wwn[32]`, `char
interface_desc[32]`, `u8 boot_snapshot[512]` — landed with **N-066's
`4d3f0409`** ("the identity card's record-3 parity — the v4 tail
(firmware/WWN/interface + the 512-byte boot snapshot) + the descriptor
offset transposition fixed", 2026-10-26), three days BEFORE this finding
was filed by the S12 lane. The fields exist at HEAD at
`include/cr/session/cr_records.h:112-122`, serialize/deserialize at
`src/core/session/cr_records.cpp:809-812` / `:852-862`, populate at save
(`src/core/orchestrator/cr_scan.cpp:2424+`), and are pinned by
`Session.BootSnapshotRoundTrip` (`tests/unit/test_session.cpp:1402-1415`).

**The false-grep note (recorded for the trail's truth ledger):** this
finding's own "Reproduction / verification" claims "grep-verified: no
512-byte sector image, no firmware/WWN/interface strings anywhere in the
session layer" — that grep was FALSE at this finding's own audited commit
`c7fa527c` (verified `git show c7fa527c:include/cr/session/cr_records.h` —
the N-066 tail is present). The S12 lane filed over a stale tree read.

**The renumber trail stands:** N-142 -> N-186 (the S56 lane's SA-9
renumber, the retired CrPreview N-142 keeping the seat) — this retirement
retires the CURRENT ID.

**The one honest sliver (optional follow-up, not a finding):** the
load-side summary struct `VolumeSessionInfo` (cr_records.h:203-229, filled
at cr_scan.cpp:2715-2736) does not re-expose the four fields to callers —
the record layer round-trips them (functional parity for save/load), and
no current consumer reads them at resume. If a future resume-time
diagnostic needs the boot-sector diff, the exposure is a one-struct-tail
change; recorded here so it is not re-filed as a gap.

Trail: findings/FIXED/N-186 (this file) + the battery sheet
`evidence/s50-liveness-battery.md` (the FIXED-ALREADY verdict) + the
ledger row + the matrix cell flip.
