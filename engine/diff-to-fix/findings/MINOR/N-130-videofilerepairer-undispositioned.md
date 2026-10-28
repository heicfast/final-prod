# MINOR N-130 — VideoViewer/VideoFileRepairer.exe has no disposition row: the Fixo video-repair console engine (1.9 MB, x86, fix_main_code lineage, MP4/QuickTime atom grammar, faad2) — the one engine-shaped binary the register missed

**Severity:** MINOR (registration gap; evidence supports SIDECAR-OOS disposition, not a capability gap)
**Module / Surface:** og-tree disposition register / repair-family boundary
**Parameters affected:** P54-adjacent (module boundary registration), P68 (register completeness)
**Corpus references:** `og-codebase/VideoViewer/VideoFileRepairer.exe` (1,919,208 B, PE32 x86 console, 5 sections — `file` output this session); strings (first-hand): source paths `d:\workspace\fix_main_code\videorepairer\src\{aviparser, asf_wmvparser, dvparser, apeparser, aiffparser, ac3frameparser, 3gpsamplechecker, swf_zlib_helper, ffmpeg_parser\apedec}.cpp`, `d:\workspace\fix_main_code\3rdlib\json\json.hpp`; CLI `VideoRepair -r fix.mp4 fixresult.mp4 [duration] [taskId] [port]` / `-a fix.mp4 fixresult.mp4 sample.mp4 …`; MP4/QuickTime atom names (moov/trak/mdhd/co64/stsc/stsz/elst); `avcodec-62.dll` import; 14 `NeAACDec*` exports (faad2); log name `easeus_video_repair.log`. Consumer: `DRWUI.exe` carries the UTF-16 string `VideoViewer\VideoFileRepairer.exe` ×2. Docs: `re-workspace/artifacts/per-binary/VideoFileRepairer.md` ("Video repair UI. VideoViewer-side repair entry"; 7,019 functions, 347,662 decompiled lines); `re-workspace/artifacts/FileRepair/repair-architecture.md:11` (x86, fix_main_code\UI, "Video repair UI host")
**Core references:** audited commit `84a1832` — `docs/PARITY-OG-CORE-INVENTORY.md` (no row names VideoFileRepairer; the VideoViewer row :107 enumerates EaseUS_MediaGraph/ffmpeg/avcodec-family but not this exe); the repair-family boundary: I-010/D-26 (EUFileRepair bundle OOS), `architecture/06-repair-chain.md:10-19` (repair = separately-sold line, DRWUI-only orchestration); the engine-side touchpoint: M-021 (CarvePostProcessor — the "repair after recovery" hook, FIXED)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** The session-10 completeness sweep (1470 files vs the register) is the first filename-keyed enumeration that reaches into VideoViewer/; the per-binary artifact exists but was never row'd, and `rg -i VideoFileRepairer` over the core repo → 0 hits.

## Summary

The og ships a 1.9 MB x86 console engine at VideoViewer/VideoFileRepairer.exe — the Fixo video-repair worker (DRWUI-spawned, MP4/QT atom grammar, ffmpeg+faad2 decode, sample-file-based repair). No register row names it; filename-keyed sweeps (the register's own access pattern) miss it because the VideoViewer row enumerates the media DLLs but not this exe. The evidence supports a SIDECAR-OOS disposition (the repair-family precedent), which this finding registers for the board.

## What the reference does

DRWUI spawns `VideoViewer\VideoFileRepairer.exe` with `-r`/`-a` CLI shapes (repair with/without a sample file), taskId/port arguments (progress IPC), writing `easeus_video_repair.log`. It is the video half of the Fixo repair line (the same product family as EUArchiveRepair.exe — which, note, is also password-aware via Dict per the register's own row).

## What our core does

Nothing video-repair-shaped exists in the core (correctly, per the registered repair-line exclusion); the engine-side touchpoint for "repair after recovery" is the M-021 CarvePostProcessor hook. The defect is purely register-side: the binary has no disposition row, so a future session auditing "every og component" cannot close it out without re-doing this sweep.

## Gap

Falsifiable: `rg -i "VideoFileRepairer" docs/ diff-to-fix/` → 0 hits; the register's VideoViewer row does not name it; `ls og-codebase/VideoViewer/` shows it beside the dispositioned media DLLs.

## Impact

Register completeness only. No core capability is implicated (the repair line is a registered exclusion; the engine never invokes it — `architecture/06:10-19`).

## Reproduction / verification

First-hand this session: `file` (PE32 x86 console), `strings -el` (the fix_main_code paths, CLI grammar, atom names, faad2 exports — quoted in Corpus references), the DRWUI consumer string, the corpus artifact reads. All cited.

## Fix plan (for the engineering team)

1. Add the register row: `| VideoViewer/VideoFileRepairer.exe | SIDECAR-OOS | Fixo video-repair console engine (x86, fix_main_code lineage; DRWUI-spawned -r/-a CLI; MP4/QT atom grammar; ffmpeg avcodec-62 + faad2; per-binary/VideoFileRepairer.md; repair-architecture.md:11) — the repair-family exclusion (I-010/D-26); engine-side touchpoint = the M-021 CarvePostProcessor hook | none | OOS (verified) |`
2. Cross-note on the VideoViewer row :107 so the family row points at the child.
3. Acceptance: the next filename-keyed sweep (the N-129 fix plan's re-count) returns 0 undispositioned files outside the tracked sfc question.

## Tests to add

None (register-only).

## Related findings

**OQ-S10-2** (the sfc file found by the same sweep — the genuinely unknown item) · **N-129** (the same sweep's numeric defects) · **I-010/D-26** (the repair-family exclusion this disposition rides) · **M-021** (the CarvePostProcessor hook — the only engine-side seam)

## Open questions

None for the disposition itself. (Whether the merged product ever wants a video-repair integration via the M-021 hook is a roadmap question, not an audit question.)
