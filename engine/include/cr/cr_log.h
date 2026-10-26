/*
 * cr_log.h - the engine logging seam (N-048, diff-to-fix).
 *
 * Corpus lineage: the reference runs a THREE-LAYER logging architecture
 * (Log.mo's EPMLOG.log: 64 KiB cross-process shared staging + named mutex
 * + 10 MB truncate-rollover, line format
 * "[%04d-%02d-%02d %02d:%02d:%02d:%03d] (T:%d/P:%d) msg"; PubLog's 2 MB
 * ring for the UI-family binaries; Logger.dll's async queue + worker with
 * 10 MB keep-second-half compaction - Log_decompiled.c :159-176/:193/
 * :604-621/:699, PubLog_decompiled.c :119-135, Logger_decompiled.c
 * :1630-1696/:1962-2015). The engine DLLs that log (FATSearchDLL's
 * ReleaseTrace, NTFS/RAWSearchDLL's LogInfo, PartitionRecoveryD's scan
 * START/END brackets + clone read errors) write through it.
 *
 * REGISTERED DIVERGENCE: the core is a single-process library - there is
 * no second process to share a mapping with, no UI-family binary set for
 * the ring, and an async worker queue buys nothing at the core's line
 * volumes (diagnostic brackets, not traces). The seam is therefore the
 * SINK, not the transport: the host registers one sink (default NULL =
 * silent, zero formatting overhead) and owns every policy the og baked
 * into its transports (file placement, rollover, compaction, async).
 * The timestamp/process decoration is the HOST's job (the og's line
 * format is its transports'; the core emits the message text).
 *
 * Levels: Error (read-failure exhausts, internal faults), Warn (memory
 * pressure, degraded paths), Info (scan brackets, stage boundaries,
 * checkpoints, candidate resolutions).
 *
 * Threading: the sink may be called from ANY thread (engine workers, the
 * heartbeat watchdog); writes are serialized by an internal mutex (the
 * seam is synchronous - a host needing async behavior wraps its sink).
 * set_sink may be called any time; the change takes effect on the next
 * write. Sink implementations must be cheap or internally deferred - a
 * blocking sink blocks the emitting engine worker.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_LOG_H_
#define CR_LOG_H_

#include "cr_types.h"
#include <cstdarg>

namespace cr {
namespace log {

enum class Level : u32 {
    Error = 0, /* read failures exhausted, internal faults */
    Warn = 1,  /* degraded paths, memory pressure */
    Info = 2,  /* scan brackets, stage boundaries, checkpoints */
};

/* The host sink: one formatted line (no trailing newline), NUL-terminated,
 * never null. Called under the seam's mutex from any thread. */
using SinkFn = void (*)(Level level, const char* line, void* user);

/* Register/unregister the sink (nullptr = silent). Any time, any thread. */
void set_sink(SinkFn fn, void* user);

/* Fast path check: false when no sink is registered (the pre-N-048 world
 * - engines call this before building any message text). */
bool enabled();

/* Format (printf-style, bounded to 512 bytes) and deliver. A no-op when
 * silent. Safe from any thread, including engine workers. */
void write(Level level, const char* fmt, ...);

} // namespace log
} // namespace cr

#endif // CR_LOG_H_
