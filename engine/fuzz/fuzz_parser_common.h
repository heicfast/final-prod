/* fuzz_parser_common.h - shared scaffolding for the per-parser libFuzzer
 * harnesses (audit row 47): a bounded MemDevice over the input bytes and
 * the three-root result tree every engine scan needs.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FUZZ_PARSER_COMMON_H_
#define CR_FUZZ_PARSER_COMMON_H_

#include "cr/orchestrator/cr_scan.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <memory>
#include <vector>

namespace crfuzz {

class MemDevice final : public cr::IStorageDevice {
public:
    MemDevice(const uint8_t* d, size_t n) : d_(d), n_(n) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = n;
        g_.sector_count = n / 512;
    }
    cr::u32 read_at(cr::u64 off, void* buf, cr::u32 len) override {
        if (off > n_ || len > n_ - off)
            return cr::ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_ + off, len);
        return cr::OK;
    }
    const cr::Geometry& geometry() const override { return g_; }

private:
    const uint8_t* d_;
    size_t n_;
    cr::Geometry g_{};
};

struct ScanSetup {
    cr::tree::ResultTree tree;
    cr::tree::FullNode* ra;
    cr::tree::FullNode* rd;
    cr::tree::FullNode* rl;
    ScanSetup() : ra(tree.add_root(u"a")), rd(tree.add_root(u"d")), rl(tree.add_root(u"l")) {}
};

} // namespace crfuzz

#endif // CR_FUZZ_PARSER_COMMON_H_
