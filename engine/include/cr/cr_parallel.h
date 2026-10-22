/*
 * cr_parallel.h - the fixed worker pool (scan parallelism, R6).
 *
 * Corpus threading model (DRWUI-exe--thread-architecture.md): the reference
 * engines themselves are single-threaded; ALL parallelism is host-side - the
 * wizard runs scan stages, per-volume searchers and result workers on
 * host-owned threads (CommonThreadPoolMgr + per-stage worker classes).
 * Our core mirrors that shape: engines stay single-threaded per unit of
 * work; the ORCHESTRATOR fans units out over this pool:
 *
 *   - per-candidate lost-partition volume scans,
 *   - carve window ranges,
 *   - remnant-sweep offset chunks,
 *   - recovery-queue items (I/O bound writer).
 *
 * Design: fixed worker count, MPMC FIFO task queue, cooperative
 * cancellation via per-run abort flag, bounded parallelism (the disk is
 * the bottleneck - more workers than spindles only adds seek storms).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_PARALLEL_H_
#define CR_PARALLEL_H_

#include "cr_types.h"
#include "cr/storage/cr_device.h" /* CancelToken */
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace cr {

/* A pool of fixed worker threads consuming void()->tasks.
 * Cancellation: tasks consult the shared abort flag (or the CancelToken
 * adapter below); aborted tasks should return promptly. */
class ThreadPool {
public:
    /* workers = 0 selects the auto size (hw concurrency - 1, clamped
     * 1..8 - disks, not CPUs, are the scan bottleneck). */
    explicit ThreadPool(u32 workers = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /* Queue one task. Safe from any thread (including tasks themselves).
     * Returns false when the pool is shutting down or was aborted. */
    bool run(std::function<void()> task);

    /* Cooperative abort: wakes all workers; queued tasks are dropped.
     * Idempotent; call from any thread (e.g. a progress callback that saw
     * the user press Stop). */
    void abort();

    bool aborted() const { return abort_.load(std::memory_order_relaxed); }

    /* Outstanding queue length (diagnostics/progress). */
    u32 pending() const;

    u32 worker_count() const { return u32(workers_.size()); }

    /* Blocks until the queue drains and all workers idle. Do not call from
     * a task. Returns immediately when aborted. */
    void wait_idle();

    /* True while any worker is executing a task. */
    bool busy() const;

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_work_;
    std::condition_variable cv_idle_;
    std::atomic<bool> abort_{false};
    std::atomic<u32> active_{0};
    bool shutdown_ = false;
};

/* Adapter: expose a pool's abort state as a CancelToken for engines. */
class PoolCancelToken final : public CancelToken {
public:
    explicit PoolCancelToken(const ThreadPool* pool) : pool_(pool) {}
    bool cancelled() const override { return pool_ == nullptr || pool_->aborted(); }

private:
    const ThreadPool* pool_;
};

/* Run `fn(0..count-1)` item indices over the pool; blocks until all done.
 * Returns ERR_CANCELLED when the pool aborted mid-run. Exceptions escaping
 * fn are captured per item and surfaced as ERR_INTERNAL (never cross the
 * thread boundary). */
u32 parallel_for(ThreadPool& pool, u64 count, const std::function<u32(u64)>& fn);

} // namespace cr

#endif // CR_PARALLEL_H_
