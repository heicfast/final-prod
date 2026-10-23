/* cr_safewriter.cpp - recovery queue + policy (R7).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/recovery/cr_safewriter.h"
#include "cr/compress/cr_decomp.h"
#include <cstring>

namespace cr {
namespace recovery {

namespace {
bool intersects(u64 a_start, u64 a_len, u64 b_start, u64 b_len) {
    return a_start < b_start + b_len && b_start < a_start + a_len;
}
} // namespace

SafeWriter::SafeWriter(DestIo& io, IStorageDevice* source, WriterConfig cfg)
    : io_(io), source_(source), cfg_(std::move(cfg)) {}

void SafeWriter::set_destination(u64 dest_disk_start, u64 dest_disk_bytes) {
    dest_start_ = dest_disk_start;
    dest_bytes_ = dest_disk_bytes;
    have_dest_ = true;
}

std::u16string SafeWriter::sanitize_name(const std::u16string& in) {
    /* corpus rule: / * : ? " < > | replaced with _ (never empty; '.' refs
     * current dir - prefix underscore) */
    std::u16string out;
    out.reserve(in.size());
    for (u16 c : in) {
        bool bad = c == u'/' || c == u'*' || c == u':' || c == u'?' || c == u'"' || c == u'<' ||
                   c == u'>' || c == u'|';
        out.push_back(bad ? u'_' : c);
    }
    if (out.empty())
        out = u"_";
    if (out == u"." || out == u"..")
        out = u"_" + out;
    return out;
}

std::u16string SafeWriter::resolve_collision(const std::u16string& dir,
                                             const std::u16string& base_name) const {
    std::u16string prefix = dir;
    if (!prefix.empty() && prefix.back() != u'\\' && prefix.back() != u'/')
        prefix.push_back(u'\\');
    std::u16string cand = base_name;
    for (u32 attempt = 2; attempt < cfg_.rename_attempts; ++attempt) {
        if (!io_.exists(prefix + cand))
            return cand;
        char buf[16];
        snprintf(buf, sizeof(buf), " (%u)", attempt);
        std::u16string suffix;
        for (const char* p = buf; *p; ++p)
            suffix.push_back(u16(u8(*p)));
        /* insert before the LAST dot (keep the extension) */
        size_t last_dot = base_name.rfind(u'.');
        if (last_dot == std::u16string::npos || last_dot == 0) {
            cand = base_name + suffix;
        } else {
            cand = base_name.substr(0, last_dot) + suffix + base_name.substr(last_dot);
        }
        (void)prefix;
    }
    return base_name + u"_dup";
}

u32 SafeWriter::enqueue(tree::FileNode* node, const std::u16string& dest_dir) {
    if (node == nullptr || node->props() == nullptr || source_ == nullptr)
        return ERR_BAD_PARAMETER;
    const tree::FileProperties* pr = node->props();

    /* R7.2 / F12: hard refusal on source/destination overlap. The node's
     * extents are ABSOLUTE device offsets; the destination extent is the
     * destination volume's disk placement. */
    if (have_dest_ && dest_bytes_ > 0) {
        for (const auto& e : pr->extents) {
            if (intersects(e.offset, e.length, dest_start_, dest_bytes_)) {
                QueueItem refused;
                refused.node = node;
                refused.dest_dir = dest_dir;
                refused.item_id = next_id_++;
                refused.state = ItemState::Refused;
                refused.error = ERR_DEST_OVERLAP;
                queue_.push_back(refused);
                return ERR_DEST_OVERLAP;
            }
        }
    }

    QueueItem it;
    it.node = node;
    it.dest_dir = dest_dir;
    if (!it.dest_dir.empty() && it.dest_dir.back() != u'\\' && it.dest_dir.back() != u'/')
        it.dest_dir.push_back(u'\\');
    const std::u16string clean = sanitize_name(pr->name);
    it.final_name = resolve_collision(it.dest_dir, clean);
    it.item_id = next_id_++;
    char suf[32];
    snprintf(suf, sizeof(suf), ".part-%llu", (unsigned long long)it.item_id);
    it.temp_suffix.clear();
    for (const char* p = suf; *p; ++p)
        it.temp_suffix.push_back(u16(u8(*p)));
    it.bytes_total = pr->logical_size;
    it.state = ItemState::Pending;
    queue_.push_back(it);
    return OK;
}

u32 SafeWriter::recover_item(QueueItem& it, CancelToken* cancel, WriterStats& stats) {
    const tree::FileProperties* pr = it.node->props();
    if (pr->extents.empty() && pr->resident_data.empty()) {
        /* metadata-only: nothing to read; an empty marker file still helps
         * the user see the name - write empty */
    }
    /* resume detection (audit row 35a): a final file that already exists
     * is counted and skipped - re-running a recovery must be idempotent */
    if (io_.exists(it.dest_dir + it.final_name)) {
        it.state = ItemState::Done;
        ++stats.resumed;
        return OK;
    }
    it.state = ItemState::Reading;
    if (u32 rc = io_.create_temp(it.item_id, it.dest_dir + it.final_name, it.temp_suffix)) {
        it.state = ItemState::Error;
        it.error = rc;
        ++stats.errors;
        return rc;
    }
    it.state = ItemState::Writing;

    crypto::Sha256 hash;
    /* resident metadata content first (NTFS resident streams, ZFS embedded
     * block-pointer payloads, Btrfs inline extents), then disk extents */
    if (!pr->resident_data.empty()) {
        const u64 emit = pr->resident_data.size() > pr->logical_size && pr->logical_size != 0
                             ? pr->logical_size
                             : u64(pr->resident_data.size());
        if (emit) {
            if (u32 rc = io_.write(it.item_id, 0, pr->resident_data.data(), u32(emit))) {
                io_.close(it.item_id);
                it.state = ItemState::Error;
                it.error = rc;
                ++stats.errors;
                return rc;
            }
            hash.update(pr->resident_data.data(), u32(emit));
        }
    }
    std::vector<u8> chunk;
    chunk.resize(size_t(cfg_.read_chunk));
    u64 done = 0;
    const u64 total = pr->logical_size;
    size_t ext_i = 0;
    u64 ext_pos = 0;
    while (done < total) {
        if (cancel && cancel->cancelled()) {
            io_.close(it.item_id);
            it.state = ItemState::Error;
            it.error = ERR_CANCELLED;
            return ERR_CANCELLED;
        }
        u64 want = total - done;
        if (want > cfg_.read_chunk)
            want = cfg_.read_chunk;
        /* locate (extent, offset) for `done` */
        while (ext_i < pr->extents.size() && ext_pos + pr->extents[ext_i].length <= done) {
            ext_pos += pr->extents[ext_i].length;
            ++ext_i;
        }
        if (ext_i >= pr->extents.size()) {
            if (!pr->resident_data.empty()) {
                /* resident-only file: logical size may exceed the payload
                 * (padded); zero-fill the remainder */
                std::memset(chunk.data(), 0, size_t(want));
                if (u32 rc = io_.write(it.item_id, done, chunk.data(), u32(want))) {
                    io_.close(it.item_id);
                    it.state = ItemState::Error;
                    it.error = rc;
                    ++stats.errors;
                    return rc;
                }
                hash.update(chunk.data(), u32(want));
                done += want;
                it.bytes_done = done;
                continue;
            }
            io_.close(it.item_id);
            it.state = ItemState::Error;
            it.error = ERR_TRUNCATED;
            ++stats.errors;
            return ERR_TRUNCATED;
        }
        const u64 in_ext = done - ext_pos;
        u64 avail_in_ext = pr->extents[ext_i].length - in_ext;
        if (want > avail_in_ext)
            want = avail_in_ext;
        if (want > chunk.size())
            want = chunk.size();
        const u32 got = u32(want);
        if (pr->extents[ext_i].codec != 0) {
            /* compressed extent: decode the whole block on demand; the
             * logical bytes come from the decoded image, not the disk */
            const u64 psize =
                pr->extents[ext_i].psize ? pr->extents[ext_i].psize : pr->extents[ext_i].length;
            const u64 lsize = pr->extents[ext_i].length;
            if (psize == 0 || lsize == 0 || psize > (256ull << 20) || lsize > (256ull << 20)) {
                io_.close(it.item_id);
                it.state = ItemState::Error;
                it.error = ERR_BAD_PARAMETER;
                ++stats.errors;
                return ERR_BAD_PARAMETER;
            }
            std::vector<u8> packed(static_cast<size_t>(psize), 0);
            std::vector<u8> plain(static_cast<size_t>(lsize), 0);
            if (source_->read_at(pr->extents[ext_i].offset, packed.data(), u32(psize)) != OK) {
                std::memset(packed.data(), 0, size_t(psize)); /* Skip policy */
                std::memset(plain.data(), 0, size_t(lsize));
            } else if (!compress::decompress_block(pr->extents[ext_i].codec, packed.data(),
                                                   size_t(psize), plain.data(), size_t(lsize))) {
                /* malformed compressed block: zero-fill (hash will
                 * mismatch) - never silently emit wrong bytes */
                std::memset(plain.data(), 0, size_t(lsize));
            }
            const u64 have = lsize - in_ext;
            const u32 take = u32(have < u64(got) ? have : u64(got));
            if (take) {
                if (u32 rc = io_.write(it.item_id, done, plain.data() + in_ext, take)) {
                    io_.close(it.item_id);
                    it.state = ItemState::Error;
                    it.error = rc;
                    ++stats.errors;
                    return rc;
                }
                hash.update(plain.data() + in_ext, take);
            }
            done += take;
            it.bytes_done = done;
            if (take < got)
                ++ext_i, ext_pos += lsize; /* consumed this extent */
            continue;
        }
        const u32 got_raw = u32(want);
        if (source_->read_at(pr->extents[ext_i].offset + in_ext, chunk.data(), got_raw) != OK) {
            /* zero-fill the unreadable span (Skip policy) and continue -
             * the hash will intentionally mismatch on damaged content */
            std::memset(chunk.data(), 0, got_raw);
        }
        if (u32 rc = io_.write(it.item_id, done, chunk.data(), got_raw)) {
            io_.close(it.item_id);
            it.state = ItemState::Error;
            it.error = rc;
            ++stats.errors;
            return rc;
        }
        hash.update(chunk.data(), got_raw);
        done += got_raw;
        it.bytes_done = done;
    }

    it.state = ItemState::Verifying;
    if (cfg_.verify_hash && pr->hash_valid) {
        u8 digest[32];
        hash.final(digest);
        if (std::memcmp(digest, pr->sha128, 16) != 0) {
            ++stats.hash_mismatch;
            it.error = ERR_HASH_MISMATCH;
            /* the content is still committed (salvage); the mismatch is
             * reported per-item and aggregated */
        } else {
            it.hash_verified = true;
        }
    } else if (cfg_.verify_hash && !pr->hash_valid && !cfg_.skip_hashless) {
        /* no stored hash: compute-only verification impossible; mark ok */
        it.hash_verified = false;
    }
    if (u32 rc = io_.commit(it.item_id)) {
        io_.close(it.item_id);
        it.state = ItemState::Error;
        it.error = rc;
        ++stats.errors;
        return rc;
    }
    io_.close(it.item_id);
    it.state = ItemState::Done;
    ++stats.done;
    return OK;
}

u32 SafeWriter::process(CancelToken* cancel, bool (*progress)(u64, u64, void*), void* progress_user,
                        WriterStats& stats) {
    u64 total_bytes = 0;
    u64 done_bytes = 0;
    for (const auto& it : queue_)
        total_bytes += it.bytes_total;
    for (auto& it : queue_) {
        if (it.state == ItemState::Refused) {
            ++stats.refused;
            continue;
        }
        if (it.state == ItemState::Done) {
            done_bytes += it.bytes_total;
            continue;
        }
        if (u32 rc = recover_item(it, cancel, stats))
            return rc;
        done_bytes += it.bytes_total;
        if (progress && !progress(done_bytes, total_bytes ? total_bytes : 1, progress_user))
            return ERR_CANCELLED;
    }
    return OK;
}

} // namespace recovery
} // namespace cr
