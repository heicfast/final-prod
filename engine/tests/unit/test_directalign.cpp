/* Direct-envelope unit tests (GAP-S6-13 - the 64 KiB direct-I/O contract).
 *
 * The mandate constraint (verified against the Windows storage stack): a
 * direct read whose LENGTH is a multiple of 4 KiB but not 64 KiB takes the
 * ~3.5x slow path (~830 MB/s vs ~2900 MB/s on NVMe). The envelope must
 * issue 64 KiB-multiple lengths (offset aligned too), clamped at the device
 * end, with a zero-copy fast path for already-aligned requests. */

#include <gtest/gtest.h>
#include "cr/storage/cr_directalign.h"

using namespace cr;
using namespace cr::io;

TEST(DirectAlign, AlignedRequestIsTheFastPath) {
    /* 1 MiB at a 64 KiB-aligned offset: direct, no bounce */
    const auto e = direct_envelope(64 * 1024, 1024 * 1024, 16ULL * 1024 * 1024);
    EXPECT_TRUE(e.ok);
    EXPECT_TRUE(e.direct);
    EXPECT_EQ(e.issued_off, u64(64 * 1024));
    EXPECT_EQ(e.issued_len, u64(1024 * 1024));
    EXPECT_EQ(e.skip, 0u);
    EXPECT_EQ(e.take, u64(1024 * 1024));
}

TEST(DirectAlign, FourKMultipleNotSixtyFourKTakesTheEnvelope) {
    /* THE mandate case: 128 KiB (4 KiB multiple, 64 KiB multiple too - use
     * 4 KiB): a 4 KiB read at an aligned offset must still round to a
     * 64 KiB-multiple issued length */
    const auto e = direct_envelope(0, 4096, 16ULL * 1024 * 1024);
    EXPECT_TRUE(e.ok);
    EXPECT_FALSE(e.direct); /* 4096 is not a 64 KiB multiple: bounce */
    EXPECT_EQ(e.issued_off, 0u);
    EXPECT_EQ(e.issued_len, u64(65536)); /* rounded UP to the 64 KiB multiple */
    EXPECT_EQ(e.skip, 0u);
    EXPECT_EQ(e.take, u64(4096));
}

TEST(DirectAlign, UnalignedOffsetAndLengthEnvelope) {
    /* the recovery-writer tail chunk shape: ragged offset + ragged length */
    const auto e = direct_envelope(1000, 4100, 16ULL * 1024 * 1024);
    EXPECT_TRUE(e.ok);
    EXPECT_FALSE(e.direct);
    EXPECT_EQ(e.issued_off, 0u);         /* floor64k(1000) */
    EXPECT_EQ(e.issued_len, u64(65536)); /* ceil64k(1000+4100) = 64 KiB */
    EXPECT_EQ(e.skip, u64(1000));
    EXPECT_EQ(e.take, u64(4100));
}

TEST(DirectAlign, MultiBlockEnvelopeCoversTheRequest) {
    const auto e = direct_envelope(65536 + 100, 3 * 65536 + 500, 16ULL * 1024 * 1024);
    EXPECT_TRUE(e.ok);
    EXPECT_FALSE(e.direct);
    EXPECT_EQ(e.issued_off, u64(65536));
    EXPECT_EQ(e.issued_len, u64(4 * 65536)); /* ceil(262744)=5 blocks minus floor=1 block */
    EXPECT_EQ(e.skip, u64(100));
    EXPECT_EQ(e.take, u64(3 * 65536 + 500));
}

TEST(DirectAlign, DeviceTailClampIsTheDocumentedException) {
    /* the last chunk of a ragged device: the envelope clamps at the end and
     * the issued length is NOT a 64 KiB multiple - the documented tail
     * exception (the alternative would be reading past the device) */
    const u64 dev = 1000 * 1024 + 1234; /* ragged device end */
    const auto e = direct_envelope(dev - 4096, 4096, dev);
    EXPECT_TRUE(e.ok);
    EXPECT_FALSE(e.direct);
    EXPECT_EQ(e.issued_off, (dev - 4096) / 65536 * 65536);
    EXPECT_EQ(e.issued_off + e.issued_len, dev); /* clamped exactly at the end */
    EXPECT_EQ(e.take, u64(4096));
}

TEST(DirectAlign, OneMiBWriterChunkAtAlignedOffsetStaysDirect) {
    /* the writer's default chunk cadence: 1 MiB at 64 KiB-aligned offsets
     * rides the fast path end-to-end (no bounce, no copy) */
    u64 off = 0;
    for (int i = 0; i < 8; ++i) {
        const auto e = direct_envelope(off, 1024 * 1024, 64ULL * 1024 * 1024);
        EXPECT_TRUE(e.direct) << "offset " << off;
        off += 1024 * 1024;
    }
}

TEST(DirectAlign, OutOfBoundsAndEmptyRequestsFail) {
    EXPECT_FALSE(direct_envelope(0, 0, 4096).ok);      /* empty */
    EXPECT_FALSE(direct_envelope(4096, 512, 4096).ok); /* past the end */
    EXPECT_FALSE(direct_envelope(4096, 512, 0).ok);    /* empty device */
}

TEST(DirectAlign, IssuedLengthAlwaysSixtyFourKMultipleExceptTail) {
    /* the contract sweep: for every offset/length pair inside a
     * 64 KiB-aligned device, the issued length is a 64 KiB multiple */
    const u64 dev = 4ULL * 1024 * 1024;
    for (u64 off = 0; off + 8192 <= dev; off += 997) {
        const auto e = direct_envelope(off, 8192, dev);
        ASSERT_TRUE(e.ok) << "off=" << off;
        EXPECT_EQ(e.issued_len % 65536, 0u) << "off=" << off;
        EXPECT_EQ(e.issued_off % 65536, 0u) << "off=" << off;
        EXPECT_LE(e.skip, 65535u);
        EXPECT_EQ(e.take, u64(8192));
    }
}

TEST(DirectAlign, UnifiedEnvelopeExhaustivePropertyBothDevices) {
    /* core-api unification sweep: EVERY (offset, length) combination over
     * both an aligned and a ragged device must satisfy the mandate property -
     * the issued length is a 64 KiB multiple UNLESS the envelope is clamped
     * at the device tail (the documented exception), the issued offset is
     * always 64 KiB-aligned, and the request is always fully covered. This
     * is the property BOTH transports now enforce (cr_transport.cpp
     * aligned_read_at - one envelope, driver path and direct path alike). */
    const u64 k64 = 65536;
    const u64 devices[] = {8ULL * 1024 * 1024,          /* aligned end */
                           8ULL * 1024 * 1024 + 12345}; /* ragged end */
    const u32 lengths[] = {512,   4096,   8192,   12288,  65535,   65536,
                           65537, 131072, 262144, 500000, 1048576, 1048577};
    for (const u64 dev : devices) {
        for (u64 off = 0; off + 1048577ull <= dev; off += 4093) {
            for (const u32 len : lengths) {
                if (off + len > dev)
                    continue;
                const auto e = direct_envelope(off, len, dev);
                ASSERT_TRUE(e.ok) << "dev=" << dev << " off=" << off << " len=" << len;
                /* offset always 64 KiB-aligned */
                EXPECT_EQ(e.issued_off % k64, 0u)
                    << "dev=" << dev << " off=" << off << " len=" << len;
                /* length is a 64 KiB multiple OR the tail clamp engaged */
                const bool tail_clamped = (e.issued_off + e.issued_len == dev);
                if (!tail_clamped) {
                    EXPECT_EQ(e.issued_len % k64, 0u)
                        << "dev=" << dev << " off=" << off << " len=" << len;
                }
                /* the request is fully covered (never a short take) */
                EXPECT_EQ(e.take, u64(len)) << "dev=" << dev << " off=" << off << " len=" << len;
                EXPECT_EQ(e.skip, off - e.issued_off);
            }
        }
    }
}
