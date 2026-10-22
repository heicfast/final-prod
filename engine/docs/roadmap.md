# Roadmap

## v0.1 (this release) - the engine layer
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

## v0.2
- Wizard UI (the DRWUI-equivalent surface) on the engine layer
- PST/OST carving after robust validation; RAR5; more RAW formats
- LDM/Dynamic disk recognition; ReFS detection (report-only)
- benchmark harness + regression baselines
- fuzzing campaigns on the parsers (structure-aware, libFuzzer)
- Windows-7-inclusive runtime test matrix vs the RDA driver CI

## Deferred (by decision)
- NAS redirector (EuMrx), filesystem repair writers (RawFixer-class),
  RAID/VM/cloud, ext4/HFS+/Btrfs/ZFS engines, file repair tools,
  full-disk decryption of any kind.
