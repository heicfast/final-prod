# OQ-S4-2 — RESOLVED (session 5): the vtable+0x48 hook is a one-shot per-scan filter-config fetch, NOT a per-file namer

**Status:** RESOLVED with one residual (session 5, 2026-10-26) · **Module:** carve/raw · **Parameters:** P16, P25
**Audited commit:** `7cf6754` · corpus `03ad46c`

## Resolution

- Registration: DRWUI FUN_140580720 passes std::function{FUN_140161fb0} (a generic
  GetSetting(key,default) UI-config getter, 209 DRWUI call sites) into CRAWSearchController
  slot +0x48; the setter (FUN_18000b8d0, RAWSearchDLL:8695) move-assigns it into ctrl+0x30.
- Sole consumption: FUN_18000beb0 (scan start, slot +0x68) invokes the function ONCE with
  the literal key **"filter_generalraw"**; the returned string is parsed with the same
  neb::CJsonObject API as raw_signs.json and installed into the global config singleton's
  filter vector (FUN_180011620). std::function::operator() has exactly ONE caller in the
  DLL — never per-file. Ini fallback: RawConfig.ini [Filter] (GetPrivateProfileSectionA
  @13137).
- Consequence: M-029's "server-side filter push" mechanism = this per-scan config fetch
  (the push happens at scan start, not per file); the og has NO FileNNNN naming scheme
  (carve names are per-analyser: MP4/MOV template "Lost Name File (...)_EaseUS",
  CZeroEndRawFileInfo content-embedded names) — filed as N-060 with the corpus deep-dive
  artifact's contrary claim queued as a corpus-errata row.

## Residual (precise next artifact)

The DRWUI merge-rename path for CRAWFileProperties names (users of vftable 0x140f5f528)
remains undecoded — it explains how carved names interact with the structured-tree merge
(the corpus's "shot0017.jpg" samples). Next artifact:
re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c around the vftable's user set.
Not blocking: the engine-side contract (per-scan filter push + per-analyser naming) is
fully pinned.
