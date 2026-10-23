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
    u64 sector_count = 0;    /* total_bytes / logical_sector */

    constexpr bool valid() const {
        return logical_sector != 0 && physical_sector != 0 && total_bytes != 0 &&
               sector_count != 0 && total_bytes == sector_count * logical_sector;
    }
    /* 4Kn and 512e reporting. */
    constexpr bool is_4kn() const { return logical_sector >= 4096; }
};

/* A contiguous run of read failures, preserved in scan results and sessions. */
struct ReadErrorRange {
    u64 offset;
    u64 length;
    u32 attempts; /* how many times we retried before giving up */
};

/* Accumulated read-failure map for one scan pass. Kept sorted by offset. */
class ReadErrorMap {
public:
    void add(u64 offset, u64 length, u32 attempts);
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
};

/* Cancellation token consulted at loop heads (R6.2; reference cancel
 * callbacks - fix F3: engines AND transports both honour it).
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
    bool paused() const {
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
