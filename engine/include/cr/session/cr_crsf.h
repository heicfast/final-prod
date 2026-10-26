/*
 * cr_crsf.h - the CRSF session container (R6.3, R6.6).
 *
 * Own documented format; the chunked-zlib framing is the scheme PROVEN by
 * the reference .rsf reversal (docs/ground-truth-contracts.md section 8):
 *   - chunk: u16 header, bit15 = deflate, bits0-11 = payload-1 (<= 4096)
 *   - stored form when compression does not shrink (explicit comparison)
 *   - level-5 deflate; reader falls back to the raw payload on inflate
 *     failure (graceful degradation)
 * Our additions over the reference (fix F10): CRC32 per record + per chunk
 * count; corruption skips FORWARD to the next well-formed record (resumable
 * by construction); explicit container/record magics and a version field.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_CRSF_H_
#define CR_CRSF_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include <ctime>
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace session {

constexpr u8 CRSF_MAGIC[8] = {'C', 'R', 'S', 'F', 'M', 'T', '0', '1'};
constexpr u32 CRSF_VERSION = 1;
constexpr u32 CRSF_UNIT = 512;
constexpr u32 CRSF_HEADER_BYTES = 64;
constexpr u32 CRSF_RECORD_HEADER_BYTES = 24;
constexpr u32 CRSF_CHUNK_MAX = 4096;

/* Container header flags (offset +0x0C, reserved 0 in v1 writes):
 *   bit 0 - CRSF_FLAG_ABS_EXTENTS: the RESULT_TREE payload carries
 *           ABSOLUTE device offsets (F-1 basis normalization). Absent =
 *           the pre-F-1 slice-relative basis; loaders normalize old
 *           sessions using the VolumeInfo.slice_offset on load. */
constexpr u32 CRSF_FLAG_ABS_EXTENTS = 0x1;

enum RecordType : u32 {
    REC_SESSION_STATE = 1,
    REC_VOLUME_INFO = 2,
    REC_PARTITION_CONTEXT = 3,
    REC_RESULT_TREE = 4,
    REC_ERROR_MAP = 5,
    /* S-023: the post-scan grading statistics (the og RecoveryAssistant
     * block). Old readers skip unknown types; the record is OPTIONAL - a
     * session saved by a build without the grading pass simply lacks it
     * and the counters re-derive as zeros on load. */
    REC_ASSISTANT_STATS = 6,
    /* S-009: the downsampled engine free map (the IBuildVolumeBitmap
     * truth, cell-granular and budget-bounded). OPTIONAL with the same
     * skip-unknown-types rule - sessions saved before the free overlay
     * lack it and the disk map degrades to unknown cells, never a
     * guess. Payload: u32 cluster_bytes, u32 reserved(0), u64 base_byte,
     * u64 cluster_count, u32 bits_len, bits bytes. */
    REC_ALLOC_MAP = 7,
    /* M-042: the whole-disk layout snapshot at save time (the og's
     * partinfo.dat). OPTIONAL with the same skip-unknown-types rule;
     * readers compare it against the freshly enumerated table at search
     * start to surface the "layout changed" flag (surfaced, never
     * enforced — the per-record candidate re-validation is the drop
     * path). Payload: u32 count + per-entry {u64 start, u64 count,
     * u32 type}. */
    REC_DISK_LAYOUT = 8,
};

/* In-memory record. */
struct Record {
    RecordType type = REC_SESSION_STATE;
    u64 session_id = 0;
    std::vector<u8> data; /* DECOMPRESSED bytes */
};

class Writer {
public:
    /* sink callbacks keep the container pure-core (no file API here):
     * write(bytes,len) -> u32 rc; seek(absolute) used by finalize patch. */
    using WriteFn = u32 (*)(void* user, const void* bytes, u32 len);
    using TellFn = u64 (*)(void* user);
    using SeekFn = u32 (*)(void* user, u64 absolute);

    Writer(WriteFn write, TellFn tell, SeekFn seek, void* user);
    ~Writer();

    u32 open(u64 session_id); /* emits the container header */
    u32 add_record(RecordType type, const void* data, u32 len);
    u32 finalize(); /* patches record_count + chain fields */
    /* F-1: container flags written into the header. The scan-side save
     * paths set CRSF_FLAG_ABS_EXTENTS; the default (0) matches v1. */
    void set_flags(u32 flags) { flags_ = flags; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    u32 flags_ = 0;
};

class Reader {
public:
    using ReadFn = u32 (*)(void* user, void* buf, u32 len);
    using SeekFn = u32 (*)(void* user, u64 absolute);
    using SizeFn = u64 (*)(void* user);

    Reader(ReadFn read, SeekFn seek, SizeFn size, void* user);
    ~Reader();

    /* Reads the header; validates magic/version. */
    u32 open(u32* record_count_out);
    /* Reads record #index (0-based, in file order). */
    u32 read_record(u32 index, Record& out);
    u32 record_count() const { return count_; }
    /* F-1: container flags from the header (0 = pre-F-1 slice-relative
     * basis; loaders normalize with the session's VolumeInfo). */
    u32 flags() const { return flags_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    u32 count_ = 0;
    u32 flags_ = 0;
};

/* Chunk framing exposed for tests. */
u32 crsf_chunk_frame(const u8* payload, u32 len, bool compress, std::vector<u8>& out);
u32 crsf_chunk_parse(const u8* data, u32 len, std::vector<u8>& out);

/* ---- N-061: the session auto-naming convention ---------------------- */

/* The og's convention (corpus qa-logs--session-forensics.md: the shipped
 * "auto_Local Disk (I_)10_14_23_13_21" .rsf): "auto_" + the volume label
 * with Windows-name-forbidden characters sanitized to '_' (the sample's
 * ':' becomes '_'), followed by the LOCAL-time stamp M_D_H_m_s (2-digit,
 * underscore-separated). UTF-8 out (the label encodes through the shared
 * text policy). auto_session_name_at takes an explicit tm (deterministic
 * tests); auto_session_name reads the local clock (naming call - single
 * thread expected, like every file-naming path). */
std::string auto_session_name_at(const std::u16string& label, const std::tm& local);
std::string auto_session_name(const std::u16string& label);

} // namespace session
} // namespace cr

#endif // CR_CRSF_H_
