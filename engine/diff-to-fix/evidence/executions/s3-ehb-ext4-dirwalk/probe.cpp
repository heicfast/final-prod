/* s3-ehb-ext4-dirwalk probe — executed comparison for S3-EHB (read-only audit).

 * Settles the CORE side of two S3-EHB decompile claims about EXT4SearchDLL's
 * directory-block walk (FUN_180005820, decompiled :4085-4283):
 *
 *   (1) og advances by the COMPUTED record size (8 + align4(name_len),
 *       :4160-4162/:4173-4174) and therefore lands ON the bytes of a
 *       delete-merged entry (kernel ext4_generic_delete_entry merges the
 *       freed entry into the previous entry's rec_len) and re-emits that
 *       stale entry with its NAME. The core advances by the on-disk rec_len
 *       (cr_ext4_engine.cpp:391 `off += rec_len`) and skips merged space.
 *   (2) og STOPS the whole dir-block walk on the first inode==0 entry
 *       (:4154-4155 `if (uVar2 == 0) goto end`); the core skips inode-0
 *       entries and keeps walking (cr_ext4_engine.cpp:396-409).
 *
 * Fixture: 1024-byte-block ext volume (GDT at block 2 — core parity with the
 * kernel; the og reads the GDT at base+block_size = the SB block on 1K
 * volumes, see the s3-ext4-inventory). Root dir block contains:
 *   "." ".." "alpha.txt"[rec_len MERGED over "beta-gone.txt"]
 *   "beta-gone.txt" (stale bytes of a deleted file, inode 7, dtime set)
 *   "hole" (inode 0) "gamma.txt" (live, after the inode-0 hole)
 *
 * Expected core output (code-determined):
 *   Found:  /alpha.txt /gamma.txt (beta-gone.txt's NAME nowhere)
 *   Deleted: "Inode7" (nameless, dtime-marked sweep node)
 *   Lost: none
 *
 * Build: see build.sh. Run: ./probe
 */
#include "cr/ext4/cr_ext4_engine.h"
#include "cr/tree/cr_node.h"
#include <cstring>
#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <vector>

using namespace cr;

namespace {

constexpr u32 BS = 1024;
constexpr u32 IPG = 16;
constexpr u32 BPG = 8192;
constexpr u32 INO_SZ = 128;

class Img {
public:
    explicit Img(u64 blocks) : blocks_(blocks) { data_.assign(size_t(blocks) * BS, 0); }
    u8* block(u64 b) { return data_.data() + b * BS; }
    std::vector<u8>& raw() { return data_; }

private:
    u64 blocks_;
    std::vector<u8> data_;
};

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> d_;
};

void put16(u8* p, u16 v) { p[0] = u8(v); p[1] = u8(v >> 8); }
void put32(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}

constexpr u32 INO_ROOT = 2;
constexpr u32 INO_ALPHA = 3;
constexpr u32 INO_GONE = 7;   /* the delete-merged stale entry points here */
constexpr u32 INO_GAMMA = 4;

/* blocks: 0 boot, 1 SB, 2 GDT, 3 inode table, 4 bitmap, 5 root dir,
 * 6 alpha data, 7 gamma data */
constexpr u64 B_INODE_TABLE = 3;
constexpr u64 B_BITMAP = 4;
constexpr u64 B_ROOTDIR = 5;

void write_sb(Img& img, u32 inodes_count) {
    u8* sb = img.block(1);
    put32(sb + 0x00, inodes_count);
    put32(sb + 0x04, u32(img.raw().size() / BS));
    put32(sb + 0x14, 1); /* first_data_block = 1 (1KiB blocks) */
    put32(sb + 0x18, 0); /* log_block_size = 0 */
    put32(sb + 0x20, BPG);
    put32(sb + 0x28, IPG);
    put16(sb + 0x38, 0xEF53);
    put32(sb + 0x4C, 1);       /* rev 1 */
    put16(sb + 0x58, INO_SZ);  /* inode_size */
    put32(sb + 0x60, 0x0002);  /* incompat: FILETYPE */
    put32(sb + 0xE0, 8);       /* journal inum */
}

void write_inode(Img& img, u32 ino, u16 mode, u64 size, u64 extent_block, u32 extent_blocks,
                 u16 links, u32 dtime) {
    u8* p = img.block(B_INODE_TABLE) + size_t((ino - 1) * INO_SZ);
    memset(p, 0, INO_SZ);
    put16(p + 0x00, mode);
    put32(p + 0x04, u32(size));
    put32(p + 0x08, 1000);
    put32(p + 0x0C, 2000);
    put32(p + 0x10, 3000);
    put32(p + 0x14, dtime);
    put16(p + 0x1A, links);
    put32(p + 0x20, 0x80000); /* EXTENTS_FL */
    u8* ib = p + 0x28;
    put16(ib + 0, 0xF30A);
    put16(ib + 2, u16(extent_blocks ? 1 : 0));
    put16(ib + 4, 4);
    put16(ib + 6, 0);
    if (extent_blocks) {
        put32(ib + 12, 0);
        put16(ib + 16, u16(extent_blocks));
        put16(ib + 18, 0);
        put32(ib + 20, u32(extent_block));
    }
}

/* raw dir-entry writer at an explicit cursor (no auto rec_len math) */
u32 add_raw(u8* dir, u32 cursor, u32 ino, const char* name, u8 type, u16 rec_len_override) {
    const u32 nlen = u32(strlen(name));
    const u32 natural = (8 + nlen + 3) & ~3u;
    const u32 rec = rec_len_override ? rec_len_override : natural;
    put32(dir + cursor, ino);
    put16(dir + cursor + 4, u16(rec));
    dir[cursor + 6] = u8(nlen);
    dir[cursor + 7] = type;
    memcpy(dir + cursor + 8, name, nlen);
    return cursor + natural; /* returns the position of the STALE entry */
}

} // namespace

int main() {
    Img img(BPG);
    write_sb(img, 8);
    /* GDT at block first_data_block + 1 = 2 */
    u8* g = img.block(2);
    put32(g + 0x00, u32(B_BITMAP)); /* block bitmap */
    put32(g + 0x04, 0);
    put32(g + 0x08, u32(B_INODE_TABLE));

    u8* bm = img.block(B_BITMAP);
    memset(bm, 0xFF, BS);
    for (u64 b = 15; b < BPG; ++b)
        bm[b / 8] &= u8(~(1u << (b % 8)));

    write_inode(img, INO_ROOT, 0x4000, BS, B_ROOTDIR, 1, 2, 0);
    write_inode(img, INO_ALPHA, 0x8000, 16, 6, 1, 1, 0);
    write_inode(img, INO_GAMMA, 0x8000, 16, 7, 1, 1, 0);
    /* the deleted file the merged entry still names: dtime set, links 0 */
    write_inode(img, INO_GONE, 0x8000, 512, 8, 1, 0, 4242);
    memset(img.block(8), 0x77, 512);

    u8* root = img.block(B_ROOTDIR);
    memset(root, 0, BS);
    u32 cur = 0;
    add_raw(root, cur, INO_ROOT, ".", 2, 12);
    cur += 12;
    add_raw(root, cur, INO_ROOT, "..", 2, 12);
    cur += 12;

    /* alpha.txt with rec_len MERGED over the deleted beta-gone.txt entry:
     * natural alpha size = 8+9 -> 20; merged rec_len = 20 + beta natural
     * (8+13=21 -> 24) = 44 — exactly what ext4_generic_delete_entry leaves */
    u32 stale = add_raw(root, cur, INO_ALPHA, "alpha.txt", 1, 44);
    cur += 44;
    if (stale != cur - 24) {
        printf("FIXTURE-BUG: stale entry not inside merged space\n");
        return 2;
    }
    /* the stale (deleted) entry bytes still in the block after the merge */
    add_raw(root, stale, INO_GONE, "beta-gone.txt", 1, 0);

    /* an inode-0 hole followed by a LIVE entry: og stops here, core must not */
    cur = add_raw(root, cur, 0, "hole", 1, 0);
    add_raw(root, cur, INO_GAMMA, "gamma.txt", 1, 0);

    MemDevice dev(img.raw());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");

    ext4::Ext4ScanFlags fl;
    ext4::Ext4ScanStats st;
    u32 rc = ext4::scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    printf("scan rc=%u (%s) groups=%llu files=%llu dirs=%llu deleted_inodes=%llu "
           "journal_salvaged=%llu\n",
           rc, err_text(rc), (unsigned long long)st.block_groups,
           (unsigned long long)st.files, (unsigned long long)st.dirs,
           (unsigned long long)st.deleted_inodes, (unsigned long long)st.journal_salvaged);

    std::function<void(tree::FileNode*, const char*)> dump = [&](tree::FileNode* n,
                                                                 const char* tag) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            printf("%s: \"%s\" deleted_state=%d logical_size=%llu\n", tag,
                   c->name_utf8().c_str(), int(c->deleted_state()),
                   (unsigned long long)c->props()->logical_size);
            dump(c, tag);
        }
    };
    dump(ra, "Found");
    dump(rd, "Deleted");
    dump(rl, "Lost");

    /* verdicts */
    int fails = 0;
    std::set<std::string> found;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props()) {
                found.insert(c->name_utf8());
                collect(c);
            }
        }
    };
    collect(ra);
    if (!found.count("alpha.txt")) { printf("FAIL: alpha.txt missing\n"); ++fails; }
    if (!found.count("gamma.txt")) { printf("FAIL: gamma.txt missing (inode-0 hole stopped the walk)\n"); ++fails; }
    if (found.count("beta-gone.txt")) { printf("FAIL: merged deleted name surfaced in Found\n"); ++fails; }
    printf(fails ? "RESULT: FAIL(%d)\n" : "RESULT: PASS - core advances by rec_len (skips merged + inode-0 space); the merged entry's NAME never surfaces\n", fails);
    return fails ? 1 : 0;
}
