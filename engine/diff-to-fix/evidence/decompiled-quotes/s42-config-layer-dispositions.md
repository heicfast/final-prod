# S42 — the config-layer disposition register (the four un-dispositioned surfaces + the I-034 census correction)

**Lane:** S42 (session-42 continuation audit) · **Tree:** core `869da779`-lineage (lane tip after rebase) · **Corpus:** `be0cfe9`.
**Method:** direct file reads + DRWUI decompile decode (`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`) + inventory cross-ref (`re-workspace/inventory.json` :233/:588).

## Why this register (the I-034 census correction)

I-034's second census claimed "no undispositioned product surface" over the 750-entry og-file
count. That claim was **wrong by four surfaces** — all in `og-codebase/config/` and `log/`,
all with ZERO trail citations (verified: `rg shellexe|TreeType|themdark|DRWLog` over the
entire `diff-to-fix/` trail → 0 hits at this lane's open). The census missed them because
the *artifacts that mention them* were cited (the config-layer artifacts), but the surfaces
themselves never received disposition rows. This register closes each one.

## 1. `config/shellexe.ini` — the Explorer context-menu verb's locale taglines (product-frame, OOS-UI)

- **Bytes:** 2,098 B, UTF-16 (BOM fffe), `[language]` section, 25 locale taglines
  (English = "Recover any deleted data with EaseUS"; German/Arabic-RTL/Japanese/… —
  the corpus artifact's decode is byte-confirmed this session). The install copy and the
  BUILDPE copy are **byte-identical** (sha256 `6278158d…`, inventory :233/:588).
- **Consumer:** the Windows Explorer context-menu integration (the shell-extension
  component the DRW installer registers — the "Recover any deleted data" verb on
  files/folders). **No binary inside the og-codebase carries the tagline or the filename**
  (grep over every `reversed/*/strings_*.txt` + the DRWUI decompile → 0 hits) — the
  consumer is the shell-extension host outside the corpus's shipped tree, reading the file
  at verb-display time.
- **Disposition:** product-frame / UI-surface — **OUT-OF-SCOPE by the no-UI convention**
  (D-26 family; the merged product is a core+CLI with no Explorer integration). The S12
  lane's I-035 register row ("the Explorer context-menu verb") named the VERB surface;
  this row adds the tagline data file behind it. No engine-side parameter exists here.

## 2. `config/version.ini` `TreeType` — the DRWUI results-view layout gate (UI-pure, OOS-UI)

The `scanext4`/`scanzfs` gates in the same file were dispositioned (I-016); **`TreeType`
shipped=1 was never dispositioned**. Decoded first-hand this session:

- **Reader 1 — `FUN_140a09c40`** (`DRWUI_decompiled.c:1561098`, the boolean gate):
  opens `config/version.ini` group `[ver]` via QSettings (UTF-8 codec, :1561183-1561190);
  returns **TRUE iff the `TreeType` key is ABSENT or its value == 0**, further ANDed with
  NOT `FUN_1401b7d50(ctx)` where the co-gate is a single context byte `*(u8*)(ctx+0x1c8)`
  (:320128-320131 — the results-context flag; the sibling settings readers in the same
  family consume `value_vrpartisize` (atoi) and `choosePath` (bool) — :1561250-1561390,
  the UI settings family).
- **Reader 2 — `FUN_1401d1ef0`** (:338209+, the results-page constructor): computes a
  default from the `value_root` settings-string comparison (:338253-338259), then overrides
  with the raw-ini read `FUN_1401d08e0("config/version.ini", <section>, "TreeType", -1)`
  (:338263) — `>= 0 && != default` wins. The Ghidra view shows the result as a dead store;
  the value feeds the view-model construction immediately below (the `CExtentTreeDataModel`
  ctor at :338325 sets its own layout byte `param_1[0x80] = 1`).
- **Consumers of the gate** (all Qt item-view internals): :1110291 (the view's
  model-index/column selection), :1635325/:1636425, :1651297/:1651363, :1666745 (a
  QPoint x-coordinate choice — column positioning), :1678635 — the classic-tree vs
  layout-tree column/selection arrangement.
- **Disposition:** **OOS-UI** — no engine, session, ABI, or CLI consumer; the shipped
  `TreeType=1` selects the modern layout. Registered so no future lane re-derives it.

## 3. `config/themdark.ini` — the Qt theme tables (UI-pure, OOS-UI)

118 numbered entries per theme (`[normal]` + `[dark]`): Qt stylesheet fragments —
`qlineargradient(...)` stops, `#rrggbb`/`rgba(...)` colors, one `border:1px solid` rule
(:109). Pure skin data for DRWUI's theme engine. **No consumer outside the UI.** OOS-UI,
registered.

## 4. `log/DRWLog.log` — the empty launcher channel (data-asset datum)

`DRWLog.log` is **0 bytes** in the shipped install (the other three channels carry data:
CalcPartition.log 205 lines — the M-017 scoring ground truth; DRWUI.log 691 — the scan-flow
ground truth; DefaultLog.txt 573 — the CDirectoryTreeBuilder tree-build trace). The DRW.exe
launcher's log target was never written in the captured install — the channel exists, the
content does not. Extends I-027's log-channel census (row 75 family); no parameter.

## 5. Completeness sweep of the remaining config-layer fields (all previously dispositioned, verified)

| File | Disposition | Carrier |
|---|---|---|
| `config/FilterFolder.ini` | junk pools + TxtViewFiles — AUDITED | M-023/M-024/M-108/N-132 |
| `config/PriorityDisplayType.xml` | type-display ordering — LANDED | N-132 fix `4e74ee2f` |
| `config/RawConfig.ini` | 34 toggles + per-format carve gates | M-024/N-102; the analyser-toggle mapping (filtering-priority artifact §4) |
| `config/usefullostfiles.xml` | the "valuable formats" pitch set | OOS-UI row (API-DELTA row 45's truthful split) |
| `config/config.ini` | the 22-locale language table + font styles (`Segoe UI`/`Meiryo UI` families) | UI-pure — the Languages OOS family (D-26); this row names the file itself (previously only the FilterFolder consumer strings at RVA 0x66bcc cited it) |
| `config/version.ini` version/market fields (`version=free`, `product-code=RE-DRW-FREE-WIN`, `xurlPageID`, `SetupBuildTime`) | the installer-stamped identity | S-038's K78 contract |
| `config/version.ini` `qq1`/`qq2` (base64 blobs) | encrypted market fields | Licensing/monetization OOS (D-26 family) |
| `config/version.ini` `forceshowcloseretain=1` | the retention-UI nag gate | UIUX/retention OOS (S-038-adjacent telemetry row) |
| `config/nasconfig.ini` | NAS connection defaults | OOS (NAS, D-26) |

## Verdict

All four un-dispositioned surfaces are **OUT-OF-SCOPE-UI / product-frame registers — no
engine-side parameter, no core defect, no finding beyond the register itself** (filed as
I-036 per the INFO convention; the I-034 census-claim correction recorded in its text).
