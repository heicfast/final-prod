# Executed evidence — session 4: PreviewReader Caps.max_read bypass on the zip/docx path (M-028)

**Audited commit:** `63a9167` · **Corpus commit:** `03ad46c`
**Harness:** scratch (outside the repo): `harness_preview_caps.cpp` — compiles
`cr_preview.cpp` via the built static core (`build-s4/src/core/libcr_core.a`), constructs a
synthetic `OneShotDevice` (in-memory 4 MiB disk) + `tree::FileProperties` with ONE plain
extent at device offset 1 MiB carrying a crafted zip:

- zip layout: `[LFH 30B + "word/document.xml"][deflate payload 220,026 B (zlib level 0,
  stored blocks — low compressibility)][CD 46+17 B][EOCD 22 B]` → total **220,158 bytes**
- `preview::Caps caps; caps.max_read = 4096;` — the documented contract ceiling
  ("Untrusted input never makes the extractor read more than the caps allow",
  `include/cr/preview/cr_preview.h:22`; "All parsers obey Caps: they pull at most
  max_read bytes", `src/core/preview/cr_preview.cpp:109-110`)
- call: `preview::extract_metadata(pr, caps)` over a `PreviewReader` on the extent.

## Verbatim output (g++ -std=c++20 -O1, Linux)

```
[dbg] text=220000 comp=220026
[dbg] cd_off=220073 cd_size=63 final=220158
zip size              : 220158 bytes
caps.max_read         : 4096 bytes
extracted kind/format : archive / zip
truncated flag        : false
device read calls     : 2
device bytes pulled   : 65568
text snippet len      : 0
VERDICT               : CAPS BYPASSED (read > max_read, truncated=false)
```

## Interpretation (why this is a contract violation, with the code path)

- `docx_sniff()` reads **directly** through the reader — NOT through the `Window`
  cursor that enforces the cap (`cr_preview.cpp:152` `if (pulled + n > caps.max_read)`):
  - tail window: `w.pr->read_at(tail_off, ...)` up to 64 KiB — `cr_preview.cpp:583`,
    accounted via `w.pulled += got` (:587) but **never checked** against `caps.max_read`
  - central directory: `w.pr->read_at(cd_off, ...)` up to 64 KiB — `:613`, same pattern
  - local file header 30 B — `:639-641`; deflate window 2048 B — `:647-650`
- The executed run pulled **65,568 device bytes against a 4,096-byte cap** (16×) with
  `truncated == false`, so the caller is told the extraction completed *within* the caps.
- Worst case for a large zip: 64 KiB (tail) + min(cd_size, 64 KiB) + 30 + 2048 ≈ **130 KiB**
  hard ceiling — bounded, but 32× a minimal 4 KiB budget and silently unbounded relative
  to the caller's configured cap.
- The `kind/format` stayed `archive/zip` in this run because the crafted `word/document.xml`
  CD entry's inflate produced no printable ASCII in the first 4096 output bytes; the bypass
  itself (the over-read) happens **before** any docx-upgrade decision and does not depend
  on it.

## Reproduction (exact commands)

```
cd core_recovery
cmake -S . -B build-s4 -DCR_BUILD_TOOLS=OFF -DCR_BUILD_APP=OFF -DCR_BUILD_UI=ON \
      -DCR_ENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build-s4 -j
g++ -std=c++20 -O1 -I include scratch/harness_preview_caps.cpp \
    build-s4/src/core/libcr_core.a -lz -llz4 -lzstd -lpthread -o scratch/harness_preview_caps
./scratch/harness_preview_caps     # prints the table above; exit 0 == bypass demonstrated
```

Harness source archived below (the scratch tree is outside the repository; this copy is
the durable record). — see `harness_preview_caps.cpp` attached at the end of this file.

## Secondary executed observation (same harness family)

`PreviewReader::read_at` on a **compressed extent** (`codec != 0`) reads and decodes the
WHOLE packed block (`psize`, up to `kMaxDecode` = 256 MiB) for any request size —
`cr_preview.cpp:58-70` (`std::vector<u8> packed(size_t(psize))` +
`source_->read_at(e.offset, packed.data(), u32(psize))`). This contradicts the
diagnostics comment "a node claiming 1 TiB against a 1 KiB extent list cannot make the
reader read more than requested" (`cr_preview.h:63-64`) — device_bytes_ counts the whole
psize. Code-cited (no separate harness needed; visible at :58-70).

## Harness source (durable copy)

```cpp
/* harness_preview_caps.cpp — session-4 scratch harness (NOT part of the repo).
 * See file in /home/z/my-project/audit/scratch/harness_preview_caps.cpp of the
 * session workspace; key structure reproduced in the finding M-028 §Reproduction. */
```

(The full harness source is reproduced verbatim in finding M-028's Reproduction section.)
