/* CRSF session container tests (R6.3, R6.6, F10). */
#include <gtest/gtest.h>
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::session;

namespace {

/* In-memory sink/source. */
struct MemFile {
    std::vector<u8> data;
    u64 pos = 0;
    static u32 wr(void* u, const void* b, u32 len) {
        auto* m = static_cast<MemFile*>(u);
        if (m->pos + len > m->data.size())
            m->data.resize(size_t(m->pos + len));
        std::memcpy(m->data.data() + m->pos, b, len);
        m->pos += len;
        return 0;
    }
    static u64 tl(void* u) { return static_cast<MemFile*>(u)->pos; }
    static u32 sk(void* u, u64 abs) {
        auto* m = static_cast<MemFile*>(u);
        m->pos = abs;
        return 0;
    }
    static u32 rd(void* u, void* b, u32 len) {
        auto* m = static_cast<MemFile*>(u);
        if (m->pos + len > m->data.size())
            return 1;
        std::memcpy(b, m->data.data() + m->pos, len);
        m->pos += len;
        return 0;
    }
    static u64 sz(void* u) { return static_cast<MemFile*>(u)->data.size(); }
};

std::vector<u8> compressible_payload() {
    std::vector<u8> p;
    for (int i = 0; i < 5000; ++i)
        p.push_back(u8('A' + (i % 3))); /* compresses well */
    return p;
}

} // namespace

TEST(Crsf, RoundTrip) {
    MemFile mf;
    {
        Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(0x1122334455667788ULL), OK);
        auto p1 = compressible_payload();
        ASSERT_EQ(w.add_record(REC_SESSION_STATE, p1.data(), u32(p1.size())), OK);
        const char* msg = "volume C: NTFS, 512e";
        ASSERT_EQ(w.add_record(REC_VOLUME_INFO, msg, u32(strlen(msg))), OK);
        u8 zeros[8192];
        std::memset(zeros, 0, sizeof(zeros)); /* zeros: deflate loses, stored wins */
        ASSERT_EQ(w.add_record(REC_ERROR_MAP, zeros, sizeof(zeros)), OK);
        ASSERT_EQ(w.finalize(), OK);
    }

    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    u32 count = 0;
    ASSERT_EQ(r.open(&count), OK);
    EXPECT_EQ(count, 3u);

    Record rec;
    ASSERT_EQ(r.read_record(0, rec), OK);
    EXPECT_EQ(rec.type, REC_SESSION_STATE);
    EXPECT_EQ(rec.session_id, 0x1122334455667788ULL);
    ASSERT_EQ(rec.data.size(), 5000u);
    auto expect = compressible_payload();
    EXPECT_EQ(std::memcmp(rec.data.data(), expect.data(), 5000), 0);

    ASSERT_EQ(r.read_record(1, rec), OK);
    EXPECT_EQ(rec.type, REC_VOLUME_INFO);
    EXPECT_EQ(rec.data.size(), strlen("volume C: NTFS, 512e"));
    EXPECT_EQ(std::memcmp(rec.data.data(), "volume C: NTFS, 512e", rec.data.size()), 0);

    ASSERT_EQ(r.read_record(2, rec), OK);
    ASSERT_EQ(rec.data.size(), 8192u);
    bool all_zero = true;
    for (u8 b : rec.data)
        if (b != 0) {
            all_zero = false;
            break;
        }
    EXPECT_TRUE(all_zero);
}

TEST(Crsf, CorruptionSkipsForward) {
    MemFile mf;
    {
        Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(1), OK);
        auto p = compressible_payload();
        ASSERT_EQ(w.add_record(REC_SESSION_STATE, p.data(), u32(p.size())), OK);
        ASSERT_EQ(w.add_record(REC_VOLUME_INFO, "v2", 2), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    /* corrupt the FIRST record's data area (mid-file) - its CRC must fail;
     * the second record must still be readable (F10 skip-forward). */
    const size_t cAt = mf.data.size() / 2; /* safely inside the buffer */
    mf.data[cAt] ^= 0x5A;

    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    u32 count = 0;
    ASSERT_EQ(r.open(&count), OK);
    ASSERT_EQ(count, 2u);
    Record rec;
    /* record 0: CRC mismatch -> BAD_CHECKSUM, data cleared */
    EXPECT_EQ(r.read_record(0, rec), 0x50030002u);
    EXPECT_TRUE(rec.data.empty());
    /* record 1: intact */
    ASSERT_EQ(r.read_record(1, rec), OK);
    EXPECT_EQ(rec.data.size(), 2u);
}

TEST(Crsf, TreeRoundTrip) {
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume C");
    t.refresh_stats();
    tree::FileProperties* p1 = new tree::FileProperties();
    p1->name = u"readme.txt";
    p1->record_ref = 1000;
    p1->logical_size = 1234;
    p1->extents.push_back({4096, 1234});
    p1->times.created = 132000000000000000ULL;
    auto* f1 = t.add_file(root, p1);
    p1->release();
    f1->set_deleted_state(tree::DeletedState::Deleted);
    f1->set_confidence(tree::Confidence::Excellent);
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"sub";
    p2->attributes = 0x10;
    auto* d1 = t.add_file(root, p2);
    p2->release();
    tree::FileProperties* p3 = new tree::FileProperties();
    p3->name = u"inner.dat";
    p3->logical_size = 99;
    (void)t.add_file(d1, p3);
    p3->release();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), OK);
    EXPECT_GT(blob.size(), 100u);

    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), OK);
    EXPECT_EQ(t2.node_count(), t.node_count());
    /* find inner under sub under root */
    tree::FileNode *sub = nullptr, *inner = nullptr, *readme = nullptr;
    for (tree::FileNode* r : t2.roots()) {
        for (tree::FileNode* c = r->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "readme.txt")
                readme = c;
            if (c->name_utf8() == "sub")
                sub = c;
        }
    }
    ASSERT_NE(sub, nullptr);
    ASSERT_NE(readme, nullptr);
    inner = sub->first_child() ? sub->first_child() : nullptr;
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->name_utf8(), "inner.dat");
    EXPECT_EQ(readme->props()->logical_size, 1234u);
    EXPECT_EQ(readme->props()->times.created, 132000000000000000ULL);
    EXPECT_EQ(readme->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_EQ(readme->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(readme->props()->extents.size(), 1u);
    EXPECT_EQ(readme->props()->extents[0].offset, 4096u);
}
