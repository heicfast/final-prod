/* fuzz_pure.cpp - libFuzzer harness for the pure parsing primitives.
 *
 * Input dispatch byte selects the function under test; the remaining
 * bytes are the argument buffer. Every target must return an error code
 * or produce output WITHOUT reading out of bounds, hanging or crashing -
 * the sanitizers (ASan/UBSan) enforce that.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_runlist.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/table/cr_table_wire.h"
#include "cr/crypto/cr_sha256.h"
#include "cr/repair/cr_rawfixer.h"
#include <cstring>
#include <vector>

namespace {
/* stable local copy to keep the harness deterministic */
cr::ntfs::RunList g_rl;
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 5) /* case 0 reads buf[0..3]; len = size-1 */
        return 0;
    const uint8_t sel = data[0];
    const uint8_t* buf = data + 1;
    const size_t len = size - 1;
    switch (sel % 8) {
    case 0: {
        /* NTFS USA fixup: parameters derived from the input tail (masked
         * to sane ranges so the validator still sees adversarial but
         * structurally plausible values) */
        const uint32_t usa_off = uint32_t(buf[0]) | (uint32_t(buf[1]) << 8);
        const uint32_t usa_cnt = uint32_t(buf[2]);
        const uint32_t sec = 512u << (buf[3] % 4);
        std::vector<uint8_t> rec(buf, buf + len);
        (void)cr::ntfs::fixup_validate(rec.data(), rec.size(), usa_off, usa_cnt, sec);
        if (cr::ntfs::fixup_validate(rec.data(), rec.size(), usa_off, usa_cnt, sec) == cr::OK) {
            /* apply only on validated records (in-place, bounds-checked) */
            (void)cr::ntfs::fixup_apply(rec.data(), rec.size(), usa_off, usa_cnt, sec);
        }
        break;
    }
    case 1: {
        /* NTFS runlist decode: first_vcn/max_clusters from the tail */
        const uint64_t first_vcn = uint64_t(buf[0]) | (uint64_t(buf[1]) << 8);
        const uint32_t max_clusters = 1u << (buf[2] % 24);
        cr::ntfs::RunList rl;
        (void)cr::ntfs::RunList::decode(buf + 3, uint32_t(len - 3), first_vcn, max_clusters, rl);
        /* encode round-trip of whatever decoded */
        (void)rl.encode();
        break;
    }
    case 2: {
        /* FAT LFN checksum + short-name conversion */
        uint8_t sn[11] = {};
        for (int i = 0; i < 11 && size_t(i + 1) < len; ++i)
            sn[i] = buf[i];
        (void)cr::fat::lfn_checksum(sn);
        (void)cr::fat::short_name_to_u16(sn);
        break;
    }
    case 3: {
        /* exFAT checksums: entry-set over the buffer (2-byte entries).
         * M-013: the dual-form (raw + InUse-restored) and the validity
         * test join the fuzzed surface - torn-set / bad-checksum mutants
         * drive the deleted-set gate's exact arithmetic. */
        (void)cr::exfat::checksum32(buf, uint32_t(len), 0);
        (void)cr::exfat::checksum16(buf, uint32_t(len), 0);
        if (len >= 32 && len % 32 == 0) {
            (void)cr::exfat::entry_set_checksum(buf, uint32_t(len / 32));
            (void)cr::exfat::entry_set_checksum_dual(buf, uint32_t(len / 32)).raw;
            (void)cr::exfat::entry_set_checksum_valid(buf, uint32_t(len / 32),
                                                      uint16_t(len));
        }
        break;
    }
    case 4: {
        /* CRC32 + SHA-256 (deterministic digest, no crash) */
        (void)cr::table::crc32(buf, len);
        uint8_t digest[32];
        cr::crypto::Sha256 s;
        s.update(buf, uint32_t(len));
        s.final(digest);
        break;
    }
    case 5: {
        /* RawFixer undo-log container deserialization */
        cr::repair::UndoLog log;
        (void)cr::repair::UndoLog::deserialize(buf, len, log);
        if (!log.entries.empty()) {
            std::vector<uint8_t> out;
            log.serialize(out);
        }
        break;
    }
    case 6: {
        /* NTFS attributes: enum over a record-sized buffer with
         * parameters from the head bytes. The record base is buf+4, so
         * the guard must cover record_size PLUS the 4-byte offset - the
         * smoke campaign's ASan heap-buffer-overflow (u16_from_bytes_le
         * at the record tail) was this harness off-by-4, not a product
         * defect: enum_attributes is bounds-guarded against its own
         * record_size. */
        if (len >= 32) {
            const uint32_t record_size = 512u << (buf[0] % 4);
            if (size_t(record_size) + 4 <= len) {
                std::vector<cr::ntfs::AttrView> attrs;
                (void)cr::ntfs::enum_attributes(buf + 4, record_size, attrs);
            }
        }
        break;
    }
    case 7: {
        /* runlist vcn_to_lcn lookups on the previously decoded list */
        std::vector<cr::ntfs::RunList::Extent> ex;
        (void)cr::ntfs::RunList::decode(buf, uint32_t(len), 0, 0x100000, g_rl);
        g_rl.extents(ex);
        for (size_t i = 0; i < ex.size() && i < 64; ++i) {
            cr::u64 lcn = 0;
            bool sparse = false;
            (void)g_rl.vcn_to_lcn(ex[i].lcn, &lcn, &sparse);
        }
        break;
    }
    }
    return 0;
}
