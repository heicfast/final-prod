/*
 * cr_fixup.h - NTFS Update Sequence Array fixup (multi-sector header) validation.
 *
 * Mandatory before ANY record parse (fix F4 - reference had paths that skipped
 * it). Corpus chain (FUN_180041bb0): min parse size 0x400; USA offset must be
 * non-zero; per-sector USN match; then apply replacement. Our API separates
 * validation (no mutation) from in-place application so callers can keep
 * shared buffers pristine (the reference's copy-on-wrap semantics).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FIXUP_H_
#define CR_FIXUP_H_

#include "../cr_types.h"
#include "../cr_error.h"

namespace cr {
namespace ntfs {

/* Validate the USA of a record/block already loaded in memory.
 *   record: buffer holding the record (>= sector_count * sector_size bytes)
 *   usa_offset: from record header (bytes)
 *   usa_count: from record header (u16 count = sectors + 1)
 *   sector_size: logical sector size in bytes (512/4096 - fix F1)
 * Returns OK, ERR_FIXUP_FAILED, ERR_BAD_PARAMETER. Never touches the buffer. */
u32 fixup_validate(const u8 *record, u64 record_bytes, u32 usa_offset, u32 usa_count,
                   u32 sector_size);

/* Apply fixups in place (after validate). Each sector's trailing u16 is
 * restored from the USA slot. Returns ERR_FIXUP_FAILED and leaves the buffer
 * in the original state on any mismatch (applies back the originals). */
u32 fixup_apply(u8 *record, u64 record_bytes, u32 usa_offset, u32 usa_count, u32 sector_size);

} // namespace ntfs
} // namespace cr

#endif // CR_FIXUP_H_
