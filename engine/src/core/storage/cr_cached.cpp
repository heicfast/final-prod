/* cr_cached.cpp - bounded LRU read cache.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_cached.h"
#include <cstring>

namespace cr {

std::unique_ptr<CachedReader> CachedReader::create(std::unique_ptr<IStorageDevice> inner,
                                                   u64 cache_bytes, u32 block_bytes) {
    if (!inner || block_bytes == 0 || cache_bytes < block_bytes) return nullptr;
    auto r = std::unique_ptr<CachedReader>(new CachedReader());
    r->inner_ = std::move(inner);
    r->block_bytes_ = block_bytes;
    r->capacity_ = static_cast<size_t>(cache_bytes / block_bytes);
    if (r->capacity_ == 0) return nullptr;
    r->blocks_.reserve(r->capacity_);
    return r;
}

CachedReader::~CachedReader() = default;

u32 CachedReader::read_at(u64 offset, void *buffer, u32 length) {
    if (length == 0) return OK;
    if (offset > inner_->size() || u64(length) > inner_->size() - offset)
        return ERR_RANGE_OUT_OF_BOUNDS;

    u8 *dst = static_cast<u8 *>(buffer);
    u64 pos = offset;
    u32 remain = length;
    while (remain > 0) {
        u64 block_start = round_down(pos, block_bytes_);
        u32 in_block_off = static_cast<u32>(pos - block_start);
        u64 block_end_avail = block_bytes_ - in_block_off;
        u32 take = static_cast<u32>(remain < block_end_avail ? remain : block_end_avail);

        /* find block; move to front (LRU) */
        size_t found = blocks_.size();
        for (size_t i = 0; i < blocks_.size(); ++i) {
            if (blocks_[i].valid && blocks_[i].start == block_start) {
                found = i;
                break;
            }
        }
        if (found == blocks_.size()) {
            ++misses_;
            Block b;
            b.start = block_start;
            b.valid = false;
            u32 blen = static_cast<u32>(
                block_start + block_bytes_ <= inner_->size() ? block_bytes_
                                                             : inner_->size() - block_start);
            b.data.resize(blen);
            u32 rc = inner_->read_at(block_start, b.data.data(), blen);
            if (rc != OK && rc != ERR_RETRY_EXHAUSTED) return rc;
            b.valid = (rc == OK);
            blocks_.push_back(std::move(b));
            found = blocks_.size() - 1;
        } else {
            ++hits_;
        }
        /* LRU move-to-front */
        if (found != 0) {
            Block tmp = std::move(blocks_[found]);
            for (size_t i = found; i > 0; --i) blocks_[i] = std::move(blocks_[i - 1]);
            blocks_[0] = std::move(tmp);
            found = 0;
        }
        if (!blocks_[0].valid) {
            /* the underlying transport failed this block: fail the read and
             * let the caller's retry policy record it (R1.6) */
            return ERR_READ_FAILED;
        }
        std::memcpy(dst, blocks_[0].data.data() + in_block_off, take);
        dst += take;
        pos += take;
        remain -= take;

        /* evict beyond capacity */
        while (blocks_.size() > capacity_) blocks_.pop_back();
    }
    return OK;
}

} // namespace cr
