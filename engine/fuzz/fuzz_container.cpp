/* fuzz_container.cpp - libFuzzer harness for the session/persistence
 * containers: CRSF reader (chunked zlib + CRC) and the record codecs.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/tree/cr_node.h"
#include <cstring>
#include <vector>

namespace {

struct Blob {
    const uint8_t* d;
    size_t n;
};

uint32_t blob_read_at(Blob* b, uint64_t off, void* buf, uint32_t len) {
    if (off > b->n || len > b->n - off)
        return 0;
    memcpy(buf, b->d + off, len);
    return len;
}
uint32_t b_read(void* user, void* buf, uint32_t len) {
    auto* ctx = static_cast<std::pair<Blob*, uint64_t>*>(user);
    const uint32_t got = blob_read_at(ctx->first, ctx->second, buf, len);
    ctx->second += got;
    return got;
}
uint32_t b_seek(void* user, uint64_t absolute) {
    auto* ctx = static_cast<std::pair<Blob*, uint64_t>*>(user);
    ctx->second = absolute;
    return cr::OK;
}
uint64_t b_size(void* user) {
    return static_cast<std::pair<Blob*, uint64_t>*>(user)->first->n;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 8)
        return 0;
    Blob blob{data, size};
    std::pair<Blob*, uint64_t> ctx{&blob, 0};
    cr::session::Reader r(b_read, b_seek, b_size, &ctx);
    uint32_t count = 0;
    if (r.open(&count) == cr::OK) {
        for (uint32_t i = 0; i < count && i < 64; ++i) {
            cr::session::Record rec;
            if (r.read_record(i, rec) == cr::OK) {
                /* typed payload codecs over decompressed data */
                if (rec.type == cr::session::REC_RESULT_TREE) {
                    cr::tree::ResultTree tree;
                    (void)cr::session::deserialize_tree(rec.data.data(), rec.data.size(), tree);
                    tree.reset();
                } else if (rec.type == cr::session::REC_ERROR_MAP) {
                    cr::ReadErrorMap m;
                    (void)cr::session::deserialize_error_map(rec.data.data(), rec.data.size(), m);
                } else if (rec.type == cr::session::REC_VOLUME_INFO) {
                    cr::session::VolumeInfo vi;
                    (void)cr::session::deserialize_volume_info(rec.data.data(), rec.data.size(),
                                                               vi);
                } else if (rec.type == cr::session::REC_SESSION_STATE) {
                    cr::session::SessionState st;
                    (void)cr::session::deserialize_session_state(rec.data.data(), rec.data.size(),
                                                                 st);
                }
            }
        }
    }
    return 0;
}
