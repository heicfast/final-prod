/* cr_carve.cpp - carving pipeline (R4).
 *
 * Scan: offset-outer with a first-byte dispatch; per-hit formats are tried
 * in specificity order (DOCX before ZIP, CR2 before TIFF) and, at a
 * position where several magics coincide, the highest accumulated-score
 * accepting candidate is emitted (N-070: the og ranks overlapping
 * candidates by accumulated header+tail confidence, searchdll-family--
 * architecture.md:73; ties break by registry order - deterministic).
 *
 * Parallel mode (flags.threads > 1): the window-index space is fanned out
 * over a ThreadPool; emission serializes inside ResultTree; per-worker
 * overlap sets backstopped by the tree extent index keep F9 dedup exact
 * across worker boundaries.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_carve.h"
#include "cr/cr_text.h" /* S-032: entry names UTF-8 -> u16 */
#include "cr/carve/cr_rawsigns.h"
#include "cr/carve/cr_usersigns.h"
#include "cr/cr_parallel.h"
#include "cr/crypto/cr_sha256.h"
#include "cr/table/cr_table_wire.h" /* M-051: crc32 for the analyser checksum */
#include <atomic>
#include <chrono>
#include <cstring>
#include <ctime>
#include <set>

namespace cr {
namespace carve {

namespace {
constexpr u64 WINDOW = 1ull << 20; /* 1 MiB */
constexpr u64 OVERLAP = 4096;      /* magic search continuity */
constexpr u64 MAX_FILE_READ_FOR_HASH = 8ull << 20;

/* trial order: most specific grammar first when magics coincide */
/* specificity-first: same-magic families order their most specific
 * member first (DOCX>ZIP, CR2>DNG>TIFF, RAR5>RAR4, M4A>MP4/MOV) */
constexpr u32 kTrialOrder[] = {
    FMT_DOCX, FMT_ZIP, FMT_CR2, FMT_DNG, FMT_TIFF, FMT_MOV, FMT_MP4, FMT_HEIC, FMT_HEIF, FMT_JPG,
    FMT_PNG, /* N-052: HEIC heic-proper first; HEIF siblings follow */
    FMT_WEBP, FMT_WAV, FMT_PDF, FMT_7Z, FMT_SQLITE, FMT_AVI, FMT_MKV, FMT_RAR5, FMT_RAR4, FMT_PST,
    FMT_ORF, FMT_RAF, FMT_TIFF_EP, FMT_RW2, FMT_PSD,
    /* N-053: TIFF/EP claims IIU before the RW2 fallback */ FMT_MP3, FMT_M4A, FMT_ASF, FMT_FLV,
    FMT_CAB, FMT_ISO, FMT_TIFF_BE, FMT_ORF_IIRS, FMT_ORF_MMOR, FMT_ZIP_CD,
    FMT_CAB_LC, /* N-069: lowercase CAB key follows its uppercase sibling (distinct magic) */
    /* S-007: the og's common formats - distinct magics (no overlap with
     * the entries above), so the placement after the specific set is
     * safe; FMT_MP3 (ID3-headed) stays ahead of the raw sync family. */
    FMT_GIF, FMT_BMP, FMT_RTF, FMT_OLE, FMT_MPEGPS, FMT_MPEGES, FMT_DWG, FMT_AAC, FMT_AAC2,
    FMT_MPGA_E2, FMT_MPGA_E3, FMT_MPGA_E4, FMT_MPGA_E5, FMT_MPGA_E6, FMT_MPGA_E7, FMT_MPGA_F2,
    FMT_MPGA_F3, FMT_MPGA_F4, FMT_MPGA_F5, FMT_MPGA_F6, FMT_MPGA_F7, FMT_MPGA_FA, FMT_MPGA_FB,
    FMT_MPGA_FC, FMT_MPGA_FD, FMT_MPGA_FE, FMT_MPGA_FF,
    /* S-017: ftyp-destroyed MOV salvage (og MovAnalyzer list layer) -
     * LAST: the ftyp-keyed entries must claim the position first */
    FMT_MOV_NAKED};
constexpr u32 kTrialCount = sizeof(kTrialOrder) / sizeof(kTrialOrder[0]);

/* Shared scan context (single-thread: one worker touches it; parallel:
 * counters are atomic, tree emission is internally locked). */
struct ScanCtx {
    IStorageDevice* slice;
    const Signature* reg;
    /* M-024: per-trial-slot and per-raw-sign-format disable maps, built
     * once in carve_slice from flags->disabled_formats (null list ->
     * all-zero = every format enabled). */
    /* sized by the trial table; the static_assert keeps the fixed array
     * honest when formats are added */
    static constexpr u32 kMaxTrialSlots = 96;
    bool trial_disabled[kMaxTrialSlots] = {};
    std::vector<bool> rawsign_disabled;
    const CarveFlags* flags;
    tree::ResultTree* tree;
    tree::FullNode* carve_root;
    const tree::ResultTree* dedup_tree;
    NameCallback namer;
    u64 total;

    std::atomic<u64> hits{0};
    std::atomic<u64> validated{0};
    std::atomic<u64> rejected{0};
    std::atomic<u64> duplicates{0};
    std::atomic<u64> hashed{0};
    std::atomic<u64> bytes_scanned{0};
    std::atomic<u64> sign_hits{0};
    std::atomic<u64> sign_validated{0};
    std::atomic<u64> windows_skipped{0}; /* unreadable after retry (FB-C01) */
    /* N-031: all-zero sign-window strides skipped at the raw-sign layer
     * (the og's zero-sector fast-skip analog) */
    std::atomic<u64> zero_strides_skipped{0};
    /* M-005: used-cluster skips at the grammar layer (the og's block-scan
     * cluster gate; counted here, surfaced as bitmap_skipped_clusters) */
    std::atomic<u64> bitmap_skipped_clusters{0};
    /* S-007: [start, end) ranges of emitted suppress_overlap streams (the
     * repeating-sync audio family); guarded by the emission path's own
     * serialization (single-threaded path: one worker; parallel: the
     * window dispatch is exclusive per range... the list is appended by
     * the same worker that scans the window, and read within the same
     * window scan - cross-window overlap re-checks by re-walking). */
    std::vector<std::pair<u64, u64>> overlap_covered;
    std::atomic<u32> file_index{0};
    std::atomic<bool> aborted{false};
    /* S-031 residual: the wall-clock scan time stamped onto every carved
     * node's times.modified (the og persists a per-carved-file timestamp
     * string written by FUN_140c41aa0 via "%d-%d-%d %d-%d-%d" from the
     * scan-time FILETIME — ifileproperties-part3 :80; captured ONCE per
     * carve_slice run so all nodes of one scan agree). */
    u64 scan_time = 0;
    /* GAP-S6-8: the disk-absolute basis for extents + dedup queries.
     * Carve positions are slice-relative (reads, bounds, emitted_at stay
     * relative); the TREE keys must be disk-absolute so the resumed carve
     * dedups against checkpoint-loaded nodes (which are absolute - F-1)
     * instead of re-emitting them as extra entries. Base 0 on synthetic /
     * whole-disk devices: behavior identical to the old code. */
    u64 ext_base = 0;

    /* ---- M-005: the allocation-bitmap gate (corpus-verified first-hand
     * this session, ghidra-searchdll/RAWSearchDLL_decompiled.c :9440-9466
     * + FUN_180058df0 :67842-67856). The og's block-scan: with a bitmap
     * object present, a candidate position whose cluster bit is SET
     * (used) is suppressed from dispatch; if the position is
     * cluster-aligned the scan jumps the whole cluster (local_1e0 +=
     * spc-1); OOB (bit-count bound [2]) or no bitmap -> dispatch proceeds
     * (raw disks keep whole-device carving). The og's bitmap is the USED
     * map; the core's FreeMap is inverted (bit set = free) - the gate
     * inverts: clear = used. ---- */
    const storage::FreeMap* free_map = nullptr;

    /* M-039: the occupancy interval gate (sorted, coalesced, non-empty
     * [start, end) pairs, SLICE-relative). Positions inside an interval
     * are suppressed from dispatch; the grammar layer jumps to the
     * interval end (the M-005 cluster-jump analog at extent
     * granularity). Binary search per probe (the set is sorted). */
    const std::vector<std::pair<u64, u64>>* occupied = nullptr;
    std::atomic<u64> occupancy_skipped{0}; /* positions suppressed by the
                                              interval gate (telemetry) */

    /* Position inside an occupied interval? Returns the interval end
     * (the jump target) via `end_out` when suppressed. */
    bool occupancy_used(u64 pos, u64* end_out = nullptr) const {
        if (occupied == nullptr || occupied->empty())
            return false;
        /* upper_bound on start: the candidate interval is the one before */
        size_t lo = 0, hi = occupied->size();
        while (lo < hi) {
            const size_t mid = lo + (hi - lo) / 2;
            if ((*occupied)[mid].first <= pos)
                lo = mid + 1;
            else
                hi = mid;
        }
        if (lo == 0)
            return false; /* before every interval */
        const auto& iv = (*occupied)[lo - 1];
        if (pos < iv.second) {
            if (end_out)
                *end_out = iv.second;
            return true;
        }
        return false;
    }

    /* Position suppressed by the gate? (slice-relative pos) */
    bool gate_used(u64 pos) const {
        if (!free_map || !free_map->valid() || pos < free_map->base_byte)
            return false; /* no map / before coverage: never gate (the og:
                             FUN_180058df0 returns 0 on OOB -> dispatch) */
        const u64 k = (pos - free_map->base_byte) / free_map->cluster_bytes;
        return !free_map->is_free(k); /* clear = used */
    }

    /* Slice-relative start of the NEXT cluster after pos (only called
     * when pos is cluster-aligned and used). */
    u64 gate_next_cluster(u64 pos) const {
        const u64 k = (pos - free_map->base_byte) / free_map->cluster_bytes;
        return free_map->base_byte + (k + 1) * u64(free_map->cluster_bytes);
    }

    /* Shared skip action for both dispatch layers: returns the jump
     * target in SLICE coords when the position is used AND cluster-aligned
     * (0 otherwise - the caller suppresses the single position only).
     * Cluster counting happens ONLY at the grammar layer (count=true);
     * the raw-sign layer rides the same gate without double-counting. */
    u64 gate_check_and_skip(u64 pos, bool count) {
        if (!gate_used(pos))
            return 0; /* not suppressed */
        if ((pos - free_map->base_byte) % free_map->cluster_bytes == 0) {
            if (count)
                bitmap_skipped_clusters.fetch_add(1, std::memory_order_relaxed);
            return gate_next_cluster(pos);
        }
        return 0; /* used but mid-cluster: suppress just this position */
    }
};

/* M-025: window-serving read context - the og's already-in-buffer model
 * (RAWSearchDLL :9195-9209 the 8 MiB ping-pong buffer; :9434-9438 chunk
 * fills; candidates validated from the buffer :9496). Wraps the slice
 * for the duration of ONE window scan: reads fully inside
 * [base, base+len) are served from the window buffer (memcpy, zero
 * device traffic); reads straddling a window edge serve the in-window
 * part from the buffer and read only the remainder from the device.
 * This removes the per-candidate device re-reads the finding prices
 * (validator marker-walk/probe re-reads + the <=8 MiB hash re-read +
 * the 512 B checksum re-read) while keeping byte-identical content -
 * the buffer IS the device's bytes for that range (read once at window
 * fill). Thread-safe by construction: one instance per window scan,
 * private to the worker that owns the window. */
class WindowDevice final : public IStorageDevice {
public:
    WindowDevice(IStorageDevice* inner, u64 base, const u8* win, u64 len)
        : inner_(inner), base_(base), win_(win), len_(len) {}

    u32 read_at(u64 off, void* buf, u32 len) override {
        if (len == 0)
            return OK;
        u8* out = static_cast<u8*>(buf);
        /* head: the read starts before the window - front from the device */
        if (off < base_) {
            const u32 head = u32(std::min<u64>(len, base_ - off));
            if (u32 rc = inner_->read_at(off, out, head))
                return rc;
            out += head;
            off += head;
            len -= head;
            if (len == 0)
                return OK;
        }
        /* middle: the in-window span - served from the buffer */
        if (off >= base_ && off < base_ + len_) {
            const u32 mid = u32(std::min<u64>(len, base_ + len_ - off));
            std::memcpy(out, win_ + (off - base_), size_t(mid));
            out += mid;
            off += mid;
            len -= mid;
            if (len == 0)
                return OK;
        }
        /* tail: past the window end - from the device */
        return inner_->read_at(off, out, len);
    }

    const Geometry& geometry() const override { return inner_->geometry(); }
    u64 size() const override { return inner_->size(); }

private:
    IStorageDevice* inner_;
    u64 base_;
    const u8* win_;
    u64 len_;
};

/* Scan ONE window [base, base+want) fully (magic dispatch, validation,
 * dedup, emit). Returns OK, or ERR_CANCELLED when aborted mid-way. */
u32 scan_one_window(ScanCtx& ctx, u64 base, const u8* win, u64 want, std::set<u64>& emitted_at) {
    /* M-025: validators, checksums, hashes and tail scans read through
     * the window-serving device for this window (per-candidate device
     * re-reads eliminated - see WindowDevice above). */
    WindowDevice wdev(ctx.slice, base, win, want);
    for (u64 off = 0; off + 12 <= want; ++off) {
        if (ctx.aborted.load(std::memory_order_relaxed))
            return ERR_CANCELLED;
        /* M-005: the used-cluster gate, grammar layer (the per-byte
         * dispatch - the dominant cost). Used -> suppress; aligned used
         * -> jump the whole cluster (the og's local_1e0 += spc-1). */
        if (ctx.free_map) {
            if (const u64 jump = ctx.gate_check_and_skip(base + off, true); jump != 0) {
                const u64 rel = jump - base; /* window coords */
                if (rel > off)
                    off = rel - 1; /* the loop's ++off lands at the next cluster */
                continue;
            } else if (ctx.gate_used(base + off)) {
                continue; /* used mid-cluster: this position only */
            }
        }
        /* M-039: the occupancy interval gate - a position inside a
         * structured node's extent is FS-accounted space: suppress (and
         * jump to the interval end - the same window-coords convention
         * as the M-005 jump; extents coalesce, so the jump skips whole
         * files). */
        if (u64 occ_end = 0; ctx.occupancy_used(base + off, &occ_end)) {
            ctx.occupancy_skipped.fetch_add(1, std::memory_order_relaxed);
            const u64 rel = occ_end - base;
            if (rel > off)
                off = rel - 1; /* lands past the interval */
            continue;
        }
        const u8 c = win[off];
        /* first-byte dispatch: only formats whose magic starts with c.
         * The match itself (magic compare + window-head clip + candidate
         * shift) is match_signature in cr_signatures.h - the SAME
         * primitive the S-017 F8 self-probe runs, so a magic the scan
         * cannot reach fails at load time (one source of truth).
         *
         * N-070 (diff-to-fix): every registry row whose magic matches is
         * validated at this position and the highest accumulated-score
         * accepting candidate wins (score = base_confidence +
         * validator confidence; ties break by registry trial order,
         * deterministic). The og ranks overlapping candidates by
         * accumulated header+tail confidence (searchdll-family--
         * architecture.md:73); the pre-fix first-accept-wins break emitted
         * whichever row happened to sit earlier in kTrialOrder. */
        const Signature* best_sig = nullptr;
        CarveResult best_cr;
        u64 best_len = 0;
        u64 best_cand_abs = 0;
        for (u32 ti = 0; ti < kTrialCount; ++ti) {
            if (ctx.trial_disabled[ti])
                continue; /* M-024: format disabled by the host */
            const Signature& sig = ctx.reg[kTrialOrder[ti]];
            if (sig.header[0] != c)
                continue; /* first-byte fast path */
            u64 cand_rel = 0;
            if (!match_signature(sig, win, want, off, cand_rel))
                continue;
            const u64 cand_abs = base + cand_rel;
            if (cand_abs + sig.min_size > ctx.total)
                continue;
            ctx.hits.fetch_add(1, std::memory_order_relaxed);

            /* try THIS format's grammar; accepting candidates accumulate
             * into best_sig (N-070) - the winner is emitted after the
             * loop. Position-resolution breaks (already-emitted here, F9
             * dedup) still exit the trial loop immediately. */
            CarveResult cr = sig.validator(&wdev, cand_abs, sig.max_size);
            if (!cr.ok) {
                ctx.rejected.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            u64 file_len = cr.end - cand_abs;
            if (file_len < sig.min_size || file_len > sig.max_size) {
                ctx.rejected.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            if (emitted_at.count(cand_abs)) {
                ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                break;
            }
            if (sig.suppress_overlap) {
                bool inside = false;
                for (const auto& cov : ctx.overlap_covered)
                    if (cand_abs > cov.first && cand_abs < cov.second) {
                        inside = true;
                        break;
                    }
                if (inside) {
                    ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                    continue; /* a mid-stream sync of an emitted stream */
                }
            }

            /* F9 dedup vs structured results + prior carved nodes (the
             * locked extent index is the cross-worker backstop) */
            u64 len_class = 0;
            if (file_len >= (1 << 20)) {
                while ((1ULL << len_class) < file_len)
                    ++len_class;
                len_class += 64;
            } else {
                len_class = file_len / 65536;
            }
            bool dup = false;
            if (ctx.dedup_tree &&
                ctx.dedup_tree->find_by_extent(cand_abs + ctx.ext_base, len_class))
                dup = true;
            if (!dup && ctx.tree->find_by_extent(cand_abs + ctx.ext_base, len_class))
                dup = true;
            if (dup) {
                ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                emitted_at.insert(cand_abs); /* position resolved: a file lives here (the
                                                raw-sign layer must not re-probe it) */
                break;
            }

            /* N-070: candidate accepted - accumulate; strictly greater
             * score replaces (ties keep the earlier trial-order row). */
            if (best_sig == nullptr ||
                u32(sig.base_confidence) + u32(cr.confidence) >
                    u32(best_sig->base_confidence) + u32(best_cr.confidence)) {
                best_sig = &sig;
                best_cr = std::move(cr);
                best_len = file_len;
                best_cand_abs = cand_abs;
            }
        }
        if (best_sig != nullptr) {
            const Signature& sig = *best_sig;
            const u64 cand_abs = best_cand_abs;
            const u64 file_len = best_len;
            CarveResult& cr = best_cr;

            tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
            if (props == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            const u32 idx = ctx.file_index.fetch_add(1, std::memory_order_relaxed);
            /* N-063: the og's per-analyser name first (the analyser's
             * own SetName - e.g. MP4/MOV's "Lost Name File (...)_EaseUS"
             * template), then the caller's namer callback, then the
             * documented File%06u deliberate default (PARITY-DELTAS:
             * formats with no decoded og naming rule). */
            props->name = !cr.embedded_name.empty()
                              ? cr.embedded_name
                              : (ctx.namer ? ctx.namer(sig, file_len, idx)
                                           : default_namer(sig, file_len, idx));
            props->fs_kind =
                u32(FsKindCode::Raw); /* carved (row 45: was invented 0xFF; factory RAW=6) */
            props->logical_size = file_len;
            props->carve_format = sig.id;
            props->carve_confidence = u16((sig.base_confidence + cr.confidence) / 2);
            props->times.modified = ctx.scan_time; /* S-031: the scan-time stamp */
            /* M-051: the analyser checksum — CRC32 over the carved
             * extent's first 512 bytes (the re-verification value the
             * og's RAW record carries at +0x56; 0xFFFFFFFF = unreadable
             * header, the og's invalid marker). */
            {
                u8 hdr[512];
                const u32 hb = u32(file_len < sizeof(hdr) ? file_len : sizeof(hdr));
                if (hb != 0 && wdev.read_at(cand_abs, hdr, hb) == OK)
                    props->carve_checksum = table::crc32(hdr, hb);
            }
            if (props->carve_confidence > 1000)
                props->carve_confidence = 1000;
            /* GAP-S6-8: disk-absolute extent + the idempotency guard so the
             * stage-boundary rebase never double-shifts carved nodes */
            props->extents.push_back({cand_abs + ctx.ext_base, file_len});
            if (ctx.ext_base != 0)
                props->flags = u16(props->flags | tree::NODE_FLAG_EXT_SHIFTED);

            if (ctx.flags->hash_small_files && file_len <= ctx.flags->hash_threshold &&
                file_len <= MAX_FILE_READ_FOR_HASH) {
                std::vector<u8> content((size_t(file_len)));
                if (wdev.read_at(cand_abs, content.data(), u32(file_len)) == OK) {
                    u8 full[32];
                    crypto::Sha256::digest(content.data(), size_t(file_len), full);
                    std::memcpy(props->sha128, full, 16);
                    props->hash_valid = true;
                    ctx.hashed.fetch_add(1, std::memory_order_relaxed);
                }
            }
            if (sig.suppress_overlap)
                ctx.overlap_covered.push_back({cand_abs, cand_abs + file_len});
            /* M-021: the post-carve processing hook - the og's deferred-poll
             * position (per finalized candidate, post-validate, pre-insertion).
             * A feed, never a gate; unbound = raw emission (the og's
             * DLL-absent degrade). */
            if (ctx.flags->post_processor) {
                CarveCandidateInfo ci;
                ci.format_tag = sig.id;
                ci.format_ext = sig.ext;
                ci.offset = cand_abs;
                ci.length = file_len;
                ci.confidence = props->carve_confidence;
                ctx.flags->post_processor->on_candidate(ctx.slice, ci);
            }
            tree::FileNode* n = ctx.tree->add_file(ctx.carve_root, props);
            props->release();
            if (n == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            n->set_origin(tree::Origin::Carved);
            /* S-031: every carved record is deleted-family state — the og's
             * CRAWFileProperties FLAGS field is the constant 0x0020 across
             * all 12,147 sampled records (ifileproperties-part3 :82); the
             * deleted_only query view and deleted_count stats include the
             * whole carve population */
            n->set_deleted_state(tree::DeletedState::Deleted);
            n->set_confidence(cr.confidence >= 850 ? tree::Confidence::Excellent
                                                   : tree::Confidence::PartialRisk);
            /* S-032: the archive's inner-entry inventory as child nodes —
             * the og's analysers return the complete inventory (names,
             * sizes, offsets) which the results tree presents as the
             * archive's contents (rawsearchdll--zip-analyser-carving.md
             * :29-38; analyser-granular-batch1 :36-41). Superset note: an
             * entry whose data span was localized carries a precise
             * extent INSIDE the parent's extent, so a consumer can recover
             * one entry without the whole archive. */
            for (const auto& e : cr.entries) {
                auto* ep = new (std::nothrow) tree::FileProperties();
                if (ep == nullptr) {
                    ctx.aborted.store(true, std::memory_order_relaxed);
                    return ERR_NO_MEMORY;
                }
                ep->name = text::utf8_to_u16(e.name);
                ep->fs_kind = u32(FsKindCode::Raw);
                ep->logical_size = e.size;
                ep->carve_format = sig.id;
                /* S-031 x S-032 (merge residue): the og stamps EVERY raw
                 * record with the per-record scan time (FUN_140c41aa0,
                 * ifileproperties-part3 :80) — the inventory entries are
                 * records in their own right, so they carry the same
                 * run-stamp as the archive parent */
                ep->times.modified = ctx.scan_time;
                if (e.data_len != 0 && e.data_offset != 0)
                    ep->extents.push_back({e.data_offset + ctx.ext_base, e.data_len});
                tree::FileNode* en = ctx.tree->add_file(n, ep);
                ep->release();
                if (en == nullptr) {
                    ctx.aborted.store(true, std::memory_order_relaxed);
                    return ERR_NO_MEMORY;
                }
                en->set_origin(tree::Origin::Carved);
                en->set_deleted_state(tree::DeletedState::Deleted);
                en->set_confidence(e.data_len != 0 ? tree::Confidence::PartialRisk
                                                   : tree::Confidence::MetadataOnly);
            }
            ctx.validated.fetch_add(1, std::memory_order_relaxed);
            emitted_at.insert(cand_abs); /* this magic position resolved (N-070: the
                                              winner's start; the raw-sign layer must
                                              not re-probe it) */
        }
    }

    /* ---- raw-sign layer 1 (audit row 32): the og's 139-format sign
     * table, probed at 512-byte strides. Grammar hits own their start
     * positions (emitted_at); the extent index dedups the rest. ---- */
    {
        const rawsigns::SignIndex& idx = rawsigns::SignIndex::get();
        std::vector<const rawsigns::RawSign*> matches;
        for (u64 off = 0; off + 512 <= want; off += 512) {
            if (ctx.aborted.load(std::memory_order_relaxed))
                return ERR_CANCELLED;
            const u64 start_abs = base + off;
            /* M-005: the same gate on the raw-sign layer's 512-stride
             * probes (the og's single block-scan loop gates all dispatch;
             * skip counting stays with the grammar layer - no double
             * count). */
            /* M-039: the occupancy gate rides the same stride - a sign
             * inside FS-accounted space is the og's skip case (its
             * per-volume bitmap covers every engine's extents). */
            if (ctx.occupancy_used(start_abs)) {
                ctx.occupancy_skipped.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            if (ctx.free_map) {
                if (const u64 jump = ctx.gate_check_and_skip(start_abs, false); jump != 0) {
                    const u64 rel = jump - base;
                    if (rel > off)
                        off = rel - 512; /* the loop's += 512 lands at the next cluster */
                    continue;
                } else if (ctx.gate_used(start_abs)) {
                    continue; /* used mid-cluster: this probe only */
                }
            }
            if (emitted_at.count(start_abs))
                continue; /* a grammar-validated file starts here */
            /* M-048: the user-extensible signature layer (the og's
             * EUUserData consumer) - probed at the same stride, after
             * the grammar layer's claims, before the built-in raw-sign
             * table (a user sign for an already-probed position is a
             * duplicate by construction; a user sign colliding with a
             * BUILT-IN fires only when the built-ins did not match -
             * built-in-first, the collision rule the finding specifies).
             * A user-sign hit emits directly: no tail walk (user signs
             * carry no tail semantics), the user's extension and MIME in
             * the record, extent to the device end (the og's user signs
             * are header-only markers - no size grammar). */
            if (usersigns::active_count() != 0) {
                if (const carve::usersigns::UserSign* us =
                        usersigns::match_at(win + off, want - off)) {
                    const u64 len = ctx.total - start_abs;
                    /* F9 dedup (extent class) - the same shape as the
                     * grammar/raw-sign layers */
                    u64 len_class = 0;
                    if (len >= (1 << 20)) {
                        while ((1ULL << len_class) < len)
                            ++len_class;
                        len_class += 64;
                    } else {
                        len_class = len / 65536;
                    }
                    bool dup = false;
                    if (ctx.dedup_tree &&
                        ctx.dedup_tree->find_by_extent(start_abs + ctx.ext_base, len_class))
                        dup = true;
                    if (!dup && ctx.tree->find_by_extent(start_abs + ctx.ext_base, len_class))
                        dup = true;
                    if (dup) {
                        ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                        emitted_at.insert(start_abs);
                    } else {
                        tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
                        if (props == nullptr) {
                            ctx.aborted.store(true, std::memory_order_relaxed);
                            return ERR_NO_MEMORY;
                        }
                        const u32 fi = ctx.file_index.fetch_add(1, std::memory_order_relaxed);
                        Signature rsig{};
                        rsig.ext = us->ext;
                        rsig.name = us->ext;
                        props->name = ctx.namer ? ctx.namer(rsig, len, fi)
                                                : default_namer(rsig, len, fi);
                        props->fs_kind = u32(FsKindCode::Raw);
                        props->logical_size = len;
                        props->carve_format = 0xFFFE0000u; /* M-048: the user-signature tag */
                        props->carve_confidence = 600; /* user-taught, no og-calibrated
                                                         * confidence - the fixed documented
                                                         * value (PARITY-DELTAS) */
                        props->times.modified = ctx.scan_time;
                        {
                            u8 hdr[512];
                            const u32 hb = u32(len < sizeof(hdr) ? len : sizeof(hdr));
                            if (hb != 0 && wdev.read_at(start_abs, hdr, hb) == OK)
                                props->carve_checksum = table::crc32(hdr, hb);
                        }
                        props->extents.push_back({start_abs + ctx.ext_base, len});
                        if (ctx.ext_base != 0)
                            props->flags = u16(props->flags | tree::NODE_FLAG_EXT_SHIFTED);
                        /* the MIME rides the note (the og's EUUserData
                         * classification field) */
                        if (us->mime[0] != 0)
                            for (const char* mp = us->mime; *mp; ++mp)
                                props->carve_note.push_back(u16(u8(*mp)));
                        tree::FileNode* n = ctx.tree->add_file(ctx.carve_root, props);
                        props->release();
                        if (n == nullptr) {
                            ctx.aborted.store(true, std::memory_order_relaxed);
                            return ERR_NO_MEMORY;
                        }
                        n->set_origin(tree::Origin::Carved);
                        n->set_deleted_state(tree::DeletedState::Deleted);
                        n->set_confidence(tree::Confidence::PartialRisk);
                        ctx.sign_hits.fetch_add(1, std::memory_order_relaxed);
                        emitted_at.insert(start_abs);
                        continue;
                    }
                }
            }
            /* N-031: the og's all-zero-sector fast-skip, core shape: no
             * header sign can live outside the first 47 bytes of the
             * stride (probe_header's whole offset span), so a stride
             * whose sign window is all zero skips the 47-lookup
             * sign-table walk entirely (the og skips the whole sector:
             * its leading/trailing zero-dword metrics, :9490-9520). The
             * grammar layer keeps its first-byte dispatch on zeros - it
             * is near-free there (the finding's own profile rule). */
            {
                const u8* zw = win + off;
                bool all_zero = true;
                for (u32 zi = 0; zi < 47; ++zi)
                    if (zw[zi]) {
                        all_zero = false;
                        break;
                    }
                if (all_zero) {
                    ctx.zero_strides_skipped.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }
            }
            idx.probe_header(win + off, want - off, matches);
            if (matches.empty())
                continue;
            /* M-024: drop disabled formats before the best-pick (the og's
             * filter removes the analyser from the working registry -
             * FUN_180006b30's removal loop - so its signs never fire) */
            if (!ctx.rawsign_disabled.empty()) {
                size_t w = 0;
                for (size_t r = 0; r < matches.size(); ++r)
                    if (!ctx.rawsign_disabled[matches[r]->fmt])
                        matches[w++] = matches[r];
                matches.resize(w);
                if (matches.empty())
                    continue;
            }
            /* highest-confidence hit drives the candidate */
            const rawsigns::RawSign* best = matches[0];
            for (const auto* m : matches)
                if (m->conf > best->conf)
                    best = m;
            const rawsigns::RawSignFormat& fmt = idx.format(best->fmt);
            u64 len = fmt.max_size;
            u16 conf = best->conf;
            bool tail_paired = false;
            u64 tail_off = 0; /* M-051: the tail anchor (0 = header-only) */
            /* tail pairing: bounded forward walk inside [start+min, start+max).
             * The og searches the FULL KiB-scaled window incrementally
             * (FUN_1800205b0 keeps returning 2 "need more data" while the
             * buffered span is below max*0x400, up to max<<10 = 200 MiB for
             * the media entries; tail acceptance gates size >= min<<10).
             * DOCUMENTED DIVERGENCE (S-015): we cap the forward-read walk
             * at kRawSignTailScanLimit instead of streaming the whole
             * window - tails beyond the bound fall back to header-only
             * emission. Chunked with a kMaxSignBytes overlap so a tail
             * sign straddling a chunk edge is never split. */
            static constexpr u64 kTailScanChunk = 1ull << 20; /* 1 MiB reads */
            if (fmt.min_size < fmt.max_size && start_abs + fmt.min_size < ctx.total) {
                const u64 lo = fmt.min_size;
                const u64 hi =
                    fmt.max_size < ctx.total - start_abs ? fmt.max_size : ctx.total - start_abs;
                if (hi > lo) {
                    const u64 window = hi - lo;
                    const u64 scan_len =
                        window < idx.tail_scan_limit() ? window : idx.tail_scan_limit();
                    std::vector<u8> tail_buf(size_t(kTailScanChunk) + rawsigns::kMaxSignBytes);
                    u64 scanned = 0;
                    rawsigns::SignIndex::TailHit th;
                    while (scanned < scan_len) {
                        const u64 chunk_want = scan_len - scanned < kTailScanChunk
                                                   ? scan_len - scanned
                                                   : kTailScanChunk;
                        /* overlap read: [lo+scanned, lo+scanned+chunk_want+signs)
                         * keeps any <=kMaxSignBytes tail sign contiguous.
                         * avail is window-relative: the device end is
                         * start_abs+hi, so at most hi-lo-scanned bytes
                         * remain (NOT hi-scanned - lo is the walk base). */
                        const u64 avail_max = hi - lo - scanned;
                        const u64 read_len = avail_max < chunk_want + rawsigns::kMaxSignBytes
                                                 ? avail_max
                                                 : chunk_want + rawsigns::kMaxSignBytes;
                        u8* buf = tail_buf.data();
                        if (wdev.read_at(start_abs + lo + scanned, buf, u32(read_len)) == OK) {
                            if (idx.scan_tail_for_format(buf, read_len, best->fmt, th)) {
                                len = lo + scanned + th.pos + 512;
                                /* M-051: anchor B = the tail sign's exact position
                                 * (stride base + the sign's intra-stride offset) */
                                tail_off = start_abs + lo + scanned + th.pos + u64(th.sign->offset);
                                if (len > hi)
                                    len = hi;
                                conf = u16((u32(best->conf) + u32(th.sign->conf)) / 2 + 100);
                                if (conf > 1000)
                                    conf = 1000;
                                tail_paired = true;
                                break;
                            }
                        } else {
                            /* unreadable tail region: stop the walk - the
                             * candidate stays header-only rather than
                             * fabricating a tail it could not read */
                            break;
                        }
                        scanned += chunk_want;
                    }
                }
            }
            if (len > ctx.total - start_abs)
                len = ctx.total - start_abs;
            if (len < fmt.min_size)
                continue; /* device too small for the format's floor */
            ctx.sign_hits.fetch_add(1, std::memory_order_relaxed);
            if (tail_paired)
                ctx.sign_validated.fetch_add(1, std::memory_order_relaxed);

            /* F9 dedup (extent class) against structured + carved */
            u64 len_class = 0;
            if (len >= (1 << 20)) {
                while ((1ULL << len_class) < len)
                    ++len_class;
                len_class += 64;
            } else {
                len_class = len / 65536;
            }
            bool dup = false;
            if (ctx.dedup_tree &&
                ctx.dedup_tree->find_by_extent(start_abs + ctx.ext_base, len_class))
                dup = true;
            if (!dup && ctx.tree->find_by_extent(start_abs + ctx.ext_base, len_class))
                dup = true;
            if (dup) {
                ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                continue;
            }

            tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
            if (props == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            const u32 fi = ctx.file_index.fetch_add(1, std::memory_order_relaxed);
            /* N-014b: ONE naming family per scan - the raw-sign layer
             * rides the SAME chain as the grammar layer (the caller's
             * namer, then the registered File%06u default) instead of
             * its own hardcoded scheme. The og has no FileNNNN/Carved
             * scheme at all (corpus erratum, OQ-S4-2's decode; naming is
             * the host callback's job - N-063), so the callback contract
             * reads only ext/name; a partial Signature view of the
             * raw-sign format is the honest projection of that contract. */
            Signature rsig{};
            rsig.ext = fmt.ext;
            rsig.name = fmt.ext;
            props->name = ctx.namer ? ctx.namer(rsig, len, fi) : default_namer(rsig, len, fi);
            props->fs_kind = u32(FsKindCode::Raw);
            props->logical_size = len;
            props->carve_format = 0xFFFF0000u + best->fmt; /* raw-sign tag */
            props->carve_confidence = conf;
            props->times.modified = ctx.scan_time; /* S-031: the scan-time stamp */
            /* M-051: the analyser checksum (+0x56; CRC32 of the extent's
             * first 512 bytes, 0xFFFFFFFF = unreadable) and the tail
             * anchor B (+0x94 — the tail-paired offset, 0 when
             * header-only, the og's 261/12,147 census). */
            {
                u8 hdr[512];
                const u32 hb = u32(len < sizeof(hdr) ? len : sizeof(hdr));
                if (hb != 0 && wdev.read_at(start_abs, hdr, hb) == OK)
                    props->carve_checksum = table::crc32(hdr, hb);
            }
            props->carve_anchor_b = tail_off;
            /* GAP-S6-8: disk-absolute extent + the idempotency guard (see
             * the grammar layer above) */
            props->extents.push_back({start_abs + ctx.ext_base, len});
            if (ctx.ext_base != 0)
                props->flags = u16(props->flags | tree::NODE_FLAG_EXT_SHIFTED);
            if (ctx.flags->hash_small_files && len <= ctx.flags->hash_threshold &&
                len <= MAX_FILE_READ_FOR_HASH) {
                std::vector<u8> content((size_t(len)));
                if (wdev.read_at(start_abs, content.data(), u32(len)) == OK) {
                    u8 full[32];
                    crypto::Sha256::digest(content.data(), size_t(len), full);
                    std::memcpy(props->sha128, full, 16);
                    props->hash_valid = true;
                    ctx.hashed.fetch_add(1, std::memory_order_relaxed);
                }
            }
            /* M-021: the post-carve hook fires on the raw-sign layer's
             * finalized candidates too (the og's block scan feeds every
             * CRawFileInfo through the same deferred poll). */
            if (ctx.flags->post_processor) {
                CarveCandidateInfo ci;
                ci.format_tag = props->carve_format;
                ci.format_ext = fmt.ext;
                ci.offset = start_abs;
                ci.length = len;
                ci.confidence = props->carve_confidence;
                ctx.flags->post_processor->on_candidate(ctx.slice, ci);
            }
            tree::FileNode* n = ctx.tree->add_file(ctx.carve_root, props);
            props->release();
            if (n == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            n->set_origin(tree::Origin::Carved);
            /* S-031: same constant-0x20 FLAGS on the raw-sign layer — both
             * carve emit sites flag deleted (see the grammar layer above) */
            n->set_deleted_state(tree::DeletedState::Deleted);
            n->set_confidence(tail_paired ? tree::Confidence::PartialRisk
                                          : tree::Confidence::MetadataOnly);
            emitted_at.insert(start_abs);
        }
    }
    return OK;
}

/* Read + scan the window with index wi (base = wi * WINDOW). */
u32 scan_window_index(ScanCtx& ctx, u64 wi, std::set<u64>& emitted_at) {
    const u64 base = wi * WINDOW;
    const u64 avail = ctx.total - base;
    u64 want = avail < WINDOW + OVERLAP ? avail : WINDOW + OVERLAP;
    if (want < 16)
        return OK;
    /* honest fill-rate accounting (G-05): the OVERLAP tail re-reads the
     * next window's head, so attribute only the window's OWN range
     * [base, base+WINDOW) - bytes_scanned must never exceed the device.
     * The last window's avail is always <= WINDOW (window_count =
     * ceil(total/WINDOW)), so the sum of attributed ranges == total. */
    const u64 counted = want < WINDOW ? want : WINDOW;
    std::vector<u8> win(static_cast<size_t>(want), 0);
    if (ctx.slice->read_at(base, win.data(), u32(want)) != OK) {
        /* Unreadable window: skipped, NOT counted as scanned - the honest
         * fill-rate contract (G-05) forbids claiming coverage of bytes
         * that were never read; the retry pipe's error map already
         * carries the failure (FB-C01). */
        ctx.windows_skipped.fetch_add(1, std::memory_order_relaxed);
        return OK;
    }
    ctx.bytes_scanned.fetch_add(counted, std::memory_order_relaxed);
    return scan_one_window(ctx, base, win.data(), want, emitted_at);
}

} // namespace

std::u16string default_namer(const Signature& sig, u64 size, u32 index) {
    (void)size;
    char buf[32];
    /* N-063 comment repair: the og has NO FileNNNN scheme anywhere (the
     * deep-dive artifact's "File0001.jpg lives in the UI" claim was a
     * corpus erratum, corrected with OQ-S4-2's decode). This is OUR
     * deliberate default for formats whose og naming rule is not
     * decoded (registered in docs/PARITY-DELTAS.md) - zero-padded for
     * stable ordering; formats WITH a decoded og rule name via
     * CarveResult::embedded_name before this fallback. */
    int n = snprintf(buf, sizeof(buf), "File%06u.%s", index, sig.ext);
    if (n <= 0)
        buf[0] = 0;
    std::u16string out;
    for (const char* p = buf; *p; ++p)
        out.push_back(u16(u8(*p)));
    return out;
}

u32 carve_slice(IStorageDevice* slice, const CarveFlags& flags, CancelToken* cancel,
                bool (*progress)(u64, u64, void*), void* progress_user, tree::ResultTree& tree,
                tree::FullNode* carve_root, const tree::ResultTree* dedup_tree,
                const NameCallback& namer, CarveStats& stats) {
    if (slice == nullptr || carve_root == nullptr)
        return ERR_BAD_PARAMETER;
    if (validate_registry(nullptr) != 0)
        return ERR_BAD_PARAMETER; /* F8 gate */

    ScanCtx ctx;
    ctx.slice = slice;
    ctx.reg = registry();
    ctx.flags = &flags;
    ctx.tree = &tree;
    ctx.carve_root = carve_root;
    ctx.dedup_tree = dedup_tree;
    ctx.namer = namer;
    ctx.total = slice->size();
    ctx.ext_base = slice->absolute_base(); /* GAP-S6-8: disk-absolute basis */
    /* M-005: the allocation-bitmap gate (nullable; slice-relative basis -
     * the orchestrator rebases stats.free_map before the call) */
    ctx.free_map = flags.free_map;
    ctx.occupied = flags.occupied_ranges;
    /* M-024: build the per-format disable maps from the host list (one
     * pass; the hot loops then test a single bool). The grammar layer
     * gates by trial slot; the raw-sign layer gates by format index. */
    if (flags.disabled_formats != nullptr && *flags.disabled_formats != 0) {
        static_assert(kTrialCount <= 96, "grow trial_disabled with the trial table");
        for (u32 ti = 0; ti < kTrialCount; ++ti)
            ctx.trial_disabled[ti] =
                carve_format_listed(flags.disabled_formats, ctx.reg[kTrialOrder[ti]]);
        const rawsigns::SignIndex& idx = rawsigns::SignIndex::get();
        ctx.rawsign_disabled.assign(rawsigns::kFormatCount, false);
        for (u32 fi = 0; fi < rawsigns::kFormatCount; ++fi)
            ctx.rawsign_disabled[fi] =
                carve_token_listed(flags.disabled_formats, idx.format(fi).ext, nullptr);
    }
    /* S-031 residual: stamp the scan time once per run — every carved
     * record carries it (the og's per-record formatted timestamp). */
    ctx.scan_time = unix_to_filetime(i64(std::time(nullptr)));

    const u64 window_count = (ctx.total + WINDOW - 1) / WINDOW;
    const bool multi = flags.threads > 1 && window_count > 1 && ctx.total > 0;

    if (!multi) {
        /* single-threaded path: identical semantics to the original scan */
        std::set<u64> emitted_at;
        for (u64 wi = 0; wi < window_count; ++wi) {
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            if (progress && !progress(wi * WINDOW, ctx.total, progress_user))
                return ERR_CANCELLED;
            if (u32 rc = scan_window_index(ctx, wi, emitted_at))
                return rc;
        }
        if (progress)
            (void)progress(ctx.total, ctx.total, progress_user);
    } else {
        /* parallel path: fan window indices out over a bounded pool */
        ThreadPool pool(flags.threads);
        PoolCancelToken pool_cancel(&pool);
        std::atomic<u64> windows_done{0};
        const u64 total = ctx.total;

        auto worker_job = [&](u64 wi) -> u32 {
            /* pause-aware gate (G-07): a PausableToken freezes carve workers
             * here; plain tokens behave exactly like the old poll. */
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            if (pool.aborted())
                return ERR_CANCELLED;
            std::set<u64> emitted_at; /* per-worker overlap set */
            u32 rc = scan_window_index(ctx, wi, emitted_at);
            const u64 done = windows_done.fetch_add(1, std::memory_order_relaxed) + 1;
            if (progress &&
                !progress(done * WINDOW > total ? total : done * WINDOW, total, progress_user)) {
                pool.abort();
                return ERR_CANCELLED;
            }
            return rc;
        };

        /* main-thread watch loop: submit all windows, watch external cancel */
        u32 first_err = OK;
        if (cancel) {
            /* poll cancel while submitting/running (N-017: the factored
             * CancelPoolWatcher - identical 50 ms abort semantics) */
            CancelPoolWatcher watcher(pool, cancel);
            first_err = parallel_for(pool, window_count, worker_job);
            watcher.done();
            if (cancel->cancelled() && first_err == OK)
                first_err = ERR_CANCELLED;
        } else {
            first_err = parallel_for(pool, window_count, worker_job);
        }
        if (first_err != OK)
            return first_err;
        if (progress)
            (void)progress(total, total, progress_user);
    }

    stats.hits += ctx.hits.load();
    stats.validated += ctx.validated.load();
    stats.rejected += ctx.rejected.load();
    stats.duplicates += ctx.duplicates.load();
    stats.hashed += ctx.hashed.load();
    stats.bytes_scanned += ctx.bytes_scanned.load();
    /* PARITY-AUDIT F-A3-6: sign_hits/sign_validated were each added TWICE
     * (copy-paste duplicate lines) — every raw-sign statistic reported
     * double its true value. */
    stats.sign_hits += ctx.sign_hits.load();
    stats.sign_validated += ctx.sign_validated.load();
    stats.windows_skipped += ctx.windows_skipped.load();
    /* N-031: the zero-skip telemetry */
    stats.zero_strides_skipped += ctx.zero_strides_skipped.load();
    /* M-005: gate telemetry */
    stats.bitmap_skipped_clusters += ctx.bitmap_skipped_clusters.load();
    /* M-039: occupancy-gate telemetry */
    stats.occupancy_skipped += ctx.occupancy_skipped.load();
    stats.bitmap_skipped_bytes =
        (flags.free_map && flags.free_map->valid())
            ? stats.bitmap_skipped_clusters * u64(flags.free_map->cluster_bytes)
            : 0;
    return OK;
}

} // namespace carve
} // namespace cr
