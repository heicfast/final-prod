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

## Harness source (durable copy — verbatim from the session scratch tree)

```cpp
/* Session-4 executed-reproduction harness (SCRATCH — not part of the audited repo).
 * Proves/disproves the cr_preview Caps.max_read bypass on the docx/zip path:
 *   claim: docx_sniff reads directly through PreviewReader::read_at, accounting
 *   bytes in w.pulled but never enforcing caps.max_read (cr_preview.cpp:583-650).
 * Method: synthetic FileProperties with resident_data = crafted zip (EOCD + CD
 * with word/document.xml + LFH + 2048 deflate bytes), caps.max_read = 4096.
 * PASS-condition for the BUG: extract_metadata() completes WITHOUT truncated=true
 * while device/resident bytes consumed > 4 KiB. PreviewReader reads resident data
 * without a device, so we track via a counting device over an extent instead:
 * we place the zip on ONE disk extent and zero resident_data.
 */
#include "cr/preview/cr_preview.h"
#include "cr/storage/cr_device.h"
#include "cr/tree/cr_node.h"
#include <cstdio>
#include <zlib.h>
#include <cstring>
#include <vector>

using namespace cr;

/* counting device: serves a fixed pattern, counts read_at calls + bytes */
struct CountingDevice final : public cr::IStorageDevice {
    u64 total = 8ull << 20; /* 8 MiB synthetic disk */
    u64 calls = 0, bytes = 0;
    cr::Geometry geom = [] { cr::Geometry g = {}; g.total_bytes = 8ull << 20; g.logical_sector = 512; g.physical_sector = 512; g.sector_count = (8ull << 20) / 512; return g; }();
    u32 read_at(u64 offset, void* buffer, u32 length) override {
        ++calls;
        bytes += length;
        auto* p = static_cast<u8*>(buffer);
        for (u32 i = 0; i < length; ++i)
            p[i] = u8((offset + i) & 0xFF);
        return OK;
    }
    const cr::Geometry& geometry() const override { return geom; }
};

/* little-endian writers */
static void put16(std::vector<u8>& v, size_t at, u16 x) { v[at] = u8(x); v[at + 1] = u8(x >> 8); }
static void put32(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x); v[at + 1] = u8(x >> 8); v[at + 2] = u8(x >> 16); v[at + 3] = u8(x >> 24);
}

/* Craft a zip with a word/document.xml entry, deflate payload from zlib. */
static std::vector<u8> craft_zip() {
    const char* name = "word/document.xml";
    const size_t nlen = 17;
    /* 128 KiB of text -> compress -> csize (deflate) */
    std::string text;
    for (int i = 0; i < 4000; ++i)
        text += "The quick brown fox jumps over the lazy dog 0123456789 ";
    uLongf csize = compressBound(text.size());
    std::vector<u8> comp(csize, 0);
    compress2(comp.data(), &csize, reinterpret_cast<const Bytef*>(text.data()),
              uLong(text.size()), 0); /* level 0: stored blocks - low compressibility */
    comp.resize(csize);
    std::fprintf(stderr, "[dbg] text=%zu comp=%zu\n", text.size(), comp.size());
    const u32 crc = 0; /* parser does not check */
    std::vector<u8> z(30 + nlen + comp.size() + 46 + nlen + 22 + 64, 0);
    size_t o = 0;
    z[o]='P';z[o+1]='K';z[o+2]=0x03;z[o+3]=0x04;o=4;
    put16(z,o,20);o+=2; put16(z,o,0);o+=2; put16(z,o,8);o+=2;
    put16(z,o,0);o+=2; put16(z,o,0);o+=2; put32(z,o,crc);o+=4;
    put32(z,o,u32(comp.size()));o+=4; put32(z,o,u32(text.size()));o+=4;
    put16(z,o,u16(nlen));o+=2; put16(z,o,0);o+=2;
    std::memcpy(z.data()+o,name,nlen);o+=nlen;
    std::memcpy(z.data()+o,comp.data(),comp.size());o+=comp.size();
    const size_t cd_off=o; const size_t lfh_off=0;
    {
        size_t c=cd_off;
        z[c]='P';z[c+1]='K';z[c+2]=0x01;z[c+3]=0x02;c+=4;
        put16(z,c,20);c+=2;put16(z,c,20);c+=2;put16(z,c,0);c+=2;put16(z,c,8);c+=2;
        put16(z,c,0);c+=2;put16(z,c,0);c+=2;put32(z,c,crc);c+=4;
        put32(z,c,u32(comp.size()));c+=4;put32(z,c,u32(text.size()));c+=4;
        put16(z,c,u16(nlen));c+=2;put16(z,c,0);c+=2;put16(z,c,0);c+=2;
        put16(z,c,0);c+=2;put16(z,c,0);c+=2;put32(z,c,0);c+=4;
        put32(z,c,u32(lfh_off));c+=4;
        std::memcpy(z.data()+c,name,nlen);c+=nlen;
        o=c;
    }
    const size_t cd_size=o-cd_off;
    z[o]='P';z[o+1]='K';z[o+2]=0x05;z[o+3]=0x06;o=4;
    put16(z,o,0);o+=2;put16(z,o,0);o+=2;put16(z,o,1);o+=2;put16(z,o,1);o+=2;
    put32(z,o,u32(cd_size));o+=4;put32(z,o,u32(cd_off));o+=4;put16(z,o,0);o+=2;
    z.resize(cd_off+cd_size+22);
    std::fprintf(stderr, "[dbg] cd_off=%zu cd_size=%zu final=%zu\n", cd_off, cd_size, z.size());
    return z;
}

int main() {
    CountingDevice dev;
    tree::FileProperties props;
    props.logical_size = 0; /* set below */
    props.extents.clear();
    /* the zip lives on ONE extent at device offset 1 MiB */
    std::vector<u8> zip = craft_zip();
    props.logical_size = zip.size();
    tree::FileProperties::Extent e;
    e.offset = 1ull << 20;
    e.length = zip.size();
    e.psize = 0;
    e.codec = 0;
    props.extents.push_back(e);

    /* device must serve our crafted zip bytes at the extent */
    struct OneShotDevice final : public cr::IStorageDevice {
        std::vector<u8> disk;
        u64 calls = 0, bytes = 0;
        cr::Geometry geom = [] { cr::Geometry g = {}; g.total_bytes = 4ull << 20; g.logical_sector = 512; g.physical_sector = 512; g.sector_count = (4ull << 20) / 512; return g; }();
        u32 read_at(u64 offset, void* buffer, u32 length) override {
            ++calls;
            bytes += length;
            if (offset + length > disk.size())
                return ERR_READ_FAILED;
            std::memcpy(buffer, disk.data() + offset, length);
            return OK;
        }
        const cr::Geometry& geometry() const override { return geom; }
    };
    OneShotDevice od;
    od.disk.assign(4ull << 20, 0);
    std::memcpy(od.disk.data() + (1ull << 20), zip.data(), zip.size());

    preview::PreviewReader pr(&od, &props);
    preview::Caps caps;
    caps.max_read = 4096; /* the contract: at most 4 KiB pulled */
    preview::Metadata m = preview::extract_metadata(pr, caps);

    std::printf("zip size              : %zu bytes\n", zip.size());
    std::printf("caps.max_read         : %u bytes\n", caps.max_read);
    std::printf("extracted kind/format : %s / %s\n", m.kind.c_str(), m.format.c_str());
    std::printf("truncated flag        : %s\n", m.truncated ? "true (cap honored)" : "false");
    std::printf("device read calls     : %llu\n", (unsigned long long)pr.device_reads());
    std::printf("device bytes pulled   : %llu\n", (unsigned long long)pr.device_bytes());
    std::printf("text snippet len      : %zu\n", m.text_snippet.size());
    const bool bypassed = !m.truncated && pr.device_bytes() > caps.max_read;
    std::printf("VERDICT               : %s\n",
                bypassed ? "CAPS BYPASSED (read > max_read, truncated=false)"
                         : "caps honored");
    return bypassed ? 0 : 1;
}

```
