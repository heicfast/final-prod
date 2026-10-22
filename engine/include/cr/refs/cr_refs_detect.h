/*
 * cr_refs_detect.h - ReFS detection (report-only).
 *
 * Corpus: ReFSSearchDLL refs:: namespace (RefsMetadataBlockHeaderImpl
 * page parser, decompile-verified): page signatures "SUPBMSB+ReFSFSRS"
 * (superblock), "CHKPSUPBMSB+ReFSFSRS" (checkpointed superblock),
 * "MSB+ReFSFSRS" (metadata session block), page size >= 0x20.
 *
 * Detection only: ReFS metadata reconstruction (MSB chains, checkpoints,
 * B+ tree extents) is out of scope for the v1 core - volumes are detected,
 * identified (version/layout) and reported so the wizard never mis-labels
 * a ReFS disk as RAW/carvable.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_REFS_DETECT_H_
#define CR_REFS_DETECT_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"

namespace cr {
namespace refs {

struct RefsInfo {
    bool found = false;
    bool checkpointed = false; /* CHKPSUPB present: newer layout */
    u64 msb_offset = 0;        /* first metadata session block hit */
    u32 sequence = 0;          /* ReFS sequence (layout generation) */
    char version[16] = {0};    /* from the identification string */
};

/* Probe the volume start (first MiB) for ReFS pages. Report-only. */
u32 detect_refs(IStorageDevice* slice, RefsInfo& out);

} // namespace refs
} // namespace cr

#endif // CR_REFS_DETECT_H_
