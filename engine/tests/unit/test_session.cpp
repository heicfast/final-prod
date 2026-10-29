/* CRSF session container tests (R6.3, R6.6, F10). */
#include <gtest/gtest.h>
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/orchestrator/cr_scan.h"
#include <algorithm>
#include <functional>
#include "cr/storage/cr_device.h"
#include <cstring>
#include "cr/cr_text.h"
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

TEST(Crsf, ExtentLogicalRoundTrip) {
    /* S-059: the logical position + hole mark persist through the
     * session wire (nflags bit 2048) - a stopped scan's sparse file
     * reloads with its holes at the right logical offsets, and the
     * writer's positioned walk resumes from them. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume C");
    t.refresh_stats();
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"holed.dat";
    p->logical_size = 3 << 20;
    tree::FileProperties::Extent e1{};
    e1.offset = 0x100000;
    e1.length = 1 << 20;
    e1.logical = 0;
    p->extents.push_back(e1);
    tree::FileProperties::Extent eh{};
    eh.offset = 0;
    eh.length = 1 << 20;
    eh.logical = 1 << 20;
    eh.hole = true;
    p->extents.push_back(eh);
    tree::FileProperties::Extent e2{};
    e2.offset = 0x200000;
    e2.length = 1 << 20;
    e2.logical = 2 << 20;
    p->extents.push_back(e2);
    (void)t.add_file(root, p);
    p->release();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), OK);
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), OK);
    tree::FileNode* holed = nullptr;
    for (tree::FileNode* r : t2.roots())
        for (tree::FileNode* c = r->first_child(); c; c = c->next_sibling())
            if (c->name_utf8() == "holed.dat")
                holed = c;
    ASSERT_NE(holed, nullptr);
    ASSERT_EQ(u64(3), holed->props()->extents.size());
    EXPECT_EQ(0x100000u, holed->props()->extents[0].offset);
    EXPECT_EQ(0u, holed->props()->extents[0].logical);
    EXPECT_FALSE(holed->props()->extents[0].hole);
    EXPECT_TRUE(holed->props()->extents[1].hole) << "the hole mark survives the wire";
    EXPECT_EQ(u64(1) << 20, holed->props()->extents[1].logical)
        << "the hole's logical position survives the wire";
    EXPECT_EQ(u64(2) << 20, holed->props()->extents[2].logical)
        << "the tail extent lands past the hole";
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
TEST(Crsf, ReconstructedCarveTreeRoundTrip) {
    /* S-048: the carve tree's intermediate folders round-trip - the
     * "Reconstructed" root, the 7 empty category skeleton folders, the
     * lazy per-volume group child, and the empty per-extension folders
     * all survive serialize/deserialize with their tree shape intact
     * (the marker-2 child-folder record; roots stay marker-1 so old
     * readers load old files unchanged). */
    tree::ResultTree t;
    auto* rec = t.add_root(u"Reconstructed");
    static const char* const kCats[] = {"Pictures", "Documents", "Archives", "Videos",
                                        "Audio",    "Emails",    "Others"};
    for (const char* cat : kCats)
        (void)t.add_folder(rec, text::utf8_to_u16(cat));
    auto* grp = t.add_folder(rec, u" (H:)");
    ASSERT_NE(grp, nullptr);
    auto* jpgf = t.add_folder(grp, u"jpg");
    ASSERT_NE(jpgf, nullptr);
    tree::FileProperties* p1 = new tree::FileProperties();
    p1->name = u"JPG_192x144_000001.JPG";
    p1->logical_size = 519;
    p1->carve_format = 0;
    p1->carve_confidence = 900;
    p1->extents.push_back({4096, 519});
    auto* f1 = t.add_file(grp, p1);
    p1->release();
    f1->set_origin(tree::Origin::Carved);
    f1->set_deleted_state(tree::DeletedState::Deleted);

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), OK);
    EXPECT_GT(blob.size(), 100u);

    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), OK);
    EXPECT_EQ(t2.node_count(), t.node_count());
    tree::FullNode* rec2 = nullptr;
    for (tree::FileNode* r : t2.roots())
        if (r->is_full() && static_cast<tree::FullNode*>(r)->role == u"Reconstructed")
            rec2 = static_cast<tree::FullNode*>(r);
    ASSERT_NE(rec2, nullptr) << "the Reconstructed root reloads as a root";
    /* the 7 skeleton folders + the group child + the ext folder + the file */
    int folders = 0, files = 0;
    tree::FullNode* grp2 = nullptr;
    tree::FullNode* jpgf2 = nullptr;
    for (tree::FileNode* c = rec2->first_child(); c; c = c->next_sibling()) {
        if (c->is_full()) {
            ++folders;
            tree::FullNode* f = static_cast<tree::FullNode*>(c);
            if (f->role == u" (H:)")
                grp2 = f;
        } else {
            ++files;
            ADD_FAILURE() << "no file directly under Reconstructed in this fixture";
        }
    }
    EXPECT_EQ(folders, 8) << "7 skeleton folders + the volume group child";
    ASSERT_NE(grp2, nullptr) << "the group child reloads as a CHILD (marker 2)";
    int g_files = 0, g_folders = 0;
    for (tree::FileNode* c = grp2->first_child(); c; c = c->next_sibling()) {
        if (c->is_full()) {
            ++g_folders;
            if (static_cast<tree::FullNode*>(c)->role == u"jpg")
                jpgf2 = static_cast<tree::FullNode*>(c);
        } else {
            ++g_files;
            EXPECT_EQ(c->name_utf8(), "JPG_192x144_000001.JPG");
            EXPECT_EQ(c->origin(), tree::Origin::Carved);
        }
    }
    EXPECT_EQ(g_files, 1) << "the carved file stays under its volume group";
    EXPECT_EQ(g_folders, 1) << "the empty per-ext folder survives";
    ASSERT_NE(jpgf2, nullptr);
    EXPECT_EQ(jpgf2->first_child(), nullptr) << "the ext folder stays EMPTY";
}

TEST(Crsf, VolumeInfoIdentityCardRoundTrip) {
    VolumeInfo vi;
    vi.fs_kind = 3;
    vi.slice_offset = 512u * 63;
    vi.slice_bytes = 4096;
    vi.flags = 0x7;
    std::snprintf(vi.label, sizeof(vi.label), "TESTVOL");
    std::snprintf(vi.label_full, sizeof(vi.label_full),
                  "012345678901234567890123456789012345678901234567890123456789"); /* N-090: 60 */
    std::snprintf(vi.model, sizeof(vi.model), "ST5000DM000-1FK178");
    std::snprintf(vi.serial, sizeof(vi.serial), "W1234567");
    vi.boot_hash = 0xDEADBEEFu;
    std::snprintf(vi.image, sizeof(vi.image), "/vm/disks/qemu.img"); /* S-030 v3 tail */
    std::vector<u8> blob;
    ASSERT_EQ(serialize_volume_info(vi, blob), OK);
    EXPECT_EQ(blob.size(), 80u + 84u + 256u + 608u + 8u + 100u)
        << "v6 layout: v5 + the W1a save_unix tail (the record-2 scanTime) + "
           "the N-090 complete-label tail (the og's volumeName[100])";

    VolumeInfo out;
    ASSERT_EQ(deserialize_volume_info(blob.data(), blob.size(), out), OK);
    EXPECT_STREQ(out.model, "ST5000DM000-1FK178");
    EXPECT_STREQ(out.serial, "W1234567");
    EXPECT_EQ(out.boot_hash, 0xDEADBEEFu);
    EXPECT_EQ(out.slice_offset, 512u * 63);
    EXPECT_STREQ(out.image, "/vm/disks/qemu.img") << "the image path round-trips";
    EXPECT_STREQ(out.label_full, "012345678901234567890123456789012345678901234567890123456789")
        << "N-090: the complete label round-trips (the v6 tail)";

    /* v2 record (pre-S-030): no image tail - empty path, not garbage */
    std::vector<u8> v2(blob.begin(), blob.begin() + 80 + 84);
    VolumeInfo old2;
    ASSERT_EQ(deserialize_volume_info(v2.data(), v2.size(), old2), OK);
    EXPECT_STREQ(old2.model, "ST5000DM000-1FK178");
    EXPECT_STREQ(old2.image, "") << "v2 session: no image identity";

    /* v1 record: the identity tail is absent - zeros, not garbage */
    std::vector<u8> v1(blob.begin(), blob.begin() + 80);
    VolumeInfo old;
    ASSERT_EQ(deserialize_volume_info(v1.data(), v1.size(), old), OK);
    EXPECT_EQ(old.fs_kind, 3u);
    EXPECT_EQ(old.boot_hash, 0u) << "v1 session: unverifiable, never verified";
    EXPECT_STREQ(old.model, "") << "no fabricated identity";
    EXPECT_STREQ(old.image, "") << "v1 session: no image identity";
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
    const tree::FullNode* lr = static_cast<const tree::FullNode*>(loaded.roots()[0]);
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

/* ------------------------------------------------------------------ */
/* M-050 + N-058 (landed together per M-050's fix plan): the node-model */
/* round-trip gaps closed + the loader hardening.                       */
/* ------------------------------------------------------------------ */

namespace {

/* wire-accurate node builder for hand-crafted loader fixtures */
struct WireNode {
    u32 parent_id = 0xFFFFFFFFu;
    u16 nflags = 0;
    u8 marker = 0;
    u8 origin = 0, deleted = 0, confidence = 0;
    u8 grade_byte = 0;
    std::u16string name;
    std::u16string short_name;
    u64 record_ref = 0;
    u32 fs_kind = 0;
    u64 logical = 0, allocated = 0;
    std::string resident;
    u32 carve_format = 0;
    u16 carve_confidence = 0;
    std::vector<u8> out;

    WireNode& emit(std::vector<u8>& blob) {
        out.clear();
        auto p32 = [&blob](u32 v) {
            for (int i = 0; i < 4; ++i)
                blob.push_back(u8(v >> (8 * i)));
        };
        auto p16 = [&blob](u16 v) {
            blob.push_back(u8(v));
            blob.push_back(u8(v >> 8));
        };
        auto p64 = [&blob](u64 v) {
            for (int i = 0; i < 8; ++i)
                blob.push_back(u8(v >> (8 * i)));
        };
        (void)p32;
        (void)p64;
        p32(parent_id);
        p16(nflags);
        blob.push_back(marker);
        blob.push_back(origin);
        blob.push_back(deleted);
        blob.push_back(confidence);
        if (nflags & 8)
            blob.push_back(grade_byte);
        p64(record_ref);
        p32(fs_kind);
        p64(logical);
        p64(allocated);
        for (int i = 0; i < 4; ++i)
            p64(0); /* times */
        p32(0);     /* attributes */
        p16(0);     /* flags */
        p16(u16(name.size()));
        for (u16 ch : name) {
            blob.push_back(u8(ch));
            blob.push_back(u8(ch >> 8));
        }
        if (nflags & 4) {
            p16(u16(short_name.size()));
            for (u16 ch : short_name) {
                blob.push_back(u8(ch));
                blob.push_back(u8(ch >> 8));
            }
        }
        p32(0); /* extent count */
        if (nflags & 2) {
            p32(u32(resident.size()));
            blob.insert(blob.end(), resident.begin(), resident.end());
        }
        p32(carve_format);
        p16(carve_confidence);
        blob.push_back(0); /* hash_valid */
        /* no provenance (nflags&16) in the hand-crafted fixtures */
        return *this;
    }
};

std::vector<u8> blob_of_nodes(std::vector<WireNode> nodes) {
    std::vector<u8> out;
    auto p32 = [&out](u32 v) {
        for (int i = 0; i < 4; ++i)
            out.push_back(u8(v >> (8 * i)));
    };
    p32(u32(nodes.size()));
    for (WireNode& w : nodes)
        w.emit(out);
    return out;
}

} // namespace

TEST(Session, GradeRoleAndProvenanceRoundTrip) {
    /* M-050's carrier audit, pinned end to end: grade + overlap (the
     * S-023 landing's byte), root roles (strings - richer than the og's
     * root-role u32 pair), short names (N-011a), and the M-051 carve
     * provenance all survive save/load. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume C");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"carved.png";
    p->short_name = u"CARVED~1.PNG";
    p->logical_size = 2048;
    p->carve_format = 7;
    p->carve_confidence = 96;
    p->carve_checksum = 0xDEADBEEFu;
    p->carve_anchor_b = 1234;
    p->carve_flag = true;
    p->extents.push_back({4096, 2048});
    auto* f = t.add_file(root, p);
    p->release();
    f->set_origin(tree::Origin::Carved);
    f->set_confidence(tree::Confidence::Excellent);
    f->set_grade_raw(tree::Grade::High);
    f->set_overlap();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    ASSERT_EQ(t2.roots().size(), 1u);
    tree::FileNode* r2 = t2.roots()[0];
    EXPECT_EQ(static_cast<tree::FullNode*>(r2)->role, u"Volume C")
        << "root roles round-trip (strings - richer than the og's u32 pair)";
    tree::FileNode* c2 = r2->first_child();
    ASSERT_NE(c2, nullptr);
    EXPECT_EQ(c2->grade(), tree::Grade::High) << "grade round-trips (S-023 carrier)";
    EXPECT_TRUE(c2->overlap()) << "overlap round-trips";
    EXPECT_EQ(c2->origin(), tree::Origin::Carved);
    ASSERT_NE(c2->props(), nullptr);
    EXPECT_EQ(c2->props()->short_name, u"CARVED~1.PNG");
    EXPECT_EQ(c2->props()->carve_checksum, 0xDEADBEEFu);
    EXPECT_EQ(c2->props()->carve_anchor_b, 1234u);
    EXPECT_TRUE(c2->props()->carve_flag);
}

TEST(Session, OldFormatStillLoads) {
    /* M-050's forward-compat guard: a legacy v1-wire node (nflags=0 -
     * no short name, no grade byte, no resident blob, no provenance)
     * loads exactly as before, under the synthetic Restored root. */
    WireNode w;
    w.parent_id = 0xFFFFFFFFu;
    w.marker = 0;
    w.name = u"legacy.txt";
    w.logical = 42;
    const std::vector<u8> blob = blob_of_nodes({w});
    tree::ResultTree t2;
    u32 reparented = 99;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2, &reparented), u32(OK));
    EXPECT_EQ(reparented, 0u);
    ASSERT_EQ(t2.roots().size(), 1u);
    EXPECT_EQ(static_cast<tree::FullNode*>(t2.roots()[0])->role, u"Restored");
    tree::FileNode* f = t2.roots()[0]->first_child();
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->name_utf8(), "legacy.txt");
    EXPECT_EQ(f->props()->logical_size, 42u);
    EXPECT_EQ(f->grade(), tree::Grade::Average) << "no grade byte: the default";
    EXPECT_TRUE(f->props()->short_name.empty()) << "no short name in v1";
}

TEST(Session, ForwardParentDeferredAndCounted) {
    /* N-058 (a): a FORWARD parent reference resolves after the loop -
     * node0 (parent=1) attaches under node1, NOT silently under the
     * synthetic root; a never-resolving reference stays + is counted. */
    {
        WireNode n0;
        n0.parent_id = 1; /* forward: node 1 has not loaded yet */
        n0.name = u"child.txt";
        WireNode n1;
        n1.parent_id = 0xFFFFFFFFu; /* legacy top-level under Restored */
        n1.name = u"parent.txt";
        const std::vector<u8> blob = blob_of_nodes({n0, n1});
        tree::ResultTree t2;
        u32 reparented = 99;
        ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2, &reparented), u32(OK));
        EXPECT_EQ(reparented, 0u) << "the forward reference resolved";
        /* node1 under Restored; node0 under node1 */
        tree::FileNode* top = nullptr;
        for (tree::FileNode* r : t2.roots())
            for (tree::FileNode* c = r->first_child(); c; c = c->next_sibling())
                if (c->name_utf8() == "parent.txt")
                    top = c;
        ASSERT_NE(top, nullptr);
        /* after resolution node0 moved under node1 (a grandchild of
         * Restored - the search must walk recursively) */
        tree::FileNode* moved = nullptr;
        std::function<void(tree::FileNode*)> hunt = [&](tree::FileNode* n) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                if (c->name_utf8() == "child.txt")
                    moved = c;
                hunt(c);
            }
        };
        for (tree::FileNode* r : t2.roots())
            hunt(r);
        ASSERT_NE(moved, nullptr) << "node0 vanished";
        EXPECT_EQ(moved->parent(), top) << "the deferred forward ref landed on its parent";
    }
    {
        /* the hostile variant: parent 999 (out of range) never resolves -
         * the node stays under Restored and the count surfaces it */
        WireNode n0;
        n0.parent_id = 999;
        n0.name = u"orphan.txt";
        WireNode n1;
        n1.parent_id = 0xFFFFFFFFu;
        n1.name = u"other.txt";
        const std::vector<u8> blob = blob_of_nodes({n0, n1});
        tree::ResultTree t2;
        u32 reparented = 0;
        ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2, &reparented), u32(OK));
        EXPECT_EQ(reparented, 1u) << "the unresolved reference is counted, never silent";
    }
}

TEST(Session, BadMarkerFailsTheRecord) {
    /* N-058 (b): marker outside {0,1} is corruption, not a light node */
    WireNode w;
    w.marker = 7;
    w.name = u"bad.txt";
    const std::vector<u8> blob = blob_of_nodes({w});
    tree::ResultTree t2;
    EXPECT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(0x50040001));
}

TEST(Session, TruncatedTailFailsAfterFullCount) {
    /* N-058 (c): a stream that fails is an error even when the node was
     * created before the failure surfaced (the old guard returned OK) */
    WireNode w;
    w.name = u"tail.txt";
    w.logical = 100;
    std::vector<u8> blob = blob_of_nodes({w});
    ASSERT_GT(blob.size(), 20u);
    blob.resize(blob.size() - 12); /* cut into the fixed tail fields */
    tree::ResultTree t2;
    EXPECT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(0x50040001));
}

TEST(Session, HostileNameAndRoleLengthBombs) {
    /* N-058 (d): a hostile u16 name/role length is bounds-checked BEFORE
     * the allocation (the old code resized up to 128 KiB per node from a
     * truncated record); the og bounds-checks per-node (rsf-record4). */
    {
        std::vector<u8> blob;
        blob_of_nodes({WireNode()}); /* sized by a real node first */
        blob.clear();
        auto p32 = [](std::vector<u8>& b, u32 v) {
            for (int i = 0; i < 4; ++i)
                b.push_back(u8(v >> (8 * i)));
        };
        p32(blob, 1); /* one node */
        p32(blob, 0xFFFFFFFFu);
        blob.push_back(0);
        blob.push_back(0); /* nflags */
        blob.push_back(0); /* marker */
        blob.push_back(0);
        blob.push_back(0);
        blob.push_back(0); /* origin/deleted/conf */
        for (int i = 0; i < 8; ++i)
            blob.push_back(0); /* record_ref */
        for (int i = 0; i < 4; ++i)
            blob.push_back(0); /* fs_kind */
        for (int i = 0; i < 8 * 2; ++i)
            blob.push_back(0); /* logical+allocated */
        for (int i = 0; i < 8 * 4; ++i)
            blob.push_back(0); /* times */
        for (int i = 0; i < 4; ++i)
            blob.push_back(0); /* attributes */
        blob.push_back(0);
        blob.push_back(0); /* flags */
        blob.push_back(0xE0);
        blob.push_back(0xEA); /* name_len = 0xEAE0 = 60128 */
        blob.push_back(0x41);
        blob.push_back(0); /* 2 bytes of name, then EOF */
        tree::ResultTree t2;
        EXPECT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(0x50040001));
    }
    {
        /* the full-node role-length bomb */
        std::vector<u8> blob;
        auto p32 = [](std::vector<u8>& b, u32 v) {
            for (int i = 0; i < 4; ++i)
                b.push_back(u8(v >> (8 * i)));
        };
        p32(blob, 1);
        p32(blob, 0xFFFFFFFFu);
        blob.push_back(0);
        blob.push_back(0);
        blob.push_back(1); /* marker = full node */
        blob.push_back(0);
        blob.push_back(0);
        blob.push_back(0);
        for (int i = 0; i < 8; ++i)
            blob.push_back(0);
        for (int i = 0; i < 4; ++i)
            blob.push_back(0);
        for (int i = 0; i < 8 * 2; ++i)
            blob.push_back(0);
        for (int i = 0; i < 8 * 4; ++i)
            blob.push_back(0);
        for (int i = 0; i < 4; ++i)
            blob.push_back(0);
        blob.push_back(0);
        blob.push_back(0);
        blob.push_back(0);
        blob.push_back(0); /* empty name */
        /* nflags&4==0: no short name; ec=0 */
        p32(blob, 0);
        p32(blob, 0); /* carve_format */
        blob.push_back(0);
        blob.push_back(0); /* carve_confidence */
        blob.push_back(0); /* hash_valid */
        blob.push_back(0xF0);
        blob.push_back(0xEA); /* role_len = 0xEAF0 = 60144 */
        blob.push_back(0x41);
        blob.push_back(0); /* 2 bytes, then EOF */
        tree::ResultTree t2;
        EXPECT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(0x50040001));
    }
}

/* ---- baseline repair (DFX17): corrupt-record early exits release props --- */

TEST(Session, CorruptRecordEarlyExitsReleaseProps) {
    /* Five loader break sites exit AFTER the per-record FileProperties
     * allocation but BEFORE the node attach: name-length bomb, truncated
     * resident data, truncated sha128, truncated carve_note, role-length
     * bomb. Each leaked the whole props object (LeakSanitizer: 792 B in
     * 3 allocations on the suite's corruption tests). The ownership guard
     * releases the creation ref on every early exit; this test drives
     * ALL FIVE paths so the ASan/LSan gate is the fail-before proof
     * (suite exits LeakSanitizer-RED pre-fix, clean post-fix). */
    auto node_prefix = [](std::vector<u8>& b, u16 nflags, u8 marker) {
        auto p32 = [&b](u32 v) {
            for (int i = 0; i < 4; ++i)
                b.push_back(u8(v >> (8 * i)));
        };
        auto p16 = [&b](u16 v) {
            b.push_back(u8(v));
            b.push_back(u8(v >> 8));
        };
        p32(1);           /* node count */
        p32(0xFFFFFFFFu); /* parent_id */
        p16(nflags);
        b.push_back(marker);
        b.push_back(0); /* origin */
        b.push_back(0); /* deleted */
        b.push_back(0); /* confidence */
        for (int i = 0; i < 8; ++i)
            b.push_back(0); /* record_ref */
        for (int i = 0; i < 4; ++i)
            b.push_back(0); /* fs_kind */
        for (int i = 0; i < 8 * 2; ++i)
            b.push_back(0); /* logical+allocated */
        for (int i = 0; i < 8 * 4; ++i)
            b.push_back(0); /* times */
        for (int i = 0; i < 4; ++i)
            b.push_back(0); /* attributes */
        b.push_back(0);
        b.push_back(0); /* flags */
    };

    { /* (1) name-length bomb: claims 512 bytes, 2 provided */
        std::vector<u8> b;
        node_prefix(b, 0, 0);
        b.push_back(0x00);
        b.push_back(0x02); /* name_len = 512 */
        b.push_back(0x41);
        b.push_back(0);
        tree::ResultTree t;
        EXPECT_EQ(deserialize_tree(b.data(), b.size(), t), u32(0x50040001));
    }
    { /* (2) truncated resident data: rlen 64, 8 provided (nflags&2) */
        std::vector<u8> b;
        node_prefix(b, 2, 0);
        b.push_back(0);
        b.push_back(0); /* name_len = 0 */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* extent count */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0x40); /* rlen = 0x40404040? no: LE 64 */
        for (int i = 0; i < 8; ++i)
            b.push_back(0xAA); /* 8 of 64 bytes */
        tree::ResultTree t;
        EXPECT_EQ(deserialize_tree(b.data(), b.size(), t), u32(0x50040001));
    }
    { /* (3) truncated sha128: hash_valid=1, 4 of 16 provided */
        std::vector<u8> b;
        node_prefix(b, 0, 0);
        b.push_back(0);
        b.push_back(0); /* name_len = 0 */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* extent count */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* carve_format */
        b.push_back(0);
        b.push_back(0); /* carve_confidence */
        b.push_back(1); /* hash_valid */
        for (int i = 0; i < 4; ++i)
            b.push_back(0x5A); /* 4 of 16 sha bytes */
        tree::ResultTree t;
        EXPECT_EQ(deserialize_tree(b.data(), b.size(), t), u32(0x50040001));
    }
    { /* (4) truncated carve_note: nl claims 8 bytes, 2 provided (nflags&16) */
        std::vector<u8> b;
        node_prefix(b, 16, 0);
        b.push_back(0);
        b.push_back(0); /* name_len = 0 */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* extent count */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* carve_format */
        b.push_back(0);
        b.push_back(0); /* carve_confidence */
        b.push_back(0); /* hash_valid */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* carve_checksum */
        b.push_back(8);
        b.push_back(0); /* note length = 8 */
        b.push_back(0x41);
        b.push_back(0); /* 2 of 8 bytes */
        tree::ResultTree t;
        EXPECT_EQ(deserialize_tree(b.data(), b.size(), t), u32(0x50040001));
    }
    { /* (5) role-length bomb: full node claims 128-byte role, 2 provided */
        std::vector<u8> b;
        node_prefix(b, 0, 1); /* marker = full node */
        b.push_back(0);
        b.push_back(0); /* name_len = 0 */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* extent count */
        for (u32 i = 0; i < 4; ++i)
            b.push_back(0); /* carve_format */
        b.push_back(0);
        b.push_back(0); /* carve_confidence */
        b.push_back(0); /* hash_valid */
        b.push_back(0x80);
        b.push_back(0x00); /* role_len = 128 */
        b.push_back(0x52);
        b.push_back(0); /* 2 bytes, then EOF */
        tree::ResultTree t;
        EXPECT_EQ(deserialize_tree(b.data(), b.size(), t), u32(0x50040001));
    }
}

/* ---- N-016b (N-029 residual): raw name bytes persist in sessions ------ */

TEST(Session, RawNameRoundTrip) {
    /* The og's .rsf keeps the ORIGINAL on-disk name bytes (substitution
     * happens at display/restore); the core's .crsf must persist them
     * alongside the lossless display name (nflags bit 32) so a reloaded
     * session carries exactly what the og's session would. Fail-before:
     * the raw field was dropped at serialize (stayed empty post-load). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"r\uE0E9sum\uE0E9.txt"; /* the lossless display form */
    p->name_raw = "r\xE9sum\xE9.txt";  /* the original Latin-1 bytes */
    p->logical_size = 10;
    auto* f = t.add_file(root, p);
    p->release();
    ASSERT_NE(f, nullptr);

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    ASSERT_EQ(t2.roots().size(), 1u);
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = t2.roots()[0]->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->name == std::u16string(u"r\uE0E9sum\uE0E9.txt"))
            found = c;
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->props()->name_raw, std::string("r\xE9sum\xE9.txt"))
        << "the original on-disk bytes must survive the round-trip";

    /* the serialized form actually carries the payload: the ORIGINAL
     * byte sequence must appear verbatim in the wire blob (node 0 is the
     * root FullNode, so per-node flag offsets are not fixed-position) */
    const std::string needle = "r\xE9sum\xE9.txt";
    const char* hb = reinterpret_cast<const char*>(blob.data());
    EXPECT_TRUE(std::search(hb, hb + blob.size(), needle.begin(), needle.end()) != hb + blob.size())
        << "the original on-disk bytes must be embedded in the .crsf wire";
}

TEST(Session, OldFormatWithoutRawNameStillLoads) {
    /* forward-compat guard: a session written BEFORE the name_raw field
     * (bit 32 clear) loads with name_raw empty - the writer then falls
     * back to the decoded name, exactly as before. */
    WireNode w;
    w.name = u"plain.txt";
    const std::vector<u8> blob = blob_of_nodes({w});
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = t2.roots()[0]->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->name == std::u16string(u"plain.txt"))
            found = c;
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(found->props()->name_raw.empty());
}

/* ---- N-060: save-path progress + cancel (the og's per-child window) --- */

namespace {
struct SaveProgressRecorder {
    std::vector<std::pair<u64, u64>> ticks;
    bool cancel_at_done_ge = false;
    u64 threshold = 0;
    static bool thunk(u64 done, u64 total, void* user) {
        auto* r = static_cast<SaveProgressRecorder*>(user);
        r->ticks.push_back({done, total});
        return !(r->cancel_at_done_ge && done >= r->threshold);
    }
};
} // namespace

TEST(Session, SaveEmitsProgress) {
    /* N-060: the tree serializer reports per-node progress (done/total in
     * NODES, the pre-order walk knowing the total up front) with the
     * terminal (total,total) tick - the og's save window
     * (rsf-format--scan-session.md flow section: 40% + 50*processed/
     * (2*remaining), checked after every child) mapped onto our save
     * path. Pre-fix: serialize_tree had no hook at all. */
    tree::ResultTree t;
    tree::FullNode* r = nullptr;
    build_nested_tree(t, r);
    t.refresh_stats();
    /* the pre-order walk emits the root + every node below it */
    u64 expected = 0;
    std::function<u64(const tree::FileNode*)> count = [&](const tree::FileNode* n) -> u64 {
        u64 sum = 1;
        for (const tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
            sum += count(c);
        return sum;
    };
    for (const tree::FileNode* root : t.roots())
        expected += count(root);
    SaveProgressRecorder rec;
    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob, &SaveProgressRecorder::thunk, &rec), u32(OK));
    ASSERT_FALSE(rec.ticks.empty());
    /* every tick carries the same total; the first is (0, total) */
    const u64 total = rec.ticks.front().second;
    EXPECT_EQ(rec.ticks.front().first, 0u);
    for (const auto& tk : rec.ticks)
        EXPECT_EQ(tk.second, total);
    EXPECT_GT(total, 0u);
    /* monotonic done, terminal (total,total) */
    for (size_t i = 1; i < rec.ticks.size(); ++i)
        EXPECT_GE(rec.ticks[i].first, rec.ticks[i - 1].first);
    EXPECT_EQ(rec.ticks.back().first, total);
    /* the total equals the tree's serialized node count (roundtrip the
     * blob and count - the header carries it) */
    ASSERT_GE(blob.size(), 4u);
    const u32 node_count =
        u32(blob[0]) | (u32(blob[1]) << 8) | (u32(blob[2]) << 16) | (u32(blob[3]) << 24);
    EXPECT_EQ(total, u64(node_count));
    EXPECT_EQ(expected, u64(node_count)); /* the full pre-order walk */
}

TEST(Session, SaveCancelsCleanly) {
    /* N-060: a false return from the hook aborts serialization with
     * ERR_CANCELLED (the og's per-child IsCancelled teardown; the caller
     * discards the partial output - the writer never sees it). */
    tree::ResultTree t;
    tree::FullNode* r = nullptr;
    build_nested_tree(t, r);
    t.refresh_stats();
    SaveProgressRecorder rec;
    rec.cancel_at_done_ge = true;
    rec.threshold = 1; /* cancel on the first per-node tick */
    std::vector<u8> blob;
    EXPECT_EQ(serialize_tree(t, blob, &SaveProgressRecorder::thunk, &rec), u32(ERR_CANCELLED));
    EXPECT_FALSE(rec.ticks.empty());
    /* even the (0,total) entry tick is cancellable */
    SaveProgressRecorder rec0;
    rec0.cancel_at_done_ge = true;
    rec0.threshold = 0;
    std::vector<u8> blob0;
    EXPECT_EQ(serialize_tree(t, blob0, &SaveProgressRecorder::thunk, &rec0), u32(ERR_CANCELLED));
    EXPECT_EQ(blob0.size(), 0u) << "a cancelled entry tick emits nothing";
}

TEST(Session, AutoNamingConvention) {
    /* N-061: the og's session auto-naming convention, byte-exact vs the
     * corpus sample - the shipped "auto_Local Disk (I_)10_14_23_13_21"
     * .rsf (qa-logs--session-forensics.md: "auto_" + label with the
     * Windows-forbidden ':' sanitized to '_' + LOCAL-time M_D_H_m_s). */
    std::tm tm{};
    tm.tm_mon = 9; /* October (0-based) */
    tm.tm_mday = 14;
    tm.tm_hour = 23;
    tm.tm_min = 13;
    tm.tm_sec = 21;
    EXPECT_EQ(auto_session_name_at(u"Local Disk (I:)", tm), "auto_Local Disk (I_)10_14_23_13_21");
    /* the full forbidden set sanitizes to '_' (never a name-breaking
     * character survives; spaces and parentheses pass through) */
    EXPECT_EQ(auto_session_name_at(u"a<b>c:d\"e/f\\g|h?i*j", tm),
              "auto_a_b_c_d_e_f_g_h_i_j10_14_23_13_21");
    /* an empty label keeps the stamp alone (the og's unlabeled fallback) */
    EXPECT_EQ(auto_session_name_at(u"", tm), "auto_10_14_23_13_21");
    /* the live variant reads the local clock: prefix + the stamp pattern */
    const std::string live = auto_session_name(u"Vol");
    EXPECT_EQ(live.substr(0, 8), "auto_Vol");
    const std::string tail = live.substr(8);
    ASSERT_EQ(tail.size(), 14u); /* MM_DD_HH_mm_ss: 10 digits + 4 '_' */
    for (size_t i = 0; i < tail.size(); ++i) {
        const bool ok = (i % 3 == 2) ? tail[i] == '_' : (tail[i] >= '0' && tail[i] <= '9');
        EXPECT_TRUE(ok) << "bad stamp character at " << i << ": " << tail;
    }
}

TEST(Session, BtrfsExtrasRoundTrip) {
    /* N-059 (diff-to-fix): the og's Btrfs record secondary payload —
     * secondary id A (+0x3B; our carrier = the owning subvol root id),
     * secondary id B (+0x3F; no decoded source, stays 0), and the
     * generation/transid qword (+0x81; our carrier = the inode item's
     * generation) — survives save/load through the nflags bit-6 block. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume B");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"snap.dat";
    p->fs_kind = u32(FsKindCode::Btrfs);
    p->logical_size = 4096;
    p->btrfs_id_a = 257; /* subvol root id (the N-047 naming datum) */
    p->btrfs_id_b = 0;   /* undecoded — never invented */
    p->btrfs_generation = 0x1122334455667788ull;
    p->extents.push_back({8192, 4096});
    auto* f = t.add_file(root, p);
    (void)f;
    p->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* c2 = t2.roots()[0]->first_child();
    ASSERT_NE(c2, nullptr);
    ASSERT_NE(c2->props(), nullptr);
    EXPECT_EQ(c2->props()->btrfs_id_a, 257u);
    EXPECT_EQ(c2->props()->btrfs_id_b, 0u);
    EXPECT_EQ(c2->props()->btrfs_generation, 0x1122334455667788ull);
    /* the ZFS/HFS+ slots ride the same block and stay zero here */
    EXPECT_EQ(c2->props()->zfs_secondary_flags, 0u);
}

TEST(Session, ZfsSecondaryFlagsRoundTrip) {
    /* N-059: the og's ZFS record secondary-flags u16 (+0x5A; our carrier
     * = the znode ZPL_FLAGS low word, census-level semantics carried
     * opaque) and — the session-6 addendum's fold — the HFS+ record's
     * u16 flags (+0xE) + Finder type id (+0x74) ride the same block. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume Z");
    tree::FileProperties* pz = new tree::FileProperties();
    pz->name = u"tank.creds";
    pz->fs_kind = u32(FsKindCode::Zfs);
    pz->zfs_secondary_flags = 0x0008; /* the immutable-family bit word */
    auto* fz = t.add_file(root, pz);
    (void)fz;
    pz->release();
    tree::FileProperties* ph = new tree::FileProperties();
    ph->name = u"readme";
    ph->fs_kind = u32(FsKindCode::Hfsplus);
    ph->hfs_secondary_flags = 0x0100; /* the catalog record's flags word */
    ph->hfs_type_id = 0x74657874;     /* 'text' — the Finder file type */
    auto* fh = t.add_file(root, ph);
    (void)fh;
    ph->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* z2 = t2.roots()[0]->first_child();
    ASSERT_NE(z2, nullptr);
    ASSERT_NE(z2->props(), nullptr);
    EXPECT_EQ(z2->props()->zfs_secondary_flags, 0x0008u);
    tree::FileNode* h2 = z2->next_sibling();
    ASSERT_NE(h2, nullptr);
    ASSERT_NE(h2->props(), nullptr);
    EXPECT_EQ(h2->props()->hfs_secondary_flags, 0x0100u);
    EXPECT_EQ(h2->props()->hfs_type_id, 0x74657874u);
    /* the btrfs slots stay zero for non-btrfs nodes */
    EXPECT_EQ(h2->props()->btrfs_generation, 0ull);
}

TEST(Session, EngineExtrasAbsentWhenUnpopulated) {
    /* N-059: the nflags bit-6 block is written ONLY when a carrier is
     * nonzero — a plain node's record stays the pre-N-059 shape (old
     * readers and old files both directions unaffected). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume F");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"plain.txt";
    p->logical_size = 10;
    auto* f = t.add_file(root, p);
    (void)f;
    p->release();
    t.refresh_stats();
    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    /* the fixed 24-byte block is absent: byte-compare against a second
     * serialization of the SAME tree after zeroing a carrier proves the
     * gate (identical bytes = no block in either) */
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* c2 = t2.roots()[0]->first_child();
    ASSERT_NE(c2, nullptr);
    EXPECT_EQ(c2->props()->btrfs_id_a, 0u);
    EXPECT_EQ(c2->props()->zfs_secondary_flags, 0u);
    EXPECT_EQ(c2->props()->hfs_type_id, 0u);
    std::vector<u8> blob2;
    ASSERT_EQ(serialize_tree(t2, blob2), u32(OK));
    EXPECT_EQ(blob, blob2) << "no extras block: the record shape is unchanged";
}

TEST(Session, FatOriginClusterRoundTrip) {
    /* OQ-SA-1 (resolved, scan-engine lane): the FAT-family first-cluster
     * origin anchor — the og's CFAT +0x3e / CExFat +0x50 slot (the
     * SetFirstCluster vtable+0x28 contract; part1 :125-127 census) —
     * rides its own nflags bit-7 block so a chainless deleted record
     * keeps its dentry origin across save/load (RED pre-fix: the carrier
     * did not exist; the datum was dropped). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume F");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"_GONE.TXT";
    p->fs_kind = u32(FsKindCode::Fat16);
    p->fat_origin_cluster = 4089; /* the dentry's origin cluster */
    p->extents.push_back({8192, 512});
    auto* f = t.add_file(root, p);
    (void)f;
    p->release();
    /* a second, plain node proves the bit-7 gate: zero carrier -> the
     * record stays the pre-OQ-SA-1 shape */
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"plain.txt";
    p2->fs_kind = u32(FsKindCode::Fat16);
    auto* f2 = t.add_file(root, p2);
    (void)f2;
    p2->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* g2 = t2.roots()[0]->first_child();
    ASSERT_NE(g2, nullptr);
    ASSERT_NE(g2->props(), nullptr);
    EXPECT_EQ(g2->props()->fat_origin_cluster, 4089u);
    tree::FileNode* q2 = g2->next_sibling();
    ASSERT_NE(q2, nullptr);
    ASSERT_NE(q2->props(), nullptr);
    EXPECT_EQ(q2->props()->fat_origin_cluster, 0u);
}

TEST(Session, NtfsOriginLcnRoundTrip) {
    /* OQ-SA-1 NTFS half (resolved, scan-engine lane s3): the og's
     * CNTFSFileProperties +0x64 anchor — the low u32 of the FIRST $DATA
     * run's start LCN (FUN_1800336f0 :41028 -> FUN_1800069e0 :5126 ->
     * FUN_180003020 :1987; persisted by FUN_180003440 :2362) — rides its
     * own nflags bit-8 block so a fragmented/deleted record keeps its
     * origin LCN across save/load (RED pre-fix: the carrier did not
     * exist; the datum was dropped). The 0xFFFFFFFF sparse-first-run
     * value is nonzero and therefore persists too. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume N");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"fragment.dat";
    p->fs_kind = u32(FsKindCode::Ntfs);
    p->ntfs_origin_lcn = 0x000C1234u; /* the first run's start LCN */
    p->extents.push_back({0x000C1234ull * 4096, 2 * 4096});
    p->extents.push_back({0x000D5678ull * 4096, 4096});
    auto* f = t.add_file(root, p);
    (void)f;
    p->release();
    /* the sparse-first-run shape: the og's stored -1 low dword */
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"holes.dat";
    p2->fs_kind = u32(FsKindCode::Ntfs);
    p2->ntfs_origin_lcn = 0xFFFFFFFFu;
    auto* f2 = t.add_file(root, p2);
    (void)f2;
    p2->release();
    /* a third, plain node proves the bit-8 gate: zero carrier -> the
     * record stays the pre-OQ-SA-1-NTFS shape */
    tree::FileProperties* p3 = new tree::FileProperties();
    p3->name = u"plain.txt";
    p3->fs_kind = u32(FsKindCode::Ntfs);
    auto* f3 = t.add_file(root, p3);
    (void)f3;
    p3->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* g2 = t2.roots()[0]->first_child();
    ASSERT_NE(g2, nullptr);
    ASSERT_NE(g2->props(), nullptr);
    EXPECT_EQ(g2->props()->ntfs_origin_lcn, 0x000C1234u);
    tree::FileNode* h2 = g2->next_sibling();
    ASSERT_NE(h2, nullptr);
    ASSERT_NE(h2->props(), nullptr);
    EXPECT_EQ(h2->props()->ntfs_origin_lcn, 0xFFFFFFFFu);
    tree::FileNode* q2 = h2->next_sibling();
    ASSERT_NE(q2, nullptr);
    ASSERT_NE(q2->props(), nullptr);
    EXPECT_EQ(q2->props()->ntfs_origin_lcn, 0u);
}

TEST(Session, NtfsTotalClustersRoundTrip) {
    /* N-100: the og's +0x60 twin — the $DATA stream's TOTAL cluster
     * count, persisted beside the origin LCN (rsf record-4 writes
     * u32(+0x64) then u32(+0x60), rsf-format :398; the og's copier
     * FUN_180003020 :1987-1988 stamps both). The semantics that make
     * the field NON-DERIVABLE: sparse-anywhere zeroes the total
     * (FUN_1800336f0 :41096-98) while allocated_size stays nonzero — a
     * sparse file's session row carries total 0 beside a nonzero
     * allocation, which no allocated_size/extents derivation can
     * reproduce. RED pre-fix: the carrier did not exist (compile-error
     * on the reverted tree — the field itself is the fix). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume N");
    /* non-sparse: 7 clusters across two runs — the total persists */
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"contiguous.dat";
    p->fs_kind = u32(FsKindCode::Ntfs);
    p->ntfs_origin_lcn = 0x000C1234u;
    p->ntfs_total_clusters = 7;
    p->allocated_size = 7 * 4096;
    p->extents.push_back({0x000C1234ull * 4096, 5 * 4096});
    p->extents.push_back({0x000D5678ull * 4096, 2 * 4096});
    auto* f = t.add_file(root, p);
    (void)f;
    p->release();
    /* the sparse shape: total 0 (sparse-anywhere rule) BESIDE a nonzero
     * allocated_size — the og's session-row truth for sparse files, and
     * exactly the pair no derivation reproduces */
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"sparseholes.dat";
    p2->fs_kind = u32(FsKindCode::Ntfs);
    p2->ntfs_origin_lcn = 0xFFFFFFFFu; /* sparse first run */
    p2->ntfs_total_clusters = 0;       /* sparse-anywhere => 0 */
    p2->allocated_size = 64 * 4096;    /* ...while allocation stays */
    auto* f2 = t.add_file(root, p2);
    (void)f2;
    p2->release();
    /* the gate: a zero-total NTFS node must stay shape-identical to a
     * pre-N-100 record (bit-9 fires only when nonzero) */
    tree::FileProperties* p3 = new tree::FileProperties();
    p3->name = u"plain.txt";
    p3->fs_kind = u32(FsKindCode::Ntfs);
    auto* f3 = t.add_file(root, p3);
    (void)f3;
    p3->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* g2 = t2.roots()[0]->first_child();
    ASSERT_NE(g2, nullptr);
    ASSERT_NE(g2->props(), nullptr);
    EXPECT_EQ(g2->props()->ntfs_total_clusters, 7u);
    EXPECT_EQ(g2->props()->ntfs_origin_lcn, 0x000C1234u); /* both anchors ride */
    tree::FileNode* h2 = g2->next_sibling();
    ASSERT_NE(h2, nullptr);
    ASSERT_NE(h2->props(), nullptr);
    EXPECT_EQ(h2->props()->ntfs_total_clusters, 0u);
    EXPECT_EQ(h2->props()->allocated_size, 64ull * 4096)
        << "the not-derivable pair: sparse total 0 beside nonzero allocation";
    tree::FileNode* q2 = h2->next_sibling();
    ASSERT_NE(q2, nullptr);
    ASSERT_NE(q2->props(), nullptr);
    EXPECT_EQ(q2->props()->ntfs_total_clusters, 0u);
    EXPECT_EQ(q2->props()->ntfs_origin_lcn, 0u);
}

TEST(Session, NtfsParentRefRoundTrip) {
    /* N-131 NTFS half: the og's CNTFSFileProperties +0x40 parent
     * reference — the $FILE_NAME parent-directory FILE reference masked
     * to the 48-bit record number (FUN_18002a340's local_41 mask, the
     * copier FUN_180003020 :1982) — rides its own nflags bit-12 block so
     * a reloaded session answers "which MFT record was this file's
     * parent?" (the structural parent_id is a session-file index, not an
     * FS datum). RED pre-fix: the carrier did not exist (compile-error
     * on the reverted tree — the field itself is the fix). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume N");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"lineage.dat";
    p->fs_kind = u32(FsKindCode::Ntfs);
    p->record_ref = 0x0011000000000021ull;      /* own ref: record 0x21, seq 0x11 */
    p->ntfs_parent_ref = 0x0000000000000010ull; /* parent: record 16 (masked) */
    p->extents.push_back({200 * 4096, 2 * 4096});
    auto* f = t.add_file(root, p);
    (void)f;
    p->release();
    /* a root-level file carries parent 5 (the root dir record) — the
     * mask means the sequence bits never ride (the og strips them) */
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"toplevel.txt";
    p2->fs_kind = u32(FsKindCode::Ntfs);
    p2->ntfs_parent_ref = 5;
    auto* f2 = t.add_file(root, p2);
    (void)f2;
    p2->release();
    /* the gate: a zero-carrier node (orphan/pseudo emission) stays
     * shape-identical to a pre-N-131 record (bit-12 fires only when
     * nonzero) */
    tree::FileProperties* p3 = new tree::FileProperties();
    p3->name = u"orphan.bin";
    p3->fs_kind = u32(FsKindCode::Ntfs);
    auto* f3 = t.add_file(root, p3);
    (void)f3;
    p3->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* g2 = t2.roots()[0]->first_child();
    ASSERT_NE(g2, nullptr);
    ASSERT_NE(g2->props(), nullptr);
    EXPECT_EQ(g2->props()->ntfs_parent_ref, 0x10ull);
    EXPECT_EQ(g2->props()->record_ref, 0x0011000000000021ull); /* own ref intact */
    tree::FileNode* h2 = g2->next_sibling();
    ASSERT_NE(h2, nullptr);
    ASSERT_NE(h2->props(), nullptr);
    EXPECT_EQ(h2->props()->ntfs_parent_ref, 5ull);
    tree::FileNode* q2 = h2->next_sibling();
    ASSERT_NE(q2, nullptr);
    ASSERT_NE(q2->props(), nullptr);
    EXPECT_EQ(q2->props()->ntfs_parent_ref, 0ull);
}

TEST(Session, FatParentClusterRoundTrip) {
    /* N-131 FAT half: the og's CFATFileProperties +0x42 parent-directory
     * anchor — the containing directory node's own +0x3e origin cluster
     * (FATSearchDLL :11654 with local_110 from :11500/:11504) — rides its
     * own nflags bit-13 block so a reloaded session answers "which
     * cluster did this file's containing directory start at?" RED
     * pre-fix: the carrier did not exist (compile-error on the reverted
     * tree — the field itself is the fix). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume F");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"INNER.TXT";
    p->fs_kind = u32(FsKindCode::Fat16);
    p->fat_origin_cluster = 15; /* own first cluster */
    p->fat_parent_cluster = 5;  /* the SUBDIR's first cluster */
    p->extents.push_back({8192, 512});
    auto* f = t.add_file(root, p);
    (void)f;
    p->release();
    /* root-level file: parent 0 by construction (the og's volume-root
     * node anchors at SetFirstCluster(0), engine init FUN_18000e130) —
     * the zero carrier stays the pre-N-131 shape */
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"README.TXT";
    p2->fs_kind = u32(FsKindCode::Fat16);
    p2->fat_origin_cluster = 10;
    p2->fat_parent_cluster = 0;
    auto* f2 = t.add_file(root, p2);
    (void)f2;
    p2->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* g2 = t2.roots()[0]->first_child();
    ASSERT_NE(g2, nullptr);
    ASSERT_NE(g2->props(), nullptr);
    EXPECT_EQ(g2->props()->fat_parent_cluster, 5u);
    EXPECT_EQ(g2->props()->fat_origin_cluster, 15u); /* both anchors ride */
    tree::FileNode* q2 = g2->next_sibling();
    ASSERT_NE(q2, nullptr);
    ASSERT_NE(q2->props(), nullptr);
    EXPECT_EQ(q2->props()->fat_parent_cluster, 0u);
    EXPECT_EQ(q2->props()->fat_origin_cluster, 10u);
}

TEST(Session, IdentityCardFirmwareWwnInterface) {
    /* N-066 (diff-to-fix): the og record-3's identity axes — firmware
     * revision (+0x206), WWN/serial-2 (+0x30A), interface description
     * (+0x414) — round-trip through the v4 tail; the CRC32 boot_hash
     * stays the fast verify path. */
    session::VolumeInfo vi;
    vi.fs_kind = 3;
    std::memcpy(vi.model, "WDC WD40EFRX", 12);
    std::memcpy(vi.serial, "WD-WCC4E1234567", 15);
    std::memcpy(vi.firmware, "82.00A82", 8);
    std::memcpy(vi.wwn, "50014ee2b8a1c2d3", 16);
    std::memcpy(vi.interface_desc, "SATA", 4);
    vi.boot_hash = 0xABCD1234u;
    std::vector<u8> blob;
    ASSERT_EQ(serialize_volume_info(vi, blob), u32(OK));
    session::VolumeInfo vo;
    ASSERT_EQ(deserialize_volume_info(blob.data(), blob.size(), vo), u32(OK));
    EXPECT_EQ(0, std::strcmp(vo.firmware, "82.00A82"));
    EXPECT_EQ(0, std::strcmp(vo.wwn, "50014ee2b8a1c2d3"));
    EXPECT_EQ(0, std::strcmp(vo.interface_desc, "SATA"));
    EXPECT_EQ(vo.boot_hash, 0xABCD1234u);
    EXPECT_EQ(0, std::strcmp(vo.model, "WDC WD40EFRX"));
}

TEST(Session, BootSnapshotRoundTrip) {
    /* N-066: the og record-3's +0x518 datum — the full 512-byte
     * first-sector image (MBR/BOOT incl. the partition table) round-trips
     * byte-exact; v1/v2/v3-length records leave it zeroed. */
    session::VolumeInfo vi;
    for (size_t i = 0; i < 512; ++i)
        vi.boot_snapshot[i] = u8(i * 7 + 3);
    vi.boot_snapshot[510] = 0x55;
    vi.boot_snapshot[511] = 0xAA;
    std::vector<u8> blob;
    ASSERT_EQ(serialize_volume_info(vi, blob), u32(OK));
    session::VolumeInfo vo;
    ASSERT_EQ(deserialize_volume_info(blob.data(), blob.size(), vo), u32(OK));
    EXPECT_EQ(0, std::memcmp(vo.boot_snapshot, vi.boot_snapshot, 512));

    /* the v3-length prefix (everything before the v4 tail) loads with a
     * zeroed snapshot — the pre-N-066 file contract */
    const size_t v3_len = blob.size() - (32 + 32 + 32 + 512);
    session::VolumeInfo v3;
    ASSERT_EQ(deserialize_volume_info(blob.data(), v3_len, v3), u32(OK));
    const session::VolumeInfo zero;
    EXPECT_EQ(0, std::memcmp(v3.boot_snapshot, zero.boot_snapshot, 512));
    EXPECT_EQ(0, v3.firmware[0]);
    EXPECT_EQ(0, v3.wwn[0]);
    EXPECT_EQ(0, v3.interface_desc[0]);
}

TEST(Session, SaveUnixVTailRoundTrip) {
    /* W1a (R-56): the v5 tail - the embedded save time (the og's
     * record-2 SYSTEMTIME scanTime, rsf-format section 5.2 +0x6C)
     * round-trips; v4-era and shorter records read 0 = "not recorded"
     * (the honest pre-v5 answer, never a fabricated time). */
    session::VolumeInfo vi;
    vi.save_unix = 1730123456ULL;
    std::vector<u8> blob;
    ASSERT_EQ(serialize_volume_info(vi, blob), u32(OK));
    ASSERT_EQ(blob.size(), size_t(80 + 84 + 256 + 32 + 32 + 32 + 512 + 8 + 100));
    session::VolumeInfo vo;
    ASSERT_EQ(deserialize_volume_info(blob.data(), blob.size(), vo), u32(OK));
    EXPECT_EQ(vo.save_unix, 1730123456ULL);
    EXPECT_EQ(0, vo.label_full[0]) << "N-090: no label set - the v6 tail stays zeros";

    /* v5-era record (the v6 tail cut): loads, label_full = the honest
     * zeros (the legacy 31-char card is the whole truth - N-090's
     * degrade contract) */
    const size_t v5_len = blob.size() - 100;
    session::VolumeInfo v5;
    ASSERT_EQ(deserialize_volume_info(blob.data(), v5_len, v5), u32(OK));
    EXPECT_EQ(v5.save_unix, 1730123456ULL);
    EXPECT_EQ(0, v5.label_full[0]);

    /* v4-era record (the v5 tail cut by one field): loads, save_unix = 0 */
    const size_t v4_len = blob.size() - 100 - 8;
    session::VolumeInfo v4;
    ASSERT_EQ(deserialize_volume_info(blob.data(), v4_len, v4), u32(OK));
    EXPECT_EQ(v4.save_unix, 0ULL);
    EXPECT_EQ(0, v4.interface_desc[0]);
    EXPECT_EQ(0, v4.label_full[0]);

    /* v1-era record (the 80-byte prefix): same honest zero */
    session::VolumeInfo v1;
    ASSERT_EQ(deserialize_volume_info(blob.data(), 80, v1), u32(OK));
    EXPECT_EQ(v1.save_unix, 0ULL);
}

TEST(Crsf, PeekRecordHeaderOnly) {
    /* W1a (R-56): peek_record returns type + session id from the 24-byte
     * record header WITHOUT touching the payload chain - the skip-by-type
     * primitive the session-list path is built on. */
    MemFile mf;
    {
        Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(0x1122334455667788ULL), OK);
        auto p1 = compressible_payload();
        ASSERT_EQ(w.add_record(REC_SESSION_STATE, p1.data(), u32(p1.size())), OK);
        auto p2 = compressible_payload();
        p2.push_back(9); /* make the payloads differ */
        ASSERT_EQ(w.add_record(REC_RESULT_TREE, p2.data(), u32(p2.size())), OK);
        auto p3 = compressible_payload();
        p3.push_back(7);
        ASSERT_EQ(w.add_record(REC_VOLUME_INFO, p3.data(), u32(p3.size())), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    u32 count = 0;
    ASSERT_EQ(r.open(&count), OK);
    ASSERT_EQ(count, 3u);

    RecordType t = REC_SESSION_STATE;
    u64 sid = 0;
    ASSERT_EQ(r.peek_record(0, t, sid), OK);
    EXPECT_EQ(t, REC_SESSION_STATE);
    EXPECT_EQ(sid, 0x1122334455667788ULL);
    ASSERT_EQ(r.peek_record(1, t, sid), OK);
    EXPECT_EQ(t, REC_RESULT_TREE);
    ASSERT_EQ(r.peek_record(2, t, sid), OK);
    EXPECT_EQ(t, REC_VOLUME_INFO);
    EXPECT_EQ(sid, 0x1122334455667788ULL);

    /* out-of-range index refuses (same code as read_record) */
    EXPECT_EQ(r.peek_record(3, t, sid), 0x50040001u);

    /* peek does not consume: full reads still decode every record */
    Record rec;
    ASSERT_EQ(r.read_record(0, rec), OK);
    EXPECT_EQ(rec.type, REC_SESSION_STATE);
    EXPECT_EQ(rec.data.size(), compressible_payload().size());
    ASSERT_EQ(r.read_record(2, rec), OK);
    EXPECT_EQ(rec.type, REC_VOLUME_INFO);
}

TEST(SessionSummary, ReadSummaryLightRecordsOnly) {
    /* W1a (R-56): read_summary decodes ONLY the light records - the
     * heavy RESULT_TREE payload is skipped by type; every summary field
     * matches what the full load would report for the same light data. */
    MemFile mf;
    session::SessionState st;
    session::VolumeInfo vi;
    {
        st.stage_done_mask = 1; /* quick */
        st.files_found = 3;
        st.deleted_found = 1;
        st.carved_found = 2;
        st.bytes_scanned = 999;
        st.error_map_ranges = 0;
        vi.fs_kind = 3;
        vi.flags = 1; /* quick configured */
        vi.scanned_bytes = 999;
        vi.elapsed_ms = 5;
        vi.save_unix = 1730123456ULL;
        std::memcpy(vi.label, "TESTVOL", 7);

        Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(0xA1B2C3D4E5F60718ULL), OK);
        std::vector<u8> blob;
        ASSERT_EQ(serialize_session_state(st, blob), OK);
        ASSERT_EQ(w.add_record(REC_SESSION_STATE, blob.data(), u32(blob.size())), OK);
        ASSERT_EQ(serialize_volume_info(vi, blob), OK);
        ASSERT_EQ(w.add_record(REC_VOLUME_INFO, blob.data(), u32(blob.size())), OK);
        /* the heavy record the summary path must never pay for: a
         * multi-chunk payload far larger than the whole summary */
        std::vector<u8> big;
        const std::vector<u8> chunk = compressible_payload();
        for (int i = 0; i < 400; ++i)
            big.insert(big.end(), chunk.begin(), chunk.end());
        ASSERT_EQ(w.add_record(REC_RESULT_TREE, big.data(), u32(big.size())), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    session::SessionSummary s;
    ASSERT_EQ(orchestrator::ScanController::read_summary(r, s), u32(OK));
    EXPECT_EQ(s.session_id, 0xA1B2C3D4E5F60718ULL);
    EXPECT_EQ(s.record_count, 3u);
    EXPECT_TRUE(s.have_state);
    EXPECT_TRUE(s.have_volume);
    EXPECT_EQ(s.state.stage_done_mask, st.stage_done_mask);
    EXPECT_EQ(s.state.files_found, st.files_found);
    EXPECT_EQ(s.state.deleted_found, st.deleted_found);
    EXPECT_EQ(s.state.carved_found, st.carved_found);
    EXPECT_EQ(s.state.bytes_scanned, st.bytes_scanned);
    EXPECT_EQ(s.volume.fs_kind, vi.fs_kind);
    EXPECT_EQ(s.volume.save_unix, vi.save_unix);
    EXPECT_EQ(s.volume.elapsed_ms, vi.elapsed_ms);
    EXPECT_EQ(0, std::strcmp(s.volume.label, "TESTVOL"));
    /* configured_mask derives from volume.flags (the load rule) */
    EXPECT_EQ(s.configured_mask, 1u);
}

TEST(SessionSummary, LegacySessionWithoutVolumeInfo) {
    /* W1a (R-56): a legacy container with no VolumeInfo still summaries:
     * have_volume=false, configured_mask = all stages (the
     * load_session_summary legacy rule), honest empty label. */
    MemFile mf;
    {
        session::SessionState st;
        st.stage_done_mask = 1;
        st.files_found = 2;
        Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(0x1ULL), OK);
        std::vector<u8> blob;
        ASSERT_EQ(serialize_session_state(st, blob), OK);
        ASSERT_EQ(w.add_record(REC_SESSION_STATE, blob.data(), u32(blob.size())), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    session::SessionSummary s;
    ASSERT_EQ(orchestrator::ScanController::read_summary(r, s), u32(OK));
    EXPECT_TRUE(s.have_state);
    EXPECT_FALSE(s.have_volume);
    EXPECT_EQ(s.state.files_found, 2u);
    EXPECT_EQ(s.configured_mask, 1u | 2u | 4u);
    EXPECT_EQ(s.volume.label[0], 0);
    EXPECT_EQ(s.volume.save_unix, 0ULL);
}

namespace {
/* W1b (R-54): the per-node load hook's observer state. */
struct LoadHookState {
    u64 calls = 0;
    u64 last_done = 0;
    u64 total_seen = 0;
    u64 cancel_at_done = 0; /* cancel once done >= cancel_at_done (a
                             * threshold, not a flag - MSVC C4804/C4305
                             * under /W4 /WX, run 40690974024) */
};
bool load_progress_counter(u64 done, u64 total, void* user) {
    auto* st = static_cast<LoadHookState*>(user);
    ++st->calls;
    EXPECT_GE(done, st->last_done); /* monotonic */
    st->last_done = done;
    st->total_seen = total;
    return st->cancel_at_done == 0 || done < st->cancel_at_done;
}
} // namespace

TEST(Session, DeserializeProgressPerNodeAndCancel) {
    /* W1b (R-54): the og's restore reports progress inside the load
     * window and polls IsCancelled after EVERY child
     * (rsf-format--scan-session.md flow section) - our per-node
     * DeserializeProgress mirror. Progress fires per attached node with
     * done/total NODE counts; false aborts ERR_CANCELLED and the caller
     * discards the partial tree. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume C");
    for (int i = 0; i < 12; ++i) {
        tree::FileProperties* p = new tree::FileProperties();
        p->name = std::u16string(u"file") + char16_t('a' + i);
        p->logical_size = 100 + i;
        (void)t.add_file(root, p);
        p->release();
    }
    t.refresh_stats();
    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), OK);

    /* silent load: no hook, no change (the pre-W1b behavior) */
    {
        tree::ResultTree t2;
        u32 reparented = 1;
        ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2, &reparented), OK);
        EXPECT_EQ(t2.node_count(), t.node_count());
    }

    /* progress fires per node, monotonic, with the persisted count */
    {
        tree::ResultTree t2;
        LoadHookState st;
        ASSERT_EQ(
            deserialize_tree(blob.data(), blob.size(), t2, nullptr, load_progress_counter, &st),
            OK);
        const u64 nodes = t.node_count(); /* the ROOT is a node too (13) */
        EXPECT_EQ(st.calls, nodes);
        EXPECT_EQ(st.last_done, nodes);
        EXPECT_EQ(st.total_seen, nodes);
    }

    /* cancel mid-tree: ERR_CANCELLED, the caller discards the partial */
    {
        tree::ResultTree t2;
        LoadHookState st;
        st.cancel_at_done = 5; /* cancel from the 5th attached node on */
        EXPECT_EQ(
            deserialize_tree(blob.data(), blob.size(), t2, nullptr, load_progress_counter, &st),
            ERR_CANCELLED);
        /* the tree object holds a PARTIAL state - by contract the caller
         * discards it (never re-queries it as truth); we only assert the
         * hook stopped early, never that the tree is complete */
        EXPECT_LT(st.calls, t.node_count());
    }
}

TEST(Session, LoadSessionSummaryPropagatesProgressAndCancel) {
    /* W1b (R-54): the hook rides the CONTAINER path too - the REC_RESULT_
     * TREE case threads it (ERR_CANCELLED propagates, never flattened
     * into ERR_SESSION_FORMAT). */
    MemFile mf;
    {
        tree::ResultTree t;
        auto* root = t.add_root(u"Volume D");
        for (int i = 0; i < 20; ++i) {
            tree::FileProperties* p = new tree::FileProperties();
            p->name = std::u16string(u"f") + char16_t('0' + i);
            (void)t.add_file(root, p);
            p->release();
        }
        t.refresh_stats();
        Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(0x99ULL), OK);
        std::vector<u8> blob;
        ASSERT_EQ(serialize_tree(t, blob), OK);
        ASSERT_EQ(w.add_record(REC_RESULT_TREE, blob.data(), u32(blob.size())), OK);
        session::VolumeInfo vi;
        ASSERT_EQ(serialize_volume_info(vi, blob), OK);
        ASSERT_EQ(w.add_record(REC_VOLUME_INFO, blob.data(), u32(blob.size())), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    tree::ResultTree tree;
    orchestrator::ScanStats stats;
    session::VolumeSessionInfo info;
    LoadHookState st;
    ASSERT_EQ(orchestrator::ScanController::load_session_summary(r, tree, stats, info, nullptr,
                                                                 load_progress_counter, &st),
              u32(OK));
    const u64 nodes2 = tree.node_count(); /* 20 files + the root */
    EXPECT_EQ(st.calls, nodes2);
    EXPECT_EQ(st.total_seen, nodes2);

    /* and the cancel path propagates as ERR_CANCELLED through the
     * container layer (the format error must not swallow it) */
    Reader r2(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    tree::ResultTree tree2;
    orchestrator::ScanStats stats2;
    session::VolumeSessionInfo info2;
    LoadHookState st2;
    st2.cancel_at_done = 3;
    EXPECT_EQ(orchestrator::ScanController::load_session_summary(r2, tree2, stats2, info2, nullptr,
                                                                 load_progress_counter, &st2),
              u32(ERR_CANCELLED));
    EXPECT_LT(st2.calls, nodes2);
}

TEST(Crsf, LostDirFoldersRoundTrip) {
    /* M-103: the synthetic LostDir folders persist as ordinary child
     * nodes (the marker-2 record) and reload with the tree shape intact
     * - the og's real sessions carry them as real nodes (398 folders in
     * the H session), so the round-trip must preserve the grouping. */
    tree::ResultTree t;
    auto* lost = t.add_root(u"File Path Lost");
    ASSERT_NE(lost, nullptr);
    auto* d5000 = t.add_folder(lost, u"LostDir5000");
    auto* d7000 = t.add_folder(lost, u"LostDir7000");
    ASSERT_NE(d5000, nullptr);
    ASSERT_NE(d7000, nullptr);
    const char* names[] = {"orphan-a1.txt", "orphan-a2.txt", nullptr};
    for (const char** p = names; *p; ++p) {
        tree::FileProperties* fp = new tree::FileProperties();
        fp->name = text::utf8_to_u16(*p);
        fp->logical_size = 100;
        fp->fs_kind = u32(FsKindCode::Ntfs);
        auto* f = t.add_file(d5000, fp);
        fp->release();
        ASSERT_NE(f, nullptr);
        f->set_origin(tree::Origin::Lost);
        f->set_deleted_state(tree::DeletedState::Deleted);
    }
    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), OK);
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), OK);
    EXPECT_EQ(t2.node_count(), t.node_count());
    tree::FullNode* lost2 = nullptr;
    for (tree::FileNode* r : t2.roots())
        if (r->is_full() && static_cast<tree::FullNode*>(r)->role == u"File Path Lost")
            lost2 = static_cast<tree::FullNode*>(r);
    ASSERT_NE(lost2, nullptr) << "the renamed root reloads";
    int folders = 0, files = 0;
    tree::FullNode* d5000_2 = nullptr;
    for (tree::FileNode* c = lost2->first_child(); c; c = c->next_sibling()) {
        if (c->is_full()) {
            ++folders;
            if (static_cast<tree::FullNode*>(c)->role == u"LostDir5000")
                d5000_2 = static_cast<tree::FullNode*>(c);
        }
    }
    EXPECT_EQ(folders, 2) << "both LostDir folders survive the round-trip";
    ASSERT_NE(d5000_2, nullptr);
    for (tree::FileNode* c = d5000_2->first_child(); c; c = c->next_sibling())
        ++files;
    EXPECT_EQ(files, 2) << "the grouped children stay under their folder";
}

/* ---- N-177: the size-exceeds-blocks damaged flag round-trips ------------ */

TEST(Session, SizeExceedsBlocksFlagRoundTrip) {
    /* the og persists the ext4 truncation-damage bit in the node FLAGS
     * (+0x39 bit 0x200); the core's props->flags ride the same put16
     * record - a flagged node keeps the bit across save/load and a
     * clean node never gains it. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Volume E");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"truncated.dat";
    p->fs_kind = u32(FsKindCode::Ext4);
    p->flags = tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS; /* N-177 */
    p->extents.push_back({4096, 1024});
    t.add_file(root, p);
    p->release();
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"whole.dat";
    p2->fs_kind = u32(FsKindCode::Ext4);
    p2->extents.push_back({8192, 1024});
    t.add_file(root, p2);
    p2->release();
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    tree::FileNode* g2 = t2.roots()[0]->first_child();
    ASSERT_NE(g2, nullptr);
    EXPECT_NE(0, int(g2->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "the damaged bit survives the save/load";
    tree::FileNode* q2 = g2->next_sibling();
    ASSERT_NE(q2, nullptr);
    EXPECT_EQ(0, int(q2->props()->flags & tree::NODE_FLAG_SIZE_EXCEEDS_BLOCKS))
        << "the clean node never gains the bit";
}
