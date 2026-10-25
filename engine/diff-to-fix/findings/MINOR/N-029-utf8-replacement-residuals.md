# MINOR N-029 — UTF-8 replacement policy residuals from the S-005 fix (session-2 group)

**Severity:** MINOR (grouped) · **Modules:** session, engines (ext4/btrfs/zfs) · **Status:** READ-ONLY FINDINGS — no fix applied
**Parameters affected:** B10 (names), D12 (session persistence), D13-adjacent
**Audited commit:** `e93decf` · corpus `03ad46c`

The S-005 fix is og-parity in net effect (verified — see `resolved/findings/S-005-*.md`). Two residuals survive, both MINOR:

**N-016a — Replacement-count policy.** Core `cr_text.h:85-88`: one U+FFFD per bad lead byte + per-byte resync — `\xE6\x96x` → `FFFD FFFD x`. The og's restore path converts via `MultiByteToWideChar(CP_UTF8, dwFlags=0)` (`DRWUI_decompiled.c:264664-694`, invoked at `:357529`), which follows Windows' maximal-subpart policy → `FFFD x`. Same direction (substitute invalid bytes), different counts on multi-byte invalid runs. Restored filenames can differ in U+FFFD count. Fix: adopt maximal-subpart in `text::utf8_to_u16` (consume as much of the invalid sequence as forms a prefix of a valid one, then one FFFD — RFC 3629 §3.1's "maximal subpart" formulation). Tests: `Text.MaximalSubpartReplacement` vectors (`\xE6\x96x`, `\xF0\x80\x80\x80`, truncated-tail classes).

**N-016b — Substitution timing in saved sessions.** og `.rsf` keeps the **original** on-disk bytes in the name field (substitution happens at display/restore). Core `.crsf` stores the **substituted** name (decode happens at scan). A re-exported name or a session-diff against the og will differ for invalid-UTF-8 names even after N-016a. Fix: persist the raw bytes alongside the decoded name (a versioned `name_raw` field), and restore-file naming from raw when valid. Tests: `Session.RawNameRoundtrip`, `Writer.InvalidUtf8NameRestoredFromRaw`.

**Related:** resolved S-005 · OQ-S2M-2 (Btrfs og anchor for name conversion — NOT-FOUND in BtrfsSearchDLL)

**Open questions:** Whether DRWUI's display conversion uses the same dwFlags=0 path for the results grid as for restore (single site found at `:534592`; a second would strengthen N-016a).
