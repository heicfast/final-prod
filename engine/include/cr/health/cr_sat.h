/*
 * cr_sat.h - the SAT (SCSI-to-ATA translation) 12-byte CDB stamps for
 * SMART over USB/IEEE1394 bridges (pure core; the win layer issues them).
 *
 * Corpus ground truth (byte-exact): EaseUS DiskHealth.exe (CrystalDiskInfo
 * lineage, REFERENCE-SMART.md §9) builds the SAME 12-byte ATA PASS-THROUGH
 * CDB in three sibling functions — SMART READ DATA (DiskHealth_decompiled.c
 * FUN_00419280), SMART READ THRESHOLDS (FUN_0041a440), IDENTIFY DEVICE (the
 * sibling at :9124) — with only the task-file bytes varying per command:
 *
 *   SMART READ DATA      A1 08 0E D0 01 01 4F C2 <dev> B0 00 00
 *   SMART READ THRESH.   A1 08 0E D1 01 01 4F C2 <dev> B0 00 00
 *   IDENTIFY DEVICE      A1 08 0E 00 01 00 00 00 <dev> EC 00 00
 *
 * The task-file mapping (SAT spec register layout, evidenced by the
 * decompile's constants): CDB[3]=ATA features, CDB[4]=sector count,
 * CDB[5..7]=LBA low/mid/high (the SMART magic 01 4F C2 — sector 1, cyl
 * C2/4F — exactly the legacy IDEREGS the corpus decodes at :14166-14176),
 * CDB[8]=device select (0xA0 master), CDB[9]=ATA command.
 *
 * The "-1A" retry variant (the corpus's "GetSmartAttributeSat - 1A" stamp,
 * FUN_00419280 case 0xb vs case 4) flips CDB[1] from 0x08 to 0x1C: some
 * bridges only answer that protocol encoding. Both stamps are tried in
 * order, exactly like the reference.
 *
 * The reference's vendor bridge dialects (cases 5-10 in FUN_00419280: the
 * Silicon-Image "SCSIDISK" miniport SRB, ASM1352R 0xE3/0xE0, JMicron, the
 * 23-branch AddDisk-USB ladder) are NOT reimplemented here: their payloads
 * are only partially decoded in the corpus (VERIFICATION V9) and the
 * generic stamps above cover every bridge that implements standard SAT.
 * Registered as DEFERRED in PARITY-DELTAS.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SAT_H_
#define CR_SAT_H_

#include "../cr_types.h"

namespace cr {
namespace health {
namespace sat {

/* Build one 12-byte SAT ATA PASS-THROUGH CDB for the (feature, count,
 * command) triple the AtaChannel contract issues. stamp_1a selects the
 * reference's -1A retry encoding (CDB[1] = 0x1C instead of 0x08).
 * Returns false for commands outside the reference's set {0xEC identify,
 * 0xB0 SMART} — this builder never invents a stamp it cannot cite.
 * count==0 is normalized to 1 (the reference bakes sector count 1 into
 * every stamp; read_smart_ata issues these commands with count 0). */
bool build_cdb12(u8 feature, u8 count, u8 command, u8 device, bool stamp_1a, u8 out[12]);

} // namespace sat
} // namespace health
} // namespace cr

#endif // CR_SAT_H_
