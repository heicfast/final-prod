/* cr_transport.cpp - the Windows transport layer: DIRECT raw I/O under
 * administrator privilege (session-96: the kernel-driver replacement),
 * with the overlapped read-ahead ring (2026-11-18 performance session:
 * PERF-LEVERS F2/F10 opened - the og's prefetch-thread model generalized).
 *
 * Direct wiring (\\.\PhysicalDriveN, no kernel component of ours):
 *   - open_disk / open_disk_rw / open_disk_apply_rw: the direct whole-disk
 *     opens (GENERIC_READ [ | GENERIC_WRITE], share R|W|D, OPEN_EXISTING) -
 *     the exact posture the grammar path used, one layer down
 *   - true byte counts: ReadFile/WriteFile report real transferred bytes
 *     (the driver-era F11 Information=Length*512 scaling is deleted with
 *     the driver)
 *   - the READ data path (DirectDevice) now opens FILE_FLAG_OVERLAPPED
 *     and serves every read through the AsyncRing (IOCP + sliding
 *     window of N outstanding 4 MiB-class 64-KiB-aligned reads,
 *     refill-on-consume): the og's dedicated prefetch thread + 10-run
 *     x 4 MiB lookahead (DRWUI:919088-919142) generalized one layer
 *     down, without the og's driver-era sector-unit grammar. Queue
 *     depth = the window; concurrent read_at on the one handle is
 *     documented-safe (unique OVERLAPPED per call).
 *   - the S-010 quiesce chain and the S-029 device-control chain operate
 *     on direct handles (the write side RwDevice stays synchronous:
 *     repair/apply writes are single-flight by their own contracts)
 *   - writes staged through a sector-aligned buffer when the caller's is
 *     unaligned (the direct-write rule: the storage stack requires
 *     sector-aligned buffers on raw handles)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_transport.h"
#include "cr/win/cr_enumerate.h" /* S-010: volume enumeration for the lock sequence */
#include "cr/win/cr_winenv.h"    /* M-030: the quiesce-chain pins */
#include "cr/cr_log.h"           /* N-048 seam: the OFFLINE failure line */
#include "cr/table/cr_table_parse.h"
#include "cr/storage/cr_directalign.h"
#include <windows.h>
#include <winioctl.h>
#include <shlwapi.h>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace cr {
namespace win {

namespace {

/* The ring tuning seam (cr_transport.h): tests and the bench lane write
 * these BEFORE open; 0 selects the production default. */
std::atomic<u32> g_ring_run{0};   /* 0 -> 4 MiB (the og's run size) */
std::atomic<u32> g_ring_slots{0}; /* 0 -> 8 slots (32 MiB window) */

/* ---------------------------------------------------------------------------
 * Shared geometry probing.
 * ------------------------------------------------------------------------- */

/* (W4, AUDIT-ARCH 8c #3: a local u32_from_bytes_le re-implementation was
 * deleted here - the identical cr_types.h fold is used unqualified.) */

/* Shared OVERLAPPED read completion (W3-10, AUDIT-ARCH 8c #1): one
 * spelling of the ReadFile + ERROR_IO_PENDING + GetOverlappedResult
 * dance. Direct handles report true byte counts: success is got == len
 * exactly (the driver-era F11 Information scaling is gone with the
 * driver). */
u32 overlapped_read(HANDLE h, u64 off, void* buf, u32 len, DWORD* gle_out) {
    if (len == 0)
        return OK;
    OVERLAPPED ov = {};
    ov.Offset = u32(off);
    ov.OffsetHigh = u32(off >> 32);
    /* hEvent: REQUIRED for correct concurrent waits on overlapped
     * handles (completions with a NULL hEvent signal the FILE HANDLE,
     * waking every waiter - the documented "incorrectly report that the
     * read operation is complete" hazard). Per-thread event: one
     * overlapped_read executes at a time per thread, so a thread-local
     * event is uniquely owned per in-flight call. */
    static thread_local HANDLE t_ev = [] {
        HANDLE e = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        return e;
    }();
    ov.hEvent = t_ev;
    ResetEvent(t_ev);
    DWORD got = 0;
    if (!ReadFile(h, buf, len, &got, &ov)) {
        const DWORD e = GetLastError();
        if (e != ERROR_IO_PENDING) {
            if (gle_out)
                *gle_out = e;
            /* N-125: the device-gone family classifies distinctly (the
             * reattach-then-resume contract); media errors stay
             * ERR_READ_FAILED (the dying-disk storm keeps its semantics) */
            return disconnect::gle_is_disconnect(e) ? ERR_DEVICE_DISCONNECTED : ERR_READ_FAILED;
        }
        if (!GetOverlappedResult(h, &ov, &got, TRUE)) {
            const DWORD e2 = GetLastError();
            if (gle_out)
                *gle_out = e2;
            return disconnect::gle_is_disconnect(e2) ? ERR_DEVICE_DISCONNECTED : ERR_READ_FAILED;
        }
    }
    if (got == len)
        return OK;
    return ERR_READ_FAILED;
}

/* Exact byte length for volume AND disk handles (IOCTL_DISK_GET_LENGTH_INFO
 * works on both; DISK_GEOMETRY_EX rounds to the disk geometry). */
bool probe_length(HANDLE h, u64* out, bool overlapped = false) {
    /* Overlapped handles (the ring's data path) MUST receive a valid
     * OVERLAPPED from DeviceIoControl (the documented requirement); the
     * wait below runs BEFORE any IOCP association exists, so the
     * synchronous GetOverlappedResult shape is safe. Sync handles take
     * the null-overlapped call unchanged. */
    GET_LENGTH_INFORMATION li = {};
    DWORD br = 0;
    OVERLAPPED ov = {};
    OVERLAPPED* ovp = overlapped ? &ov : nullptr;
    if (DeviceIoControl(h, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &li, sizeof(li), &br, ovp)) {
        *out = u64(li.Length.QuadPart);
        return true;
    }
    if (overlapped && GetLastError() == ERROR_IO_PENDING) {
        if (GetOverlappedResult(h, &ov, &br, TRUE)) {
            *out = u64(li.Length.QuadPart);
            return true;
        }
    }
    return false;
}

Geometry probe_geometry(HANDLE h, u64* out_size, bool overlapped = false) {
    Geometry g{};
    DISK_GEOMETRY_EX dg = {};
    DWORD br = 0;
    OVERLAPPED ov = {};
    OVERLAPPED* ovp = overlapped ? &ov : nullptr;
    u64 exact = 0;
    const bool have_exact = probe_length(h, &exact, overlapped);
    if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &dg, sizeof(dg), &br,
                        ovp)) {
        g.logical_sector = dg.Geometry.BytesPerSector;
        g.physical_sector = dg.Geometry.BytesPerSector;
        /* corrected via the alignment query below (512e/4Kn) */
        STORAGE_PROPERTY_QUERY q = {};
        q.PropertyId = StorageAccessAlignmentProperty;
        q.QueryType = PropertyStandardQuery;
        u8 buf[4096];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf), &br,
                            ovp)) {
            /* STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR authoritative layout
             * (winioctl.h / MS-DDK): Version@0, Size@4, BytesPerCacheLine@8,
             * BytesOffsetForCacheAlignment@0xC, BytesPerLogicalSector@0x10,
             * BytesPerPhysicalSector@0x14, BytesOffsetForSectorAlignment@0x18.
             * The previous code read +8/+12 - the CACHE-LINE fields - so
             * on VHDs (cache line 0) every direct-path device reported
             * logical_sector = 0 and each PartitionSlice creation for a
             * volume scan failed (both benchmark legs, slice create
             * ERR_BAD_PARAMETER). Values accepted only in a sane sector
             * range; otherwise the geometry-EX value stands. */
            const u8* d = buf;
            const u32 align_logical = u32_from_bytes_le(d + 0x10);
            const u32 align_physical = u32_from_bytes_le(d + 0x14);
            if (align_logical >= 256 && align_logical <= 65536)
                g.logical_sector = align_logical;
            if (align_physical >= 256 && align_physical <= 65536)
                g.physical_sector = align_physical;
        } else {
            g.physical_sector = g.logical_sector;
        }
        g.total_bytes = have_exact ? exact : u64(dg.DiskSize.QuadPart);
        /* N-206: the og's geometry-EX CEIL (Device_decompiled.c:56536-
         * 56538: (L-1+ss)/ss - the 0x7405C result overwriting the
         * 0x74004 floor): the final partial sector of an odd-sized source
         * is addressable, driver-padded. */
        const u32 ss = g.logical_sector ? g.logical_sector : 512;
        g.sector_count = (g.total_bytes + ss - 1) / ss;
    } else if (have_exact) {
        /* volume handles on some stacks: geometry IOCTL not forwarded */
        g.logical_sector = 512;
        g.physical_sector = 512;
        g.total_bytes = exact;
        /* N-206: the ceil form here too (the og's shipped behavior is
         * the geometry-EX overwrite; this fallback mirrors it) */
        g.sector_count = (exact + 511) / 512;
    }
    if (out_size)
        *out_size = g.total_bytes;
    return g;
}

/* ---------------------------------------------------------------------------
 * THE 64 KiB DIRECT-READ ENVELOPE - UNIFIED across BOTH transports
 * (core-api session; mandate section 3, non-negotiable).
 *
 * Two original constraints, one envelope:
 *
 * (a) Sector rejection (bench run 40073093997): the Windows disk stack
 *     rejects ReadFile IRPs whose LENGTH or offset is not a multiple of
 *     the logical sector size - evidence: every recovered file with
 *     size % 512 != 0 was zero-filled 'salvaged' while all sector-multiple
 *     files hash-verified, on BOTH transports. 64 KiB is a multiple of
 *     every real Windows logical sector size (512 / 4096 / 65536), so the
 *     64 KiB envelope subsumes sector alignment.
 *
 * (b) GAP-S6-13 (mandate section 4): direct reads whose length is a
 *     multiple of 4 KiB but not 64 KiB take the ~3.5x slow path in the
 *     Windows storage stack (~830 MB/s vs ~2900 MB/s on NVMe). The
 *     mandate requires this property on EVERY direct read - through the
 *     driver OR through the direct fallback. The corpus confirms the
 *     reference satisfies it by construction: every chunk size the og
 *     issues is a 64 KiB multiple (4 MiB device chunks
 *     Device_decompiled.c:56545, 7 MiB volume chunks :56912, 2 MiB MFT
 *     batches NTFSSearchDLL_decompiled.c:5649-5678, 1 MiB $UsnJrnl, 64 KiB
 *     boot region).
 *
 * The pre-unification RDA path used a sector-only envelope (the 'kernel
 * IRP path is exempt' argument - technically true for the cache-manager
 * slow path, but it left the mandate's property unproven for the driver
 * transport and let probe/tail reads issue sector-granular lengths).
 * Unified: BOTH paths round offset down / length up to 64 KiB multiples,
 * clamped at the device end; already-aligned requests pass through with
 * no bounce copy. The F11 Information-scale compensation is orthogonal
 * (it keys on the ISSUED length, not the caller's).
 *
 * Threading: since the 2026-11-18 performance session the engine pipe's
 * readers reach the transport CONCURRENTLY (the sharded CachedReader
 * fills outside its registry locks - cr_cached.cpp). The data-path
 * handle is FILE_FLAG_OVERLAPPED and every read rides the AsyncRing
 * (unique OVERLAPPED per issued read: the documented-safe concurrent
 * shape); this aligned_read_at helper is now the WRITE-side (RwDevice)
 * and degraded-path reader, whose callers are single-flight by their
 * own contracts (repair/apply) - the per-thread bounce below tolerates
 * any concurrent stragglers.
 * ------------------------------------------------------------------------ */
u32 aligned_read_at(HANDLE h, u64 off, void* buf, u32 len, u64 dev_size, DWORD* gle_out) {
    if (len == 0)
        return OK;
    const io::DirectEnvelope e = io::direct_envelope(off, len, dev_size);
    if (!e.ok)
        return ERR_RANGE_OUT_OF_BOUNDS;
    if (e.direct)
        return overlapped_read(h, off, buf, len, gle_out); /* fast path: no copy */
    /* bounded bounce: the writer chunks at 1 MiB; anything bigger means
     * caller-side chunking broke - refuse loudly instead of allocating
     * wildly */
    if (e.issued_len > (1u << 20) + io::kDirectAlignBytes)
        return ERR_BAD_PARAMETER;
    /* per-thread bounce (raw-handle reads can arrive from multiple
     * worker items concurrently) */
    thread_local std::vector<u8> bounce;
    bounce.resize(size_t(e.issued_len));
    const u32 rc = overlapped_read(h, e.issued_off, bounce.data(), u32(e.issued_len), gle_out);
    if (rc != OK)
        return rc;
    if (e.take != u64(len))
        return ERR_READ_FAILED; /* envelope did not cover the request */
    std::memcpy(buf, bounce.data() + size_t(e.skip), size_t(e.take));
    return OK;
}

/* ---------------------------------------------------------------------------
 * THE OVERLAPPED READ-AHEAD RING (2026-11-18 performance session).
 *
 * The og's latency-hiding, generalized: their CStorageDeviceWithCache runs a
 * dedicated prefetch thread with a 10-run look-ahead ring of 4 MiB runs
 * (DRWUI_decompiled.c:919088-919142, Device:56545) on top of synchronous
 * QD1 ReadFile. Ours: the data-path handle is FILE_FLAG_OVERLAPPED and
 * reads are served from a sliding window of outstanding overlapped reads,
 * completed through ONE I/O completion port served by ONE completion thread
 * (GetQueuedCompletionStatus; NumberOfConcurrentThreads=1 - a completion
 * here is mark-state + SetEvent, trivially serial), with per-slot
 * manual-reset events and lock-free predicate waits. GetOverlappedResult
 * is NEVER called on the port-associated handle (the documented
 * IOCP+GetOverlappedResult hazard: the port thread may dequeue the
 * completion the waiter would wait on).
 *
 *   - refill-on-consume: after a slot's bytes are consumed, the next run
 *     after the read cursor is issued on a free slot (follow-the-cursor -
 *     the og's "CacheThread find read sector" policy, vendor log
 *     DRWUI:962634).
 *   - pivot-on-miss: a read outside the window issues at its own 64 KiB
 *     floor (random-access streams degrade to sync-like behavior, never
 *     worse than the pre-ring transport).
 *   - every ISSUED read: 64 KiB-multiple length at a 64 KiB-floor offset
 *     (the GAP-S6-13 mandate property), clamped at the device tail (the
 *     documented tail exception).
 *   - concurrency: multiple reader threads share the ring (the sharded
 *     CachedReader fills concurrently above). Lock order is one-way:
 *     ring_mu_ THEN slot.mu, never the reverse; waits sleep on events,
 *     never on a lock held across I/O. Slot claims happen under
 *     ring_mu_ (no double-issue race); ReadFile queueing happens with
 *     no lock held.
 *   - teardown: CancelIoEx per pending slot, stop post, join, drain,
 *     free (the documented OVERLAPPED-lifetime discipline: no slot
 *     memory goes away before its completions are observed).
 *   - heal: rebind() re-associates a reopened handle onto the same port
 *     and resets the slots (the DirectDevice error-6 retry path).
 * ------------------------------------------------------------------------ */
class AsyncRing final {
public:
    struct Slot {
        enum State : int { kIdle = 0, kInflight = 1, kDone = 2, kFailed = 3 };
        /* OVERLAPPED first: slot_from_ov() is a container_of. */
        OVERLAPPED ov;
        HANDLE ev = nullptr; /* manual-reset: wakes all predicate waiters */
        u8* buf = nullptr;   /* VirtualAlloc'd page-aligned, run_ capacity */
        u64 off = 0;         /* claimed issued offset (64 KiB floor) */
        u32 issued = 0;      /* claimed issued length (tail-clamped) */
        std::atomic<int> state{kIdle};
        std::atomic<u64> used_seq{0}; /* LRU clock for DONE-slot reclaim */
        u32 got = 0;                  /* completed byte count (published before state) */
        DWORD gle = 0;                /* failing GLE (published before state) */
        std::mutex mu;                /* claim/serve/retire serialization: holding it
                                         excludes a concurrent reclaim of this slot
                                         (claim_slot_locked takes it before repurposing
                                         the buffer), so a server copying under the lock
                                         can never race a fresh ReadFile into the same
                                         buffer */
    };

    AsyncRing(HANDLE h, u64 dev_size, u32 run_bytes, u32 slot_count) {
        u64 run = run_bytes ? run_bytes : 4ull * 1024 * 1024;
        const u64 kA = io::kDirectAlignBytes;
        run = (run / kA) * kA; /* 64 KiB multiple (at least one below) */
        if (run < kA)
            run = kA;
        if (dev_size < run) { /* small device: fit the run to it */
            run = (dev_size / kA) * kA;
            if (run < kA)
                run = kA;
        }
        run_ = u32(run);
        u64 want_slots = slot_count ? slot_count : 8;
        const u64 by_dev = (dev_size + run - 1) / run;
        if (want_slots > by_dev)
            want_slots = by_dev;
        if (want_slots < 1)
            want_slots = 1;
        h_.store(h);
        dev_size_ = dev_size;
        cursor_.store(0);
        port_ = CreateIoCompletionPort(h, nullptr, 0 /*device key*/, 1 /*concurrency*/);
        if (port_ == nullptr)
            return; /* ring disabled: DirectDevice falls back to the sync path */
        for (u64 i = 0; i < want_slots; ++i) {
            auto sp = std::make_unique<Slot>();
            sp->ev = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            sp->buf = static_cast<u8*>(
                VirtualAlloc(nullptr, run_, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            if (sp->ev && sp->buf)
                slots_.push_back(std::move(sp));
        }
        if (slots_.empty()) {
            CloseHandle(port_); /* disabled ring: no handle leaks */
            port_ = nullptr;
            return;
        }
        worker_ = std::thread([this] { completion_loop(); });
        active_.store(true);
    }

    ~AsyncRing() { stop(); }

    bool active() const { return active_.load(); }

    /* Heal: associate the reopened handle onto the same port, reset all
     * slots (inflight ones were CancelIoEx'd; their aborted completions
     * are drained here; the worker keeps serving). */
    bool rebind(HANDLE nh) {
        std::lock_guard<std::mutex> g(ring_mu_);
        if (!active_.load() || nh == nullptr || nh == INVALID_HANDLE_VALUE)
            return false;
        cancel_inflight_locked();
        drain_port_locked();
        if (CreateIoCompletionPort(nh, port_, 0, 1) == nullptr)
            return false;
        h_.store(nh);
        for (auto& sp : slots_)
            reset_slot_locked(*sp);
        return true;
    }

    /* Serve [off, off+len). Returns the transport rc; *gle_out carries
     * the failing GLE for the DirectDevice heal decision. */
    u32 read(u64 off, void* buf, u32 len, u64 dev_size, DWORD* gle_out) {
        u8* dst = static_cast<u8*>(buf);
        u64 pos = off;
        u32 remain = len;
        while (remain > 0) {
            Slot* s = nullptr;
            bool do_issue = false;
            {
                std::lock_guard<std::mutex> g(ring_mu_);
                const u32 take = min_u32(remain, run_);
                /* 1. DONE slot fully covering the next slice */
                for (auto& sp : slots_) {
                    if (sp->state.load(std::memory_order_acquire) == Slot::kDone &&
                        sp->off <= pos && pos + take <= sp->off + sp->got) {
                        s = sp.get();
                        break;
                    }
                }
                /* 2. INFLIGHT slot whose issued range covers it: wait (the
                 * read-ahead payoff - the consumer-blocks-on-run model) */
                if (!s) {
                    for (auto& sp : slots_) {
                        if (sp->state.load(std::memory_order_acquire) == Slot::kInflight &&
                            sp->off <= pos && pos + take <= sp->off + sp->issued) {
                            s = sp.get();
                            break;
                        }
                    }
                }
                /* 3. pivot: claim a slot at this request's 64 KiB floor */
                if (!s) {
                    s = pick_victim_locked();
                    if (!s)
                        return ERR_NO_MEMORY;
                    const u64 issue_off = (pos / io::kDirectAlignBytes) * io::kDirectAlignBytes;
                    u64 want_end = issue_off + run_;
                    const u64 need_end = pos + take;
                    if (want_end < need_end)
                        want_end = need_end;
                    if (want_end > dev_size)
                        want_end = dev_size; /* the documented tail clamp */
                    if (want_end <= issue_off)
                        return ERR_RANGE_OUT_OF_BOUNDS;
                    const u32 issue_len = u32(want_end - issue_off);
                    claim_slot_locked(*s, issue_off, issue_len);
                    do_issue = true;
                }
            }

            if (do_issue && !issue(*s)) {
                /* issue-side hard failure (not pending): the slot is
                 * already FAILED with its gle published; report like a
                 * completion failure (the heal path rides the gle). */
                if (gle_out)
                    *gle_out = s->gle;
                return ERR_READ_FAILED;
            }

            /* lock-free predicate wait: the terminal publish (state
             * release-store after got/gle writes) makes the event a pure
             * wake mechanism; NO lock is held while sleeping. */
            for (;;) {
                if (s->state.load(std::memory_order_acquire) != Slot::kInflight)
                    break;
                WaitForSingleObject(s->ev, INFINITE);
            }
            /* serve under the slot lock: excludes a concurrent reclaim
             * (a pivot claiming this slot as victim takes the same lock
             * BEFORE repurposing the buffer), and re-validates the slot
             * still holds OUR bytes - a found-DONE slot can have been
             * claimed-and-reissued between our ring-lock release and
             * here (state INFLIGHT again, or a new off). */
            bool retry = false;
            {
                std::lock_guard<std::mutex> g(s->mu);
                const int st = s->state.load(std::memory_order_acquire);
                if (st == Slot::kFailed) {
                    const DWORD g2 = s->gle;
                    if (gle_out)
                        *gle_out = g2;
                    ResetEvent(s->ev);
                    s->state.store(Slot::kIdle, std::memory_order_release);
                    return disconnect::gle_is_disconnect(g2) ? ERR_DEVICE_DISCONNECTED
                                                             : ERR_READ_FAILED;
                }
                if (st != Slot::kDone || s->off > pos || s->off + s->got <= pos) {
                    /* reclaimed/repurposed under us (or completed short of
                     * this request): re-select from the top - the slot's
                     * new state is another reader's business, never ours */
                    retry = true;
                } else {
                    const u32 take = min_u32(remain, u32(s->off + s->got - pos));
                    std::memcpy(dst, s->buf + (pos - s->off), take);
                    dst += take;
                    pos += take;
                    remain -= take;
                    s->used_seq.fetch_add(1, std::memory_order_relaxed);
                }
            }
            if (retry)
                continue;
            advance_cursor(pos);
            /* refill-on-consume: the next run after the cursor on a free
             * slot (NO slot lock held here - one-way lock order). */
            kick_readahead();
        }
        return OK;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> g(ring_mu_);
            if (!active_.exchange(false))
                return;
            cancel_inflight_locked();
        }
        if (worker_.joinable()) {
            PostQueuedCompletionStatus(port_, 0, kStopKey, nullptr);
            worker_.join();
        }
        {
            std::lock_guard<std::mutex> g(ring_mu_);
            drain_port_locked();
            for (auto& sp : slots_) {
                if (sp->ev)
                    CloseHandle(sp->ev);
                if (sp->buf)
                    VirtualFree(sp->buf, 0, MEM_RELEASE);
            }
            slots_.clear();
            if (port_)
                CloseHandle(port_);
            port_ = nullptr;
        }
    }

private:
    static constexpr ULONG_PTR kStopKey = 1; /* any value: identified by ov==NULL */

    void completion_loop() {
        for (;;) {
            DWORD bytes = 0;
            ULONG_PTR key = 0;
            OVERLAPPED* ov = nullptr;
            const BOOL ok = GetQueuedCompletionStatus(port_, &bytes, &key, &ov, INFINITE);
            if (ov == nullptr)
                break;                                 /* the stop post, or a hard port failure */
            const DWORD gle = ok ? 0 : GetLastError(); /* captured immediately:
                                                            no intervening call clobbers it */
            Slot* s = slot_from_ov(ov);
            {
                std::lock_guard<std::mutex> lg(s->mu);
                if (ok)
                    s->got = bytes;
                else
                    s->gle = gle;
            }
            /* got/gle are published BEFORE the state release-store */
            s->state.store(ok ? Slot::kDone : Slot::kFailed, std::memory_order_release);
            SetEvent(s->ev);
        }
    }

    static Slot* slot_from_ov(OVERLAPPED* ov) {
        return reinterpret_cast<Slot*>(reinterpret_cast<char*>(ov) - offsetof(Slot, ov));
    }

    /* Ring lock held: mark a slot claimed (IDLE/FAILED/DONE -> INFLIGHT
     * with its issued coordinates). The claim is the double-issue guard:
     * only a ring-lock holder transitions a slot to INFLIGHT. */
    void claim_slot_locked(Slot& s, u64 a_off, u32 ilen) {
        std::lock_guard<std::mutex> g(s.mu);
        ResetEvent(s.ev);
        s.ov = OVERLAPPED{};
        s.ov.Offset = u32(a_off);
        s.ov.OffsetHigh = u32(a_off >> 32);
        s.ov.hEvent = s.ev;
        s.off = a_off;
        s.issued = ilen;
        s.got = 0;
        s.gle = 0;
        s.state.store(Slot::kInflight, std::memory_order_release);
    }

    /* Queue the ReadFile for an already-claimed slot. No lock held (and
     * none taken: the claim already serialized this slot). */
    bool issue(Slot& s) {
        BOOL ok = ReadFile(h_.load(), s.buf, s.issued, nullptr, &s.ov);
        if (!ok) {
            const DWORD e = GetLastError();
            if (e != ERROR_IO_PENDING) {
                std::lock_guard<std::mutex> g(s.mu);
                s.gle = e;
                s.state.store(Slot::kFailed, std::memory_order_release);
                SetEvent(s.ev);
                return false;
            }
        }
        return true; /* pending - or completed synchronously: the port
                        still queues the completion packet */
    }

    /* Ring lock held: pick the reclaim target. IDLE first, then FAILED,
     * then the least-recently-used DONE. INFLIGHT never (waiters). */
    Slot* pick_victim_locked() {
        Slot* failed = nullptr;
        Slot* oldest_done = nullptr;
        u64 oldest_seq = ~u64(0);
        for (auto& sp : slots_) {
            const int st = sp->state.load(std::memory_order_acquire);
            if (st == Slot::kIdle)
                return sp.get();
            if (st == Slot::kFailed && !failed)
                failed = sp.get();
            if (st == Slot::kDone) {
                const u64 seq = sp->used_seq.load(std::memory_order_relaxed);
                if (!oldest_done || seq < oldest_seq) {
                    oldest_done = sp.get();
                    oldest_seq = seq;
                }
            }
        }
        return failed ? failed : oldest_done;
    }

    /* Refill-on-consume: issue the next run after the cursor on a free
     * slot. Ring lock for the scan/claim (claim takes the slot lock -
     * one-way order); ReadFile queues with no lock held. */
    void kick_readahead() {
        std::lock_guard<std::mutex> g(ring_mu_);
        if (!active_.load())
            return;
        const u64 next = (cursor_.load(std::memory_order_relaxed) / io::kDirectAlignBytes) *
                         io::kDirectAlignBytes;
        if (next >= dev_size_)
            return;
        for (auto& sp : slots_) {
            const int st = sp->state.load(std::memory_order_acquire);
            if (st == Slot::kInflight || st == Slot::kDone) {
                if (sp->off == next)
                    return; /* the window already leads here */
                continue;
            }
            u64 end = next + run_;
            if (end > dev_size_)
                end = dev_size_;
            if (end <= next)
                return;
            claim_slot_locked(*sp, next, u32(end - next));
            issue(*sp);
            return; /* one kick per consume */
        }
    }

    void advance_cursor(u64 end) {
        u64 cur = cursor_.load(std::memory_order_relaxed);
        while (end > cur && !cursor_.compare_exchange_weak(cur, end))
            ;
    }

    void reset_slot_locked(Slot& s) {
        std::lock_guard<std::mutex> g(s.mu);
        ResetEvent(s.ev);
        s.got = 0;
        s.gle = 0;
        s.off = 0;
        s.issued = 0;
        s.state.store(Slot::kIdle, std::memory_order_release);
    }

    void cancel_inflight_locked() {
        for (auto& sp : slots_) {
            if (sp->state.load(std::memory_order_acquire) == Slot::kInflight)
                CancelIoEx(h_.load(), &sp->ov);
        }
    }

    /* Non-blocking drain of the port queue: marks aborted completions
     * terminal. Used under ring_mu_ (slot lock per packet - one-way). */
    void drain_port_locked() {
        for (;;) {
            DWORD bytes = 0;
            ULONG_PTR key = 0;
            OVERLAPPED* ov = nullptr;
            const BOOL ok = GetQueuedCompletionStatus(port_, &bytes, &key, &ov, 0);
            if (ov == nullptr)
                break; /* empty (ERROR_TIMEOUT) or the stop post */
            const DWORD gle = ok ? 0 : GetLastError();
            Slot* s = slot_from_ov(ov);
            {
                std::lock_guard<std::mutex> lg(s->mu);
                if (ok)
                    s->got = bytes;
                else
                    s->gle = gle;
            }
            s->state.store(ok ? Slot::kDone : Slot::kFailed, std::memory_order_release);
            SetEvent(s->ev);
        }
    }

    static u32 min_u32(u64 a, u64 b) { return u32(a < b ? a : b); }

    std::atomic<HANDLE> h_{nullptr};
    u64 dev_size_ = 0;
    u32 run_ = 4u * 1024 * 1024;
    HANDLE port_ = nullptr;
    std::vector<std::unique_ptr<Slot>> slots_;
    std::thread worker_;
    std::mutex ring_mu_;
    std::atomic<bool> active_{false};
    std::atomic<u64> cursor_{0}; /* read-ahead anchor: advances with consumption */
};

/* ---------------------------------------------------------------------------
 * Handle self-healing (og parity, core-api session).
 *
 * The reference's device layer recovers a dead handle IN PLACE: on
 * ERROR_INVALID_HANDLE (6) during the read loop it CloseHandle's, re-
 * CreateFile's the same target and retries the seek
 * (Device_decompiled.c:55947-55959, CWindowsStorageDevice::Read; the
 * same guard pattern rides the write loop :56182-56183). Without this,
 * our retry layer cannot heal a stale handle (every attempt fails
 * identically, bisects, and Skip-mode zero-fills data that a reopen
 * would have recovered).
 *
 * One heal attempt per failing call; the reopen keeps the ORIGINAL open
 * parameters (path / access / share - the og reopens with its clone
 * default 0xC0000000 share-3; we keep the per-site flags, which is the
 * same semantics: a fresh handle to the same target). Geometry is NOT
 * re-probed (the og doesn't either). heal_mu_ serializes concurrent
 * healers; the winning reopen swaps the handle under the lock and the
 * loser's retry rides the fresh handle too.
 * ------------------------------------------------------------------------ */
bool reopen_handle(const wchar_t* path, DWORD access, DWORD share, HANDLE* h_inout) {
    if (!path || !path[0])
        return false;
    HANDLE nh =
        CreateFileW(path, access, share, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nh == INVALID_HANDLE_VALUE)
        return false;
    HANDLE old = *h_inout;
    *h_inout = nh;
    if (old != INVALID_HANDLE_VALUE && old != nh)
        CloseHandle(old);
    return true;
}

/* The overlapped twin (the ring's handles reopen FILE_FLAG_OVERLAPPED -
 * reopening without the flag would put the healed handle back on the
 * serialized I/O-manager path). */
bool reopen_handle_ov(const wchar_t* path, DWORD access, DWORD share, HANDLE* h_inout) {
    if (!path || !path[0])
        return false;
    HANDLE nh =
        CreateFileW(path, access, share, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (nh == INVALID_HANDLE_VALUE)
        return false;
    HANDLE old = *h_inout;
    *h_inout = nh;
    if (old != INVALID_HANDLE_VALUE && old != nh)
        CloseHandle(old);
    return true;
}

/* ---------------------------------------------------------------------------
 * Direct device: true byte counts from the IO manager. THE data-path
 * device of the direct era (the RDA grammar-path twin was removed with
 * the driver - session-96).
 * ------------------------------------------------------------------------- */
class DirectDevice final : public IStorageDevice {
public:
    DirectDevice(HANDLE h, Geometry g, std::wstring path, DWORD access, DWORD share)
        : h_(h), geom_(std::move(g)), path_(std::move(path)), access_(access), share_(share) {
        /* 2026-11-18: the data path rides the overlapped read-ahead ring
         * (FILE_FLAG_OVERLAPPED handles + IOCP sliding window - the og's
         * prefetch model generalized). The handle MUST have been opened
         * with FILE_FLAG_OVERLAPPED by the open site (open_path); the
         * ring construction is best-effort: on failure (no memory for
         * the 32 MiB window) the device degrades to the pre-ring sync
         * envelope path - honest, never a silent stall. */
        ring_ = std::make_unique<AsyncRing>(h, geom_.total_bytes,
                                            g_ring_run.load(std::memory_order_relaxed),
                                            g_ring_slots.load(std::memory_order_relaxed));
    }
    ~DirectDevice() override {
        if (ring_)
            ring_->stop(); /* cancels, drains, joins - before the handle dies */
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        DWORD gle = 0;
        u32 rc = OK;
        if (ring_ && ring_->active()) {
            rc = ring_->read(off, buf, len, size(), &gle);
            if (rc == ERR_READ_FAILED && gle == ERROR_INVALID_HANDLE) {
                std::lock_guard<std::mutex> g(heal_mu_);
                if (reopen_handle_ov(path_.c_str(), access_, share_, &h_) && ring_->rebind(h_))
                    rc = ring_->read(off, buf, len, size(), nullptr);
            }
            return rc;
        }
        /* degraded (ring construction failed): the pre-ring sync path */
        rc = aligned_read_at(h_, off, buf, len, size(), &gle);
        if (rc == ERR_READ_FAILED && gle == ERROR_INVALID_HANDLE) {
            std::lock_guard<std::mutex> g(heal_mu_);
            if (reopen_handle(path_.c_str(), access_, share_, &h_))
                rc = aligned_read_at(h_, off, buf, len, size(), nullptr);
        }
        return rc;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 size() const { return geom_.total_bytes; }

private:
    HANDLE h_;
    Geometry geom_;
    std::wstring path_;
    DWORD access_ = GENERIC_READ;
    DWORD share_ = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    std::mutex heal_mu_;
    std::unique_ptr<AsyncRing> ring_;
};

std::unique_ptr<IStorageDevice> open_path(const wchar_t* path, u32* err) {
    /* 2026-11-18: FILE_FLAG_OVERLAPPED on the data-path open (the ring's
     * precondition; concurrent unique-OVERLAPPED ReadFile on one handle
     * is the documented-safe shape - ReadFile remarks). The
     * DEVICE-ioctls below run BEFORE the ring exists (port not yet
     * created), so GetOverlappedResult-style waits are safe here. */
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr, true /*overlapped handle now*/);
    if (g.total_bytes == 0) {
        /* last resort for exotic handles: file-system size */
        LARGE_INTEGER sz = {};
        if (GetFileSizeEx(h, &sz) && sz.QuadPart > 0) {
            g.logical_sector = 512;
            g.physical_sector = 512;
            g.total_bytes = u64(sz.QuadPart);
            g.sector_count = g.total_bytes / 512;
        }
    }
    if (err)
        *err = OK;
    return std::unique_ptr<IStorageDevice>(
        new DirectDevice(h, g, std::wstring(path), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
}

} // namespace

/* =========================================================================
 * Public transport API (cr_transport.h).
 * ======================================================================= */

/* CONTROL-PATH open (enumeration's whole-disk view - the og's own
 * enumeration is WMI topology only, getdriverinfo:20-29). Shares the one
 * direct mechanism with the data path (open_disk); kept as the classified
 * control-consumer entry point. */
std::unique_ptr<IStorageDevice> open_physical_drive(u32 disk, u32* err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    return open_path(path, err);
}

bool volume_extent(wchar_t letter, u32* disk, u64* start, u64* len) {
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h =
        CreateFileW(vol, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    bool ok = false;
    VOLUME_DISK_EXTENTS ext[8] = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext, sizeof(ext), &br,
                        nullptr) &&
        ext->NumberOfDiskExtents >= 1) {
        if (disk)
            *disk = u32(ext->Extents[0].DiskNumber);
        if (start)
            *start = u64(ext->Extents[0].StartingOffset.QuadPart);
        if (len)
            *len = u64(ext->Extents[0].ExtentLength.QuadPart);
        ok = true;
    }
    CloseHandle(h);
    return ok;
}

std::unique_ptr<IStorageDevice> open_disk(u32 disk, u32* err) {
    /* Direct raw I/O (session-96: the driver replacement): the whole-disk
     * view is \\.\PhysicalDriveN opened GENERIC_READ share R|W|D - the
     * exact posture the grammar path used, one layer down, no kernel
     * component of ours in the path. Callers gate the privilege first
     * (require_privilege in rrecov.cpp / the cr_ui open impls); a failure
     * here under an elevated token is a TARGET error (absent disk number,
     * protected or busy device), not a privilege refusal. WinPE needs no
     * registration - the direct device is OS-native everywhere. */
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    return open_path(path, err);
}

/* ---------------------------------------------------------------------------
 * Write-capable device for the repair engine (RawFixer) and the apply
 * engine (S-029) - the base is apply::IApplyDevice (IS-A IWriteDevice),
 * so one class carries the S-010 quiesce + the corpus PartitionRecovery
 * device-control chain.
 * ------------------------------------------------------------------------- */
namespace {

class RwDevice final : public cr::apply::IApplyDevice {
public:
    RwDevice(HANDLE h, Geometry g, u32 disk = 0, std::wstring path = std::wstring(),
             DWORD access = GENERIC_READ | GENERIC_WRITE,
             DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE)
        : h_(h), geom_(g), disk_(disk), path_(std::move(path)), access_(access), share_(share) {}
    ~RwDevice() override {
        /* S-010: closing the held volume handles releases their locks
         * (the Win32 contract: a volume lock lasts until every handle
         * that took it closes). */
        release_locked_volumes_();
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }

    /* S-010 volume quiesce + M-030's terminal step (corpus
     * RecoveryModule_decompiled.c :60401-60447 - the repair-write
     * quiesce chain; the earlier :44686-44720 cite was stale, corrected
     * this session). Per MOUNTED volume on this disk -
     *   FSCTL_LOCK_VOLUME (0x90018)    :60401 - a busy volume = refuse
     *   DeleteVolumeMountPointA         :60412 - the OS must not re-open it
     *   FlushFileBuffers                :60422
     *   Sleep(2000) settle              :60428
     *   FSCTL_DISMOUNT_VOLUME (0x90020) :60431
     *   FSCTL_UNLOCK_VOLUME (0x9001c)   :60436 - the og unlocks before
     *     the terminal step; the core holds its lock to the close below
     *     (the unlock releases at handle close - same terminal state)
     *   IOCTL_VOLUME_OFFLINE (0x56c00c) :60440 - M-030: THE TERMINAL
     *     STEP, previously missing: a mounted-but-locked volume can
     *     still receive third-party I/O through pre-existing handles;
     *     OFFLINE closes that window (the og offlines, never just locks).
     * The volume handles are held for the device lifetime. Unmounted
     * volumes (no letter) have nothing to quiesce. A lock that cannot
     * be acquired = ERR_VOLUME_LOCK_FAILED: execute() refuses the whole
     * repair before the first write - the concurrent user keeps the
     * volume, never a raced write. */
    u32 lock_volume() override {
        if (locked_)
            return OK; /* idempotent */
        VolumeEntryList vols;
        if (enumerate_volumes(vols) != OK)
            return OK; /* enumeration unavailable (e.g. no letters at all) */
        for (const auto& v : vols) {
            if (v.disk != disk_ || v.letter == 0)
                continue;
            wchar_t vol_path[16];
            _snwprintf_s(vol_path, 16, _TRUNCATE, L"\\\\.\\%c:", v.letter);
            HANDLE vh = CreateFileW(vol_path, GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
            if (vh == INVALID_HANDLE_VALUE)
                return ERR_VOLUME_LOCK_FAILED; /* in use: refuse, never race */
            DWORD bytes_ret = 0;
            if (!DeviceIoControl(vh, FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0, &bytes_ret,
                                 nullptr)) {
                CloseHandle(vh);
                return ERR_VOLUME_LOCK_FAILED; /* busy volume: clean refusal */
            }
            /* M-030: capture the volume's GUID path BEFORE the mount
             * point goes away - the OFFLINE terminal step below removes
             * the volume from the mountable set, and the restore path
             * (release_locked_volumes_) needs \??\Volume{...}\ to bring
             * it back online. The corpus restore (FUN_1800459b0
             * :34465-34502) reopens the volume device by identity. */
            wchar_t mount[8] = {v.letter, L':', L'\\', 0};
            wchar_t guid[64] = {};
            VolumeRestore vr;
            vr.letter = v.letter;
            (void)GetVolumeNameForVolumeMountPointW(mount, guid, 64);
            vr.guid = guid; /* empty on failure: the restore degrades to
                             * handle-close only (never worse than the
                             * pre-M-030 behavior) */
            /* the letter must go so the OS does not hand the volume back
             * out (corpus :60412); flush + settle + verify + dismount */
            DeleteVolumeMountPointW(mount);
            FlushFileBuffers(vh);
            Sleep(2000); /* corpus :60428 settle */
            DeviceIoControl(vh, FSCTL_IS_VOLUME_MOUNTED, nullptr, 0, nullptr, 0, &bytes_ret,
                            nullptr); /* verify */
            DeviceIoControl(vh, FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0, &bytes_ret,
                            nullptr); /* corpus :60431 */
            /* M-030: the terminal OFFLINE step (corpus :60440,
             * IOCTL_VOLUME_OFFLINE 0x56c00c). Failure is recorded, not
             * fatal: the og's chain treats every step's error the same
             * way (GetLastError() into the chain's error slot) and
             * proceeds - the volume is still dismounted and locked; a
             * failed OFFLINE degrades to exactly the pre-M-030 posture
             * (surfaced through the N-048 log seam, never silent). */
            if (!DeviceIoControl(vh, IOCTL_VOLUME_OFFLINE, nullptr, 0, nullptr, 0, &bytes_ret,
                                 nullptr)) {
                const DWORD offline_gle = GetLastError();
                cr::log::write(cr::log::Level::Warn,
                               "quiesce: volume offline failed gle=%lu (degraded "
                               "to dismount+lock posture)",
                               (unsigned long)offline_gle);
            }
            locked_volumes_.push_back(vh);
            restores_.push_back(vr);
        }
        locked_ = true;
        return OK;
    }
    void unlock_volume() override {
        /* closing the handles releases their locks (the destructor does
         * the same); dropping them here matches the symmetric contract
         * for callers that finish early */
        release_locked_volumes_();
    }

    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        /* M-009 (GAP-S6-13 mandate) + core-api unification: route through
         * the 64 KiB direct-I/O envelope exactly like the scan-path devices
         * (DirectDevice) - one envelope, one path. */
        DWORD gle = 0;
        u32 rc = aligned_read_at(h_, off, buf, len, size(), &gle);
        if (rc == ERR_READ_FAILED && gle == ERROR_INVALID_HANDLE) {
            /* og parity: heal the dead handle, retry once */
            std::lock_guard<std::mutex> g(heal_mu_);
            if (reopen_handle(path_.c_str(), access_, share_, &h_))
                rc = aligned_read_at(h_, off, buf, len, size(), nullptr);
        }
        return rc;
    }

    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        u32 rc = write_impl_(off, buf, len);
        if (rc == ERR_WRITE_FAILED && write_gle_ == ERROR_INVALID_HANDLE) {
            /* og parity (Device_decompiled.c:56182-56183 guard pattern):
             * heal the dead handle, retry the write once. */
            std::lock_guard<std::mutex> g(heal_mu_);
            if (reopen_handle(path_.c_str(), access_, share_, &h_))
                rc = write_impl_(off, buf, len);
        }
        return rc;
    }

    u64 size() const override { return geom_.total_bytes; }
    u32 sector_bytes() const override { return geom_.logical_sector ? geom_.logical_sector : 512; }

    /* ---- S-029: the corpus PartitionRecovery device-control chain ----
     * (PartitionRecovery_decompiled.c FUN_1800566d0/FUN_180038b10; the
     * raw hex values are the decompile's own - cited per method in
     * cr_apply.h. Error convention: OK = the DeviceIoControl succeeded;
     * failures return the transport's ERR_WRITE_FAILED so execute_apply
     * treats them as hard device errors and rolls back.) */
    u32 delete_drive_layout() override {
        /* 0x7C100 IOCTL_DISK_DELETE_DRIVE_LAYOUT (legacy path, pre-write) */
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x7C100, nullptr, 0, nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    u32 set_drive_layout_mbr(u32 signature, u32 entry_count) override {
        /* 0x7C010 IOCTL_DISK_SET_DRIVE_LAYOUT with the 0xA8-byte
         * DRIVE_LAYOUT_INFORMATION: PartitionCount = entry_count (the
         * corpus stamps 4 in the rebuild flow), Mbr.Signature = the
         * effective signature; the entry array mirrors the sector-0
         * bytes the core already wrote (the kernel validates/commits
         * them into its layout view). */
        struct {
            u32 partition_count;
            u32 signature;
            u8 entries[4 * 40]; /* 4 x PARTITION_INFORMATION-shaped slots */
        } dli = {};
        dli.partition_count = entry_count ? entry_count : 4;
        dli.signature = signature;
        /* the per-entry payload is zero except RewriteBoot on the FIRST
         * entry: the sector-0 write already carried the real bytes; the
         * ioctl's job here is the layout-signature commit (corpus
         * FUN_1800566d0 zeroes the whole 0xA8 buffer and stamps count+sig). */
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x7C010, &dli, sizeof(dli), nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    u32 set_partition_info_ex(const PartitionInfoExRecord& rec) override {
        /* 0x7C058 with the 0x18-byte record (FUN_180056bc0 tail:
         * {u32 partition number, 16-byte identity, u32 boot flag}) */
        struct {
            u32 partition_number;
            u8 identity[16];
            u32 boot_flag;
        } rec18 = {};
        rec18.partition_number = rec.partition_number;
        for (int i = 0; i < 16; ++i)
            rec18.identity[i] = rec.identity[i];
        rec18.boot_flag = rec.boot_flag;
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x7C058, &rec18, sizeof(rec18), nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    u32 update_properties() override {
        /* 0x70140 IOCTL_DISK_UPDATE_PROPERTIES */
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x70140, nullptr, 0, nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    void settle_refresh() override {
        Sleep(1000); /* corpus PR:47015-47025: settle between retries */
    }
    bool legacy_os_path() const override {
        /* corpus GetVersionExA gate (legacy = 5.0 or older). The build's
         * envelope is Win7 SP1+ (6.1+): the modern path is the only live
         * branch - the constant keeps the corpus contract visible. */
        return false;
    }
    u32 generate_disk_signature() override {
        /* corpus GPT_GenerateSignature (PARTITION.MO) - a fresh 32-bit
         * disk signature. Entropy from kernel32-only sources (the import
         * allowlist stays untouched - no Ole32 dependency for this):
         * performance counter + tick count + the disk number, mixed
         * FNV-style; two consecutive rebuilds never stamp the same value. */
        LARGE_INTEGER pc = {};
        QueryPerformanceCounter(&pc);
        const u64 mix = u64(pc.QuadPart) ^ (u64(GetTickCount()) << 32) ^ (u64(disk_) << 48);
        u32 h = 0x811C9DC5u;
        for (int shift = 0; shift < 64; shift += 8) {
            h ^= u32((mix >> shift) & 0xFF);
            h *= 0x01000193u;
        }
        return h ? h : 1u;
    }

private:
    /* The staged WriteFile: the direct path needs a sector-aligned DATA
     * BUFFER (the storage stack requires sector-aligned buffers on raw
     * handles). Records the failing GLE in write_gle_ for the heal
     * decision. Success is put == len exactly (direct handles report
     * true byte counts - the driver-era F11 scaling is gone). */
    u32 write_impl_(u64 off, const void* buf, u32 len) {
        write_gle_ = 0;
        const u32 ssec = geom_.logical_sector ? geom_.logical_sector : 512;
        void* src = const_cast<void*>(buf);
        void* staged = nullptr;
        if ((reinterpret_cast<uintptr_t>(buf) % ssec) != 0) {
            staged = _aligned_malloc(len, ssec);
            if (staged == nullptr)
                return ERR_NO_MEMORY;
            memcpy(staged, buf, len);
            src = staged;
        }
        OVERLAPPED ov = {};
        ov.Offset = u32(off);
        ov.OffsetHigh = u32(off >> 32);
        DWORD put = 0;
        const BOOL ok = WriteFile(h_, src, len, &put, &ov);
        if (!ok) {
            const DWORD e = GetLastError();
            if (e != ERROR_IO_PENDING) {
                if (staged)
                    _aligned_free(staged);
                write_gle_ = e;
                return ERR_WRITE_FAILED;
            }
            if (!GetOverlappedResult(h_, &ov, &put, TRUE)) {
                if (staged)
                    _aligned_free(staged);
                write_gle_ = GetLastError();
                return ERR_WRITE_FAILED;
            }
        }
        if (staged)
            _aligned_free(staged);
        if (put != len)
            return ERR_WRITE_FAILED;
        return OK;
    }

    /* close every held volume handle - that is what releases the locks.
     * M-030: for every volume the quiesce OFFLINED, run the og's restore
     * chain first (corpus FUN_1800459b0, RecoveryModule_decompiled.c
     * :34465-34502, decoded this session - OQ-2 resolved): reopen the
     * volume by its GUID path, IOCTL_VOLUME_ONLINE (0x56c008 :34484),
     * re-create the mount point (SetVolumeMountPointW - the saved
     * letter; the og's vtable +0x78 restore step), Sleep(1000) settle
     * (:34499), close. Without this the quiesced volume would stay
     * letterless after repair - the pre-M-030 core deleted the mount
     * point and never restored it. A volume whose GUID was not
     * captured degrades to the handle-close only (the pre-M-030
     * behavior, never worse). */
    struct VolumeRestore {
        wchar_t letter = 0;
        std::wstring guid; /* \\??\\Volume{...}\\ or empty */
    };

    void release_locked_volumes_() {
        for (size_t k = 0; k < locked_volumes_.size(); ++k) {
            HANDLE vh = locked_volumes_[k];
            if (vh == INVALID_HANDLE_VALUE)
                continue;
            CloseHandle(vh); /* releases the volume lock first */
            if (k < restores_.size() && !restores_[k].guid.empty()) {
                const VolumeRestore& vr = restores_[k];
                HANDLE vh2 = CreateFileW(vr.guid.c_str(), GENERIC_READ | GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL, nullptr);
                if (vh2 != INVALID_HANDLE_VALUE) {
                    DWORD br = 0;
                    (void)DeviceIoControl(vh2, IOCTL_VOLUME_ONLINE, nullptr, 0, nullptr, 0, &br,
                                          nullptr);
                    CloseHandle(vh2);
                }
                wchar_t mount[8] = {vr.letter, L':', L'\\', 0};
                (void)SetVolumeMountPointW(vr.guid.c_str(), mount);
                Sleep(1000); /* corpus :34499 settle */
            }
        }
        locked_volumes_.clear();
        restores_.clear();
        locked_ = false;
    }

    HANDLE h_;
    Geometry geom_;
    u32 disk_ = 0;
    std::wstring path_;
    DWORD access_ = GENERIC_READ | GENERIC_WRITE;
    DWORD share_ = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD write_gle_ = 0;
    std::mutex heal_mu_;
    bool locked_ = false;
    std::vector<HANDLE> locked_volumes_;
    std::vector<VolumeRestore> restores_; /* pushed in lockstep with
                                           * locked_volumes_ (same loop) */
};

} // namespace

/* shared RW open (session-96: the direct era): the concrete RwDevice is an
 * apply::IApplyDevice (IS-A IWriteDevice), so the two public factories are
 * typed views of one open path - \\.\PhysicalDriveN opened GENERIC_READ|
 * GENERIC_WRITE, share R|W|D (write-capable raw access needs the narrower
 * share so concurrent writers surface honestly). A failure here under an
 * elevated token is a target error (ERR_OPEN_FAILED); the privilege gate is
 * the caller's precondition (require_privilege / the cr_ui impls). */
namespace {
std::unique_ptr<RwDevice> open_rw_device(u32 disk, u32* err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.total_bytes == 0) {
        CloseHandle(h);
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    if (err)
        *err = OK;
    return std::unique_ptr<RwDevice>(
        new RwDevice(h, g, disk, std::wstring(path), GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
}
} // namespace

std::unique_ptr<cr::repair::IWriteDevice> open_disk_rw(u32 disk, u32* err) {
    return open_rw_device(disk, err);
}

std::unique_ptr<cr::apply::IApplyDevice> open_disk_apply_rw(u32 disk, u32* err) {
    return open_rw_device(disk, err);
}

u32 refresh_disk_properties(u32 disk) {
    /* S-029 (apply): the corpus settle pattern - IOCTL_DISK_UPDATE_PROPERTIES
     * (0x70140) with up to 3 x Sleep(1000) retries (PartitionRecovery
     * PR:46873-46880). The table write already succeeded when this runs;
     * the refresh makes the OS re-read the layout so the recovered volume
     * becomes visible. Best-effort by contract: a refusal (permissions,
     * transient busy) reports its code, the caller surfaces it without
     * failing the apply. */
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return ERR_OPEN_FAILED;
    DWORD bytes = 0;
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (DeviceIoControl(h, IOCTL_DISK_UPDATE_PROPERTIES, nullptr, 0, nullptr, 0, &bytes,
                            nullptr)) {
            CloseHandle(h);
            return OK;
        }
        Sleep(1000); /* corpus :46877 settle between retries */
    }
    CloseHandle(h);
    return ERR_WRITE_FAILED;
}

/* ---------------------------------------------------------------------------
 * N-013: destination-side overlap arming, full-extent edition.
 * (M-055: these two definitions must sit INSIDE namespace cr::win to match
 * cr_transport.h's declarations - they were born after the namespace close,
 * which both broke the VolExtent lookup and left the declared functions
 * undefined at link time.)
 * ------------------------------------------------------------------------- */
bool volume_extents(wchar_t letter, std::vector<VolExtent>* out) {
    if (!out)
        return false;
    out->clear();
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h = CreateFileW(vol, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    VOLUME_DISK_EXTENTS ext[8] = {};
    DWORD br = 0;
    BOOL io = DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext, sizeof(ext),
                              &br, nullptr);
    CloseHandle(h);
    /* ERROR_MORE_DATA: the volume has more than 8 extents - the first 8
     * are filled and used (documented cap). */
    if (!io && GetLastError() != ERROR_MORE_DATA)
        return false;
    for (DWORD i = 0; i < ext->NumberOfDiskExtents && i < 8; ++i) {
        VolExtent ve{};
        ve.disk = u32(ext->Extents[i].DiskNumber);
        ve.start = u64(ext->Extents[i].StartingOffset.QuadPart);
        ve.len = u64(ext->Extents[i].ExtentLength.QuadPart);
        out->push_back(ve);
    }
    return !out->empty();
}

bool dest_overlap_spans(const std::vector<VolExtent>& ve, bool source_disk_known, u32 source_disk,
                        std::vector<std::pair<u64, u64>>* out_spans) {
    /* F12 same-disk precondition (see cr_transport.h for the full defect
     * record): numeric offset comparison is only meaningful when both
     * coordinate spaces are the same disk's. Unknown source disk (the
     * image class is gated by the callers before this point) never arms. */
    if (out_spans == nullptr || !source_disk_known)
        return false;
    bool on_source = false;
    for (const auto& v : ve) {
        if (v.disk == source_disk) {
            on_source = true;
            break;
        }
    }
    if (!on_source)
        return false;
    out_spans->clear();
    out_spans->reserve(ve.size());
    for (const auto& v : ve)
        out_spans->push_back({v.start, v.len});
    return true;
}

bool destination_volume_letter(const std::wstring& path, wchar_t* letter) {
    if (!letter || path.empty())
        return false;
    /* The og's PathIsNetworkPathW branch: network destinations are never
     * opened raw, never armed (cannot be evaluated). */
    if (PathIsNetworkPathW(path.c_str()))
        return false;
    /* Resolve reparse points (junctions/mount points) to the FINAL path:
     * a junction into another volume must yield the TARGET's volume, not
     * the junction's host volume. */
    std::wstring final_path = path;
    HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        wchar_t buf[1024];
        const DWORD n = GetFinalPathNameByHandleW(h, buf, 1024, VOLUME_NAME_DOS);
        if (n > 0 && n < 1024) {
            final_path = buf;
            /* strip the \\?\ prefix if present (GetVolumePathNameW wants a
             * plain drive path) */
            if (final_path.rfind(L"\\\\?\\", 0) == 0)
                final_path.erase(0, 4);
        }
        CloseHandle(h);
    }
    /* Fall back (destination folder may not exist yet): GetVolumePathNameW
     * on the path itself, no reparse resolution possible. */
    wchar_t root[MAX_PATH] = {};
    if (!GetVolumePathNameW(final_path.c_str(), root, MAX_PATH) || root[0] == L'\0')
        return false;
    if (root[0] == L'\\') /* mapped network root slipped past the probe */
        return false;
    *letter = root[0];
    return true;
}

/* ---- ring_tuning (cr_transport.h): the sweep seam ---- */
namespace ring_tuning {
std::atomic<u32>& run_bytes() {
    return g_ring_run;
}
std::atomic<u32>& slot_count() {
    return g_ring_slots;
}
} // namespace ring_tuning

/* ---- open_file_async (cr_transport.h): the ring on an ordinary file ---- */
std::unique_ptr<IStorageDevice> open_file_async(const wchar_t* path, u32 run_, u32 slots_,
                                                u32* err) {
    if (!path || !path[0]) {
        if (err)
            *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    /* the explicit run/slots take precedence over the globals for this
     * one open (the test/bench surface); 0 = the production defaults */
    if (run_ || slots_) {
        const u32 prev_run = g_ring_run.exchange(run_);
        const u32 prev_slots = g_ring_slots.exchange(slots_);
        std::unique_ptr<IStorageDevice> dev = open_path(path, err);
        g_ring_run.store(prev_run);
        g_ring_slots.store(prev_slots);
        return dev;
    }
    return open_path(path, err);
}

} // namespace win
} // namespace cr
