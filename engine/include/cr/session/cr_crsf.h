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
#include <memory>
#include <vector>

namespace cr {
namespace session {

constexpr u8 CRSF_MAGIC[8] = {'C', 'R', 'S', 'F', 'M', 'T', '0', '1'};
constexpr u32 CRSF_VERSION = 1;
constexpr u32 CRSF_UNIT = 512;
constexpr u32 CRSF_HEADER_BYTES = 64;
constexpr u32 CRSF_RECORD_HEADER_BYTES = 24;
constexpr u32 CRSF_CHUNK_MAX = 4096;

enum RecordType : u32 {
    REC_SESSION_STATE = 1,
    REC_VOLUME_INFO = 2,
    REC_PARTITION_CONTEXT = 3,
    REC_RESULT_TREE = 4,
    REC_ERROR_MAP = 5,
};

/* In-memory record. */
struct Record {
    RecordType type = REC_SESSION_STATE;
    u64 session_id = 0;
    std::vector<u8> data;    /* DECOMPRESSED bytes */
};

class Writer {
public:
    /* sink callbacks keep the container pure-core (no file API here):
     * write(bytes,len) -> u32 rc; seek(absolute) used by finalize patch. */
    using WriteFn = u32 (*)(void *user, const void *bytes, u32 len);
    using TellFn = u64 (*)(void *user);
    using SeekFn = u32 (*)(void *user, u64 absolute);

    Writer(WriteFn write, TellFn tell, SeekFn seek, void *user);
    ~Writer();

    u32 open(u64 session_id);          /* emits the container header */
    u32 add_record(RecordType type, const void *data, u32 len);
    u32 finalize();                    /* patches record_count + chain fields */

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class Reader {
public:
    using ReadFn = u32 (*)(void *user, void *buf, u32 len);
    using SeekFn = u32 (*)(void *user, u64 absolute);
    using SizeFn = u64 (*)(void *user);

    Reader(ReadFn read, SeekFn seek, SizeFn size, void *user);
    ~Reader();

    /* Reads the header; validates magic/version. */
    u32 open(u32 *record_count_out);
    /* Reads record #index (0-based, in file order). */
    u32 read_record(u32 index, Record &out);
    u32 record_count() const { return count_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    u32 count_ = 0;
};

/* Chunk framing exposed for tests. */
u32 crsf_chunk_frame(const u8 *payload, u32 len, bool compress, std::vector<u8> &out);
u32 crsf_chunk_parse(const u8 *data, u32 len, std::vector<u8> &out);

} // namespace session
} // namespace cr

#endif // CR_CRSF_H_
