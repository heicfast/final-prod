/*
 * cr_retry.h - retrying, bad-sector-isolating transport wrapper (R1.6).
 *
 * Sits between the engines and any IStorageDevice transport:
 *   - retries failed reads per RetryPolicy (max_attempts),
 *   - isolates failing ranges by bisection down to sector granularity
 *     (a 1 MiB read that dies on one bad sector does not lose the megabyte),
 *   - Skip mode: zero-fills ONLY the failing sectors and returns OK
 *     (opt-in; the default Fail mode reports ERR_RETRY_EXHAUSTED),
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

    /* The accumulated read-failure map for this scan pass. Reference
     * semantics: valid for the post-run merge on the scan thread only. */
    const ReadErrorMap& error_map() const;

    /* Thread-safe snapshot (G-06): a VALUE copy under the internal lock -
     * safe to call from any thread while workers record failures. */
    ReadErrorMap error_map_copy() const;

    /* Read-only access to the wrapped device (e.g. slice construction). */
    IStorageDevice* inner() const { return inner_.get(); }

    /* Failed bytes seen so far (diagnostics / progress reporting). */
    u64 failed_bytes() const;

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
    u64 failed_bytes_ = 0;
};

} // namespace cr

#endif // CR_RETRY_H_
