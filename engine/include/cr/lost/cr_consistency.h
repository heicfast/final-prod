/*
 * cr_consistency.h - the lost-candidate content-consistency scorer (M-035).
 *
 * Clean-room reimplementation of the og's ENGINE-SIDE validation semantics
 * (exFATSearchDLL FUN_180025390 :30973-31156, decoded first-hand against the
 * corpus decompile; exFAT-only per OQ-FLT-1's cross-engine scan - zero
 * <= 0.3 compare sites outside exFATSearchDLL):
 *
 *   per candidate file (non-directory, from the walked tree):
 *     - data start = the file's first extent offset (the og computes
 *       cluster_size*(start_cluster-2)+base :31057-31063 - the same
 *       quantity our engines carry as the first extent),
 *     - checkable = the extension (last dot, uppercased) belongs to a
 *       signature family we can content-check. The og consults its
 *       FileFilter.xml Graphic/RAW set + TIFF/JPEG specials
 *       (FUN_180025d60 :31566-31680); ours is the carve registry's
 *       extension vocabulary + the same two specials (the shared-vocabulary
 *       decision blessed by OQ-FLT-1: the og's 169-entry table is the
 *       4-engine shared vocabulary, set-identical to DRWUI's grading set;
 *       camera-RAW families without a core signature stay uncheckable -
 *       bounded vocabulary, registered with the finding),
 *     - matched = a signature at the data start whose family matches the
 *       name: SUBSTRING strstr(name, family_ext) (FUN_180024fe0 :30785-
 *       30960) with the MOV/MP4/JPG family specials (:30858-30920: a
 *       .JPG/.JPEG name matches a MOV/MP4/JPG family signature).
 *
 *   verdict counters mirror the decompile exactly:
 *     considered = non-directory files (extentless files included - the
 *                  og's list count minus the 0x10-attribute skips),
 *     filterable = checkable files WITH extents (the og's start_cluster>1
 *                  gate; a checkable extentless file is considered but
 *                  neither filterable nor unfilterable, exactly like the
 *                  og's start<2 entries),
 *     ratio      = matched / filterable (0 when filterable == 0),
 *     reject     = considered/2 <= filterable && ratio < 0.3 &&
 *                  filterable != 0 (:31152-31155 - the decompile's
 *                  `<= 0.3 && != 0.3` is a STRICT <; ratio == 0.3 does NOT
 *                  reject. Documented correction to the finding text.)
 *
 * A file whose data start lies beyond the volume end aborts scoring with
 * rejected = true (the og's *param_3 invalidation, :31077-31083 - the
 * candidate's cluster math is structurally impossible).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_CONSISTENCY_H_
#define CR_CONSISTENCY_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include <string>
#include <vector>

namespace cr {
namespace lost {

/* One walked file, borrowed from the results tree (no copies). */
struct ConsistencyFile {
    const std::u16string* name = nullptr; /* null = unnamed (unfilterable) */
    u64 data_start = 0;                   /* slice-relative first-extent offset */
    u64 length = 0;                       /* logical size (bounds the probe) */
    bool has_extent = false;              /* false = the og's start_cluster<2 */
    bool is_directory = false;            /* the og's 0x10-attribute skip */
};

struct ConsistencyVerdict {
    u64 considered = 0;
    u64 filterable = 0;
    u64 matched = 0;
    double ratio = 0.0;         /* matched / filterable; 0 when filterable == 0 */
    bool rejected = false;      /* the og reject rule (see header comment) */
    bool consistent = false;    /* matched != 0 (the og's param_6 evidence flag) */
    bool out_of_bounds = false; /* a data start beyond the volume end */
};

/* Score one candidate's file list against the candidate slice. Read-bounded:
 * at most one 512-byte probe read per filterable file (through the caller's
 * device chain - the scan pipe's cache absorbs repeats). A read failure at
 * a data start is NOT an error here: the og's signature map simply has no
 * entry for unreadable offsets, so the file stays filterable-but-unmatched
 * (the failure itself surfaces through the retry layer's error map). */
ConsistencyVerdict score_content_consistency(IStorageDevice* slice, const ConsistencyFile* files,
                                             size_t count);

/* ---- pure helpers (unit-testable without a device) ---- */

/* Extension of a name: the substring after the last '.', uppercased ASCII.
 * Empty when no dot (the og's dot-less names are unfilterable). */
std::string consistency_extension(const std::u16string& name);

/* Checkable vocabulary membership: the extension has a content-signature
 * family (the carve registry's exts, uppercased) or is one of the og's two
 * filter-set specials (TIFF/JPEG - FUN_180025d60 :31659-31680). */
bool consistency_extension_checkable(const std::string& ext_upper);

/* The classifier: does a file NAME match a signature family's extension?
 * SUBSTRING over the uppercase ASCII projection of the name (the og's
 * strstr, FUN_180024fe0 :30843-30854) plus the MOV/MP4/JPG family specials
 * (:30858-30920 - a name whose extension is JPG or JPEG matches those
 * three families). sig_ext_upper is the registry signature's ext,
 * uppercased by the caller. */
bool consistency_name_matches_family(const std::u16string& name, const std::string& ext_upper,
                                     const char* sig_ext_upper);

} // namespace lost
} // namespace cr

#endif // CR_CONSISTENCY_H_
