/*
 * cr_rescue.cpp - the ddrescue-style two-pass imager (F-33/F-34).
 *
 * See cr_rescue.h for the behavioral contract and the SectorForge/ddrescue
 * citations. Implementation notes:
 *
 *  - the copy pass reads sector-aligned chunks (align_down_min1: a short
 *    tail is still one sector); a failed read marks
 *    max(skip_size, chunk_len) bytes NonTrimmed and jumps past the damage;
 *  - the retry pass binary-splits NonTrimmed/NonScraped regions toward
 *    sector granularity; a sector-failing read retries up to max_retries
 *    times before BadSector;
 *  - checkpoints: dest flush -> map save, at <= cfg.checkpoint_interval_ms
 *    cadence and at each pass boundary (forced); a PERIODIC save failure
 *    is swallowed (aborting a live rescue over an AV lock would be far
 *    worse - the next checkpoint retries), a FORCED failure propagates;
 *  - the reverse copy pass mirrors find_prev; current_pos keeps the
 *    LOWEST point reached (0 = the fresh-map convention);
 *  - resume: the caller preloads the map (validating total vs source);
 *    find_next's from-current_pos with the from-0 fallback picks up
 *    untried areas a reverse run left behind current_pos.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_rescue.h"

#include <algorithm>
#include <chrono>
#include <cstring>

namespace cr {
namespace storage {

char rescue_status_char(RescueStatus s) {
    switch (s) {
    case RescueStatus::NonTried:
        return '?';
    case RescueStatus::NonTrimmed:
        return '*';
    case RescueStatus::NonScraped:
        return '/';
    case RescueStatus::BadSector:
        return '-';
    case RescueStatus::Finished:
        return '+';
    }
    return '?';
}

bool rescue_status_from_char(char c, RescueStatus* out) {
    switch (c) {
    case '?':
        *out = RescueStatus::NonTried;
        return true;
    case '*':
        *out = RescueStatus::NonTrimmed;
        return true;
    case '/':
        *out = RescueStatus::NonScraped;
        return true;
    case '-':
        *out = RescueStatus::BadSector;
        return true;
    case '+':
        *out = RescueStatus::Finished;
        return true;
    }
    return false;
}

u64 rescue_align_down_min1(u64 v, u64 sector, u64 cap) {
    if (sector == 0)
        sector = 512;
    if (cap == 0)
        cap = sector;
    if (cap > sector)
        cap = cap - cap % sector; /* cap itself sector-aligned down */
    if (cap < sector)
        cap = sector;
    u64 out = v - v % sector;
    if (out == 0 && v > 0)
        out = sector; /* never below one sector (a single sector can
                         always be attempted) */
    return std::min(out, cap);
}

/* ---- RescueMap ------------------------------------------------------------- */

void RescueMap::reset(u64 total) {
    total_ = total;
    current_pos_ = 0;
    current_status_ = RescueStatus::NonTried;
    blocks_.clear();
    if (total > 0)
        blocks_.push_back({0, total, RescueStatus::NonTried});
}

bool RescueMap::mark(u64 pos, u64 size, RescueStatus status) {
    if (size == 0)
        return true; /* a zero-size mark is a no-op, not an error */
    if (pos >= total_ || size > total_ - pos)
        return false; /* out of range: refuse, never clamp */

    /* carve [pos, pos+size) out of any overlapping blocks, then insert */
    std::vector<RescueBlock> out;
    out.reserve(blocks_.size() + 3);
    const u64 end = pos + size;
    for (const RescueBlock& b : blocks_) {
        const u64 b_end = b.pos + b.size;
        if (b_end <= pos || b.pos >= end) {
            out.push_back(b); /* disjoint */
            continue;
        }
        if (b.pos < pos)
            out.push_back({b.pos, pos - b.pos, b.status});
        if (b_end > end)
            out.push_back({end, b_end - end, b.status});
    }
    /* insert the marked span in order (blocks_ was sorted; the carve kept
     * order, the new span fits before the first block starting >= end) */
    size_t at = 0;
    while (at < out.size() && out[at].pos < pos)
        ++at;
    out.insert(out.begin() + std::ptrdiff_t(at), {pos, size, status});
    blocks_ = std::move(out);
    merge_adjacent_();
    return true;
}

void RescueMap::merge_adjacent_() {
    if (blocks_.empty())
        return;
    std::vector<RescueBlock> out;
    out.reserve(blocks_.size());
    out.push_back(blocks_.front());
    for (size_t i = 1; i < blocks_.size(); ++i) {
        RescueBlock& last = out.back();
        const RescueBlock& b = blocks_[i];
        if (last.status == b.status && last.pos + last.size == b.pos) {
            last.size += b.size;
        } else {
            out.push_back(b);
        }
    }
    blocks_ = std::move(out);
}

bool RescueMap::find_next(u64 from, const RescueStatus* statuses, size_t n_statuses, u64* pos,
                          u64* size) const {
    if (from >= total_)
        return false;
    for (const RescueBlock& b : blocks_) {
        if (b.pos + b.size <= from)
            continue;
        for (size_t k = 0; k < n_statuses; ++k) {
            if (b.status == statuses[k]) {
                if (pos)
                    *pos = std::max(b.pos, from);
                if (size)
                    *size = b.pos + b.size - std::max(b.pos, from);
                return true;
            }
        }
    }
    return false;
}

bool RescueMap::find_prev(u64 before, const RescueStatus* statuses, size_t n_statuses, u64* pos,
                          u64* size) const {
    if (before == 0)
        return false;
    for (auto it = blocks_.rbegin(); it != blocks_.rend(); ++it) {
        if (it->pos >= before)
            continue;
        for (size_t k = 0; k < n_statuses; ++k) {
            if (it->status == statuses[k]) {
                const u64 end = std::min(it->pos + it->size, before);
                if (pos)
                    *pos = it->pos;
                if (size)
                    *size = end - it->pos;
                return true;
            }
        }
    }
    return false;
}

u64 RescueMap::bytes_of(RescueStatus s) const {
    u64 sum = 0;
    for (const RescueBlock& b : blocks_)
        if (b.status == s)
            sum += b.size;
    return sum;
}

bool RescueMap::all_finished() const {
    if (total_ == 0)
        return true;
    return blocks_.size() == 1 && blocks_[0].status == RescueStatus::Finished &&
           blocks_[0].pos == 0 && blocks_[0].size == total_;
}

std::string RescueMap::to_mapfile_text() const {
    /* ddrescue-compatible: comments, the current-position line, then one
     * line per block "0x%08X  0x%08X  status". ASCII, deterministic. */
    char line[96];
    std::string out;
    out.reserve(blocks_.size() * 40 + 128);
    out += "# rescue mapfile (ddrescue-compatible; F-33/F-34)\n";
    out += "# pos size status per block; '?' non-tried '*' non-trimmed '/'\n";
    out += "# non-scraped '-' bad '+' finished\n";
    std::snprintf(line, sizeof(line), "0x%llX  %c\n", static_cast<unsigned long long>(current_pos_),
                  rescue_status_char(current_status_));
    out += line;
    for (const RescueBlock& b : blocks_) {
        std::snprintf(line, sizeof(line), "0x%08llX  0x%08llX  %c\n",
                      static_cast<unsigned long long>(b.pos),
                      static_cast<unsigned long long>(b.size), rescue_status_char(b.status));
        out += line;
    }
    return out;
}

bool RescueMap::parse(const std::string& text, RescueMap* out) {
    if (out == nullptr)
        return false;
    RescueMap m;
    bool have_current = false;
    bool saw_block = false;
    u64 prev_end = 0;
    std::vector<RescueBlock> blocks;
    const char* p = text.c_str();
    while (*p != '\0') {
        /* skip comments and blank lines */
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
            ++p;
        if (*p == '\0')
            break;
        if (*p == '#') {
            while (*p != '\0' && *p != '\n')
                ++p;
            continue;
        }
        /* parse one line: either "0xPOS  STATUS" (current) or
         * "0xPOS  0xSIZE  STATUS" (block) */
        char* endp = nullptr;
        const unsigned long long v1 = std::strtoull(p, &endp, 0);
        if (endp == p)
            return false; /* not a number */
        p = endp;
        while (*p == ' ' || *p == '\t')
            ++p;
        if (*p == '\0' || *p == '\r' || *p == '\n') {
            /* single-number line: not part of the documented format */
            return false;
        }
        if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
            /* second number: a block line */
            char* endp2 = nullptr;
            const unsigned long long v2 = std::strtoull(p, &endp2, 0);
            if (endp2 == p)
                return false;
            p = endp2;
            while (*p == ' ' || *p == '\t')
                ++p;
            RescueStatus st;
            if (*p == '\0' || !rescue_status_from_char(*p, &st))
                return false;
            ++p;
            while (*p != '\0' && *p != '\n')
                ++p; /* rest of line tolerated */
            if (v2 == 0)
                return false; /* zero-size block */
            if (v1 != prev_end)
                return false; /* out of order or a hole/overlap */
            prev_end = v1 + v2;
            blocks.push_back({v1, v2, st});
            saw_block = true;
        } else {
            /* status char directly: the current-position line */
            RescueStatus st;
            if (!rescue_status_from_char(*p, &st))
                return false;
            ++p;
            while (*p != '\0' && *p != '\n')
                ++p;
            if (have_current)
                return false; /* duplicate current line */
            m.current_pos_ = v1;
            m.current_status_ = st;
            have_current = true;
        }
    }
    m.total_ = prev_end;
    m.blocks_ = std::move(blocks);
    if (m.total_ == 0 && saw_block)
        return false;
    if (m.current_pos_ > m.total_)
        return false;
    *out = std::move(m);
    return true;
}

/* ---- RescueImager ----------------------------------------------------------- */

RescueImager::RescueImager(IStorageDevice& src, RescueImageSink& sink, RescueMapIo& map_io,
                           RescueImagerConfig cfg)
    : src_(src), sink_(sink), map_io_(map_io), cfg_(cfg) {
    sector_ = src_.geometry().logical_sector;
    if (sector_ == 0)
        sector_ = 512;
    if (cfg_.initial_chunk == 0)
        cfg_.initial_chunk = 1 << 20;
    if (cfg_.skip_size == 0)
        cfg_.skip_size = 1 << 20;
    if (cfg_.max_retries == 0)
        cfg_.max_retries = 3;
    buf_.resize(size_t(std::max<u64>(512, std::min<u64>(cfg_.initial_chunk, 16u << 20))));
}

RescueStats RescueImager::stats_() const {
    RescueStats s;
    s.rescued_bytes = map_.bytes_of(RescueStatus::Finished);
    s.bad_bytes = map_.bytes_of(RescueStatus::BadSector);
    s.pending_bytes = map_.bytes_of(RescueStatus::NonTried) +
                      map_.bytes_of(RescueStatus::NonTrimmed) +
                      map_.bytes_of(RescueStatus::NonScraped);
    s.current_pos = map_.current_pos();
    s.cancelled = cancelled_;
    return s;
}

void RescueImager::emit_progress_(RescueProgressFn progress, void* user) {
    if (progress != nullptr) {
        RescueStats s = stats_();
        progress(user, s);
    }
}

bool RescueImager::checkpoint_(bool forced, u32* rc_out) {
    /* dest flush BEFORE map save: the map never claims bytes the image
     * does not hold. */
    const u32 frc = sink_.flush();
    if (frc != OK) {
        if (forced) {
            if (rc_out)
                *rc_out = frc;
            return false;
        }
        /* periodic: swallowed (an AV/indexer lock aborting a live rescue
         * would be far worse); the next checkpoint retries */
    }
    const u32 wrc = map_io_.write_map(map_.to_mapfile_text(), forced);
    if (wrc != OK) {
        if (forced) {
            if (rc_out)
                *rc_out = wrc;
            return false;
        }
    }
    return true;
}

u32 RescueImager::run(RescueMap* preloaded, CancelToken* cancel, RescueProgressFn progress,
                      void* progress_user, RescueStats* stats) {
    if (preloaded != nullptr) {
        if (preloaded->total() != src_.size()) {
            /* wrong map for this source: refuse, never a silent mismatch */
            if (stats)
                *stats = stats_();
            return ERR_BAD_PARAMETER;
        }
        map_ = *preloaded;
    } else {
        map_.reset(src_.size());
        /* fresh runs persist the initial all-NonTried map immediately (a
         * crash before the first checkpoint still leaves a loadable map) */
        u32 rc = OK;
        if (!checkpoint_(true, &rc)) {
            if (stats)
                *stats = stats_();
            return rc;
        }
    }

    /* pass 1: copy (forward or reverse) */
    u32 rc = run_copy_pass_(cfg_.reverse, cancel, progress, progress_user);
    if (rc != OK) {
        if (stats)
            *stats = stats_();
        /* cancel/failure: the map is checkpointed so the run is resumable */
        u32 crc = OK;
        checkpoint_(true, &crc);
        return rc;
    }
    /* pass boundary: forced checkpoint */
    rc = OK;
    if (!checkpoint_(true, &rc)) {
        if (stats)
            *stats = stats_();
        return rc;
    }

    /* pass 2: retry (split damaged regions toward sector granularity) */
    rc = run_retry_pass_(cancel, progress, progress_user);
    if (rc != OK) {
        if (stats)
            *stats = stats_();
        u32 crc = OK;
        checkpoint_(true, &crc);
        return rc;
    }
    /* final forced checkpoint (the completion certificate) */
    rc = OK;
    if (!checkpoint_(true, &rc)) {
        if (stats)
            *stats = stats_();
        return rc;
    }
    if (stats)
        *stats = stats_();
    return OK;
}

u32 RescueImager::run_copy_pass_(bool reverse, CancelToken* cancel, RescueProgressFn progress,
                                 void* progress_user) {
    const RescueStatus wanted[1] = {RescueStatus::NonTried};
    if (map_.total() == 0)
        return OK; /* the empty-device parity */
    if (!have_checkpoint_clock_) {
        last_checkpoint_ = std::chrono::steady_clock::now();
        have_checkpoint_clock_ = true;
    }

    if (!reverse) {
        u64 pos = map_.current_pos() - map_.current_pos() % sector_; /* align down */
        for (;;) {
            if (cancel != nullptr && cancel->cancelled()) {
                cancelled_ = true;
                return ERR_CANCELLED;
            }
            u64 avail = 0, at = 0;
            if (!map_.find_next(pos, wanted, 1, &at, &avail)) {
                /* a resumed run (or one that last ran in reverse) picks
                 * up untried areas BEFORE current_pos */
                if (!map_.find_next(0, wanted, 1, &at, &avail))
                    break; /* nothing left */
            }
            const u64 chunk_len = rescue_align_down_min1(
                std::min(std::min(cfg_.initial_chunk, avail), u64(buf_.size())), sector_, avail);
            const u32 rrc = src_.read_at(at, buf_.data(), u32(chunk_len));
            if (rrc == OK) {
                const u32 wrc = sink_.pwrite(at, buf_.data(), u32(chunk_len));
                if (wrc != OK)
                    return wrc;
                map_.mark(at, chunk_len, RescueStatus::Finished);
            } else {
                /* skip past the damage: one bad spot cannot stall the
                 * rescue of everything after it */
                const u64 skip = std::min(
                    rescue_align_down_min1(std::max(cfg_.skip_size, chunk_len), sector_, u64(-1)),
                    avail);
                map_.mark(at, skip, RescueStatus::NonTrimmed);
            }
            map_.set_current_pos(at + chunk_len);
            emit_progress_(progress, progress_user);
            /* periodic checkpoint (<= interval) */
            const auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_checkpoint_)
                    .count() >= std::ptrdiff_t(cfg_.checkpoint_interval_ms)) {
                u32 crc = OK;
                checkpoint_(false, &crc); /* periodic: swallowed on failure */
                last_checkpoint_ = now;
            }
            pos = at + chunk_len;
        }
        return OK;
    }

    /* reverse: mirrors with find_prev - each found region is read from
     * its TAIL ("reading a damaged area from the other side often gets
     * more of it"); current_pos keeps the LOWEST point reached (the
     * fresh-map convention 0). */
    u64 before = map_.total();
    if (map_.current_pos() > 0)
        before = map_.current_pos();
    for (;;) {
        if (cancel != nullptr && cancel->cancelled()) {
            cancelled_ = true;
            return ERR_CANCELLED;
        }
        u64 avail = 0, at = 0;
        if (!map_.find_prev(before, wanted, 1, &at, &avail))
            break;
        const u64 block_end = at + avail;
        const u64 chunk_len = rescue_align_down_min1(
            std::min(std::min(cfg_.initial_chunk, avail), u64(buf_.size())), sector_, avail);
        const u64 read_at = block_end - chunk_len; /* the region's tail */
        const u32 rrc = src_.read_at(read_at, buf_.data(), u32(chunk_len));
        if (rrc == OK) {
            const u32 wrc = sink_.pwrite(read_at, buf_.data(), u32(chunk_len));
            if (wrc != OK)
                return wrc;
            map_.mark(read_at, chunk_len, RescueStatus::Finished);
        } else {
            const u64 skip = std::min(
                rescue_align_down_min1(std::max(cfg_.skip_size, chunk_len), sector_, u64(-1)),
                avail);
            map_.mark(read_at, skip, RescueStatus::NonTrimmed);
        }
        map_.set_current_pos(read_at);
        emit_progress_(progress, progress_user);
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_checkpoint_).count() >=
            std::ptrdiff_t(cfg_.checkpoint_interval_ms)) {
            u32 crc = OK;
            checkpoint_(false, &crc);
            last_checkpoint_ = now;
        }
        before = read_at;
    }
    return OK;
}

u32 RescueImager::run_retry_pass_(CancelToken* cancel, RescueProgressFn progress,
                                  void* progress_user) {
    const RescueStatus wanted[2] = {RescueStatus::NonTrimmed, RescueStatus::NonScraped};
    for (;;) {
        if (cancel != nullptr && cancel->cancelled()) {
            cancelled_ = true;
            return ERR_CANCELLED;
        }
        u64 at = 0, len = 0;
        if (!map_.find_next(0, wanted, 2, &at, &len))
            break; /* every damaged region resolved */
        const u32 rc = split_and_retry_(at, len, cancel, progress, progress_user);
        if (rc != OK)
            return rc;
    }
    return OK;
}

u32 RescueImager::split_and_retry_(u64 pos, u64 len, CancelToken* cancel, RescueProgressFn progress,
                                   void* progress_user) {
    if (cancel != nullptr && cancel->cancelled())
        return ERR_CANCELLED;
    if (len == 0)
        return OK;

    /* whole-region read succeeds -> Finished. The attempt is clamped to
     * the buffer capacity; a larger region simply comes back around the
     * outer loop (or splits below) - never an unbounded read. */
    const u64 try_len = rescue_align_down_min1(std::min(len, u64(buf_.size())), sector_, len);
    const u32 rrc = src_.read_at(pos, buf_.data(), u32(try_len));
    if (rrc == OK) {
        const u32 wrc = sink_.pwrite(pos, buf_.data(), u32(try_len));
        if (wrc != OK)
            return wrc;
        map_.mark(pos, try_len, RescueStatus::Finished);
        emit_progress_(progress, progress_user);
        return OK;
    }
    if (try_len <= sector_) {
        /* sector-granular: retry up to max_retries before BadSector */
        for (u32 attempt = 1; attempt < cfg_.max_retries; ++attempt) {
            if (cancel != nullptr && cancel->cancelled())
                return ERR_CANCELLED;
            if (src_.read_at(pos, buf_.data(), u32(try_len)) == OK) {
                const u32 wrc = sink_.pwrite(pos, buf_.data(), u32(try_len));
                if (wrc != OK)
                    return wrc;
                map_.mark(pos, try_len, RescueStatus::Finished);
                emit_progress_(progress, progress_user);
                return OK;
            }
        }
        map_.mark(pos, try_len, RescueStatus::BadSector);
        emit_progress_(progress, progress_user);
        return OK;
    }
    /* larger: mark NonScraped and split into sector-aligned halves (each
     * at least one sector); checkpoint + progress fire INSIDE the damaged
     * region - each failed read on a dying drive can take seconds */
    map_.mark(pos, try_len, RescueStatus::NonScraped);
    emit_progress_(progress, progress_user);
    u32 crc = OK;
    checkpoint_(false, &crc);
    last_checkpoint_ = std::chrono::steady_clock::now();

    const u64 half = rescue_align_down_min1(try_len / 2, sector_, try_len - sector_);
    u32 rc = split_and_retry_(pos, half, cancel, progress, progress_user);
    if (rc != OK)
        return rc;
    return split_and_retry_(pos + half, try_len - half, cancel, progress, progress_user);
}

} // namespace storage
} // namespace cr
