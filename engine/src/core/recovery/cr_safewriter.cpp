/* cr_safewriter.cpp - recovery queue + policy (R7).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/recovery/cr_safewriter.h"
#include "cr/tree/cr_snapshot.h"
#include "cr/compress/cr_decomp.h"
#include <chrono>
#include <cstring>

namespace cr {
namespace recovery {

namespace {
bool intersects(u64 a_start, u64 a_len, u64 b_start, u64 b_len) {
    return a_start < b_start + b_len && b_start < a_start + a_len;
}
} // namespace

SafeWriter::SafeWriter(DestIo& io, IStorageDevice* source, WriterConfig cfg)
    : io_(io), source_(source), cfg_(std::move(cfg)) {}

void SafeWriter::set_destination(u64 dest_disk_start, u64 dest_disk_bytes) {
    dest_start_ = dest_disk_start;
    dest_bytes_ = dest_disk_bytes;
    have_dest_ = true;
}

std::u16string SafeWriter::sanitize_name(const std::u16string& in) {
    /* corpus rule: / * : ? " < > | replaced with _ (never empty; '.' refs
     * current dir - prefix underscore) */
    std::u16string out;
    out.reserve(in.size());
    for (u16 c : in) {
        bool bad = c == u'/' || c == u'*' || c == u':' || c == u'?' || c == u'"' || c == u'<' ||
                   c == u'>' || c == u'|';
        out.push_back(bad ? u'_' : c);
    }
    if (out.empty())
        out = u"_";
    if (out == u"." || out == u"..")
        out = u"_" + out;
    return out;
}

std::u16string SafeWriter::resolve_collision(const std::u16string& dir,
                                             const std::u16string& base_name) const {
    std::u16string prefix = dir;
    if (!prefix.empty() && prefix.back() != u'\\' && prefix.back() != u'/')
        prefix.push_back(u'\\');
    std::u16string cand = base_name;
    for (u32 attempt = 1; attempt < cfg_.rename_attempts; ++attempt) {
        const std::u16string key = prefix + u"|" + cand;
        const bool intra = claimed_names_.count(key) != 0;
        /* pre-existing destination finals only conflict under Rename
         * (corpus R7.1); Skip/Overwrite keep the name so the recover pass
         * can detect + skip (idempotent) or replace the final */
        const bool dest_taken =
            cfg_.conflict == ConflictPolicy::Rename && io_.exists(prefix + cand);
        if (!intra && !dest_taken) {
            claimed_names_.insert(key);
            return cand;
        }
        char buf[16];
        snprintf(buf, sizeof(buf), " (%u)", attempt + 1);
        std::u16string suffix;
        for (const char* p = buf; *p; ++p)
            suffix.push_back(u16(u8(*p)));
        /* insert before the LAST dot (keep the extension) */
        size_t last_dot = base_name.rfind(u'.');
        if (last_dot == std::u16string::npos || last_dot == 0) {
            cand = base_name + suffix;
        } else {
            cand = base_name.substr(0, last_dot) + suffix + base_name.substr(last_dot);
        }
    }
    return base_name + u"_dup";
}

u32 SafeWriter::enqueue(tree::FileNode* node, const std::u16string& dest_dir) {
    if (node == nullptr || node->props() == nullptr || source_ == nullptr)
        return ERR_BAD_PARAMETER;
    return enqueue_common(node->props(), node, nullptr, dest_dir);
}

u32 SafeWriter::enqueue(const tree::Snapshot& snap, u32 snap_index,
                        const std::u16string& dest_dir) {
    /* G-02b: snapshot rows materialize into writer-owned props; the live
     * tree is never touched (mid-scan recovery is race-free by
     * construction - every byte the writer reads is snapshot-owned or
     * device-served). */
    if (source_ == nullptr || snap_index >= snap.node_count())
        return ERR_BAD_PARAMETER;
    std::unique_ptr<tree::FileProperties> owned(snap.materialize_props(snap_index));
    if (!owned)
        return ERR_NO_MEMORY;
    value_store_.push_back(std::move(owned));
    const tree::FileProperties* value = value_store_.back().get();
    return enqueue_common(value, nullptr, value, dest_dir);
}

u32 SafeWriter::enqueue_common(const tree::FileProperties* pr, tree::FileNode* node,
                               const tree::FileProperties* value_props,
                               const std::u16string& dest_dir) {
    if (pr == nullptr)
        return ERR_BAD_PARAMETER;
    const bool is_dir = cfg_.reconstruct_dirs && (pr->attributes & 0x10) != 0;

    /* R7.2 / F12: hard refusal on source/destination overlap. The extents
     * are ABSOLUTE device offsets; the destination extent is the
     * destination volume's disk placement. Directory nodes carry no
     * extents - never refused (G-08). */
    if (!is_dir && have_dest_ && dest_bytes_ > 0) {
        for (const auto& e : pr->extents) {
            if (intersects(e.offset, e.length, dest_start_, dest_bytes_)) {
                QueueItem refused;
                refused.node = node;
                refused.value_props = value_props;
                refused.dest_dir = dest_dir;
                refused.item_id = next_id_++;
                refused.state = ItemState::Refused;
                refused.error = ERR_DEST_OVERLAP;
                queue_.push_back(refused);
                return ERR_DEST_OVERLAP;
            }
        }
    }

    QueueItem it;
    it.node = node;
    it.value_props = value_props; /* G-02b: null for node-based enqueues */
    it.dest_dir = dest_dir;
    if (!it.dest_dir.empty() && it.dest_dir.back() != u'\\' && it.dest_dir.back() != u'/')
        it.dest_dir.push_back(u'\\');
    const std::u16string clean = sanitize_name(pr->name);
    it.final_name = resolve_collision(it.dest_dir, clean);
    it.item_id = next_id_++;
    it.is_dir = is_dir;
    if (is_dir) {
        /* G-08 structure item: no content, no temp file - the recover pass
         * materializes the directory (mkdir) and stamps it Done. */
        it.bytes_total = 0;
        it.state = ItemState::Pending;
        queue_.push_back(it);
        return OK;
    }
    /* GAP-S6-7 (bench 40073093997 permanent_delete: 991 zero-filled
     * 'salvaged' false positives vs manifest): a file node claiming a
     * POSITIVE size with NO extents and NO resident data is a metadata-
     * only reconstruction - the writer must not commit a full-size zero
     * file for it (the zero-fill Skip policy exists for PARTIAL spans of
     * real files). The og emits no content for metadata-only entries.
     * Empty files (size 0) are real content and keep the normal path. */
    if (pr->logical_size > 0 && pr->extents.empty() && pr->resident_data.empty()) {
        it.nothing_to_recover = true;
        it.bytes_total = 0; /* nothing attempted - the report stays truthful */
        it.state = ItemState::Done; /* counted at process(); never Pending */
        queue_.push_back(it);
        return OK;
    }
    char suf[32];
    snprintf(suf, sizeof(suf), ".part-%llu", (unsigned long long)it.item_id);
    it.temp_suffix.clear();
    for (const char* p = suf; *p; ++p)
        it.temp_suffix.push_back(u16(u8(*p)));
    it.bytes_total = pr->logical_size;
    it.state = ItemState::Pending;
    queue_.push_back(it);
    return OK;
}

u32 SafeWriter::ensure_dir_(const std::u16string& dir) {
    /* one mkdir per unique directory per run (the DestIo implementation
     * creates parents recursively) */
    if (dir.empty() || made_dirs_.count(dir))
        return OK;
    if (u32 rc = io_.mkdir(dir))
        return rc;
    made_dirs_.insert(dir);
    return OK;
}

u32 SafeWriter::recover_item(QueueItem& it, CancelToken* cancel, WriterStats& stats) {
    /* G-09: per-item duration for EVERY exit path (report evidence) */
    const auto t0 = std::chrono::steady_clock::now();
    const u32 rc = recover_item_impl_(it, cancel, stats);
    it.duration_ms = u64(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0)
            .count());
    return rc;
}

u32 SafeWriter::recover_item_impl_(QueueItem& it, CancelToken* cancel, WriterStats& stats) {
    /* G-02b: node-based items read the live props; snapshot items read
     * the writer-owned value - exactly one of the two is non-null. */
    const tree::FileProperties* pr = it.node ? it.node->props() : it.value_props;
    if (pr == nullptr)
        return ERR_BAD_PARAMETER;
    if (it.is_dir) {
        /* G-08: materialize the directory itself (idempotent - an existing
         * directory is success), so empty leaves survive reconstruction */
        if (u32 rc = ensure_dir_(it.dest_dir + it.final_name)) {
            it.state = ItemState::Error;
            it.error = rc;
            ++stats.errors;
            return rc;
        }
        it.state = ItemState::Done;
        ++stats.dirs;
        return OK;
    }
    if (pr->extents.empty() && pr->resident_data.empty()) {
        /* metadata-only: nothing to read; an empty marker file still helps
         * the user see the name - write empty */
    }
    /* G-08: the destination directory chain must exist before the temp
     * file can be created in it */
    if (cfg_.reconstruct_dirs) {
        if (u32 rc = ensure_dir_(it.dest_dir)) {
            it.state = ItemState::Error;
            it.error = rc;
            ++stats.errors;
            return rc;
        }
    }
    /* resume detection (audit row 35a): a final file that already exists
     * is counted and skipped - under ConflictPolicy::Skip this makes
     * re-running a recovery idempotent (the name is kept, so the second
     * run lands HERE instead of colliding into "(2)" copies); under
     * Rename the fresh name should not exist (kept for the race window) */
    if (cfg_.conflict != ConflictPolicy::Overwrite && io_.exists(it.dest_dir + it.final_name)) {
        it.state = ItemState::Done;
        it.resumed = true; /* G-09: per-item evidence (stats-level below) */
        ++stats.resumed;
        return OK;
    }
    it.state = ItemState::Reading;
    if (u32 rc = io_.create_temp(it.item_id, it.dest_dir + it.final_name, it.temp_suffix)) {
        it.state = ItemState::Error;
        it.error = rc;
        ++stats.errors;
        return rc;
    }
    it.state = ItemState::Writing;

    crypto::Sha256 hash;
    /* resident metadata content first (NTFS resident streams, ZFS embedded
     * block-pointer payloads, Btrfs inline extents), then disk extents */
    u64 emit = 0;
    if (!pr->resident_data.empty()) {
        emit = pr->resident_data.size() > pr->logical_size && pr->logical_size != 0
                   ? pr->logical_size
                   : u64(pr->resident_data.size());
        if (emit) {
            if (u32 rc = io_.write(it.item_id, 0, pr->resident_data.data(), u32(emit))) {
                io_.close(it.item_id);
                it.state = ItemState::Error;
                it.error = rc;
                ++stats.errors;
                return rc;
            }
            hash.update(pr->resident_data.data(), u32(emit));
        }
    }
    std::vector<u8> chunk;
    chunk.resize(size_t(cfg_.read_chunk));
    /* the resident prefix is already on disk (and hashed); continue the
     * stream AFTER it - extents carry the post-resident content, and a
     * resident-only file zero-fills only the tail beyond the payload */
    u64 done = emit;
    it.bytes_done = done;
    const u64 total = pr->logical_size;
    size_t ext_i = 0;
    u64 ext_pos = 0;
    while (done < total) {
        if (cancel && cancel->checkpoint()) {
            io_.close(it.item_id);
            it.state = ItemState::Error;
            it.error = ERR_CANCELLED;
            return ERR_CANCELLED;
        }
        u64 want = total - done;
        if (want > cfg_.read_chunk)
            want = cfg_.read_chunk;
        /* locate (extent, offset) for `done` */
        while (ext_i < pr->extents.size() && ext_pos + pr->extents[ext_i].length <= done) {
            ext_pos += pr->extents[ext_i].length;
            ++ext_i;
        }
        if (ext_i >= pr->extents.size()) {
            if (!pr->resident_data.empty()) {
                /* resident-only file: logical size may exceed the payload
                 * (padded); zero-fill the remainder */
                std::memset(chunk.data(), 0, size_t(want));
                if (u32 rc = io_.write(it.item_id, done, chunk.data(), u32(want))) {
                    io_.close(it.item_id);
                    it.state = ItemState::Error;
                    it.error = rc;
                    ++stats.errors;
                    return rc;
                }
                hash.update(chunk.data(), u32(want));
                done += want;
                it.bytes_done = done;
                continue;
            }
            io_.close(it.item_id);
            it.state = ItemState::Error;
            it.error = ERR_TRUNCATED;
            ++stats.errors;
            return ERR_TRUNCATED;
        }
        const u64 in_ext = done - ext_pos;
        u64 avail_in_ext = pr->extents[ext_i].length - in_ext;
        if (want > avail_in_ext)
            want = avail_in_ext;
        if (want > chunk.size())
            want = chunk.size();
        const u32 got = u32(want);
        if (pr->extents[ext_i].codec != 0) {
            /* compressed extent: decode the whole block on demand; the
             * logical bytes come from the decoded image, not the disk */
            const u64 psize =
                pr->extents[ext_i].psize ? pr->extents[ext_i].psize : pr->extents[ext_i].length;
            const u64 lsize = pr->extents[ext_i].length;
            if (psize == 0 || lsize == 0 || psize > (256ull << 20) || lsize > (256ull << 20)) {
                io_.close(it.item_id);
                it.state = ItemState::Error;
                it.error = ERR_BAD_PARAMETER;
                ++stats.errors;
                return ERR_BAD_PARAMETER;
            }
            std::vector<u8> packed(static_cast<size_t>(psize), 0);
            std::vector<u8> plain(static_cast<size_t>(lsize), 0);
            if (source_->read_at(pr->extents[ext_i].offset, packed.data(), u32(psize)) != OK) {
                std::memset(packed.data(), 0, size_t(psize)); /* Skip policy */
                std::memset(plain.data(), 0, size_t(lsize));
                it.had_unreadable = true; /* FB-R02: zero-filled span */
            } else if (!compress::decompress_block(pr->extents[ext_i].codec, packed.data(),
                                                   size_t(psize), plain.data(), size_t(lsize))) {
                /* malformed compressed block: zero-fill (hash will
                 * mismatch) - never silently emit wrong bytes */
                std::memset(plain.data(), 0, size_t(lsize));
                it.had_unreadable = true; /* FB-R02 */
            }
            const u64 have = lsize - in_ext;
            const u32 take = u32(have < u64(got) ? have : u64(got));
            if (take) {
                if (u32 rc = io_.write(it.item_id, done, plain.data() + in_ext, take)) {
                    io_.close(it.item_id);
                    it.state = ItemState::Error;
                    it.error = rc;
                    ++stats.errors;
                    return rc;
                }
                hash.update(plain.data() + in_ext, take);
            }
            done += take;
            it.bytes_done = done;
            if (take < got)
                ++ext_i, ext_pos += lsize; /* consumed this extent */
            continue;
        }
        const u32 got_raw = u32(want);
        if (source_->read_at(pr->extents[ext_i].offset + in_ext, chunk.data(), got_raw) != OK) {
            /* zero-fill the unreadable span (Skip policy) and continue -
             * the hash will intentionally mismatch on damaged content */
            std::memset(chunk.data(), 0, got_raw);
            it.had_unreadable = true; /* FB-R02: committed-but-partial */
        }
        if (u32 rc = io_.write(it.item_id, done, chunk.data(), got_raw)) {
            io_.close(it.item_id);
            it.state = ItemState::Error;
            it.error = rc;
            ++stats.errors;
            return rc;
        }
        hash.update(chunk.data(), got_raw);
        done += got_raw;
        it.bytes_done = done;
    }

    it.state = ItemState::Verifying;
    /* G-09: the stream hash is ALWAYS finalized and captured (the report
     * carries computed hashes even when no stored hash exists to verify
     * against - hash_status tells the truth about which happened) */
    u8 digest[32];
    hash.final(digest);
    {
        static const char* hexd = "0123456789abcdef";
        char hex[65];
        for (int i = 0; i < 32; ++i) {
            hex[i * 2] = hexd[digest[i] >> 4];
            hex[i * 2 + 1] = hexd[digest[i] & 0xF];
        }
        hex[64] = 0;
        it.sha256_hex.assign(hex);
    }
    if (cfg_.verify_hash && pr->hash_valid) {
        if (std::memcmp(digest, pr->sha128, 16) != 0) {
            ++stats.hash_mismatch;
            it.error = ERR_HASH_MISMATCH;
            it.hash_status = report::HashStatus::Mismatch;
            /* the content is still committed (salvage); the mismatch is
             * reported per-item and aggregated */
        } else {
            it.hash_verified = true;
            it.hash_status = report::HashStatus::Verified;
        }
    } else {
        /* no stored hash to compare against: the streamed digest is
         * reported as computed, never claimed as verified */
        it.hash_status = report::HashStatus::Computed;
    }
    if (u32 rc = io_.commit(it.item_id)) {
        io_.close(it.item_id);
        it.state = ItemState::Error;
        it.error = rc;
        ++stats.errors;
        return rc;
    }
    io_.close(it.item_id);
    it.state = ItemState::Done;
    ++stats.done;
    if (it.had_unreadable)
        ++stats.salvaged; /* committed sub-bucket: partial-content commits
                             visible in the report (FB-R02) */
    return OK;
}

u32 SafeWriter::process(CancelToken* cancel, bool (*progress)(u64, u64, void*), void* progress_user,
                        WriterStats& stats) {
    u64 total_bytes = 0;
    u64 done_bytes = 0;
    for (const auto& it : queue_)
        total_bytes += it.bytes_total;
    stats.queued = queue_.size(); /* stamped upfront: an aborted pass
                                    still reports the true queue size */
    for (auto& it : queue_) {
        if (it.state == ItemState::Refused) {
            ++stats.refused;
            continue;
        }
        if (it.nothing_to_recover) {
            /* GAP-S6-7: counted, reported, never written - zero bytes of
             * I/O (process() short-circuit; the report row tells the truth) */
            ++stats.nothing_to_recover;
            continue;
        }
        if (it.state == ItemState::Done) {
            done_bytes += it.bytes_total;
            continue;
        }
        const u32 rc = recover_item(it, cancel, stats);
        /* WRITER-1 (AUDIT-PIPELINE, F-A4-8): per-item failures are
         * COLLECTED, never fatal - the og's ExportThread keeps going and
         * reports "%1 files/%2 failed; for details, you may check the
         * recovery log" at the end (04-recovery-workflow §7.5). One
         * unreadable file must not cancel the remaining ten thousand.
         * recover_item_impl_ already stamps ItemState::Error + the error
         * code + stats.errors on every failure path; cancellation is the
         * only hard abort. */
        if (rc == ERR_CANCELLED)
            return ERR_CANCELLED;
        done_bytes += it.bytes_total;
        if (progress && !progress(done_bytes, total_bytes ? total_bytes : 1, progress_user))
            return ERR_CANCELLED;
    }
    return OK;
}

/* ---- G-09 report material ---- */

namespace {
/* utf16 -> utf8 (the report path must not depend on tree internals;
 * lone surrogates pass through as their code value - the report escaper
 * keeps the document valid JSON regardless) */
std::string utf16_to_utf8_local(const std::u16string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        u32 cp = in[i];
        if (in[i] >= 0xD800 && in[i] <= 0xDBFF && i + 1 < in.size() && in[i + 1] >= 0xDC00 &&
            in[i + 1] <= 0xDFFF) {
            cp = 0x10000u + ((u32(in[i]) - 0xD800u) << 10) + (u32(in[i + 1]) - 0xDC00u);
            ++i;
        }
        if (cp < 0x80) {
            out.push_back(char(cp));
        } else if (cp < 0x800) {
            out.push_back(char(0xC0 | (cp >> 6)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(char(0xE0 | (cp >> 12)));
            out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(char(0xF0 | (cp >> 18)));
            out.push_back(char(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}
} // namespace

report::Totals SafeWriter::report_totals(const std::string& source, const std::string& destination,
                                         u64 elapsed_ms, const WriterStats& stats) const {
    report::Totals t;
    t.queued = stats.queued;
    t.done = stats.done;
    t.dirs = stats.dirs;
    t.refused = stats.refused;
    t.errors = stats.errors;
    t.hash_mismatch = stats.hash_mismatch;
    t.resumed = stats.resumed;
    t.salvaged = stats.salvaged;
    t.nothing_to_recover = stats.nothing_to_recover;
    {
        /* partial-run truth: items still Pending at report time were
         * queued but never attempted - without this count, queued cannot
         * be reconciled after an aborted pass */
        u64 pend = 0;
        for (const auto& it : queue_)
            if (it.state == ItemState::Pending)
                ++pend;
        t.pending = pend;
    }
    u64 bytes = 0;
    for (const auto& it : queue_)
        if (!it.is_dir && it.state == ItemState::Done && !it.resumed)
            bytes += it.bytes_total;
    t.bytes = bytes;
    t.elapsed_ms = elapsed_ms;
    t.source = source;
    t.destination = destination;
    return t;
}

std::vector<report::Item> SafeWriter::report_items() const {
    std::vector<report::Item> out;
    out.reserve(queue_.size());
    for (const auto& it : queue_) {
        report::Item r;
        r.item_id = it.item_id;
        r.path = utf16_to_utf8_local(it.dest_dir + it.final_name);
        r.name = utf16_to_utf8_local(it.final_name);
        r.is_dir = it.is_dir;
        r.bytes = it.bytes_total;
        r.duration_ms = it.duration_ms;
        r.sha256_hex = it.sha256_hex;
        r.hash_status = it.hash_status;
        if (it.state == ItemState::Refused) {
            r.outcome = report::Outcome::Refused;
            r.reason = it.error;
        } else if (it.nothing_to_recover) {
            /* GAP-S6-7: skipped before any write - metadata-only item */
            r.outcome = report::Outcome::NothingToRecover;
        } else if (it.state == ItemState::Error) {
            r.outcome = report::Outcome::Error;
            r.reason = it.error;
        } else if (it.resumed) {
            r.outcome = report::Outcome::Resumed;
        } else if (it.state == ItemState::Pending) {
            /* the pass aborted before this item - never claimed as done */
            r.outcome = report::Outcome::Pending;
        } else if (it.had_unreadable) {
            /* FB-R02: committed, but some spans were zero-filled - the
             * commit is real, the content is not complete; surfaced as
             * Salvaged (never silently Done) */
            r.outcome = report::Outcome::Salvaged;
        } else if (it.error == ERR_HASH_MISMATCH) {
            r.outcome = report::Outcome::HashMismatch;
            r.reason = it.error;
        } else {
            r.outcome = report::Outcome::Done;
        }
        out.push_back(std::move(r));
    }
    return out;
}

} // namespace recovery
} // namespace cr
