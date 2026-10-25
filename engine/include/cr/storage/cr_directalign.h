/*
 * cr_directalign.h - the Windows direct-I/O envelope contract (GAP-S6-13).
 *
 * THE CONSTRAINT (mandate §4, non-negotiable): on Windows, direct reads
 * (FILE_FLAG_NO_BUFFERING semantics through raw \\.\PhysicalDriveN /
 * \\.\HarddiskVolumeN handles) whose LENGTH is a multiple of 4 KiB but not
 * of 64 KiB take a ~3.5x throughput slow path in the storage stack
 * (~830 MB/s vs ~2900 MB/s on NVMe). Every direct read path in core must
 * round lengths UP to a 64 KiB multiple, clamped at the device end.
 *
 * The envelope below also aligns the OFFSET (a 64 KiB-multiple length at an
 * unaligned offset is still a slow path): the issued read covers
 * [floor64k(off), ceil64k(off+len)) clamped to the device end, and the
 * requested bytes are copied out of it. When the request is already
 * 64 KiB-aligned and bounded, the caller reads straight into the caller's
 * buffer (no bounce, no copy).
 *
 * Portable pure math (unit-tested on every platform); the Windows transport
 * is the consumer. The RDA driver transport is a kernel IRP path and keeps
 * the logical-sector envelope.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_DIRECTALIGN_H_
#define CR_DIRECTALIGN_H_

#include "cr/cr_types.h"

namespace cr {
namespace io {

constexpr u64 kDirectAlignBytes = 64 * 1024; /* the fast-path granularity */

/* Envelope result for one [off, off+len) request against a device of
 * dev_size bytes with the 64 KiB direct alignment. */
struct DirectEnvelope {
    u64 issued_off = 0;  /* offset handed to ReadFile (64 KiB-aligned) */
    u64 issued_len = 0;  /* length handed to ReadFile (64 KiB multiple,
                          * EXCEPT the device-tail clamp) */
    u64 skip = 0;        /* bytes to skip from the issued read start */
    u64 take = 0;        /* bytes to copy out of the issued read */
    bool direct = false; /* true: the request itself is the fast path (no
                          * bounce buffer needed) */
    bool ok = false;     /* false: the request is out of bounds / empty */
};

inline DirectEnvelope direct_envelope(u64 off, u32 len, u64 dev_size) {
    DirectEnvelope e;
    if (len == 0 || off >= dev_size)
        return e;                   /* empty or past the end: nothing to issue */
    const u64 end = off + u64(len); /* off+len <= 16 TiB devices: no overflow
                                     * with u64 math on u32 len */
    const u64 a_off = (off / kDirectAlignBytes) * kDirectAlignBytes;
    u64 a_end = ((end + kDirectAlignBytes - 1) / kDirectAlignBytes) * kDirectAlignBytes;
    /* clamp at the device end (the documented tail exception: the last
     * envelope is ragged by exactly the device tail) */
    if (a_end > dev_size)
        a_end = dev_size;
    if (a_end <= a_off)
        return e; /* degenerate after clamping */
    e.issued_off = a_off;
    e.issued_len = a_end - a_off;
    e.skip = off - a_off;
    const u64 avail = a_end - off; /* bytes of the request the envelope covers */
    e.take = u64(len) < avail ? u64(len) : avail;
    /* the fast path: aligned offset, 64 KiB-multiple length, fully inside
     * the device (no tail clamp) - read directly into the caller's buffer */
    e.direct = (a_off == off) && (e.issued_len % kDirectAlignBytes == 0) && (a_end == end) &&
               (e.take == u64(len));
    e.ok = true;
    return e;
}

} // namespace io
} // namespace cr

#endif /* CR_DIRECTALIGN_H_ */
