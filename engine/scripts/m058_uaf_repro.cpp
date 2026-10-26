/* M-058 fail-before reproduction: the ORIGINAL run() inline construction,
 * verbatim shape, with a cache geometry that makes CachedReader::create
 * fail. create() destroys the retry device it took by value; the old
 * fallback then reads through the dangling raw pointer -> ASan
 * heap-use-after-free. Compiled against the real headers. */
#include "cr/storage/cr_cached.h"
#include "cr/storage/cr_retry.h"
#include "cr/storage/cr_slice.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

using namespace cr;

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(size_t n) : data_(n, 0) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = n / 512;
        geom_.total_bytes = n;
        if (n >= 512) {
            data_[510] = 0x55;
            data_[511] = 0xAA;
        }
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

int main() {
    MemDevice slice(64 * 1024);
    /* the pre-M-058 inline sequence (cr_scan.cpp run(), audited shape) */
    RetryPolicy policy;
    policy.max_attempts = 3;
    policy.mode = RetryMode::Skip;
    auto shim = std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(&slice));
    auto retry = RetryingDevice::create(std::move(shim), policy);
    RetryingDevice* retry_raw = retry.get();
    /* FAILING cache geometry (run() passes valid constants - this is the
     * defensive path; the geometry is parameterized here to reach it) */
    auto cached = CachedReader::create(std::move(retry), 4096, 8192);
    IStorageDevice* pipe = cached ? static_cast<IStorageDevice*>(cached.get())
                                  : (retry_raw ? static_cast<IStorageDevice*>(retry_raw) : &slice);
    std::printf("cached=%p pipe=%p retry_raw=%p\n", (void*)cached.get(), (void*)pipe,
                (void*)retry_raw);
    /* the read the engines would do: dereferences the destroyed device */
    u8 boot[512] = {};
    u32 rc = pipe->read_at(0, boot, sizeof(boot));
    std::printf("rc=%u boot[510]=%02x\n", rc, boot[510]);
    return 0;
}
