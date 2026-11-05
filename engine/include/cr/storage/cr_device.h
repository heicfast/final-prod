/*
 * cr_device.h - the storage device abstraction (IStorageDevice contract).
 *
 * Clean-room equivalent of the reference IO stack
 * (CStorageDeviceWithCache -> IStorageDevice -> CPartitionVirtualVolumeDevice,
 * corpus: ntfs-engine-internals.md; fat-exfat-engine-algorithms-v2 section 2):
 *
 *   - engines only ever see ReadAt(offset, buffer, length) in BYTES, bounded.
 *   - a PartitionSlice (cr_slice.h) bounds every engine view to its candidate
 *     partition - engines cannot read outside by construction.
 *   - the cached reader (cr_cached.h) sits in front of transports, exactly like
 *     CStorageDeviceWithCache did for MFT re-reads.
 *   - errors are strong: every failed region lands in the ReadErrorMap carried
 *     by the scan result and session (R1.6).
 *
 * REGISTERED DIVERGENCE (M-034, PARITY-DELTAS D-88): NAS/remote scanning
 * is EXCLUDED from the core's device scope. The og wraps an EuMrx
 * redirector (CRemoteDevice/CDoScanRemoteDevice) to scan shares; the
 * core's device layers are local-only by design — the EuMrx interop is
 * host-layer surface, the corpus proves the scan-path plumbing only
 * (deleted-file yield over SMB is undecoded), and a network stack is not
 * pure-core surface. The IStorageDevice contract itself is transport-
 * agnostic: a host MAY layer an SMB-backed device.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_DEVICE_H_
#define CR_DEVICE_H_

#include "../cr_types.h"
#include "../cr_error.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace cr {

/* Probed geometry of a disk or volume (R1.2 - no fixed 512 anywhere).
 * Reference defect F1 (fixed 512-byte sector math) fixed here: every offset
 * in the stack is byte-anchored; sector math derives from these fields. */
struct Geometry {
    u64 total_bytes = 0;     /* derived size in bytes */
    u32 logical_sector = 0;  /* bytes per logical sector (512 / 4096) */
    u32 physical_sector = 0; /* bytes per physical sector (512e reports 4096) */
    u64 sector_count = 0;    /* CEIL(total_bytes / logical_sector) - N-206 */

    /* N-206: the sector count is the og's geometry-EX CEIL form
     * (Device_decompiled.c:56536-56538: (L-1+ss)/ss, the 0x7405C result
     * overwriting the 0x74004 floor) - the final partial sector of a
     * non-sector-multiple source is ADDRESSABLE, driver-padded. The
     * validity range is therefore the ceil window: the byte size lies
     * in ((count-1)*ss, count*ss]. Sector-multiple sources (real disks)
     * sit at the exact top of the window - unchanged behavior. */
    constexpr bool valid() const {
        return logical_sector != 0 && physical_sector != 0 && total_bytes != 0 &&
               sector_count != 0 && total_bytes <= sector_count * logical_sector &&
               total_bytes > (sector_count - 1) * logical_sector;
    }
    /* 4Kn and 512e reporting. */
    constexpr bool is_4kn() const { return logical_sector >= 4096; }
};

/* A contiguous run of read failures, preserved in scan results and sessions. */
struct ReadErrorRange {
    u64 offset;
    u64 length;
    u32 attempts; /* how many times we retried before giving up */
    /* M-049: the module attribution recorded when the range failed (the
     * engine-side subset of the og's per-module error registrars -
     * cr_errmod.h). NOT serialized: sessions persist the geometric trio
     * (the wire format predates this field and stays readable both
     * directions); module attribution is live-scan event metadata, and a
     * range reloaded from a session is honestly kNone. When runs
     * coalesce, the NEWEST constituent's attribution wins (the record
     * keeps the latest failure's context - the og's error-track
     * semantics). */
    u32 module = 0;
};

/* Accumulated read-failure map for one scan pass. Kept sorted by offset. */
class ReadErrorMap {
public:
    void add(u64 offset, u64 length, u32 attempts, u32 module = 0);
    const std::vector<ReadErrorRange>& ranges() const { return ranges_; }
    u64 total_bytes() const;
    void merge(const ReadErrorMap& other);

    /* Serialization support (session records): fixed-u32 count + entries. */
    void serialize(std::vector<u8>& out) const;
    static ReadErrorMap deserialize(const u8* data, size_t len, u32* consumed);

private:
    std::vector<ReadErrorRange> ranges_;
};

/* Retry policy for transport reads (R1.6). skip_or_zero is opt-in only. */
enum class RetryMode : u32 {
    Fail = 0, /* report ERR_RETRY_EXHAUSTED, record range, stop stage */
    Skip = 1, /* record range, return zeroed buffer, continue */
};

struct RetryPolicy {
    u32 max_attempts = 3;
    RetryMode mode = RetryMode::Fail;
    /* N-004 (og CatchBuffer parity): on a persistently failing range, one
     * shifted-window second-chance re-read before the range is given up -
     * the og's CEXT4CatchBufferDeviceProxy 1 KB-backtrack salvage, placed
     * at this layering boundary (the core's designated bad-sector point).
     * The backtrack step is max(0x400, logical_sector) so the shifted
     * request stays sector-aligned on 4Kn devices (the og hardcodes
     * 0x400; on 4Kn its shifted request would be misaligned - ours works).
     * Default ON (the og always catches). */
    bool backtrack = true;
};

/*
 * IStorageDevice - the one interface engines see.
 * Contract:
 *   - ReadAt returns OK and fills exactly length bytes, or an error code.
 *   - offset + length must be within bounds (checked: ERR_RANGE_OUT_OF_BOUNDS).
 *   - implementations must be safe for calls from engine worker threads.
 */
class IStorageDevice {
public:
    virtual ~IStorageDevice() = default;
    virtual u32 read_at(u64 offset, void* buffer, u32 length) = 0;
    virtual const Geometry& geometry() const = 0;
    virtual u64 size() const { return geometry().total_bytes; }
    /* Absolute placement on the parent device when this is a bounded window
     * (PartitionSlice overrides); 0 for whole-device views. Feeds the
     * session VolumeInfo (G-14) and destination-overlap math. */
    virtual u64 absolute_base() const { return 0; }
    /* N-072 (ZFS label retry): true when this device is STATIC MEDIA - an
     * image file (or a window over one). Static media cannot heal: a torn
     * label stays torn, so robustness retries (the og's delay-execution
     * import polling) are skipped for it. Default false = a live handle
     * (a real disk/volume); the image devices and every wrapper forward
     * accordingly. */
    virtual bool is_static_media() const { return false; }
};

/* Cancellation token consulted at loop heads (R6.2; reference cancel
 * callbacks - fix F3: engines poll it at every loop head. Transports do
 * NOT poll the token - an in-flight read completes or fails on its own
 * (F-B-16: the old 'engines AND transports' claim was an overclaim; the
 * honest scope is engine-side cooperative cancellation).)
 *
 * checkpoint() is the cooperative PAUSE point (G-07): a plain token behaves
 * exactly like cancelled(); a PausableToken blocks in checkpoint() while the
 * scan is paused and only returns once cancelled or resumed. Loop heads that
 * abort on cancel should call checkpoint() so pause works everywhere cancel
 * does; one-shot determinations and progress callbacks must keep cancelled()
 * (never block a callback). */
class CancelToken {
public:
    virtual ~CancelToken() = default;
    virtual bool cancelled() const = 0;
    /* Pause-aware cancel poll: waits while paused, then reports cancelled. */
    virtual bool checkpoint() const { return cancelled(); }
    /* Total time spent paused so far (ms) - for elapsed-time compensation
     * (RE parity: the og subtracts pause duration from used-time). */
    virtual u64 paused_ms() const { return 0; }
    /* True while a PausableToken is paused (telemetry surface). */
    virtual bool paused() const { return false; }
};

/*
 * PausableToken (G-07) - cancel + pause/resume with wait semantics.
 * Threading: pause()/resume()/cancel() from any thread (e.g. a UI button
 * handler); checkpoint() from engine worker threads. cancel() wakes paused
 * waiters - engines then see cancelled() and abort cooperatively. All
 * methods are thread-safe.
 */
class PausableToken final : public CancelToken {
public:
    void pause() {
        std::lock_guard<std::mutex> lock(m_);
        if (cancelled_ || paused_)
            return;
        paused_ = true;
        pause_started_ = std::chrono::steady_clock::now();
    }
    void resume() {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (!paused_)
                return;
            paused_ms_ += std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - pause_started_)
                              .count();
            paused_ = false;
        }
        cv_.notify_all();
    }
    void cancel() {
        {
            std::lock_guard<std::mutex> lock(m_);
            cancelled_ = true;
            if (paused_) {
                paused_ms_ += std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - pause_started_)
                                  .count();
                paused_ = false;
            }
        }
        cv_.notify_all();
    }

    bool cancelled() const override {
        std::lock_guard<std::mutex> lock(m_);
        return cancelled_;
    }
    bool paused() const override { /* fuzz-lane -Winconsistent-missing-override */
        std::lock_guard<std::mutex> lock(m_);
        return paused_;
    }
    bool checkpoint() const override {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock, [this] { return !paused_ || cancelled_; });
        return cancelled_;
    }
    u64 paused_ms() const override {
        std::lock_guard<std::mutex> lock(m_);
        u64 total = paused_ms_;
        if (paused_) {
            total += std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - pause_started_)
                         .count();
        }
        return total;
    }

private:
    mutable std::mutex m_;
    mutable std::condition_variable cv_;
    bool paused_ = false;
    bool cancelled_ = false;
    std::chrono::steady_clock::time_point pause_started_{};
    u64 paused_ms_ = 0;
};

} // namespace cr

#endif // CR_DEVICE_H_
