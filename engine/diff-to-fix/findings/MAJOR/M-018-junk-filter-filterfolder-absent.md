# MAJOR M-018 — Result-set junk filter (FilterFolder.ini family) absent from the core and unregistered

**Severity:** MAJOR (user-visible result-set difference on every scan; borders SEVERE for og-comparison inventories)
**Module:** orchestration / session (result-set policy), cross-cutting
**Parameters affected:** B1 (result-set membership), D16 (filter model), D13-adjacent
**Corpus references:** `re-workspace/artifacts/config-files/filtering-priority--config-layer.md` §3 (uncited by session 1) — the FilterFolder.ini family: hidden folders `;Config.Msi;$Windows.~BT;$Windows.~WS;$Windows.~TMP;`; hidden files `desktop.ini;._DS_Store;.DS_Store;$UsnJrnl;IndexerVolumeGuid;WPSettings.dat;MountPointManagerRemoteDatabase`; localized "New Folder" names (新建文件夹 / Neuer Ordner / … 10 locales) + `System Volume Information` + BitLocker artifacts (`AadRecoveryPasswordDelete;ClientRecoveryPasswordRotation;FveDecryptedVolumeFolder`); wired at scan setup — `re-workspace/raw-evidence/ui-interaction-recovery-flow/applog_log_DRWUI.log:22:57:57.920` `run to CDoScanPartition::PushFolderFilter:84`; config inventory `config-files/config-data-reference.md:36-41` ("hide OS junk from results")
**Core references:** no equivalent anywhere — `grep -rn -i "filterfolder\|desktop.ini\|UsnJrnl" src/ include/` clean; `cr_ui` query filters are user-specified only; no registration in `docs/parity/PARITY-DELTAS.md` or any GAP list — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og applies a default result-set junk filter wired at scan setup (PushFolderFilter): OS droppings, metadata files, localized "New Folder" names, System Volume Information, and BitLocker password-rotation artifacts are hidden from the presented tree. The core emits all of them. The difference is user-visible on essentially every scan of a real Windows volume, contaminates every og-vs-core inventory comparison (the exact mechanical divergence class the S-001 analysis warned about), and — uniquely in this audit — was never registered anywhere as a deliberate deviation.

## What the reference does

The artifact §3 lists the exact hidden sets (quoted above); the DRWUI log line proves the filter is pushed per scan-partition during real runs; `config-data-reference.md` documents the intent ("hide OS junk from results").

## What our core does

No default filter of any kind; results are the raw emitted tree. The `cr_ui` filter API is user-predicate driven only.

## Gap

Falsifiable claim: scanning a volume containing `desktop.ini`, `$UsnJrnl`, `System Volume Information`, and a Chinese-locale `新建文件夹`: the og's presented tree hides all four; the core's tree (CLI `--json`, UI query, recover-all writer) contains all four.

## Impact

- **User-visible noise** on every real-world scan.
- **Bench parity:** every comparative inventory counts these as core-only entries — precision metrics diverge mechanically.
- **Recover-all writes junk** the og would not have offered.

## Reproduction / verification

Corpus docs + live-run log line quoted (agent S2-F; `evidence/decompiled-quotes/carve-rawfixer-bitlocker-session2.md` carries the excerpt); core absence grep-verified first-hand.

## Fix plan

1. Add a default result-set filter pass in the scan pipeline (post-walk, pre-emission): the FilterFolder.ini sets above, including the 10-locale "New Folder" list, System Volume Information, and the three BitLocker artifact names.
2. Ship the lists as data (a versioned ini/json beside the engine defaults), not code literals — the og loads them from config (`GetPrivateProfileSectionA` on FilterFolder.ini; the core's RawConfig loader already reads shipped config data).
3. Register the policy in `docs/parity/PARITY-DELTAS.md` either way: if the product decides NOT to hide junk, that is a legitimate deviation — but then bench `compare.py` must filter these names from og comparisons. Today it is neither implemented nor registered.
4. Add a scan stat `junk_filtered` for telemetry parity.

## Tests to add

- `Scan.FilterFolderJunkHidden` — fixture with the four representative entries: absent from tree, counted in `junk_filtered` (fails before).
- `Scan.FilterLocalizedNewFolder` — the CJK + German + French "New Folder" names.
- Bench: re-run inventory comparisons with the filter on; precision delta should close.

## Related findings

- Session-1's S-001 analysis (mechanical inventory divergence class) · M-017 (ordering — the same PushFilter pass family in the og)

## Open questions

Whether the og's filter also applies to recover-all writes (vs presentation only) — the corpus artifact scopes it to presentation ("hide … from results"); recommend the core apply at emission so sessions/reports agree, and register the choice.
