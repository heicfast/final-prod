# s10-preview-text-eligibility.md — EXECUTED proof: the text-preview eligibility model (M-108)

**Session 10 (2026-10-29). Core `84a1832` (product lineage `35adcb9`+fixes). Corpus `be0cfe9`.**
**Method: compiled `src/core/preview/cr_preview.cpp` (+ `cr_decomp`/`cr_devmap`/`cr_err*`/`cr_node`
deps) against a minimal in-memory `IStorageDevice` harness; ran `extract_metadata` on six
fixtures. Read-only with respect to the repo — the harness lives outside the tree
(`/home/z/my-project/scripts/s10-preview-probe.cpp`).**

## Build + run (reproducible)

```
g++ -std=c++20 -Wall -Wextra -I<core>/include \
    s10-preview-probe.cpp \
    <core>/src/core/preview/cr_preview.cpp \
    <core>/src/core/compress/cr_decomp.cpp \
    <core>/src/core/storage/cr_devmap.cpp \
    <core>/src/core/err/cr_errmod.cpp <core>/src/core/err/cr_error.cpp \
    <core>/src/core/err/cr_log.cpp <core>/src/core/tree/cr_node.cpp \
    -lz -o s10-preview-probe && ./s10-preview-probe
```

## Output (verbatim, 2026-10-29)

```
1 ASCII text                 -> kind=text        fmt=txt  snippet="just a plain text file second line "
2 UTF-8 BOM text             -> kind=unsupported fmt=     snippet=""
3 UTF-16LE BOM text          -> kind=unsupported fmt=     snippet=""
4 Latin-1 text (0xE9)        -> kind=unsupported fmt=     snippet=""
5 printable-head binary      -> kind=text        fmt=txt  snippet="JFIF-like printable head !!! then  binar"
6 MZ head (0x90 at +2)       -> kind=unsupported fmt=     snippet=""
```

## Reading

- **Fixture 2 (UTF-8 BOM `EF BB BF`)** — the single most common Windows text-file shape
  (Notepad's historic default) — returns `kind=unsupported`, no snippet. The head gate at
  `src/core/preview/cr_preview.cpp:868-871` rejects any first-16-bytes value ≥ 0x7F.
- **Fixture 3 (UTF-16LE BOM `FF FE`)** and **fixture 4 (Latin-1 `0xE9` in the head)** — same
  refusal class.
- **Fixture 5 (printable-head binary)** — accepted as `text` with a garbage snippet
  (control bytes dropped per `text_sniff` :587-597): a false-positive text classification
  for any unknown-binary format whose first 16 bytes happen to be ASCII.

## The reference side (decompile, first-hand this session)

`DRWUI_decompiled.c:535858` and `:535937` — the og loads
`config/FilterFolder.ini` `[Filter]` `TxtViewFiles` **once** (thread-safe static init at
`DAT_142643850`), extracts the candidate file's extension, builds the `;ext;` haystack, and
substring-searches the whitelist (`FUN_140310aa0`); membership sets the text-viewable flag
(`local_e8 = 1`). **The og decides text previewability by EXTENSION membership in the
119-entry TxtViewFiles list** — never by content sniffing. The list itself:
`og-codebase/config/FilterFolder.ini` (`.TXT;.CFG;…119 case-mixed extensions…;.msbak;`),
documented at `artifacts/config-files/filtering-priority--config-layer.md:40,48`.

## Conclusion

The two engines accept DISJOINT sets of files as text:

| Input | og (extension whitelist) | core (content sniff) |
|---|---|---|
| `.txt` UTF-8 BOM | text (whitelisted) | **unsupported** |
| `.txt` UTF-16LE | text (whitelisted) | **unsupported** |
| `.log` Latin-1 | text (whitelisted) | **unsupported** |
| `.xyz` printable-head binary | not text (not whitelisted) | **text + garbage snippet** |
| ASCII `.txt` | text | text (the only agreement class) |

Filed as **M-103** (MAJOR). The gate-doc false claim ("landed", API-DELTA row 44) is filed
as **N-128**.
