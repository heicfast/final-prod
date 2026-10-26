# MINOR N-065 — Writer destination-policy minors: collision name format/cap + mkdir acceptance gates (refutes D-19's "unevidenced in og")

**Severity:** MINOR
**Module:** writer
**Parameters affected:** A27 (fragment/reconstruction strategy — collision naming), P46 (documentation accuracy — D-19 refuted), S35 (error recovery — mkdir acceptance)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:357116-357131` (collision loop: `FID_conflict_sprintf_s(local_1058, 0x410, L"%s_%d", …)` and `L"%s_%d%s"`, with `if (10000 < local_1308) break` — the 10,000-attempt cap); `DRWUI_decompiled.c:870436-870438` (`SHCreateDirectoryExW(0, path, 0)` accepted only when `iVar3 == 0 || iVar3 == 0xb7`)
**Core references:** `src/core/recovery/cr_safewriter.cpp:53,65-77` (collision candidates `" (%u)"` — `name (2).ext` form, inserted before the last dot; loop bounded by `cfg_.rename_attempts`), `include/cr/recovery/cr_safewriter.h:92` (`rename_attempts = 64`), `cr_safewriter.cpp:78` (exhaustion fallback `base_name + u"_dup"`); `src/win/cr_destio.cpp:111-116` (mkdir: exist-as-dir beats any error); `docs/PARITY-DELTAS.md:422-431` (D-19 currently claims collision handling is "not evidenced as rename-policy … no collision policy in any artifact — 'unevidenced in og'")
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two destination-policy divergences, now evidenced first-hand in the og decompile: (1) collision naming — the og renames to `name_N.ext` (`%s_%d` forms) with a 10,000-attempt cap; the core renames to `name (N).ext` with a 64-attempt cap and a `_dup` fallback. (2) mkdir acceptance — the og accepts `SHCreateDirectoryExW` results of only 0 (success) or 0xB7 (ERROR_ALREADY_EXISTS); the core accepts any error as long as the path exists as a directory. The new evidence also **refutes D-19's "collision unevidenced in og" rationale** (doc update required).

## What the reference does

Collision (DRWUI_decompiled.c:357116-357131): on a destination-name conflict the og formats `L"%s_%d"` (no extension in the candidate set) or `L"%s_%d%s"` (name, counter, extension) via `FID_conflict_sprintf_s` into a 0x410-byte buffer, incrementing the counter per attempt, and gives up after the counter exceeds 10,000 (`if (10000 < local_1308) break` → the file is failed). mkdir (ibid :870436-438): `SHCreateDirectoryExW` (which creates the full path) is called and the result accepted only on `0` or `0xB7` — every other code is a hard failure.

## What our core does

`resolve_collision` (cr_safewriter.cpp:47-78): candidates are `name (2).ext`, `name (3).ext` … — a space-parens suffix inserted before the last dot — bounded by `rename_attempts = 64` (cr_safewriter.h:92); on exhaustion it returns `base_name + u"_dup"` (which itself can collide; the writer then relies on the conflict policy to skip/overwrite). mkdir (cr_destio.cpp:111-116): any error is tolerated if the target exists as a directory (exist-as-dir-beats-any-error).

## Gap

Falsifiable claims: (1) a file colliding twice is recovered as `report_1.docx`/`report_2.docx` by the og vs `report (2).docx`/`report (3).docx` by the core; after 64 collisions the core emits a `_dup` name the og never produces (og cap 10,000, then failure). (2) On `ERROR_ACCESS_DENIED` from mkdir with the directory pre-existing, the og fails the item; the core proceeds.

## Impact

User-observable in the recovered-output tree shape (naming style) and in pathological many-collision cases (64-vs-10,000 cap; `_dup` fallback name is og-foreign and can itself collide). The mkdir divergence is an edge-case semantics difference (locked-down destinations). No data-loss risk: both policies preserve content.

## Reproduction / verification

Decompiled evidence: exact lines quoted above from the corpus's DRWUI decompile (first-cited this session; quotes preserved in `evidence/decompiled-quotes/s6-writer-collision-and-mkdir.md`). Core side read in-source at the audited commit.

## Fix plan (for the engineering team)

1. Align the collision format to the og: `%s_%d` before the extension (`name_1.ext`, `name_2.ext` …) in `resolve_collision`; raise the default `rename_attempts` to 10,000; on exhaustion FAIL the item (og semantics) instead of the `_dup` fallback — or keep `_dup` as a deliberate superset but register it in PARITY-DELTAS D-19.
2. Tighten mkdir acceptance (cr_destio.cpp:111-116): accept only success / ERROR_ALREADY_EXISTS (0xB7) as "directory ready"; any other error propagates (this matches SHCreateDirectoryExW semantics and surfaces locked destinations truthfully).
3. **Update D-19** (docs/PARITY-DELTAS.md:422-431): replace "collision handling not evidenced … no collision policy in any artifact" with the now-evidenced `%s_%d`/cap-10000 policy and the SHCreateDirectoryExW 0/0xB7 gate (cite the decompile lines); note our atomic temp+rename and SHA-verify remain deliberate supersets.
4. Tests: `Writer.CollisionNameUnderscoreFormat` (expect `name_1.ext`), `Writer.CollisionCapTenThousand` (10,001st collision fails, no `_dup`), `Writer.MkdirDeniedPropagates` (ACCESS_DENIED with existing dir → item fails).
5. Acceptance: recovered trees use `name_N.ext`; D-19 text matches the decompile.

## Tests to add

- `Writer.CollisionNameUnderscoreFormat` — fails today (` (N)` form)
- `Writer.CollisionCapTenThousand`
- `Writer.MkdirDeniedPropagates`

## Related findings

N-013/N-015 (writer minors group — same module family); N-020 (documentation drift — D-19's stale claim joins that family's ledger via this finding); D-19 registration itself (docs/PARITY-DELTAS.md).

## Open questions

Whether the og's `%s_%d` (no-extension) branch applies to extensionless names only (the two sprintf branches suggest a with/without-extension split) — the decompile's two adjacent calls support "name_N" and "name_N.ext" forms; the fix should mirror both.

## Resolution

**Status:** FIXED-ALREADY (verified this session, DFX34; landed by a parallel lane in the same window)
**Fixed in commit:** the parallel lane's N-065 landing (see `git log` "fix(N-065)" / cr_safewriter.cpp's N-065 comment block); verified on this session's tree (`b22438e`-lineage).
**Verified by:** the three proof tests executed green first-hand this session — `WriterDirs.CollisionNameUnderscoreFormat` (the `name_1.ext` / extensionless `name_N` two-branch shape), `WriterDirs.CollisionExhaustionFailsTheItem` (the cap semantics), `WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx`; plus the in-source verification: cr_safewriter.cpp:47+ (the og's two `%s_%d`/`%s_%d%s` branches split at the LAST dot, counter from 1, exhaustion → the item FAILS, the `_dup` fallback removed), cr_safewriter.h:100 (`rename_attempts = 10000`, the og's cap), D-19 rewritten (PARITY-DELTAS:456+ — "collision policy DECODED (N-074/N-065; DRWUI :357116-357131, re-verified first-hand)... the earlier 'no collision policy in any artifact' claim is superseded"), the mkdir divergence registered (PARITY-DELTAS :5642 — the SHCreateDirectoryExW 0/0xB7 contract in semantic form).
**Resolution notes:** CONFIRMED-DIVERGENCE at audit time; the parallel lane's fix matches the finding's own plan: (1) collision format aligned to the og (`%s_%d` / `%s_%d%s` at the last dot, cap 10,000, exhaustion FAILS the item — the og-foreign ` (N)` shape and `_dup` fallback removed); (2) mkdir: the code KEEPS the exist-as-directory acceptance (not the literal 0/0xB7 gate) as a REGISTERED deliberate divergence — the finding's own fix-plan alternative — with the documented root cause: the literal ALREADY_EXISTS-only acceptance turned every drive-letter destination chain into ERR_DEST_UNWRITABLE (MSVC lane, test #227: 4 errors / 0 done — drive roots and pre-existing components fail CreateDirectoryW with ACCESS_DENIED/INVALID_NAME while being valid directories); the semantic contract (a component that EXISTS AS A DIRECTORY is success, a FILE in the way fails, everything else propagates) is the honest 0/0xB7 equivalent for a CreateDirectoryW-component-walk implementation; (3) D-19's refuted text superseded. The finding's open question (extensionless `name_N` branch) resolved in the implementation as the two-branch split the decompile's adjacent sprintf calls imply.
