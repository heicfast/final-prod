# SEVERE S-002 — HFS+ Attributes B-tree not walked: decmpfs-compressed files and named forks unrecoverable

**Severity:** SEVERE
**Module:** hfsplus
**Parameters affected:** B2 (recovery correctness — content of compressed files), B7 (named-fork/alternate-stream handling), A26 (tree walk — missing tree)
**Corpus references:** `re-workspace/artifacts/SearchDLL/hfsplus-engine--deep-dive.md` :12, :42 (`CHFSPlusAttribute` — the Attributes B-tree (named forks/compression decmpfs support)); `re-workspace/artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md` (HFS+ section: embedded hfspluslib `attribute.cpp`); decompiled `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c` (665 functions incl. the attribute-tree walker family)
**Core references:** `src/core/hfsplus/cr_hfsplus_engine.cpp` — `CNID_ATTRIBUTES_FILE = 8` defined at :43 and **never used** (grep: single occurrence); no attribute-tree walk exists — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The reference engine walks the HFS+ **Attributes B-tree** (cnid 8) to resolve named forks and
**decmpfs-compressed file content** — the mechanism macOS uses for file compression (app
bundles, system files, and any user file compressed via `afsctool`/APFS-migrated content).
Our core defines the attributes-file CNID constant but never opens or walks the tree. On any
volume containing decmpfs-compressed files, the og recovers their content and we recover an
empty (or tiny) data fork — the file appears present but recovers nothing useful.

## What the reference does

The og embeds the full hfspluslib source set — `volume/btree/btnode/btreeoption/catalog/
extents/attribute/journal/rawfile` (deep-dive :6, verified quote above). `CHFSPlusAttribute`
(:42) walks the Attributes B-tree; decmpfs-compressed files store their compressed payload
(or an inline reference) in a named attribute (`com.apple.decmpfs`) on that tree, and the
engine surfaces the decompressed content through it. Named forks (user extended attributes
with fork semantics) surface the same way.

## What our core does

`src/core/hfsplus/cr_hfsplus_engine.cpp:43` defines `CNID_ATTRIBUTES_FILE = 8`; a grep over
the whole module shows the constant is referenced nowhere else. The engine walks the Catalog
B-tree (leaf chain, cnid parent reconstruction) and the Extents Overflow B-tree, parses
resource forks (`..namedfork/rsrc`, :535–546), and parses `journal_info_block` (:356) — but
has no attribute-tree reader and no decmpfs handling. A decmpfs-compressed catalog record
typically shows a zero-length data fork plus the compression attribute we never read.

## Gap

Falsifiable claim: for an HFS+ volume containing a decmpfs-compressed file (data fork
length 0, `com.apple.decmpfs` attribute present), the og's recovered output contains the
decompressed content and our core's output contains a 0-byte file.

## Impact

- **Recovery correctness:** macOS-compressed files (a large share of system/app content on
  HFS+ volumes) recover as empty shells. This is silent-ish data loss from the user's point
  of view — the file is listed, recovers "successfully", and is empty.
- Named-fork/extended-attribute content is absent from results.

## Reproduction / verification

- Corpus-derived: artifact quotes (deep-dive :6/:42, remaining-engines HFS+ section listing
  `attribute.cpp` in the embedded source set).
- Core-derived: constant-unused grep result; module read (agent D sheet: "no
  hard-links/attrs/journal").
- Executed: not run (fixture cost); construct an HFS+ image with `afsctool`-style compressed
  file → assert 0-byte recovery (current) vs decompressed content (expected after fix).

## Fix plan

1. Add an attributes-tree walker alongside the catalog/extents walkers in
   `src/core/hfsplus/cr_hfsplus_engine.cpp`: open cnid 8 fork from the volume header,
   B-tree walk (reuse the existing btnode machinery), key type = attribute name keys.
2. Implement `com.apple.decmpfs` resolution: both storage forms —
   `data[:4] == magic, [4] == 3` → inline ZLIB payload in the attribute value; `[4] == 4` →
   the payload lives in the file's **resource fork** at a computed offset (ZLIB stream).
   Link zlib (already required by the build).
3. At emission time, for catalog records whose data fork is empty and a decmpfs attribute
   exists: synthesize extents from the decompressed buffer (in-memory blob, resident-style)
   or recover-into-temp — match the og's observable (content present) with the core's
   extent model.
4. Surface other named attributes as child nodes (`..namedfork/<name>` convention) — parity
   with the resource-fork convention already implemented.
5. Tests: `test_hfsplus_volume.cpp::Decmpfs_Inline_Zlib_Content_Recovered`;
   `Decmpfs_ResourceFork_Payload_Content_Recovered`; `NamedFork_Surfaces_As_Child_Node`.

## Tests to add

(as in fix plan step 5 — all fail before, pass after)

## Related findings

- S-003 (journal replay — the other missing HFS+ subsystem).

## Open questions

Whether the og surfaces non-decmpfs extended attributes as nodes or ignores them is not
decoded; the fix plan surfaces them (superset is acceptable if registered).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for `fix: S-002`)
**Verified by:** runtime fail-before/pass-after — with only the engine .cpp
reverted (header kept so the tests compile), `HfsPlus.DecmpfsInlineAndRsrc-
ContentRecovered` fails at every assertion line (decmpfs_files, named_forks,
attr_unsupported, attr_records all 0/absent); restored, it passes end-to-end:
mode-3 and mode-4 extents decode to the EXACT original content through the
writer's own `compress::decompress_block` path (the recovery-time contract,
not an in-test shortcut), the named-fork child carries the raw value bytes,
and `HfsPlus.DecmpfsDisabledLeavesEmptyShell` pins the flag-off mode. Suite
291/291 (Linux ASan); the existing fuzz harness (fuzz_hfsplus.cpp) drives
scan_hfsplus, so the new attribute parser is inside the structure-aware
fuzz surface.
**Resolution notes:** first-hand decompile verification replaced the thin
audit trail: the og looks up "com.apple.decmpfs" in the attributes B-tree
via a key whose keyLength COUNTS ITSELF (=(nameLen+7)*2, FUN_180009c10
:7771), gates on attribute recordType == 0x10 (inline data, :7825 — the
"unsupported attribute node format" log branch confirms non-0x10 records
are skipped, which the core now replicates with a stats counter), reads
size@+12 with the payload at +16 (:7826-7829), and extracts the value's u64
at +8 in one native load (:7841 — the value fields are little-endian x86
structs, unlike the big-endian HFS+ structures around them). Implementation:
the attributes B-tree leaf chain is walked once per scan collecting
{cnid, name, on-disk value position, decmpfs facts}; decmpfs files emit a
CODEC-TAGGED EXTENT (new compress::kZlib, RFC-1950) instead of a memory
blob — mode 3 points into the attributes-B-tree node where the zlib stream
lives, mode 4 points into the resource fork at the recorded offset — so
the recovery writer inflates at write time through the exact contract it
already uses for ZFS/Btrfs/NTFS compressed extents (no memory-resident
blob, no session bloat, any file size below the writer's 256 MiB guard —
that guard is pre-existing P1-family discipline, noted). Named attributes
surface as `..namedfork/<name>` children (D-46 superset registered in
docs/PARITY-DELTAS.md per the finding's own instruction). The mode-4
on-disk semantics implemented: value = {magic LE, mode=4, uncompressed
size u32 LE, resource-fork offset u32 LE}; the mode-3 zlib stream starts
at value+12. HfsScanFlags::resolve_decmpfs/named_forks (default on) gate
both surfaces.
