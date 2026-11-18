/*
 * cr_rescue.h - ddrescue-style two-pass imaging with bad-sector isolation
 * and a resumable mapfile (F-33/F-34, Wave 6).
 *
 * SectorForge-parity behavior (MIT reference, behavioral re-implementation
 * from the documented algorithm - worklog/SECTORFORGE-FEATURES.md sheet
 * sections 2.1/2.2, citing sf-core imager.rs + mapfile.rs; no code copied;
 * the on-disk mapfile format follows GNU ddrescue's documented format,
 * which is a de-facto interchange standard):
 *
 *   Pass 1 ("copy") sweeps in large chunks, skipping past damage (a whole
 *   skip-sized span marked NonTrimmed) to maximize rescued data fast;
 *   Pass 2 ("retry") binary-splits every damaged region down toward
 *   individual sectors, retrying each sector up to N times before
 *   declaring it BadSector. Every outcome lands in the mapfile so a run
 *   stops and resumes anywhere; the destination is flushed BEFORE the map
 *   is saved (the map never claims bytes the image does not hold).
 *
 * Truth contract (the product's): stats derive from the map - rescued /
 * bad / pending bytes are the map's own sums; a partial image is always
 * distinguishable. No SHA-256 sidecar for rescue images BY DESIGN: an
 * image with NonTried holes is not a byte-image of the source (holes read
 * as zeros that were never read from the disk - a digest would vouch for
 * synthesized bytes). The mapfile IS the truth artifact; a fully-rescued
 * map (all Finished) is the completion certificate.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RESCUE_H_
#define CR_RESCUE_H_

#include "cr_device.h"
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace storage {

/* ---- F-34: the map -------------------------------------------------------- */

enum class RescueStatus : u8 {
    NonTried = 0,   /* '?' - never attempted */
    NonTrimmed = 1, /* '*' - skipped over at chunk granularity (pass 2 candidate) */
    NonScraped = 2, /* '/' - a split candidate below chunk granularity */
    BadSector = 3,  /* '-' - retried at sector granularity, still failing */
    Finished = 4,   /* '+' - rescued bytes in the image */
};

/* the ddrescue mapfile status characters (interop: ddrescueview-readable) */
char rescue_status_char(RescueStatus s);
bool rescue_status_from_char(char c, RescueStatus* out);

struct RescueBlock {
    u64 pos = 0;
    u64 size = 0;
    RescueStatus status = RescueStatus::NonTried;
};

/* Sorted, non-overlapping, contiguous block list covering [0, total).
 * mark() splits/carves holes at arbitrary (sector-multiple) offsets and
 * re-merges same-status neighbors; find_next/find_prev locate the next
 * block in a status set, clipped to the map bounds. */
class RescueMap {
public:
    RescueMap() = default;

    /* a fresh all-NonTried map over [0, total). total == 0 is legal (the
     * empty-device parity: zero blocks, everything trivially Finished). */
    void reset(u64 total);

    u64 total() const { return total_; }
    u64 current_pos() const { return current_pos_; }
    void set_current_pos(u64 pos) { current_pos_ = pos; }
    RescueStatus current_status() const { return current_status_; }

    const std::vector<RescueBlock>& blocks() const { return blocks_; }

    /* mark [pos, pos+size) with the status. pos/size must be sector
     * multiples and inside the map (pos + size <= total); out-of-range
     * marks refuse with false (never a silent clamp). */
    bool mark(u64 pos, u64 size, RescueStatus status);

    /* the first block >= `from` whose status is in the set; found=false
     * when none. Both clip to [0, total). */
    bool find_next(u64 from, const RescueStatus* statuses, size_t n_statuses, u64* pos,
                   u64* size) const;
    /* the last block that ENDS before `before` (mirrored for reverse). */
    bool find_prev(u64 before, const RescueStatus* statuses, size_t n_statuses, u64* pos,
                   u64* size) const;

    /* status sums (the stats' source of truth) */
    u64 bytes_of(RescueStatus s) const;
    bool all_finished() const;
    u64 finished_bytes() const { return bytes_of(RescueStatus::Finished); }

    /* ddrescue-compatible mapfile text: "#"-comments, the
     * "current_pos current_status" line, then "0x%08X  0x%08X  status"
     * per block. ASCII; deterministic. */
    std::string to_mapfile_text() const;
    /* parse the mapfile text (tolerant of comments/blank lines/CRLF);
     * refuses (false) on overlapping/out-of-order blocks, unknown status
     * characters, or a block running past the declared total. */
    static bool parse(const std::string& text, RescueMap* out);

private:
    void merge_adjacent_();
    std::vector<RescueBlock> blocks_;
    u64 total_ = 0;
    u64 current_pos_ = 0;
    RescueStatus current_status_ = RescueStatus::NonTried;
};

/* ---- the sinks ------------------------------------------------------------ */

/* Positioned-write destination (the image file): the rescue flow writes
 * chunks at absolute offsets (skip-past-damage means the write order is
 * not sequential), and the map-flush ordering needs an explicit flush. */
class RescueImageSink {
public:
    virtual ~RescueImageSink() = default;
    /* write len bytes at the absolute image offset; OK or an error code */
    virtual u32 pwrite(u64 offset, const void* data, u32 len) = 0;
    /* flush buffered image bytes to the OS (called BEFORE every map save
     * - the map never claims unwritten data) */
    virtual u32 flush() = 0;
};

/* Mapfile persistence (atomic tmp+rename on the implementing side). */
class RescueMapIo {
public:
    virtual ~RescueMapIo() = default;
    /* atomically persist the mapfile text. `forced`: a failure propagates
     * (pass boundaries, shutdown); periodic (false) failures are the
     * caller's to swallow by policy - see RescueImager. */
    virtual u32 write_map(const std::string& text, bool forced) = 0;
    /* read the existing mapfile; returns an error when absent/unreadable */
    virtual u32 read_map(std::string* out) = 0;
    /* true when a mapfile exists (resume detection) */
    virtual bool map_exists() const = 0;
};

/* ---- F-33: the two-pass imager -------------------------------------------- */

struct RescueImagerConfig {
    u64 initial_chunk = 1 << 20;       /* pass-1 read granularity */
    u64 skip_size = 1 << 20;           /* damage jump: max(skip, chunk) marked NonTrimmed */
    u32 max_retries = 3;               /* per-sector retries in pass 2 */
    bool reverse = false;              /* pass 1 runs backwards (damage from the
                                        * other side; dest should be sparse) */
    u64 checkpoint_interval_ms = 5000; /* periodic map-save cadence */
};

struct RescueStats {
    u64 rescued_bytes = 0; /* Finished */
    u64 bad_bytes = 0;     /* BadSector */
    u64 pending_bytes = 0; /* NonTried + NonTrimmed + NonScraped */
    u64 current_pos = 0;
    bool pass_retry = false; /* false = copy pass, true = retry pass */
    bool cancelled = false;
    bool resumed = false;
};

typedef void (*RescueProgressFn)(void* user, const RescueStats& stats);

class RescueImager {
public:
    RescueImager(IStorageDevice& src, RescueImageSink& sink, RescueMapIo& map_io,
                 RescueImagerConfig cfg = RescueImagerConfig());

    /* Resume contract (refusals, never silent):
     *  - a loaded map whose total != src.size() refuses (wrong map for
     *    this source) with ERR_BAD_PARAMETER;
     *  - a map with Finished bytes but a missing/short destination is the
     *    CALLER's precondition (the sink knows the destination state) -
     *    checked by the platform driver, documented here.
     * `preloaded` (optional): the caller's loaded map (resume); null = a
     * fresh all-NonTried map is initialized and saved immediately. */
    u32 run(RescueMap* preloaded, CancelToken* cancel, RescueProgressFn progress,
            void* progress_user, RescueStats* stats);

    const RescueMap& map() const { return map_; }
    RescueMap& map() { return map_; }

private:
    u32 run_copy_pass_(bool reverse, CancelToken* cancel, RescueProgressFn progress,
                       void* progress_user);
    u32 run_retry_pass_(CancelToken* cancel, RescueProgressFn progress, void* progress_user);
    u32 split_and_retry_(u64 pos, u64 len, CancelToken* cancel, RescueProgressFn progress,
                         void* progress_user);
    bool checkpoint_(bool forced, u32* rc_out);
    void emit_progress_(RescueProgressFn progress, void* user);
    RescueStats stats_() const;

    IStorageDevice& src_;
    RescueImageSink& sink_;
    RescueMapIo& map_io_;
    RescueImagerConfig cfg_;
    RescueMap map_;
    std::vector<u8> buf_;
    u64 sector_ = 512;
    std::chrono::steady_clock::time_point last_checkpoint_{};
    bool have_checkpoint_clock_ = false;
    bool cancelled_ = false; /* rides stats_() to the caller */
};

/* round `v` down to a sector multiple, never below one sector (a single
 * sector can always be attempted); also caps at `cap` */
u64 rescue_align_down_min1(u64 v, u64 sector, u64 cap);

} // namespace storage
} // namespace cr

#endif // CR_RESCUE_H_
