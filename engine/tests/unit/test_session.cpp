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

/* SESS-4 (GAP-W3-9b): the reference compresses EVERY chunk and stores only
 * when deflate does not shrink it. The old `take >= 64` carve-out kept tiny
 * tails stored unconditionally - this pins the tiny-tail deflate behavior
 * at the frame unit (the strictly-smaller rule lives in crsf_chunk_frame). */
TEST(Crsf, TinyChunkDeflatesWhenSmaller) {
    std::vector<u8> tiny(40, 'B'); /* 40 highly-compressible bytes */
    std::vector<u8> framed;
    ASSERT_EQ(crsf_chunk_frame(tiny.data(), u32(tiny.size()), true, framed), OK);
    ASSERT_GE(framed.size(), 2u);
    const u16 hdr = u16(framed[0]) | (u16(framed[1]) << 8);
    EXPECT_NE(0u, hdr & 0x8000u) << "tiny compressible chunk must take the deflate path";
    EXPECT_LT(framed.size(), tiny.size() + 2) << "and must actually be smaller";
    /* parse inverts it */
    std::vector<u8> back;
    ASSERT_EQ(crsf_chunk_parse(framed.data(), u32(framed.size()), back), OK);
    EXPECT_EQ(back, tiny);
    /* incompressible tiny chunk stays stored (not-smaller rule) */
    std::vector<u8> noise(40);
    for (size_t i = 0; i < noise.size(); ++i)
        noise[i] = u8((i * 131 + 17) & 0xFF);
    std::vector<u8> stored;
    ASSERT_EQ(crsf_chunk_frame(noise.data(), u32(noise.size()), true, stored), OK);
    const u16 shdr = u16(stored[0]) | (u16(stored[1]) << 8);
    EXPECT_EQ(0u, shdr & 0x8000u) << "incompressible chunk must stay stored";
}

/* SESS-2 (GAP-W3-9a): the volume identity card round-trips, and v1 records
 * (52-byte payloads, pre-identity layout) deserialize with a zero card -
 * zeros mean "unverifiable", never "verified". */
TEST(Crsf, VolumeInfoIdentityCardRoundTrip) {
    VolumeInfo vi;
    vi.fs_kind = 3;
    vi.slice_offset = 512u * 63;
    vi.slice_bytes = 4096;
    vi.flags = 0x7;
    std::snprintf(vi.label, sizeof(vi.label), "TESTVOL");
    std::snprintf(vi.model, sizeof(vi.model), "ST5000DM000-1FK178");
    std::snprintf(vi.serial, sizeof(vi.serial), "W1234567");
    vi.boot_hash = 0xDEADBEEFu;
    std::vector<u8> blob;
    ASSERT_EQ(serialize_volume_info(vi, blob), OK);
    EXPECT_EQ(blob.size(), 80u + 84u) << "v2 layout: 80-byte v1 prefix + identity tail";

    VolumeInfo out;
    ASSERT_EQ(deserialize_volume_info(blob.data(), blob.size(), out), OK);
    EXPECT_STREQ(out.model, "ST5000DM000-1FK178");
    EXPECT_STREQ(out.serial, "W1234567");
    EXPECT_EQ(out.boot_hash, 0xDEADBEEFu);
    EXPECT_EQ(out.slice_offset, 512u * 63);

    /* v1 record: the identity tail is absent - zeros, not garbage */
    std::vector<u8> v1(blob.begin(), blob.begin() + 80);
    VolumeInfo old;
    ASSERT_EQ(deserialize_volume_info(v1.data(), v1.size(), old), OK);
    EXPECT_EQ(old.fs_kind, 3u);
    EXPECT_EQ(old.boot_hash, 0u) << "v1 session: unverifiable, never verified";
    EXPECT_STREQ(old.model, "") << "no fabricated identity";
}

/* BENCH-F1 (torn-checkpoint salvage): a writer killed mid-emit leaves the
 * header claiming N records with the Nth record header present but
 * unfinalized (chain=0, crc=0 - the finalize patch never ran). The reader
 * must expose the indexed N-1 prefix, never let read_record(N-1) hit the
 * "index >= indexed size" path and surface ERR_SESSION_FORMAT. Reproduces
 * the CI hard-kill cancel probe artifact byte-for-byte in structure. */
TEST(Crsf, TornCheckpointSalvagesPrefix) {
    MemFile mf;
    {
        Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(0x52434B50ULL), OK);
        auto p = compressible_payload();
        ASSERT_EQ(w.add_record(REC_SESSION_STATE, p.data(), u32(p.size())), OK);
        ASSERT_EQ(w.add_record(REC_VOLUME_INFO, "vi", 2), OK);
        ASSERT_EQ(w.add_record(REC_ERROR_MAP, "em", 2), OK);
        ASSERT_EQ(w.finalize(), OK); /* header says 3, all complete */
    }
    /* simulate the torn 4th record: header bytes with chain/crc zeroed */
    {
        std::vector<u8> torn(CRSF_RECORD_HEADER_BYTES, 0);
        std::memcpy(torn.data(), "CRSR", 4);
        const u32 type = REC_SESSION_STATE;
        std::memcpy(torn.data() + 4, &type, 4);
        const u64 sid = 0x52434B50ULL;
        std::memcpy(torn.data() + 8, &sid, 8);
        const u64 at = mf.data.size();
        mf.data.resize(size_t(at + torn.size()));
        mf.pos = at;
        ASSERT_EQ(MemFile::wr(&mf, torn.data(), u32(torn.size())), 0);
    }
    /* also patch the header to claim 4 (the killed writer's in-progress count):
     * record_count at container offset 0x10 */
    const u32 four = 4;
    std::memcpy(mf.data.data() + 0x10, &four, 4);

    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    u32 count = 0;
    ASSERT_EQ(r.open(&count), OK);
    EXPECT_EQ(count, 3u) << "header claims 4, indexed prefix is 3 - clamp wins";
    Record rec;
    ASSERT_EQ(r.read_record(0, rec), OK);
    ASSERT_EQ(r.read_record(1, rec), OK);
    ASSERT_EQ(r.read_record(2, rec), OK);
    /* the absent 4th record is out of range of the salvageable prefix - but it
     * is never reached through open()'s clamped count; direct probing still
     * reports format-error honestly */
    EXPECT_EQ(r.read_record(3, rec), 0x50040001u);
}

/* ===================================================================== */
/* M-047: root stats round-trip — the persisted recursive accumulator is  */
/* stored on load (the og trusts the image, rsf-format :318-320), and   */
/* refresh_stats accumulates RECURSIVELY (the og's +0x58 accumulator).  */
/* ===================================================================== */

#include "cr/tree/cr_node.h"

namespace {

void build_nested_tree(tree::ResultTree& t, tree::FullNode*& root_out) {
    tree::FullNode* r = t.add_root(u"Found Files");
    root_out = r;
    /* direct file under the root */
    {
        auto* p = new tree::FileProperties();
        p->name = u"direct.txt";
        p->logical_size = 100;
        p->attributes = 0x20;
        auto* n = t.add_file(r, p);
        n->set_origin(tree::Origin::Structured);
        p->release();
    }
    /* nested: dir/ dir2/ three files two levels deep */
    {
        auto* d = new tree::FileProperties();
        d->name = u"docs";
        d->attributes = 0x10; /* ATTR_DIRECTORY */
        auto* dn = t.add_file(r, d);
        dn->set_origin(tree::Origin::Structured);
        d->release();
        {
            auto* d2 = new tree::FileProperties();
            d2->name = u"inner";
            d2->attributes = 0x10;
            auto* d2n = t.add_file(dn, d2);
            d2n->set_origin(tree::Origin::Structured);
            d2->release();
            for (int i = 0; i < 3; ++i) {
                auto* p = new tree::FileProperties();
                char nm[32];
                snprintf(nm, sizeof(nm), "file%d.bin", i);
                std::u16string u;
                for (const char* q = nm; *q; ++q)
                    u.push_back(u16(u8(*q)));
                p->name = u;
                p->logical_size = 10 + u64(i);
                p->attributes = 0x20;
                auto* n = t.add_file(d2n, p);
                n->set_origin(tree::Origin::Structured);
                if (i == 1)
                    n->set_deleted_state(tree::DeletedState::Deleted);
                p->release();
            }
        }
    }
}

} // namespace

TEST(SessionTree, RefreshStatsIsRecursive) {
    /* M-047: the accumulator counts the WHOLE subtree — nested files
     * contribute to the root totals (the og's +0x58 recursive walk), not
     * just direct children. Pre-fix: root showed 1 file/100 bytes. */
    tree::ResultTree t;
    tree::FullNode* r = nullptr;
    build_nested_tree(t, r);
    t.refresh_stats();
    EXPECT_EQ(r->stats().file_count, 4u); /* direct.txt + 3 nested */
    EXPECT_EQ(r->stats().dir_count, 2u);
    EXPECT_EQ(r->stats().total_logical, 100u + 10u + 11u + 12u);
    EXPECT_EQ(r->stats().deleted_count, 1u); /* file1.bin nested 2 levels deep */
}

TEST(SessionTree, ReloadedRootStatsAreRecursive) {
    /* M-047's discriminator: save -> load -> root totals equal the
     * pre-save recursive totals (the persisted image is STORED on load —
     * the og's trust model; pre-fix the reader discarded it and the
     * direct-only refresh re-derived 1 file/100 bytes). */
    tree::ResultTree t;
    tree::FullNode* r = nullptr;
    build_nested_tree(t, r);
    t.refresh_stats();
    const u64 files = r->stats().file_count;
    const u64 dirs = r->stats().dir_count;
    const u64 logical = r->stats().total_logical;
    const u64 deleted = r->stats().deleted_count;
    ASSERT_EQ(files, 4u);

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), OK);
    tree::ResultTree loaded;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), loaded), OK);
    ASSERT_FALSE(loaded.roots().empty());
    const tree::FullNode* lr =
        static_cast<const tree::FullNode*>(loaded.roots()[0]);
    /* the persisted image wins on load: same totals by construction */
    EXPECT_EQ(lr->stats().file_count, files);
    EXPECT_EQ(lr->stats().dir_count, dirs);
    EXPECT_EQ(lr->stats().total_logical, logical);
    EXPECT_EQ(lr->stats().deleted_count, deleted);
}

TEST(SessionTree, StatsDeletedCountRecursive) {
    /* deleted files nested two levels count at the root — the direct-only
     * refresh never saw them (pre-fix: deleted_count 0 at the root) */
    tree::ResultTree t;
    tree::FullNode* r = nullptr;
    build_nested_tree(t, r);
    t.refresh_stats();
    EXPECT_EQ(r->stats().deleted_count, 1u);
}
