/* cr_sat.cpp - the SAT CDB stamps (pure; see cr_sat.h for the corpus
 * citations).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_sat.h"

namespace cr {
namespace health {
namespace sat {

bool build_cdb12(u8 feature, u8 count, u8 command, u8 device, bool stamp_1a, u8 out[12]) {
    /* Base stamp (the reference's case-4 encoding). CDB[1]=0x08 for the
     * primary stamp, 0x1C for the -1A retry (FUN_00419280 case 0xb). */
    out[0] = 0xA1; /* ATA PASS-THROUGH (12) opcode */
    out[1] = stamp_1a ? 0x1C : 0x08;
    out[2] = 0x0E;                       /* data-in direction + length encoding */
    out[3] = feature;                    /* ATA features register */
    out[4] = count != 0 ? count : u8(1); /* sector count (reference: 1) */
    out[5] = 0x00;                       /* LBA low */
    out[6] = 0x00;                       /* LBA mid */
    out[7] = 0x00;                       /* LBA high */
    out[8] = device;                     /* device select (0xA0: LBA mode + master) */
    out[9] = command;                    /* ATA command register */
    out[10] = 0x00;
    out[11] = 0x00;

    if (command == 0xB0) {
        /* SMART (0xB0): the SMART magic task file — sector 1, cyl low 0x4F,
         * cyl high 0xC2 (the same IDEREGS the reference bakes into its SAT,
         * legacy and CSMI routes). */
        out[5] = 0x01;
        out[6] = 0x4F;
        out[7] = 0xC2;
        return true;
    }
    if (command == 0xEC) {
        /* IDENTIFY DEVICE: feature 0, LBA fields zero (the reference's
         * identify sibling at DiskHealth_decompiled.c:9124 — local_1048 =
         * 0x000E08A1, local_1044 = 1, command 0xEC). */
        return true;
    }
    return false; /* not one of the reference's stamps — refuse, never invent */
}

} // namespace sat
} // namespace health
} // namespace cr
