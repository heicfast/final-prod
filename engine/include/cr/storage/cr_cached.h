/*
 * cr_cached.h - bounded LRU read cache in front of a transport.
 *
 * Clean-room equivalent of CStorageDeviceWithCache (corpus:
 * ntfs-engine-internals.md): "in front to survive MFT re-reads economically".
 * Bounded by construction; hit rate reported for benchmarking.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_CACHED_H_
#define CR_CACHED_H_

#include "cr_device.h"
#include <memory>

namespace cr {

class CachedReader final : public IStorageDevice {
public:
    /* cache_bytes is rounded down to a multiple of block_bytes (default 64 KiB
     * - the reference MFT batch size, corpus section 2). */
    static std::unique_ptr<CachedReader> create(std::unique_ptr<IStorageDevice> inner,
                                                u64 cache_bytes = 1 << 20,
                                                u32 block_bytes = 65536);

    u32 read_at(u64 offset, void *buffer, u32 length) override;
    const Geometry &geometry() const override { return inner_->geometry(); }

    IStorageDevice *inner() const { return inner_.get(); }
    u64 hits() const { return hits_; }
    u64 misses() const { return misses_; }

    ~CachedReader() override;

private:
    CachedReader() = default;
    struct Block {
        u64 start;
        std::vector<u8> data;
        bool valid;
    };
    std::unique_ptr<IStorageDevice> inner_;
    u32 block_bytes_ = 65536;
    size_t capacity_ = 0;
    /* simple true-LRU over a vector (small capacity, O(n) move-to-front is
     * fine at these sizes and keeps the code auditable) */
    std::vector<Block> blocks_;
    u64 hits_ = 0;
    u64 misses_ = 0;
};

} // namespace cr

#endif // CR_CACHED_H_
