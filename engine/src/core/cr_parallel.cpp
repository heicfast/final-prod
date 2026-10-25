/* cr_parallel.cpp - the fixed worker pool (scan parallelism).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_parallel.h"
#include "cr/cr_error.h"
#include <chrono>

namespace cr {

namespace {
u32 auto_workers() {
    const unsigned hw = std::thread::hardware_concurrency();
    /* leave one core for the caller/progress; disks cap the useful width */
    u32 n = hw > 1 ? hw - 1 : 1;
    if (n > 8)
        n = 8;
    return n;
}
} // namespace

ThreadPool::ThreadPool(u32 workers) {
    if (workers == 0)
        workers = auto_workers();
    if (workers > 64)
        workers = 64;
    workers_.reserve(workers);
    for (u32 i = 0; i < workers; ++i)
        workers_.emplace_back([this] { worker_loop(); });
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
    cv_work_.notify_all();
    for (auto& t : workers_)
        t.join();
}

void ThreadPool::worker_loop() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_work_.wait(lock, [this] { return shutdown_ || abort_ || !tasks_.empty(); });
            if (tasks_.empty())
                return; /* shutdown or abort with nothing left to run */
            task = std::move(tasks_.front());
            tasks_.pop();
            active_.fetch_add(1, std::memory_order_relaxed);
        }
        task();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const u32 now = active_.fetch_sub(1, std::memory_order_acq_rel) - 1;
            if (now == 0 && tasks_.empty())
                cv_idle_.notify_all();
        }
    }
}

bool ThreadPool::run(std::function<void()> task) {
    if (!task || aborted())
        return false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_)
            return false;
        tasks_.push(std::move(task));
    }
    cv_work_.notify_one();
    return true;
}

void ThreadPool::abort() {
    abort_.store(true, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!tasks_.empty())
            tasks_.pop();
    }
    cv_work_.notify_all();
    cv_idle_.notify_all();
}

u32 ThreadPool::pending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return u32(tasks_.size());
}

void ThreadPool::wait_idle() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_idle_.wait(lock, [this] { return tasks_.empty() && active_.load() == 0; });
}

bool ThreadPool::busy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !tasks_.empty() || active_.load() != 0;
}

u32 parallel_for(ThreadPool& pool, u64 count, const std::function<u32(u64)>& fn) {
    if (count == 0)
        return OK;
    if (!fn)
        return ERR_BAD_PARAMETER;

    std::atomic<u64> next{0};
    std::atomic<u32> first_err{OK};

    auto job = [&]() {
        for (;;) {
            if (pool.aborted()) {
                u32 expected = OK;
                first_err.compare_exchange_strong(expected, ERR_CANCELLED);
                break;
            }
            const u64 i = next.fetch_add(1, std::memory_order_relaxed);
            if (i >= count)
                break;
            u32 rc = OK;
            try {
                rc = fn(i);
            } catch (...) {
                rc = ERR_INTERNAL; /* exceptions never cross the thread edge */
            }
            if (rc != OK) {
                u32 expected = OK;
                first_err.compare_exchange_strong(expected, rc);
                pool.abort();
                break;
            }
        }
    };

    /* workers + this thread all pull from the shared item counter */
    const u32 extra = pool.worker_count();
    for (u32 j = 0; j < extra; ++j) {
        if (!pool.run(job))
            break; /* pool shutting down: run inline below */
    }
    job();

    /* normal path: queue drains; abort path: running tasks break out and
     * abort() already emptied the queue - both terminate. */
    pool.wait_idle();
    return first_err.load(std::memory_order_acquire);
}

CancelPoolWatcher::CancelPoolWatcher(ThreadPool& pool, const CancelToken* token) {
    if (token == nullptr) {
        submit_done_.store(true, std::memory_order_release);
        return; /* no token: nothing to watch */
    }
    const CancelToken* t = token;
    ThreadPool* p = &pool;
    std::atomic<bool>* flag = &submit_done_;
    thread_ = std::thread([p, t, flag]() {
        while (!flag->load(std::memory_order_acquire)) {
            if (t->cancelled()) {
                p->abort();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}

CancelPoolWatcher::~CancelPoolWatcher() { done(); }

void CancelPoolWatcher::done() {
    submit_done_.store(true, std::memory_order_release);
    if (thread_.joinable())
        thread_.join();
}

} // namespace cr
