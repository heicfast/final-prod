/*
 * cr_preview.h - bounded preview reader + metadata extractors (G-10, R6.7).
 *
 * Read-only, chunked access to one node's logical content WITHOUT a
 * recovery pass: hex/image/text previews stream straight off the extent
 * list (resident data first, then disk extents, sparse gaps zero-filled -
 * the writer's semantics, so previews match recovered bytes).
 *
 * Bounded by construction (the core's own engineering contract: range
 * reads under explicit caps - M-028 corrected the provenance: the og
 * has NO in-engine preview reader; its preview payload cap is 1 MiB at
 * the UI socket layer (I-015, DRWUI_decompiled.c :654557/:654601), and
 * IInWindowStream is the 7-Zip/LZMA SDK's LZ input interface
 * (vftable :800729-800735), not a preview contract. The core's 4 MiB
 * kMaxRead is a deliberate superset - different layers, kept and
 * documented):
 *   - read_at never materializes the file: exactly `len` bytes are
 *     assembled per call, `len` capped at kMaxRead (4 MiB);
 *   - logical_size bounds every access (EOF short reads);
 *   - compressed extents decode once per call with the 256 MiB guard;
 *     NOTE (M-028 disclosure): any window into a compressed extent
 *     reads and decodes the WHOLE packed block - a preview request over
 *     a compressed extent costs psize device bytes regardless of the
 *     requested length (compressed-extent previews need
 *     caps.max_read >= psize to avoid the truncated disclosure);
 *   - device read failures zero-fill AND are reported in
 *     unreadable_spans() - never silently swallowed;
 *   - stateless per call: safe for concurrent readers over one device
 *     (the IStorageDevice contract); cancellation is between calls.
 *
 * extract_metadata() adds size/time caps on top (bounded reads + work-unit
 * counters, no recursion); all parsers are fuzz targets. Untrusted input
 * never makes the extractor read more than the caps allow: every fetch is
 * cost-gated through PreviewReader::read_cost BEFORE the device is touched
 * (M-028 - the docx/zip path's direct reader reads and the compressed-extent
 * whole-block cost are both inside the budget now), and a post-extraction
 * belt marks truncated=true if any path still exceeded it.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_PREVIEW_H_
#define CR_PREVIEW_H_

#include "../storage/cr_device.h"
#include "../storage/cr_retry.h"
#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace preview {

class PreviewReader {
public:
    /* Hard cap per read_at call (the core's own engineering choice; the
     * og's preview payload cap is 1 MiB at the UI socket layer - I-015 -
     * a different layer; kept as a documented deliberate superset, M-028). */
    static constexpr u32 kMaxRead = 4u << 20;
    /* Compressed-extent decode guard (writer parity). */
    static constexpr u64 kMaxDecode = 256ull << 20;

    /* `source` is the device the node's ABSOLUTE extents address (the
     * whole-disk view the scan used); `props` must outlive the reader. */
    PreviewReader(IStorageDevice* source, const tree::FileProperties* props);

    u64 size() const { return size_; }

    /* Assemble [logical_offset, logical_offset+len) into buf. Returns the
     * number of bytes placed; 0 only at/after EOF. `len` is clamped to
     * kMaxRead. Short results happen only at the logical EOF or when the
     * request itself exceeds it. Unreadable device spans are zero-filled
     * and recorded (query unreadable_spans()). */
    u32 read_at(u64 logical_offset, void* buf, u32 len);

    /* Truthful failure map: device spans that could not be read and were
     * zero-filled instead (mirrors the retry pipe's Skip policy). */
    const ReadErrorMap& unreadable_spans() const { return unreadable_; }

    /* Diagnostics: device read_at calls + bytes issued (bounded-read
     * proofs; a node claiming 1 TiB against a 1 KiB extent list cannot
     * make the reader read more than requested). */
    u64 device_reads() const { return device_reads_; }
    u64 device_bytes() const { return device_bytes_; }

    /* M-108: the previewed node's properties (the name the text-preview
     * eligibility gate reads the extension from - the og's
     * FUN_140089050 cached-extension consumer). Null only for the
     * synthetic test readers that bypass the node. */
    const tree::FileProperties* props() const { return props_; }

    /* M-028: the device-byte cost of a read_at(logical_offset, len)
     * BEFORE issuing it. Plain extents cost their overlapped length;
     * COMPRESSED extents cost the whole packed block (psize — any window
     * into a compressed extent decodes the block, so previewing one byte
     * of a 64 KiB block costs 64 KiB of device reads); resident data and
     * sparse gaps cost nothing; malformed compressed caps cost nothing
     * (the read path refuses them without a device access). Mirrors
     * read_at's extent arithmetic exactly — the Caps consumer consults
     * this to enforce its budget BEFORE the bytes move. */
    u64 read_cost(u64 logical_offset, u32 len) const;

private:
    IStorageDevice* source_;
    const tree::FileProperties* props_;
    u64 size_;
    ReadErrorMap unreadable_;
    u64 device_reads_ = 0;
    u64 device_bytes_ = 0;
};

/* ---- bounded metadata extraction (G-10) ---- */

struct Caps {
    u32 max_read = 4u << 20; /* total bytes pulled from the reader */
    u32 max_work = 1u << 22; /* work-unit guard (parser loop steps) */
    u32 text_snippet = 256;  /* extracted text bytes */
    /* M-077: the og's shipped preview PRODUCT caps (UIUX/10 §3,
     * QM-verified copy) as config-overridable defaults: files larger
     * than 100 MiB are "not available for preview" (the eligibility
     * gate) and decode-heavy extraction is bounded by a time guard.
     * Zero-init = the defaults ON (the og's caps are product policy,
     * not user opt-in); a host overrides by setting its own values
     * (UINT32_MAX disables a gate). */
    u32 max_file_bytes = 100u << 20; /* eligibility gate (the 100 MB line) */
    u32 max_decode_ms = 15000;       /* decode-work time guard (the 15 s line) */
};

/* M-066: one previewed archive entry (the ArchivePreviewModel row):
 * name, sizes, and per-entry CRC presence (the damaged-zip browsing
 * contract - a zero CRC marks the entries the repair chain cares
 * about). Same shape as the carve path's S-032 CarveEntry plus the
 * CRC flag the preview surface reports. */
struct ArchiveEntry {
    std::string name;         /* entry name (archive-native bytes, UTF-8 pass-through) */
    u64 size = 0;             /* uncompressed size */
    u64 compressed = 0;       /* compressed size */
    bool crc_present = false; /* CD CRC-32 != 0 (the damaged-entry marker) */
};

/* M-066: the hostile-archive bound on the preview inventory (512 entries
 * = the preview budget model's scale, not a decoded og constant - the
 * og's model is unbounded because the UI streams lazily; our extractor
 * is eager, so we cap + flag). */
constexpr u32 kMaxPreviewArchiveEntries = 512;

/* M-108: the text-preview eligibility gate (the og's TxtViewFiles
 * whitelist). The og decides which files preview as text by EXTENSION
 * MEMBERSHIP in config/FilterFolder.ini [Filter] TxtViewFiles -
 * decompile-verified (DRWUI_decompiled.c:535858/:535937): the ini loads
 * once (the ini-absent default is ".txt;"), the node's cached extension
 * is tested with the toupper-folded delimited search
 * (FUN_140310aa0 - ASCII case-insensitive), and content is NEVER
 * sniffed for eligibility. Consequences ported exactly: a whitelisted
 * extension (.txt/.log/.xml/... - 97 shipped tokens, 92 unique
 * case-folded; the generated pool is byte-exact from the corpus ini)
 * previews as text whatever its bytes (UTF-8 BOM, UTF-16, Latin-1 all
 * render); a non-whitelisted extension never previews as text (a
 * printable-head binary is NOT text); an extensionless name fails the
 * consumers' empty-needle early return (not viewable). The generated
 * pool: src/core/preview/cr_preview_text_exts_data.cpp (regenerate via
 * scripts/gen_preview_text_exts.py after corpus updates). */
namespace textexts {
/* Is this dot-prefixed ASCII extension (e.g. ".txt") a text-preview
 * format per the og's TxtViewFiles whitelist? Case-insensitive. */
bool eligible(std::string_view dot_ext);
namespace data {
extern const u8 kTxtViewPool[];
extern const u32 kTxtViewPoolLen;
extern const u32 kTxtViewTokenCount;
} // namespace data
} // namespace textexts

struct Metadata {
    std::string kind;   /* image|video|audio|document|archive|text|unsupported */
    std::string format; /* jpeg|png|gif|bmp|tiff|webp|mp4|wav|pdf|docx|zip|txt (N-088: no mov - no
                           producer) */
    u32 width = 0, height = 0;
    std::string orientation;  /* EXIF orientation as "N" (1..8), "" when absent */
    std::string datetime;     /* EXIF DateTimeOriginal (YYYY:MM:DD HH:MM:SS) */
    u64 duration_ms = 0;      /* media duration */
    std::string codec;        /* container brand / audio codec */
    std::string text_snippet; /* bounded printable text (documents) */
    bool truncated = false;   /* extraction hit a cap (caller may re-run
                               * with higher caps - never a silent loss) */
    /* M-066: the archive-content inventory (the og's ArchivePreviewModel
     * browse-inside-archive contract, UIUX/10:17) - emitted for ANY
     * archive whose central directory is walkable at preview time,
     * carved or FS-origin alike. Bounded: kMaxPreviewArchiveEntries +
     * the truncated flag (the preview budget model). */
    std::vector<ArchiveEntry> entries;
};

/* Sniff + extract within the caps. `pr` supplies the bytes; the extractor
 * never exceeds Caps. Unknown/oversized formats return
 * kind="unsupported" with the sniffed format named - never garbage.
 * Note: `pr` is non-const - read_at accumulates the reader's diagnostics
 * (device_reads/unreadable_spans); concurrent extractors each construct
 * their own PreviewReader (they are two pointers - cheap). */
Metadata extract_metadata(PreviewReader& pr, const Caps& caps = Caps{});

} // namespace preview
} // namespace cr

#endif // CR_PREVIEW_H_
