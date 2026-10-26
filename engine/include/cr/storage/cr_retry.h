/*
 * cr_retry.h - retrying, bad-sector-isolating transport wrapper (R1.6).
 *
 * Sits between the engines and any IStorageDevice transport:
 *   - retries failed reads per RetryPolicy (max_attempts),
 *   - isolates failing ranges by bisection down to sector granularity
 *     (a 1 MiB read that dies on one bad sector does not lose the megabyte),
 *   - Skip mode: zero-fills ONLY the failing sectors and returns OK
 *     (opt-in at the API level; the default Fail mode reports
 *     ERR_RETRY_EXHAUSTED - but PRODUCTION scans always opt in: the
 *     orchestrator constructs the pipe with Skip (cr_scan.cpp) so one
 *     dead spot never aborts a scan - the reference surfaces the same
 *     state as BadSectorTip/"trying to skip bad sectors", FB-O03),
 *   - records every permanent failure in a ReadErrorMap that callers
 *     preserve in scan results and sessions.
 *
 * Thread-safety: read_at may be called from multiple engine worker threads
 * concurrently (the error map is mutex-guarded; the inner device contract
 * already permits concurrent read_at).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RETRY_H_
#define CR_RETRY_H_

#include "cr_device.h"
#include "cr_slice.h"
#include "cr/cr_errmod.h"
#include <atomic>
#include <memory>
#include <mutex>

namespace cr {

class RetryingDevice final : public IStorageDevice {
public:
    /* Takes ownership of inner. geometry() is forwarded from inner. */
    static std::unique_ptr<RetryingDevice> create(std::unique_ptr<IStorageDevice> inner,
                                                  const RetryPolicy& policy = RetryPolicy());

    /* IStorageDevice - see cr_device.h contract. */
    u32 read_at(u64 offset, void* buffer, u32 length) override;
    const Geometry& geometry() const override { return inner_->geometry(); }
    u64 size() const override { return inner_->size(); }
    bool is_static_media() const override { return inner_->is_static_media(); }

    /* The accumulated read-failure map for this scan pass. Reference
     * semantics: valid for the post-run merge on the scan thread only. */
    const ReadErrorMap& error_map() const;

    /* Thread-safe snapshot (G-06): a VALUE copy under the internal lock -
     * safe to call from any thread while workers record failures. */
    ReadErrorMap error_map_copy() const;

    /* M-049 (error-context attribution): the sub-pass "owner" module -
     * the fallback recorded on a range when the reading thread carries
     * no stamp (engine-INTERNAL worker pools). Written only by the
     * orchestrator thread between sub-passes (they are sequential -
     * pools join first), read by recording workers; a plain atomic is
     * the whole synchronization story. */
    void set_owner_module(u32 module) { owner_module_.store(module, std::memory_order_relaxed); }
    u32 owner_module() const { return owner_module_.load(std::memory_order_relaxed); }

private:
    RetryingDevice(std::unique_ptr<IStorageDevice> inner, const RetryPolicy& policy);

    /* One isolation attempt over [offset, offset+length): tries the whole
     * span first; on persistent failure bisects until the failing range is
     * within 2x sector granularity. Returns OK when every byte was filled
     * (Skip mode zero-fills bad spots); error otherwise (Fail mode). */
    u32 read_isolated(u64 offset, u8* buffer, u32 length, u32 depth_budget);
    u32 try_read(u64 offset, void* buffer, u32 length);

    std::unique_ptr<IStorageDevice> inner_;
    RetryPolicy policy_;
    mutable std::mutex map_mutex_;
    ReadErrorMap map_;
    std::atomic<u32> owner_module_{0}; /* M-049: see set_owner_module */
};

} // namespace cr

#endif // CR_RETRY_H_
