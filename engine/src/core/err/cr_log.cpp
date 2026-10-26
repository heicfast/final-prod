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
/* written under g_mu, read lock-free on the write fast path (sites are
 * immutable once published; set_sink replaces the pointer atomically and
 * leaks the old site - sinks are set once per process in practice, and a
 * use-after-free on a racing write is worse than a bounded leak on an
 * exotic swap) */
std::atomic<Site*> g_site{nullptr};

void write_va(Level level, const char* fmt, va_list ap) {
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
    std::lock_guard<std::mutex> lock(g_mu);
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
    /* publish under the write mutex; the old site is leaked by design
     * (see g_site's comment) */
    std::lock_guard<std::mutex> lock(g_mu);
    g_site.store(next, std::memory_order_release);
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

} // namespace log
} // namespace cr
