/*
 * cr_refs_detect.h - ReFS page-signature probe (quick identification).
 *
 * Corpus: ReFSSearchDLL refs:: namespace (RefsMetadataBlockHeaderImpl
 * page parser, decompile-verified): page signatures "SUPBMSB+ReFSFSRS"
 * (superblock), "CHKPSUPBMSB+ReFSFSRS" (checkpointed superblock),
 * "MSB+ReFSFSRS" (metadata session block), page size >= 0x20.
 *
 * This is the lightweight probe used where a full scan is not warranted
 * (partition triage). The FULL engine - MSB-chain reconstruction,
 * checkpoint selection, ObjectID/directory-table walks, $DATA data runs,
 * deleted/ID2 harvest and allocator grading - lives in cr_refs_engine.h
 * (scan_refs) and is dispatched by the orchestrator's FsFamily::Refs path.
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
