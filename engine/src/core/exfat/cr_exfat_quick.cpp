/* cr_exfat_quick.cpp - exFAT scan engine (R5.2).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/exfat/cr_exfat_engine.h"
#include "cr/fat/cr_fat_wire.h"     /* shared FAT-family datetime core (W4 dedup) */
#include "cr/fat/cr_visited_runs.h" /* M-127: the run-coalescing cycle guard */
#include "cr/cr_recycle.h"          /* M-052: shared $I/$R pairing model */
#include <cstring>
#include <map>
#include <set>

namespace cr {
namespace exfat {

namespace {

constexpr u32 EXFAT_EOC = 0xFFFFFFFF;
constexpr u32 EXFAT_EOC_MIN = 0xFFFFFFF8; /* spec EOC family (EXFAT-3) */
constexpr u32 EXFAT_BAD = 0xFFFFFFF7;
constexpr u32 EXFAT_FREE = 0;

struct ExfatFat {
    std::vector<u8> data; /* whole-FAT fast path (<= 32 MiB volumes) */
    u32 byte_len = 0;
    /* S-013: windowed mode (og FUN_180021c60 :28039-28213 - 1 MiB cached
     * windows, `win + (cluster & 0x3ffff) * 4`, NO total-size cap). The
     * whole-FAT load only stays on small volumes; every big FAT runs
     * windowed so have_fat is never false and chains never truncate to
     * one cluster. Window state is mutable because entry()/next() are
     * const like the whole-load path - the cache refill is transparent.
     * Single-thread contract: the exFAT quick pass walks one volume on
     * one thread (the shared scan worker model). */
    IStorageDevice* win_dev = nullptr;
    u64 win_base = 0;               /* FAT byte offset on the device */
    u64 total_bytes = 0;            /* whole FAT size in bytes */
    mutable std::vector<u8> win;    /* 1 MiB window cache (og: 0x100000) */
    mutable u64 win_first_byte = 0; /* FAT-relative start of the loaded window */
    mutable u32 refills = 0;        /* drained into stats at scan end */

    bool windowed() const { return win_dev != nullptr; }
    bool load_window(u64 widx) const {
        if (win.empty())
            win.resize(1u << 20);
        u64 len = 1u << 20;
        if (widx + len > total_bytes)
            len = total_bytes - widx;
        if (win_dev->read_at(win_base + widx, win.data(), u32(len)) != OK)
            return false;
        win_first_byte = widx;
        ++refills;
        return true;
    }
    u32 entry(u32 c) const {
        if (windowed()) {
            const u64 off = u64(c) * 4;
            if (off + 4 > total_bytes)
                return EXFAT_FREE;
            const u64 widx = off & ~u64((1u << 20) - 1); /* 1 MiB-aligned window */
            if (win.empty() || widx != win_first_byte) {
                if (!load_window(widx))
                    return EXFAT_FREE;
            }
            return u32_from_bytes_le(win.data() + size_t(off - win_first_byte));
        }
        if (u64(c) * 4 + 4 > byte_len)
            return EXFAT_FREE;
        return u32_from_bytes_le(data.data() + c * 4);
    }
    u32 next(u32 c) const {
        u32 v = entry(c);
        if (v == EXFAT_FREE || v == EXFAT_BAD)
            return 0;
        /* EXFAT-3 (AUDIT-FAT): the spec EOC family is 0xFFFFFFF8..0xFFFFFFFF
         * (and og treats the range as chain end); only exact 0xFFFFFFFF
         * previously terminated, so 0xFFFFFFF8..E were followed as cluster
         * numbers -> bounds failure -> broken chains on spec-legal volumes. */
        if (v >= EXFAT_EOC_MIN)
            return EXFAT_EOC;
        return v;
    }
};

struct ChainResult {
    std::vector<u32> clusters;
    bool cycle = false;
    bool broken = false;
    bool assumed_contiguous = false; /* EXFAT-2: FREE-entry continuation */
};

ChainResult follow_exfat_chain(const ExfatFat& fat, u32 start, u32 max_clusters,
                               u32 cluster_count = 0) {
    ChainResult r;
    /* M-127: the run-coalescing guard (the og's range-tree shape - the
     * exFAT twin of follow_chain's fix; the FREE-continuation path can
     * manufacture long contiguous runs, the run-tree's best case) */
    cr::fat::VisitedRuns visited;
    u32 c = start;
    u32 guard = 0;
    while (c >= 2) {
        if (++guard > max_clusters) {
            r.cycle = true;
            break;
        }
        if (visited.visit(c)) {
            r.cycle = true;
            return r;
        }
        r.clusters.push_back(c);
        if (r.clusters.size() >= max_clusters)
            break;
        u32 n = fat.next(c);
        if (n == 0) {
            /* EXFAT-2 (AUDIT-FAT): og FUN_180021c60:28158-28163 - a FREE
             * (zero) FAT entry mid-chain means "no chain info here"; the og
             * continues with cluster+1 under an "assumed" flag (the exFAT
             * salvage heuristic for FAT-zeroed volumes - deletion zeroes
             * the chain). The old hard-break truncated extents and
             * downgraded confidence on damaged-but-recoverable volumes.
             * Bounded by the data-length expectation (max_clusters) and the
             * heap size. */
            if (cluster_count != 0 && c + 1 >= cluster_count + 2) {
                r.broken = true;
                break;
            }
            r.assumed_contiguous = true;
            n = c + 1;
        }
        if (n >= EXFAT_EOC_MIN)
            break;
        c = n;
    }
    return r;
}

/* contiguous chain for NoFatChain streams (corpus: no-FAT behavior) */
ChainResult contiguous_chain(u32 first, u32 count) {
    ChainResult r;
    if (count == 0)
        return r;
    r.clusters.reserve(count);
    for (u32 i = 0; i < count; ++i)
        r.clusters.push_back(first + i);
    return r;
}

u64 cluster_offset(const ExfatParams& p, u32 c) {
    return (u64(c) - 2) * p.cluster_bytes + u64(p.cluster_heap_offset) * p.bytes_per_sector;
}

struct Bitmap {
    std::vector<u8> data;
    u32 cluster_count = 0;
    /* M-054 (NEW-FINDING, fixed): the on-disk allocation bitmap's bit i
     * describes cluster i+2 (Microsoft exFAT spec section 7.1: "the first
     * bit in the Allocation Bitmap describes the allocation status of the
     * cluster with index 2" - the first cluster has index 2 for historical
     * FAT reasons). The old test read bit c for cluster c - a two-cluster
     * shift on every real volume; the core's own fixtures encoded the
     * same shift (setbit(c) for cluster c), so the suite was
     * self-consistently wrong. test() now reads bit c-2 for cluster c;
     * the fixtures write bit c-2 for cluster c. This also fixes the
     * S-009 FreeMap producer (free bits were derived from the shifted
     * test on the landed base). */
    bool test(u32 c) const {
        if (c < 2)
            return false; /* clusters 0/1 are reserved - never heap bits */
        const u64 bit = u64(c) - 2;
        if (bit / 8 >= data.size())
            return false;
        return (data[size_t(bit / 8)] & (1u << (bit % 8))) != 0;
    }
};

/* exFAT time: packed u32 (date<<16 | time, FAT field layout) -> 100ns since 1601.
 *
 * S-011 parity: the og's exFATSearchDLL passes the raw date/time words to
 * DosDateTimeToFileTime and NEVER reads the dentry's tz bytes (verified
 * first-hand: imports census - zero TZ/time-conversion APIs beyond
 * DosDateTimeToFileTime itself; six decompiled call sites at
 * exFATSearchDLL_decompiled.c:5215-5235 and :45174-45194, each taking only
 * (date_word, time_word, &ft); zero quarter-hour (*900) arithmetic anywhere
 * in the engine). Its FILETIME is the on-disk local time carried through
 * the SCANNING HOST's timezone. The parity model - already established by
 * the FAT engine on the same shared datetime core (host-independent,
 * on-disk time as UTC, MATCH verdict) - is applied here too: the tz bytes
 * stay parsed in the wire struct for completeness but are deliberately
 * NOT applied. The spec-correct UTC interpretation (ELM-Chan UtcOffset
 * 7-bit two's-complement quarter-hours) would diverge from the og's .rsf
 * bytes on every non-UTC file; see docs/PARITY-DELTAS.md. */
u64 exfat_time_to_unix100(u32 ts) {
    const u16 date = u16(ts >> 16);
    const u16 time = u16(ts & 0xFFFF);
    if (date == 0)
        return 0;
    /* shared FAT-family datetime core (W4 dedup, AUDIT-FAT §6 row 20) */
    const u64 base_secs = cr::fat::fat_datetime_to_unix_seconds(date, time);
    if (base_secs == 0)
        return 0;
    return base_secs * 10000000ULL + 116444736000000000ULL;
}

struct ScanCtx {
    IStorageDevice* dev;
    CancelToken* cancel = nullptr;               /* P2-8 */
    bool (*progress)(u64, u64, void*) = nullptr; /* live progress (G-04) */
    void* progress_user = nullptr;
    u64 dentries_done = 0; /* cumulative dentries examined (progress unit) */
    ExfatParams p;
    ExfatFat fat;
    Bitmap bitmap;
    bool have_fat = false;
    bool have_bitmap = false;
    bool bitmap_reads_ok = true; /* S-009 hardening: a failed bitmap read
                                  * leaves ZEROS (reads as "free" after the
                                  * inversion) - a torn bitmap must feed
                                  * NOTHING to the free map */
    ExfatScanFlags flags;
    tree::ResultTree* tree;
    tree::FullNode* root_active;
    tree::FullNode* root_deleted;
    tree::FullNode* root_lost;
    ExfatScanStats* stats;
    std::set<u64> visited_dirs;
    /* M-013 (diff-to-fix): the reconstruction park - deleted sets dropped
     * by the dual-form checksum gate (or torn at the buffer end) wait
     * here for the oracle; orphan stream/name runs (a destroyed set's
     * surviving tail) are the substitution candidates. The corpus model:
     * PartitionRecovery's parked-list (FUN_1800203a0's callers iterate
     * it, trying each parked head against each parked tail). */
    struct ParkedSet {
        std::vector<u8> entries; /* the set bytes as found (the prefix) */
        tree::FileNode* parent = nullptr;
        u32 depth = 0;
        bool in_bin = false; /* M-052: the re-walk keeps the bin context */
    };
    struct ParkedTail {
        std::vector<u8> entries; /* the orphan stream/name run */
    };
    std::vector<ParkedSet> parked_sets;
    std::vector<ParkedTail> parked_tails;
    /* M-052: the recycle-bin pairing state (the FAT walk's model -
     * $I parsed during the walk, $R remapped at emission or by the
     * post-walk fixup in scan_exfat) */
    std::map<std::u16string, recycle::Info> recycle_info;
    struct PendingRecycleR {
        tree::FileNode* node; /* arena-stable; props mutated post-walk */
        std::u16string token;
    };
    std::vector<PendingRecycleR> pending_recycle_r;
};

/* read a directory chain's content */
std::vector<u8> read_dir_chain(ScanCtx& ctx, u32 first, u32 max_bytes, bool* ok) {
    *ok = false;
    ChainResult chain;
    if (ctx.have_fat)
        chain = follow_exfat_chain(ctx.fat, first, 0x10000, ctx.p.cluster_count);
    if (chain.clusters.empty() && first >= 2) {
        /* no FAT or FAT lost: try contiguous single cluster */
        chain = contiguous_chain(first, 1);
        chain.broken = true;
    }
    u64 total = u64(chain.clusters.size()) * ctx.p.cluster_bytes;
    if (total > max_bytes)
        total = max_bytes;
    if (total == 0)
        return {};
    std::vector<u8> buf(size_t(total), 0);
    u64 filled = 0;
    size_t i = 0;
    while (i < chain.clusters.size() && filled < total) {
        size_t j = i;
        while (j + 1 < chain.clusters.size() && chain.clusters[j + 1] == chain.clusters[j] + 1)
            ++j;
        u64 off = cluster_offset(ctx.p, chain.clusters[i]);
        u32 run = u32((j - i + 1) * ctx.p.cluster_bytes);
        if (filled + run > total)
            run = u32(total - filled);
        if (ctx.dev->read_at(off, buf.data() + filled, run) != OK)
            std::memset(buf.data() + filled, 0, run);
        filled += run;
        i = j + 1;
    }
    *ok = filled > 0;
    return buf;
}

void walk_entries(ScanCtx& ctx, const std::vector<u8>& buf, bool deleted_context,
                  tree::FileNode* parent_host, u32 depth, bool in_bin);

constexpr u32 EXFAT_MAX_DEPTH = 64; /* P0-6: stack-exhaustion guard */

/* M-014 (diff-to-fix): the og's structural entry gates, decoded from
 * FUN_180002f90 (exFATSearchDLL_decompiled.c :1779-1802), its timestamp
 * helper FUN_180002f40 (:1762-1777), the stream emission gates in
 * FUN_180029280 (:34562-34564) and the name-fragment gates in
 * FUN_180003030 (:1811-1834). The og's per-entry switch applies them to
 * live AND deleted entries side by side (case 0x85/5, 0xC0/0x40,
 * 0xC1/0x41 - :25507-25560), so they run here for live sets, deleted
 * sets and oracle re-walks alike. A failing set-head is SKIPPED - no
 * park, no emission (the og's case handlers fall straight through). */

/* FAT-family date/time field sanity (FUN_180002f40, applied to the
 * create (+8), modify (+0xC) AND access (+0x10) words of the file
 * dentry):
 *   day   (bits 16-20) != 0       - a zero/all-zero date FAILS
 *   month (bits 21-24) in [1,12]  - ((month - 1) < 12)
 *   hour  (bits 11-15) <= 23      - (v & 0xF800) < 0xB801
 *   min   (bits  5-10) <= 59      - (v & 0x7E0)  < 0x761
 *   2sec  (bits  0- 4) <= 29      - second value <= 58
 * The finding's fix-plan guess ("treat all-zero as valid") is corrected
 * by the decompile: day==0 fails, so an unstamped entry is rejected. */
bool fat_datetime_fields_ok(u32 ts) {
    return ((ts & 0x1f0000u) != 0) && (((ts >> 21) & 0xfu) - 1u < 12u) &&
           ((ts & 0xf800u) < 0xb801u) && ((ts & 0x7e0u) < 0x761u) && ((ts & 0x1fu) < 0x1eu);
}

/* File-dentry structural gates (FUN_180002f90, param_1 = file entry +1):
 *   secondary_count (byte +1) in [2,18]   ((u8)(sc - 2) < 0x11)
 *   create/modify/access timestamps all pass fat_datetime_fields_ok
 *   reserved1 (u16 +6) == 0
 *   reserved2 (bytes +0x19..+0x1F) == 0   (the tz bytes +0x16..+0x18
 *                                          are NOT zero-checked - S-011)
 * The secondary-count bound is also the walk's OOB-shape guard: a
 * corrupt sc=40 header used to drag the walker through arbitrary
 * following entries; the og refuses the set at the head. */
bool file_entry_structural_ok(const FileEntry* fe) {
    if (u8(fe->secondary_count - 2) >= 0x11u)
        return false;
    if (!fat_datetime_fields_ok(fe->create_timestamp) ||
        !fat_datetime_fields_ok(fe->modify_timestamp) ||
        !fat_datetime_fields_ok(fe->access_timestamp))
        return false;
    if (fe->reserved1 != 0)
        return false;
    for (u8 b : fe->reserved2)
        if (b != 0)
            return false;
    return true;
}

/* Name-fragment gates (FUN_180003030, param_1 = name entry +1):
 *   flags (byte +1): (flags & 3) == 0 && flags < 4
 *   the fragment's first UTF-16 char (+2) must be nonzero
 *   each char whose LOW byte is zero must pass the 256-entry validity
 *   table indexed by its HIGH byte (PTR_180045a00, :1821-1823)
 *
 * Table provenance (DFX24 completion, correcting the first landing's
 * "not recoverable" note): the corpus DOES ship the raw binary -
 * easeus-drw-research/og-codebase/exFATSearchDLL.dll. The 256 bytes at
 * .rdata RVA 0x45A00 (file offset 0x44600 via the PE section map:
 * .rdata vaddr 0x45000 / rawptr 0x43C00) are the table FUN_180003030
 * indexes; dumped byte-exact and cross-checked against the raw
 * disassembly at VA 0x180003030 (cmp BYTE [rcx+r8*1+0x1],0 -> the LOW
 * byte zero test; movzx ecx,[rcx+r8*1+0x2]; cmp BYTE [rcx+r10*1],0 ->
 * table[HIGH byte]) - the zero-flag positions are the control /
 * Windows-illegal set {0x00-0x19, 0x22, 0x2A, 0x2F, 0x3A, 0x3C, 0x3E,
 * 0x3F, 0x5C, 0x7C, 0xFF}. Chars with a nonzero low byte are never
 * table-checked (the og's exact wiring, quirk included). */
const u8* exfat_utf16_name_validity() {
    static const u8 table[256] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01,
        0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01,
        0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x00,
    };
    return table;
}

bool name_fragment_gates_ok(const u8* nf, bool first_needed) {
    if ((nf[1] & 3) != 0 || nf[1] >= 4)
        return false;
    const u8* table = exfat_utf16_name_validity();
    for (int c = 0; c < 15; ++c) {
        const u16 ch = u16(nf[2 + c * 2]) | u16(nf[3 + c * 2]) << 8;
        if (ch == 0)
            break; /* terminator: the og's loop returns valid here */
        if ((ch & 0xFF) == 0 && table[ch >> 8] == 0)
            return false; /* zero-low-byte char the og's table rejects */
    }
    if (first_needed && (u16(nf[2]) | u16(nf[3]) << 8) == 0)
        return false;
    return true;
}

void recurse_dir(ScanCtx& ctx, u32 first, bool deleted, tree::FileNode* host_node, u32 depth = 0,
                 bool in_bin = false) {
    if (depth > EXFAT_MAX_DEPTH) {
        ++ctx.stats->chains_broken;
        return;
    }
    u64 key = first;
    if (ctx.visited_dirs.count(key)) {
        ++ctx.stats->chains_broken;
        return;
    }
    ctx.visited_dirs.insert(key);
    bool ok = false;
    std::vector<u8> buf = read_dir_chain(ctx, first, 4u << 20, &ok);
    if (ok)
        walk_entries(ctx, buf, deleted, host_node, depth, in_bin);
}

void walk_entries(ScanCtx& ctx, const std::vector<u8>& buf, bool deleted_context,
                  tree::FileNode* parent_host, u32 depth, bool in_bin) {
    if (buf.empty() || (ctx.cancel && ctx.cancel->checkpoint())) /* P2-8 */
        return;
    (void)deleted_context; /* host selection now driven by parent_host */
    const size_t count = buf.size() / 32;
    size_t i = 0;
    while (i < count) {
        /* live progress (G-04): one dentry examined per iteration - honest
         * cumulative counts; the orchestrator clamps permille monotonic. */
        ++ctx.dentries_done;
        if (ctx.progress && (ctx.dentries_done & 63) == 0)
            (void)ctx.progress(ctx.dentries_done, ctx.dentries_done + (count - i),
                               ctx.progress_user);
        const u8* f = buf.data() + i * 32;
        const u8 raw_type = f[0];
        if (raw_type == TYPE_NONE)
            break; /* end of directory */
        const u8 t = entry_type(raw_type);
        const bool in_use = entry_in_use(raw_type);

        if (t == TYPE_BITMAP && in_use) {
            /* allocation bitmap dentry (live only; deleted bitmap has 0x80
             * cleared - useless for allocation tests) */
            if (!ctx.have_bitmap && buf.size() >= (i + 1) * 32 + sizeof(BitmapEntry)) {
                BitmapEntry be;
                std::memcpy(&be, f, sizeof(be));
                if (be.first_cluster >= 2) {
                    ChainResult bc = ctx.have_fat ? follow_exfat_chain(ctx.fat, be.first_cluster,
                                                                       0x10000, ctx.p.cluster_count)
                                                  : contiguous_chain(be.first_cluster, 1);
                    if (bc.clusters.empty())
                        bc = contiguous_chain(be.first_cluster, 1);
                    u64 total = u64(bc.clusters.size()) * ctx.p.cluster_bytes;
                    if (total > (8u << 20))
                        total = 8u << 20;
                    ctx.bitmap.data.resize(size_t(total), 0);
                    u64 filled = 0;
                    size_t k = 0;
                    while (k < bc.clusters.size() && filled < total) {
                        u64 off = cluster_offset(ctx.p, bc.clusters[k]);
                        u32 run = ctx.p.cluster_bytes;
                        if (filled + run > total)
                            run = u32(total - filled);
                        if (ctx.dev->read_at(off, ctx.bitmap.data.data() + filled, run) != OK)
                            ctx.bitmap_reads_ok = false; /* S-009: torn
                                                          * bitmap never feeds the free map */
                        filled += run;
                        ++k;
                    }
                    ctx.bitmap.cluster_count = ctx.p.cluster_count;
                    ctx.have_bitmap = filled > 0;
                }
            }
            ++i;
            continue;
        }
        if (t == TYPE_UPCASE && in_use) {
            /* N-067 (diff-to-fix): the og's volume analyzer validates the
             * upcase table as part of volume integrity (boot + upcase
             * checks, CExFatVolumeAnalyer - fat-exfat-engine-internals.md
             * :63-64; the 0x82-dentry shape gates are the og's own walk
             * validation, decompile :24440-24452). GRADE, NOT REJECT: a
             * corrupt table raises the integrity signal but never stops
             * the scan (the og's corrupt-volume posture, the EXFAT-2
             * salvage lineage - the finding's open question records that
             * the og's failure path is not decoded; this follows the
             * plan's grade-not-reject default). */
            UpcaseEntry ue;
            std::memcpy(&ue, f, sizeof(ue));
            auto flag_mismatch = [&ctx]() { ++ctx.stats->upcase_mismatches; };
            /* the og's shape gates: reserved1 @+1..+3 and reserved2
             * @+8..+0x13 zero-checked, the TableChecksum field @+4 free,
             * first_cluster >= 2 (the bitmap entry's +1 flags byte is
             * NOT free here - that difference IS the shape test) */
            bool shape_ok = true;
            for (u32 k = 1; k <= 3; ++k)
                if (f[k] != 0)
                    shape_ok = false;
            for (u32 k = 8; k <= 0x13; ++k)
                if (f[k] != 0)
                    shape_ok = false;
            /* length sanity: a contentless table cannot carry the
             * mandatory upcase mapping; an absurd length is flagged
             * without any stream access (the 1 MiB bar is OUR honest
             * cap - real tables are 5836/14942 bytes; the og's shape
             * gate only requires the length to fit the volume) */
            const bool length_ok = ue.data_length != 0 && ue.data_length <= (1ull << 20) &&
                                   ue.data_length <= ctx.dev->size();
            if (!shape_ok || ue.first_cluster < 2 || !length_ok) {
                flag_mismatch();
            } else {
                /* stream read: chain-follow with the contiguous fallback
                 * (the bitmap branch's contract), bounded, incremental
                 * checksum32 - the same ROL31+add the boot region and
                 * entry sets use */
                ChainResult bc = ctx.have_fat ? follow_exfat_chain(ctx.fat, ue.first_cluster,
                                                                   0x10000, ctx.p.cluster_count)
                                              : contiguous_chain(ue.first_cluster, 1);
                if (bc.clusters.empty())
                    bc = contiguous_chain(ue.first_cluster, 1);
                std::vector<u8> chunk;
                chunk.resize(size_t(std::min<u64>(ue.data_length, 64 * 1024)));
                u64 remaining = ue.data_length;
                u32 sum = 0;
                bool read_ok = true;
                size_t k = 0;
                while (remaining > 0 && k < bc.clusters.size()) {
                    const u64 off = cluster_offset(ctx.p, bc.clusters[k]);
                    const u64 take = std::min<u64>(ctx.p.cluster_bytes, remaining);
                    u64 done = 0;
                    while (done < take) {
                        const u32 piece = u32(std::min<u64>(take - done, chunk.size()));
                        if (ctx.dev->read_at(off + done, chunk.data(), piece) != OK) {
                            read_ok = false;
                            break;
                        }
                        sum = checksum32(chunk.data(), piece, sum);
                        done += piece;
                    }
                    if (!read_ok)
                        break;
                    remaining -= take;
                    ++k;
                }
                if (read_ok && remaining == 0 && sum == ue.table_checksum)
                    ++ctx.stats->upcase_validated;
                else
                    flag_mismatch(); /* torn read or checksum mismatch */
            }
            ++i;
            continue;
        }
        if (t == TYPE_VOLUME_LABEL) {
            ++i;
            continue; /* not results */
        }
        if (t == TYPE_FILE) {
            /* entry set: file + stream + names (attribute_count streams) */
            const FileEntry* fe = reinterpret_cast<const FileEntry*>(f);
            const u8 stream_count = fe->secondary_count;
            /* M-014: the og's file-dentry structural gates run BEFORE set
             * completion (case 0x85/5: FUN_180002f90 first, then
             * FUN_18001fef0) - a set-head with an out-of-range secondary
             * count, garbage timestamps or nonzero reserved fields is
             * skipped outright: no park, no emission, no oracle
             * candidacy. The og advances per entry, so the surviving
             * stream/name runs fall to the per-entry cases (the M-013
             * orphan-tail pool here). */
            if (!file_entry_structural_ok(fe)) {
                ++ctx.stats->sets_gated;
                ++i;
                continue;
            }
            if (i + 1 + stream_count > count) {
                /* M-013: a torn set is PARKED, not discarded - the og's
                 * DelexFat path parks the prefix and the oracle may
                 * complete it from a surviving tail (FUN_1800203a0) */
                ScanCtx::ParkedSet ps;
                ps.entries.assign(f, buf.data() + buf.size());
                ps.parent = parent_host;
                ps.depth = depth;
                ps.in_bin = in_bin;
                ctx.parked_sets.push_back(std::move(ps));
                break; /* torn set */
            }
            const u8* stream = buf.data() + (i + 1) * 32;
            if (entry_type(stream[0]) != TYPE_STREAM) {
                ++i;
                continue;
            }
            const StreamEntry* se = reinterpret_cast<const StreamEntry*>(stream);
            /* og stream-dentry gates (FUN_180003090, audit row 11):
             * reserved fields - damaged-set counting parity. N-010e
             * (diff-to-fix): the og's tests are BYTE-granular with an
             * OR form (single-byte loads in the decompile, :1843-1846):
             * reserved1 byte@+2 must be 0; reserved2 passes when EITHER
             * byte (+6/+7) is 0; reserved3 passes when ANY of its four
             * bytes (+0x10..0x13) is 0. The old word-nonzero tests
             * rejected partially-nonzero reserved fields the og accepts
             * (BIOS/embedded formatters leave stray low bytes). A
             * failing set is downgraded (damaged), not skipped. */
            const u8* sb = reinterpret_cast<const u8*>(se);
            const bool stream_reserved_bad =
                in_use && (sb[0x02] != 0 || !(sb[0x06] == 0 || sb[0x07] == 0) ||
                           !(sb[0x10] == 0 || sb[0x11] == 0 || sb[0x12] == 0 || sb[0x13] == 0));
            if (stream_reserved_bad)
                ++ctx.stats->sets_damaged;
            const u8 name_len = se->name_length;
            u32 name_entries = (name_len + 14) / 15;
            if (u32(name_entries) > stream_count)
                name_entries = stream_count;

            /* entry-set checksum. M-013 (diff-to-fix): the og's dual-form
             * gate on DELETED sets is UNCONDITIONAL og parity - the raw
             * ROL15 form OR the InUse-restored form (0x85/0xC0/0xC1 type
             * substitutions, FUN_18001eba0 :25153-25198) must match the
             * stored field. A failed deleted set is dropped from the
             * direct emission path (the og's set completion refuses it,
             * FUN_18001fef0 :26287-26292) and PARKED for the
             * reconstruction oracle - never emitted as a confident node
             * (the pre-fix core accepted torn deleted sets unconditionally:
             * the FP surface this finding closes). LIVE sets keep the
             * flag-gated downgrade (the registered deliberate divergence
             * for live sets - see cr_exfat_engine.h). */
            bool set_ok = true;
            if (!in_use) {
                if (!entry_set_checksum_valid(f, 1 + stream_count, fe->set_checksum)) {
                    ++ctx.stats->sets_rejected;
                    ScanCtx::ParkedSet ps;
                    ps.entries.assign(f, f + size_t(1 + stream_count) * 32);
                    ps.parent = parent_host;
                    ps.depth = depth;
                    ps.in_bin = in_bin;
                    ctx.parked_sets.push_back(std::move(ps));
                    i += 1 + stream_count;
                    continue; /* parked: the oracle decides */
                }
            } else if (ctx.flags.validate_entry_sets) {
                u16 chk = entry_set_checksum(buf.data() + i * 32, 1 + 1 + name_entries);
                if (chk != fe->set_checksum) {
                    set_ok = false;
                    ++ctx.stats->sets_damaged;
                }
            }

            /* M-014: the og's stream emission gates (FUN_180029280
             * :34562-34564): data_length, first_cluster and name_length
             * must ALL be nonzero, and the entry after the stream must be
             * a name entry. ZERO-LENGTH FILES ARE NOT EMITTED - the og's
             * documented quirk (an empty file carries no recoverable
             * content and never appeared in og results; the core matched
             * the og by inheriting it). The next-entry-is-name half is
             * already enforced by the name-assembly path below (a
             * non-name successor yields an empty name and no emission).
             * A failing set does not emit; the stream/name entries park
             * as orphan tails (the M-013 pool) and the oracle re-applies
             * these gates on any reconstruction. */
            if (se->data_length == 0 || se->first_cluster == 0 || se->name_length == 0) {
                ++ctx.stats->sets_gated;
                ++i;
                continue;
            }

            /* assemble name from fragments */
            /* M-014: name-fragment gates (FUN_180003030) run over the
             * needed fragments before assembly: flags & 3 == 0, flags < 4,
             * and the first needed fragment carries a nonzero first char.
             * A failing set does not emit (the og's early-fragment path
             * skips the fragment; its uVar6>=5 late-fragment tolerance
             * only covers padding fragments beyond the needed count,
             * which this walk never consumes - the conservative reject
             * is documented in the finding's Resolution). */
            {
                bool name_gates_ok = true;
                size_t chars_in = 0;
                for (u32 ne = 0; ne < name_entries; ++ne) {
                    const u8* nf = buf.data() + (i + 2 + ne) * 32;
                    if (entry_type(nf[0]) != TYPE_NAME)
                        break;
                    if (!name_fragment_gates_ok(nf, ne == 0)) {
                        name_gates_ok = false;
                        break;
                    }
                    chars_in += 15;
                    if (chars_in >= name_len)
                        break;
                }
                if (!name_gates_ok) {
                    ++ctx.stats->sets_gated;
                    ++i;
                    continue;
                }
            }
            std::u16string name;
            name.reserve(name_len);
            for (u32 ne = 0; ne < name_entries && name.size() < name_len; ++ne) {
                const u8* nf = buf.data() + (i + 2 + ne) * 32;
                if (entry_type(nf[0]) != TYPE_NAME)
                    break;
                const NameEntry* nm = reinterpret_cast<const NameEntry*>(nf);
                for (int c = 0; c < 15 && name.size() < name_len; ++c)
                    name.push_back(nm->name[c]);
            }
            if (name.empty()) {
                i += 1 + 1 + name_entries;
                continue;
            }
            /* N-174 (diff-to-fix): the og's exFAT props builder refuses
             * names starting "._" (exFATSearchDLL :45151 strncmp at +0x29
             * vs "._" - the macOS AppleDouble sidecar class Finder writes
             * on exFAT media) and returns 0 before the properties object
             * is built: the entry never surfaces, silently. The HFS+ sink
             * (N-028) is the same noise-suppression family on the volume
             * the sidecars came from. Regular dotfiles are NOT touched -
             * the gate is prefix-specific ("._" only), and directories
             * named "._foo" are sunk the same way (the og's gate keys on
             * the name alone, before any type check in the props build).
             */
            if (name.size() >= 2 && name[0] == u'.' && name[1] == u'_') {
                ++ctx.stats->apple_doubles_sunk;
                i += 1 + 1 + name_entries;
                continue;
            }

            const bool deleted = !in_use;
            const bool is_dir = (fe->file_attribute & 0x10) != 0;
            const u64 data_len = se->data_length;
            const u32 first_cluster = se->first_cluster;
            const bool no_fat = (se->flags & 0x02) != 0;

            /* EXFAT-1 (AUDIT-FAT): og rejects first_cluster == 1 and
             * first_cluster >= 0xFFFFFFF7 (FUN_180003090:1857-1859, inlined
             * 24404-24412, backward-walk form 2109) - 1 is reserved and
             * 0xFFFFFFF7+ are BAD/EOC-family sentinels, never positions.
             * (The v2 artifact's "must be 1 or >= 0xFFFFFF7" polarity was
             * inverted; the decompile is unambiguous across three sites.)
             * Damaged set: counted, entry kept as metadata-only. */
            const bool fc_invalid = first_cluster == 1 || first_cluster >= 0xFFFFFFF7u;
            if (fc_invalid)
                ++ctx.stats->sets_damaged;

            /* chain resolution */
            ChainResult chain;
            if (!fc_invalid && (is_dir || first_cluster >= 2)) {
                u32 expect = u32((data_len + ctx.p.cluster_bytes - 1) / ctx.p.cluster_bytes);
                if (no_fat && !is_dir && expect > 0) {
                    chain = contiguous_chain(first_cluster, expect);
                    ++ctx.stats->no_fat_chains;
                } else if (no_fat && is_dir) {
                    /* EXFAT-6 (AUDIT-FAT): NoFatChain directories read
                     * ceil(data_length / cluster_bytes) clusters, not 1
                     * (spec-legal multi-cluster dirs truncated before). */
                    u32 dir_clusters =
                        u32((data_len + ctx.p.cluster_bytes - 1) / ctx.p.cluster_bytes);
                    chain = contiguous_chain(first_cluster, dir_clusters ? dir_clusters : 1);
                    ++ctx.stats->no_fat_chains;
                } else if (ctx.have_fat) {
                    chain = follow_exfat_chain(ctx.fat, first_cluster,
                                               is_dir ? 0x10000 : (expect ? expect : 1),
                                               ctx.p.cluster_count);
                    if (chain.assumed_contiguous)
                        ++ctx.stats->assumed_contiguous;
                } else {
                    chain = contiguous_chain(first_cluster, 1);
                }
            }

            /* M-052: recycle-bin pairing inside the recognized bin roots
             * (the og's four-name _stricmp set, exFATSearchDLL :40583-
             * :40592 - "mirrored from FAT engine", multiple _stricmp
             * sites). $I<token> is bin metadata: parse the record image
             * from the chain, remember it, never surface the file.
             * $R<token> is the content: remapped at emission below. */
            if (in_bin && !is_dir) {
                const std::u16string tok = recycle::token_of(name);
                if (!tok.empty() && name[1] == u'I') {
                    std::vector<u8> img;
                    u64 want = data_len < 2076 ? data_len : 2076;
                    if (want >= 30) {
                        img.resize(size_t(want), 0);
                        u64 got = 0;
                        size_t k = 0;
                        while (k < chain.clusters.size() && got < want) {
                            const u64 off = cluster_offset(ctx.p, chain.clusters[k]);
                            const u64 take = std::min<u64>(
                                std::min<u64>(want - got, ctx.p.cluster_bytes), img.size() - got);
                            if (ctx.dev->read_at(off, img.data() + got, u32(take)) != OK)
                                break;
                            got += take;
                            ++k;
                        }
                        recycle::Info ri;
                        if (got >= 30 && recycle::parse_record(img.data(), u32(got), ri)) {
                            ctx.recycle_info[tok] = ri;
                            ++ctx.stats->recycle_info;
                        }
                    }
                    i += 1 + 1 + u32(name_entries);
                    continue; /* bin metadata, not recoverable content */
                }
            }

            tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
            if (!props) {
                i += 1 + 1 + name_entries;
                continue;
            }
            props->name = name;
            props->fs_kind = u32(FsKindCode::Exfat); /* corpus factory code */
            props->logical_size =
                is_dir ? u64(chain.clusters.size()) * ctx.p.cluster_bytes : data_len;
            props->times.created = exfat_time_to_unix100(fe->create_timestamp);
            props->times.modified = exfat_time_to_unix100(fe->modify_timestamp);
            props->times.changed = props->times.modified;
            props->times.accessed = exfat_time_to_unix100(fe->access_timestamp);
            props->attributes = fe->file_attribute;
            props->flags = in_use ? u16(0x10) : u16(0);
            /* OQ-SA-1 (resolved): the og's exFAT props carry the first-
             * cluster anchor at +0x50 (the same virtual SetFirstCluster
             * contract as FAT's +0x3e; vt+0x28) — the orphan/LostDir path
             * stamps it unconditionally from the entry's first cluster
             * (exFATSearchDLL :5246-5252), and the node carries the
             * cluster's absolute offset. Stamped here from the stream
             * dentry's FirstCluster, independent of chain resolution. */
            props->fat_origin_cluster = first_cluster;

            /* extents (contiguous runs, slice-relative) */
            size_t k = 0;
            while (k < chain.clusters.size()) {
                size_t j = k;
                while (j + 1 < chain.clusters.size() &&
                       chain.clusters[j + 1] == chain.clusters[j] + 1)
                    ++j;
                u64 off = cluster_offset(ctx.p, chain.clusters[k]);
                u64 len = u64(j - k + 1) * ctx.p.cluster_bytes;
                if (is_dir && props->extents.size() >= 64)
                    break;
                props->extents.push_back({off, len});
                k = j + 1;
            }

            /* grading */
            tree::Confidence conf;
            if (!deleted) {
                conf = set_ok ? tree::Confidence::Excellent : tree::Confidence::PartialRisk;
            } else if (ctx.have_bitmap && !chain.clusters.empty()) {
                u64 total = 0, reused = 0;
                for (u32 c : chain.clusters) {
                    ++total;
                    if (ctx.bitmap.test(c))
                        ++reused;
                }
                const double ratio = double(reused) / double(total);
                if (ratio >= 1.0)
                    conf = tree::Confidence::Overwritten;
                else if (ratio > 0.0)
                    conf = tree::Confidence::PartialRisk;
                else
                    conf = tree::Confidence::Excellent;
            } else if (chain.clusters.empty()) {
                conf = tree::Confidence::Overwritten;
            } else {
                conf = tree::Confidence::PartialRisk;
            }
            /* An assumed-contiguous chain (EXFAT-2) is never graded above
             * PartialRisk: the FAT did not confirm the layout. */
            if (chain.assumed_contiguous && conf == tree::Confidence::Excellent)
                conf = tree::Confidence::PartialRisk;

            /* children attach under their directory node; top-level under
             * the roots. deleted sets land in the deleted/lost roots. */
            tree::FileNode* host =
                parent_host ? parent_host
                            : (deleted ? (ctx.root_lost ? ctx.root_lost : ctx.root_deleted)
                                       : static_cast<tree::FileNode*>(ctx.root_active));
            if (deleted && !parent_host && ctx.root_deleted)
                host = ctx.root_deleted;
            tree::FileNode* n = ctx.tree->add_file(host, props);
            props->release();
            if (n) {
                n->set_origin(tree::Origin::Structured);
                /* M-052: $R recycle remap - the ORIGINAL name/size/time
                 * from the $I sibling (the set stays live inside the bin
                 * - the file was deleted from the user's location, so
                 * the node is Deleted). An unseen $I queues for the
                 * post-walk fixup (the $I may sit later in the walk). */
                if (in_bin && !is_dir) {
                    const std::u16string tok = recycle::token_of(name);
                    if (!tok.empty() && name[1] == u'R') {
                        auto it = ctx.recycle_info.find(tok);
                        if (it != ctx.recycle_info.end()) {
                            props->name = it->second.orig_name;
                            if (it->second.size != 0)
                                props->logical_size = it->second.size;
                            props->times.modified = it->second.time;
                            ++ctx.stats->recycle_remapped;
                            if (!deleted)
                                n->set_deleted_state(tree::DeletedState::Deleted);
                        } else {
                            ctx.pending_recycle_r.push_back({n, tok});
                        }
                    }
                }
                if (deleted) {
                    ++ctx.stats->deleted_entries;
                    n->set_deleted_state(set_ok ? tree::DeletedState::Deleted
                                                : tree::DeletedState::Damaged);
                }
                n->set_confidence(conf);
            }
            if (is_dir)
                ++ctx.stats->dirs;
            else
                ++ctx.stats->files;

            /* recursion: children of this directory attach under its node;
             * a bin root turns the recycle context on for the subtree */
            if (is_dir && first_cluster >= 2)
                recurse_dir(ctx, first_cluster, deleted, n, depth + 1,
                            in_bin || recycle::is_bin_root(name));

            i += 1 + 1 + u32(name_entries);
            continue;
        }
        if (t == TYPE_STREAM || t == TYPE_NAME) {
            /* M-013: an orphan stream/name run at a set-head position - a
             * destroyed set's surviving tail. PARK it as an oracle
             * candidate (the og's parked-tail pool) instead of silently
             * skipping entry by entry. */
            size_t j = i;
            while (j < count) {
                const u8 tj = entry_type(buf[size_t(j) * 32]);
                if (tj != TYPE_STREAM && tj != TYPE_NAME)
                    break;
                ++j;
            }
            ScanCtx::ParkedTail pt;
            pt.entries.assign(buf.data() + i * 32, buf.data() + j * 32);
            ctx.parked_tails.push_back(std::move(pt));
            i = j;
            continue;
        }
        ++i; /* unknown dentry type: skip */
    }
    if (ctx.progress && count)
        (void)ctx.progress(ctx.dentries_done, ctx.dentries_done, ctx.progress_user);
}

/* M-013 (diff-to-fix): the reconstruction oracle - the og's parked-set
 * reassembly (FUN_1800203a0 :26539-26830, decoded first-hand this
 * session). For each parked head, each parked tail, and each
 * substitution position from the set's SECONDARY COUNT DOWN TO 1
 * (LAB_18002083f: local_54 = local_54 - 1; uVar8 = local_54 - the FILE
 * entry is never substituted), the candidate tail is overlaid over the
 * set's [pos, count) suffix in a scratch copy and the DUAL-form
 * checksum is re-tested: `if ((uVar19 == stored) || (uVar20 == stored))`
 * (:26662-26664) - accept on either match. The og's parked-list callers
 * iterate head x tail pairs with a not-self guard (:26299, :26403) -
 * same-shape head/tail combinations here. An accepted reconstruction is
 * re-walked through the standard emission path (the dual gate passes on
 * the substituted bytes), landing in the tree exactly like a set that
 * was never damaged. */
void reconstruct_parked_sets(ScanCtx& ctx) {
    for (auto& head : ctx.parked_sets) {
        if (head.entries.empty() || head.entries.size() < 32)
            continue;
        const u32 declared = u32(head.entries[1]) + 1; /* secondary_count + 1 */
        /* M-014: parked heads pass the file gates before parking, so
         * secondary_count is in [2,18] and declared in [3,19]; the old
         * 512 sanity cap is tightened to the gated bound (a wider
         * candidate can never re-pass the gates in the re-walk). */
        if (declared < 3 || declared > 19)
            continue; /* no defensible set shape to reconstruct */
        const u16 stored = u16(head.entries[2]) | u16(head.entries[3]) << 8;
        bool accepted = false;
        for (const auto& tail : ctx.parked_tails) {
            if (tail.entries.empty())
                continue;
            const u32 tail_entries = u32(tail.entries.size() / 32);
            /* substitution position from the declared tail start DOWN to
             * 1: suffix lengths 1..declared-1 (the file entry stays) */
            for (u32 pos = declared - 1; pos >= 1; --pos) {
                const u32 suffix = declared - pos;
                if (tail_entries < suffix)
                    continue;
                std::vector<u8> cand(size_t(declared) * 32, 0);
                const size_t avail = std::min(head.entries.size(), size_t(pos) * 32);
                std::memcpy(cand.data(), head.entries.data(), avail);
                std::memcpy(cand.data() + size_t(pos) * 32, tail.entries.data(),
                            size_t(suffix) * 32);
                if (entry_set_checksum_valid(cand.data(), declared, stored)) {
                    ++ctx.stats->sets_reconstructed;
                    /* the re-walk drives the standard emission path: the
                     * substituted set parses, passes the dual gate, and
                     * lands in the tree under the parked host */
                    walk_entries(ctx, cand, true, head.parent, head.depth, head.in_bin);
                    accepted = true;
                    break;
                }
            }
            if (accepted)
                break;
        }
        head.entries.clear(); /* consumed or unrecoverable */
    }
}

} // namespace

u32 scan_exfat(IStorageDevice* slice, const ExfatScanFlags& flags, CancelToken* cancel,
               bool (*progress)(u64, u64, void*), void* progress_user, tree::ResultTree& tree,
               tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
               ExfatScanStats& stats, storage::FreeMap* free_out) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    ExfatParams p;
    u32 rc = validate_boot_region(slice, p);
    if (rc != OK)
        return rc;
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;
    /* S-066: the recognition re-base is surfaced, never silent (the
     * skip-honesty counter family) */
    if (p.base_shift_sectors != 0)
        ++stats.base_shifted;

    ScanCtx ctx;
    ctx.cancel = cancel; /* P2-8: cancellable walk */
    ctx.progress = progress;
    ctx.progress_user = progress_user;
    ctx.dev = slice;
    ctx.p = p;
    ctx.flags = flags;
    ctx.tree = &tree;
    ctx.root_active = root_active;
    ctx.root_deleted = root_deleted;
    ctx.root_lost = root_lost;
    ctx.stats = &stats;

    /* FAT load (if the volume has one). S-013: the whole-FAT fast path
     * covers small volumes; EVERY other case (FAT over 32 MiB, FAT
     * extending past the device end, or a failed whole read) falls back
     * to the og's windowed accessor (FUN_180021c60: 1 MiB cached
     * windows, no total-size cap) - have_fat is NEVER false while
     * fat_length != 0, so FAT-chained files never truncate to a single
     * cluster on large volumes (the pre-S-013 silent one-cluster
     * truncation for every file on >32 MiB-FAT exFAT volumes). */
    if (p.fat_length != 0) {
        const u64 fat_base = u64(p.fat_offset) * p.bytes_per_sector;
        u64 fat_bytes = u64(p.fat_length) * p.bytes_per_sector;
        bool whole = false;
        if (fat_bytes <= (32u << 20) && fat_base + fat_bytes <= slice->size()) {
            ctx.fat.data.resize(size_t(fat_bytes));
            if (slice->read_at(fat_base, ctx.fat.data.data(), u32(fat_bytes)) == OK) {
                ctx.fat.byte_len = u32(fat_bytes);
                ctx.have_fat = true;
                whole = true;
            } else {
                ctx.fat.data.clear();
                ctx.fat.data.shrink_to_fit();
            }
        }
        if (!whole && fat_base < slice->size()) {
            /* windowed mode: per-window bounds/read failures degrade the
             * affected entries to FREE (the EXFAT-2 assumed-contiguous
             * salvage applies downstream), never to a 1-cluster stub */
            ctx.fat.win_dev = slice;
            ctx.fat.win_base = fat_base;
            ctx.fat.total_bytes = fat_bytes;
            ctx.have_fat = true;
        }
    }

    /* root directory (chain from FAT; single-cluster fallback) */
    if (p.first_cluster_of_root >= 2)
        recurse_dir(ctx, p.first_cluster_of_root, false, nullptr, 0, false);
    if (ctx.cancel && ctx.cancel->checkpoint())
        return ERR_CANCELLED;

    /* M-013 (diff-to-fix): the parked-set reconstruction oracle runs
     * AFTER the whole walk (every directory's orphans are candidates for
     * every parked head - the og's park is scan-global, its callers
     * iterate the parked list at set-completion boundaries). */
    if (!ctx.parked_sets.empty())
        reconstruct_parked_sets(ctx);

    /* M-052 post-walk fixup: $R sets emitted raw during the walk whose
     * $I sat later in the walk order (or in a sibling bin subfolder).
     * Mutating props through the node is safe post-emission (arena-
     * stable nodes; the tree's indexes key on record_ref/extent). Orphan
     * $R sets (missing/unparseable $I) keep their raw bin name - og
     * parity. */
    for (const auto& pr : ctx.pending_recycle_r) {
        auto it = ctx.recycle_info.find(pr.token);
        if (it == ctx.recycle_info.end())
            continue;
        tree::FileProperties* pp = pr.node->props();
        if (pp == nullptr)
            continue;
        pp->name = it->second.orig_name;
        if (it->second.size != 0)
            pp->logical_size = it->second.size;
        pp->times.modified = it->second.time;
        pr.node->set_deleted_state(tree::DeletedState::Deleted);
        ++stats.recycle_remapped;
    }

    /* S-009 (free overlay): the allocation bitmap dentry walked above is
     * the exFAT allocation truth (bit set = cluster allocated). Invert
     * to free; clusters beyond the loaded bitmap (the 8 MiB loader cap)
     * stay NOT-free (unknown is not free). Cluster 0's byte = the heap
     * base minus the two virtual reserved clusters (linear like the
     * engine's cluster_offset math).
     * Hardening (S-009 follow-up): (a) clusters 0/1 are VIRTUAL - their
     * bits in the allocation bitmap are spec-zero and their byte span
     * [base, heap_base) is the boot/FAT region, not free space; the fill
     * starts at cluster 2 (the first fill started at 0 and painted the
     * two pre-heap sectors free on every exFAT volume). (b) a bitmap
     * whose reads FAILED is refused whole - the zero fill reads as
     * "everywhere allocated" -> inverted "everywhere free", a
     * false-free map; honesty demands no map over a lie. */
    if (free_out != nullptr) {
        free_out->clear();
        if (ctx.have_bitmap && ctx.bitmap_reads_ok && p.cluster_count != 0 &&
            p.cluster_bytes != 0) {
            const u64 heap_base = u64(p.cluster_heap_offset) * p.bytes_per_sector;
            const u64 two_cb = u64(2) * p.cluster_bytes;
            free_out->base_byte = heap_base >= two_cb ? heap_base - two_cb : 0;
            free_out->cluster_bytes = p.cluster_bytes;
            /* cluster numbers 0..cluster_count+1 (the two virtual + ALL
             * data clusters 2..cluster_count+1 - the first landing copied
             * p.cluster_count verbatim and the LAST TWO data clusters
             * fell out of the map's coverage, never reading free) */
            free_out->cluster_count = u64(p.cluster_count) + 2;
            free_out->bits.assign(size_t((free_out->cluster_count + 7) / 8), 0);
            const size_t avail = free_out->bits.size() < ctx.bitmap.data.size()
                                     ? free_out->bits.size()
                                     : ctx.bitmap.data.size();
            const u64 covered = u64(avail) * 8;
            for (u64 c = 2; c < covered && c < free_out->cluster_count; ++c) {
                if (!ctx.bitmap.test(u32(c)))
                    free_out->bits[size_t(c >> 3)] |= u8(1u << (c & 7));
            }
        }
    }

    stats.fat_window_refills = ctx.fat.refills; /* S-013 windowed-FAT telemetry */
    if (progress)
        (void)progress(1, 1, progress_user);
    return OK;
}

} // namespace exfat
} // namespace cr
