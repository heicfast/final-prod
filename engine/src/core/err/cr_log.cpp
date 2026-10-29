/* cr_log.cpp - the engine logging seam (N-048): sink registry + bounded
 * formatting. Silent by default; the fast path is one atomic load.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_log.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>

namespace cr {
namespace log {

namespace {

struct Site {
    SinkFn fn = nullptr;
    void* user = nullptr;
};

std::mutex g_mu;
/* written under g_mu, read lock-free on the write fast path. Sites live
 * in a static deque's stable storage: retired sites are never freed
 * while a racing reader may hold them (same no-UAF swap semantics the
 * original leak-by-design intended), but the storage is OWNED and
 * destroyed at exit - LeakSanitizer-clean (the N-062 landing's bare
 * `new` left the final Site leaked on every process, red on the ASan
 * lane; the deque/pool repair rode the N-062 landing commit 14229f2f).
 * [N-154: this comment earlier named N-079 as the filing seat for the
 * Site-leak repair - false; N-079 is the NTFS recycle-bin finding, a
 * different lane's filing. The Site-leak fix has no separate finding
 * number, it rode N-062's landing (14229f2f).]
 * deque push_back never moves existing
 * elements, so published &site pointers stay valid. */
std::atomic<Site*> g_site{nullptr};

std::deque<Site>& site_pool() {
    static std::deque<Site> pool;
    return pool;
}

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
    /* publish under the write mutex; retired sites stay alive in the
     * pool (see g_site's comment) - no free-while-read, no heap leak */
    std::lock_guard<std::mutex> lock(g_mu);
    if (fn) {
        site_pool().push_back(Site{fn, user});
        g_site.store(&site_pool().back(), std::memory_order_release);
    } else {
        g_site.store(nullptr, std::memory_order_release);
    }
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
