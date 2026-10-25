# INFORMATIONAL I-022 — Session-5 core-superset registers (things the core does that the og does not — deliberate, keep)

**Severity:** INFORMATIONAL (grouped registrations)
**Modules:** session, carve, btrfs, zfs, transport
**Parameters affected:** D12 (session format — core protections), A27 (fragment handling), Q67 (sanitizer hygiene)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` + `rsf-record4--tree-node-grammar.md` (the og's loader has NO record CRC, no resync, no bomb caps — first full read this session); `ifileproperties-part3--btrfs-zfs-raw-generic.md` (the og's Btrfs compressed extents persist as an opaque raw buffer with a dual-shape switch; ZFS runlist polymorphic but unvalidated)
**Core references:** `src/core/session/cr_records.cpp` (per-record CRC32 + F10 resync + bomb caps), `src/core/btrfs/cr_btrfs_engine.cpp` (structured codec/psize extents), `src/core/zfs/cr_zfs_engine.cpp` (decoded absolute-extent runlists + allocated_size) — audited commit `7cf6754`
**Status:** READ-ONLY REGISTRATION — no fix applied

## Summary

The session-5 deep read of the never-cited SaveScan artifacts confirmed several core-side
supersets over the og. These are deliberate hardening/expressiveness advantages — registered
so a future "og-parity only" pass does not remove them.

## Registrations

1. **Session robustness supersets:** per-record CRC32, F10 record resync, session bomb
   caps — none exist in the og's loader (rsf-record4's validation is per-field only). Keep.
2. **Btrfs compressed extents:** the core persists structured codec/psize extents; the og
   persists an opaque buffer behind a dual-shape switch (+0xB5). Core is more expressive;
   keep (the og's shape is documented in M-0xx/N-059's carriers for parity reference).
3. **ZFS runlist:** decoded absolute extents + allocated_size superset vs the og's
   polymorphic ZfsDataRunlist; keep.
4. **Naming wire:** single UTF-16LE name field vs the og's per-class u8/u16/u32/u64-length
   mix — deliberate unification; keep (documented in the session contract).
5. **`changed` time + `allocated_size` props** — core superset fields with no og carrier;
   keep.
6. **ISO9660 carve + RAR5 + M4A + ZIP-CD/naked-MOV anchors** — core-only carve formats
   (S-007's register; re-confirmed by the family-artifact sweep this session).

## Action

Register rows in `docs/parity/PARITY-DELTAS.md`; no engine changes.

## Related findings

- N-058 (the loader deltas that are NOT supersets — the fix side of the same read)
- I-009/I-010 (prior superset/scope registers)

## Open questions

None.
