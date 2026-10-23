/*
 * cr_refs_detect.h - ReFS page-signature probe (diagnostic surface).
 *
 * Corpus: ReFSSearchDLL refs:: namespace (RefsMetadataBlockHeaderImpl
 * page parser, decompile-verified): page signatures "SUPBMSB+ReFSFSRS"
 * (superblock), "CHKPSUPBMSB+ReFSFSRS" (checkpointed superblock),
 * "MSB+ReFSFSRS" (metadata session block), page size >= 0x20.
 *
 * STATUS (W4, AUDIT-LINUX-FS DC-11): DIAGNOSTIC AND TEST SURFACE ONLY -
 * production has ZERO callers. The production ReFS identification gate is
 * the DBR OEM check in the orchestrator's probe_family ("ReFS" at +3,
 * CRefsVolumeBootRecord decompile gate), and the FULL engine (MSB-chain
 * reconstruction, checkpoint selection, ObjectID/directory-table walks,
 * $DATA data runs, deleted/ID2 harvest, allocator grading - see
 * cr_refs_engine.h / scan_refs) re-validates every page signature itself.
 * A future lost-ReFS-partition probe may wire this in; until then this is
 * not on any production path.
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
