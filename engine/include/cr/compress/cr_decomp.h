/*
 * cr_decomp.h - block decompression layer for compressed extents.
 *
 * Engines that encounter on-disk compression (ZFS lzjb/lz4/zle/gzip/zstd;
 * any future engine with compressed extents) tag the extent with a generic
 * codec id; the recovery writer decodes at stream time so scan memory stays
 * flat. Reference algorithms: OpenZFS module/zfs/{lzjb.c,zle.c} (in-house
 * ports, decompile-grade fidelity), zlib (gzip levels are one deflate
 * family), liblz4 LZ4_decompress_safe (ZFS lz4 is the reference block
 * format), libzstd (ZSTD_decompress; skippable frames handled internally).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_DECOMP_H_
#define CR_DECOMP_H_

#include "../cr_types.h"
#include <cstddef>

namespace cr {
namespace compress {

/* Generic extent codecs (ZFS zio_compress values map onto these). */
enum Codec : u32 {
    kRaw = 0,  /* plain bytes: psize == length */
    kLzjb = 1, /* OpenZFS lzjb */
    kZle = 2,  /* OpenZFS zero-length encoding */
    kGzip = 3, /* zlib raw deflate (gzip-1..9) */
    kLz4 = 4,  /* LZ4 block format */
    kZstd = 5, /* zstd frame (incl. ZFS skippable-frame prefix) */
};

/* Decompress psize bytes at src into exactly lsize bytes at dst.
 * Returns false on malformed input, wrong output size, or when the codec
 * was compiled out (CR_HAVE_* gates) - never silently truncates. */
bool decompress_block(u32 codec, const u8* src, size_t psize, u8* dst, size_t lsize);

/* True when the codec is available in this build. */
bool codec_available(u32 codec);

/* ZFS zio_compress enum (OpenZFS include/sys/zio_compress.h) -> generic
 * codec. Returns kRaw for inherit/on/off/empty (no payload transform). */
u32 codec_from_zfs(u32 zio_compress);

} // namespace compress
} // namespace cr

#endif // CR_DECOMP_H_
