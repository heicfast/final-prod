/* cr_log.cpp - the engine logging seam (N-048): sink registry + bounded
 * formatting. Silent by default; the fast path is one atomic load.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_log.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace cr {
namespace log {

namespace {

struct Site {
    SinkFn fn = nullptr;
    void* user = nullptr;
};

std::mutex g_mu;
/* The site pointer: replaced under g_mu and deleted under g_mu
 * (N-079: the N-048 seam's original "leaked by design" stance flagged
 * LeakSanitizer on every sink-registering test - the ASan lane was
 * pushed red by the landing window). write_va loads the site INSIDE
 * the mutex, so a replaced site can never be dereferenced after its
 * deletion; the exit-time release rides a static-duration deleter. */
std::atomic<Site*> g_site{nullptr};

/* Deletes the live site at process exit (static duration: runs after
 * main returns; any later log call sees a null site and stays silent,
 * the default posture). */
struct SinkReleaser {
    ~SinkReleaser() {
        Site* s = g_site.exchange(nullptr, std::memory_order_acq_rel);
        delete s;
    }
};
const SinkReleaser g_sink_releaser;

void write_va(Level level, const char* fmt, va_list ap) {
    /* N-079: the site is loaded INSIDE the mutex so a set_sink swap can
     * safely delete the retired site (no reader can hold it past the
     * lock); the seam is synchronous by contract, so the lock is the
     * delivery serializer it already was. */
    std::lock_guard<std::mutex> lock(g_mu);
    Site* site = g_site.load(std::memory_order_acquire);
    if (!site || !site->fn)
        return;
    /* bounded formatting: the og's lines are short; 512 bytes covers the
     * bracket/error set with room for paths */
    char buf[512];
    int n = std::vsnprintf(buf, sizeof(buf), fmt, ap);
    if (n < 0)
        return;
    if (n >= int(sizeof(buf)))
        n = int(sizeof(buf)) - 1; /* truncated, still NUL-terminated */
    /* serialize deliveries: the seam is synchronous (the registered
     * divergence - the og's async queue buys nothing at these volumes) */
    site->fn(level, buf, site->user);
}

} // namespace

void set_sink(SinkFn fn, void* user) {
    Site* next = nullptr;
    if (fn) {
        next = new Site();
        next->fn = fn;
        next->user = user;
    }
    /* publish under the write mutex and retire the old site under the
     * same mutex (N-079: write_va holds the mutex across its load+use,
     * so the retired site is unreferenced by the time we delete it) */
    Site* old = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        old = g_site.exchange(next, std::memory_order_acq_rel);
    }
    delete old;
}

bool enabled() {
    Site* site = g_site.load(std::memory_order_acquire);
    return site != nullptr && site->fn != nullptr;
}

void write(Level level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    write_va(level, fmt, ap);
    va_end(ap);
}

void check_memory_pressure(u32 threshold_mb, u64 working_set_mb) {
    /* N-062: pure check - the caller owns the platform sampling. The
     * og's CMemMonitorManager (Log.mo census) feeds logs/UI only; no
     * scheduling coupling (the finding's rule). */
    if (threshold_mb == 0 || working_set_mb < threshold_mb)
        return;
    write(Level::Warn, "memory pressure: working_set=%llu MB threshold=%u MB",
          (unsigned long long)working_set_mb, threshold_mb);
}

} // namespace log
} // namespace cr
