# s25 — OQ-S6-1 decode: PartitionRecoveryD generator-set semantics + lostpart.dat / partinfo.dat

Session 25 (DFX25), 2026-10-26. Corpus `03ad46c`. All evidence first-hand
from `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c`
(28,463 lines) + raw vtable dumps from
`og-codebase/PartitionRecovery/PartitionRecoveryD.mo` (objdump + byte-level
.rdata reads).

## 1. CGeneratorSetRegSearch vs CGeneratorSetCompleteSearch — behaviorally IDENTICAL

- Both derive from the SAME chain: `CGeneralGeneratorSetImpl` base ctor
  `FUN_180006d90` (:4028) → three containers (+1/+4/+6); both dtors call
  `CSettingsStore::~CSettingsStore` (:6461-6477 Complete, :6556-6572 Reg).
- **Vtables dumped byte-level from the binary**: CompleteSearch
  @ 0x18002c1a8, RegSearch @ 0x18002c2e8 — both FIVE slots, and slots
  +0x08/+0x10/+0x18/+0x20 are the SAME four functions (0x180006780 /
  0x1800067c0 / 0x1800068a0 / 0x180006960). ONLY slot +0x00 (the dtor
  thunk: 0x18000b150 vs 0x18000b330) differs. There is no overridden
  behavior — the classes are registration identities, not algorithms.
- The mode registry (`FUN_18000d3b0` :7919 — "register generator-set
  factory under mode key in the settings-store map"): **Complete=4**
  (factory thunk 0x18000afa0 :6488), **Reg=1** (0x18000b200 :6583),
  **Fast=2** (0x18000f270 :8747), **Fast1M=3** (0x18000f0a0 :8691).
  The factory thunks are `operator new(0x38)` + the shared ctor —
  nothing else.
- The dispatch consumer (`FUN_18000d210` :7858): looks the set up by
  mode key and calls the registered factory — the mode selects WHICH
  identity is constructed, and since the identities are behaviorally
  identical, **Reg and Complete generate through the same machinery**.
  The "RegSearch = registry-driven second pass with unique recall"
  hypothesis is REFUTED — and the name does not refer to the Windows
  registry at all.

## 2. lostpart.dat / partinfo.dat — fully decoded

**Location** (FUN_180025d30 :23126 + FUN_180025dc0 :23151):
`GetModuleFileNameW` → truncate at the last `\` → append the filename —
**the module's own directory (the DLL dir)**. Not %APPDATA%, not the
registry, not the volume root.

**partinfo.dat** — the cross-run partition-TABLE snapshot:
- Record shape: **0x18 (24) bytes**, magic "Part" (0x74726150) at +0
  (readers FUN_180023120 :21241 / writer FUN_180025f90 :23220).
- Writer (FUN_180025e30 :23171): on the FIRST use in the process
  lifetime (latch at `*param_1 == 0`), **both .dat files are DELETED**,
  then partinfo.dat is rewritten from the CURRENT live table
  (FUN_1800264e0 enumerates it); records append at EOF.
- Reader + comparator (FUN_1800229c0 :20998): loads partinfo.dat,
  enumerates the current live table, and compares record-by-record
  (count, then memcmp of each 0x18 record) → **the layout_changed
  flag**. Consumed by the scan driver FUN_180021e90 (:20704): a layout
  change (or any new candidate) resets the progress counters
  (pool+0xe4/+0xe8).

**lostpart.dat** — the cross-run lost-partition CANDIDATE cache:
- Record shape: **0x448 (1096) bytes**, magic "Part" (readers
  FUN_180023280 :21289 / writer FUN_1800260f0 :23276).
- Per discovered candidate, the found-handler FUN_180020a60 (:20154)
  appends the candidate's 0x448 record to lostpart.dat (:20230).
- At scan start the driver loads it (FUN_180023280, param_3=0); when
  the layout is UNCHANGED, the run's candidates are deduped against
  the cache — match key {+0x28, +0x30, +8} (the start/size/type-ish
  triple; :20817-20840) — and a candidate NOT in the cache sets the
  changed signal → progress reset.

## 3. The doubled block parsers

The RTTI census doubling (CBlockParser{Linux,Win32,CExtX,CFatx} ×2,
CGeneratorSet{Complete,Reg} ×2, ILocationGeneratorSet ×2,
IBlockparserSet ×2, CGeneralGeneratorSetImpl ×2) is consistent with
**two INSTANCES of the whole search stack** (e.g., one per active
search controller / disk), not one parser family per generator set —
the generator-set classes themselves are behaviorally identical, so
they cannot own disjoint parser families.

## 4. Consequences

1. **No second search pass with unique recall exists** — I-020's
   upgrade condition ("upgrades to MAJOR if the decode shows a second
   search pass with unique recall") is REFUTED; I-020 stays the
   acknowledged INFORMATIONAL register.
2. **M-042 CONFIRMED at the mechanism level**: the og's partinfo.dat
   semantics (live table snapshot at save → layout_changed at the next
   run → progress reset) is the same information flow as the core's
   REC_DISK_LAYOUT type 8 ("live table at save, layout_changed surfaced
   at resume, CLI warns, never enforced") — the og uses DLL-dir files,
   the core embeds the same truth in the session. The lostpart.dat
   candidate cache + dedup + revalidation flow matches M-042's
   per-candidate persistence + revalidate-on-resume design.
3. **The core needs no new surface** — no .dat files (the core's
   session-embedded equivalents already landed with M-042; DLL-dir
   cross-run files would be a host-level concern and the og's
   delete-on-first-use makes them per-boot scratch, not durable
   knowledge).
4. Residual (small, honest): which UI surface requests mode 1 vs mode
   4 is not decoded (the mode constant originates outside this module);
   it cannot change the answer — the constructed objects are
   behaviorally identical — but the origin note is recorded for a
   future UI-layer pass.

**OQ-S6-1 RESOLVED — no code change; the evidence file + the I-020
disposition note + the M-042 confirmation addendum are the landing.**
