# Roadmap

## v0.1 (shipped) - the engine layer
- Device/partition layer: enumeration, geometry (4Kn/512e), GPT/MBR/EBR,
  PartitionSlice, read-error map. DONE
- NTFS engine: bootstrap, MFT parser, fixups, $ATTRIBUTE_LIST, hard links,
  ADS, $Bitmap viability grading, post-format remnant sweep. DONE
- Lost partition discovery: signature scan, format probes, scoring,
  virtual candidates. DONE
- RAW carving: 16-format registry, grammar validators, dedup, hashing. DONE
- FAT + exFAT engines (spec-exact). DONE
- Orchestrator: staged scans, merge, weighted progress, sessions. DONE
- Safe recovery writer: overlap refusal, collisions, atomic commits. DONE
- BitLocker-aware handling: detect + legitimate unlock only. DONE
- Windows platform layer + rrecov CLI. DONE

## v0.2 (shipped) - the complete core
- Parallelism: worker pool, thread-safe result tree, parallel carve /
  remnant sweeps / lost-partition candidates (extends the corpus's
  host-fans-out model; the corpus also shows engine-side workers - FAT
  QuickSearchWorker, FATSearchDLL:26931). DONE
- Full RDA driver integration: transport with the F11 Information=Lengthx512
  contract, GET_INFO/OPEN_TARGET/GET_VERSION IOCTLs, bad-sector-isolating
  retry layer with ReadErrorMap. DONE
- ext2/3/4 engine (superblock gate, GDT, extents, deleted sweep, journal
  salvage). DONE
- HFS+/HFSX engine (catalog B-tree, forks, overflow extents, freed-node
  orphan sweep). DONE
- Btrfs engine (mirror superblock select, chunk mapping, subvolume trees). DONE
- ZFS engine (label nvlist, uberblock ring, MOS/DSL/ZPL walk). DONE
- ReFS FULL engine (MSB-chain reconstruction: superblock -> dual
  checkpoints -> ObjectID table -> directory tables -> embedded file
  tables -> $DATA data runs; ID2 + deleted harvest; allocator-bitmap
  viability grading). DONE
- PST/OST analyser (robust validation) + RAR5 analyser. DONE
- Carve format set extended to 30 (DNG/ORF/RAF/RW2/PSD/MP3/WAV/M4A/ASF/
  FLV/CAB/ISO added). DONE
- RawFixer: transactional filesystem repair (NTFS/FAT/exFAT boot repair,
  checksum recompute, geometry rederivation; S1-S6 safety standard;
  sector-coalesced writes, full rollback, durable undo log). DONE
- SMART disk health (ATA passthrough, NVMe health log, TRIM state, SSD
  heuristics) + `rrecov health`. DONE
- LDM dynamic-disk layer: full database decode (PRIVHEAD / TOCBLOCK / VMDB,
  VBLK records incl. composite groups) and dynamic-volume reconstruction
  (simple / spanned / striped / mirrored / RAID-5 extent mapping) exposed
  as read-only virtual devices. DONE (v0.2 shipped recognition; full
  decode + reconstruction + cross-disk resolver landed - ReFS/LDM are
  first-class).
- `rrecov fix` / `rrecov bench` commands. DONE
- Structure-aware fuzzing: 4 libFuzzer targets (pure primitives, engines,
  carve pipeline, session containers) + CI fuzz job. DONE
- Recovery benchmark (benchmark.yml): seeded VHD, RDA driver, ground-truth
  recall/precision scoring across recycle/permanent/wipe/folder scenarios. DONE
- EaseUS comparison (comparison.yml): og DRW vs rrecov on the same seeded
  disk, .rsf extraction (byte-exact validated), side-by-side reports. DONE
- Full-codebase security audit: all P0/P1/P2 findings fixed (session
  deserialization OOB, GPT CRC length walk, NTFS resident-attribute wraps,
  ZFS nblkptr, ext4 extent bounds, exFAT recursion depth, decompression
  bomb, rollback integrity, DoS caps). DONE

## Deferred (by decision - REMOTE features only)
- NAS redirector (EuMrx-class) and cloud recovery (cloud_session/EuICloud)
- Any remote / network recovery feature

## v0.3 candidates (local features, tracked - not remote-deferred)
- Wizard UI (the DRWUI-equivalent surface) on the engine layer — CORE-SIDE
  SURFACE LANDED (cr_ui C-ABI shim + snapshots + query + auto-checkpoint +
  tree --json, issues #1/#2/#8/#9 closed; the wizard front-end itself lives
  in the rda-wizard repo)
- File repair tools (video/PDF/office) - corpus Fixo chain
- WinPE boot media builder
- Surface test / disk wipe utilities (ToolsModule-class)
- VM disk-image container mounting (libvmdk-class VMDK/VDI readers; the
  benchmark layer already attaches VHD/VHDX through the OS today)
