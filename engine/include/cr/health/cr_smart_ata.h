/*
 * cr_smart_ata.h - SMART read flow over an injectable ATA transport
 * chain (pure core). The win layer provides the transports; this seam
 * owns the command sequence, the 30x12 attribute-table parse, the
 * IDENTIFY model-string extraction, and the S-024 grading pass.
 *
 * Corpus (N-032): the og DiskHealth companion's CAtaSmart engine has
 * per-transport accessors (GetSmartAttributeSat -1A/-2/-2A,
 * GetSmartAttributeCsmi, GetSmartAttributeNVMeRealtek) - one accessor
 * family serves the WHOLE command set for a disk: IDENTIFY (0xEC),
 * SMART READ DATA (0xB0 feature 0xD0), SMART READ THRESHOLDS (0xB0
 * feature 0xD1, S-024). The first transport whose IDENTIFY succeeds
 * serves the rest; when every transport rejects IDENTIFY the health
 * source is unavailable. This seam reproduces exactly that contract
 * with the chain order supplied by the platform layer.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SMART_ATA_H_
#define CR_SMART_ATA_H_

#include "../cr_types.h"
#include "cr_grade.h"
#include <string>
#include <vector>

namespace cr {
namespace health {

/* One SMART attribute row (ATA spec 512-byte table, 12-byte entries).
 * Same shape as the win-layer SmartAttribute - the win struct inherits
 * this one (N-032 refactor; consumers unchanged). */
struct SmartAttrInfo {
    u8 id = 0;
    u16 value = 0; /* current normalized */
    u16 worst = 0;
    u8 raw[6] = {0};          /* vendor raw */
    u16 vendor_threshold = 0; /* 0xD1 READ_THRESHOLDS (0 = unavailable) */
    bool failing = false;
    std::string name;
    HealthGrade grade = HealthGrade::Unknown; /* S-024 */
};

/* The parsed SMART report minus the platform SSD probe (seek-penalty
 * stays win-side; model-based SSD detection runs here via
 * model_says_ssd). */
struct SmartInfo {
    std::string transport; /* winning channel's name() */
    u16 ata_version = 0;
    u16 smart_version = 0;
    std::vector<SmartAttrInfo> attributes;
    bool failing_now = false;
    HealthGrade grade = HealthGrade::Unknown; /* S-024: worst attr grade */
    std::string model; /* IDENTIFY words 27-46, byte-swapped, space-stripped */
    bool model_says_ssd = false;
    /* derived verdicts (attr-id keyed) */
    u64 reallocated_sectors = 0; /* attr 5 raw */
    u64 pending_sectors = 0;     /* attr 197 raw */
    u64 uncorrectable = 0;       /* attr 198 raw */
    u64 power_on_hours = 0;      /* attr 9 raw */
    u64 spin_retry = 0;          /* attr 10 raw */
    u64 ssd_wear_level = 0;      /* attr 231/233 (vendor) */
    u64 temperature_c = 0;       /* attr 194/190 raw (vendor) */
};

/* One ATA transport (the og's accessor family). issue() runs a single
 * 512-byte data-in ATA command; false = this transport cannot serve
 * this disk right now. name() is recorded in SmartInfo::transport. */
struct AtaChannel {
    virtual ~AtaChannel() = default;
    virtual const char* name() const = 0;
    virtual bool issue(u8 feature, u8 count, u8 command, u8* out512) = 0;
};

/* Run the og's SMART flow over the chain: the first channel answering
 * IDENTIFY (0xEC) serves READ DATA (0xB0/D0) and READ THRESHOLDS
 * (0xB0/D1, non-fatal). Returns OK, or ERR_HEALTH_UNAVAILABLE when no
 * channel answers IDENTIFY / the table read fails. Empty chain is a
 * bad parameter. Channels are borrowed (not owned); nullptr entries
 * are skipped. */
u32 read_smart_ata(const std::vector<AtaChannel*>& chain, SmartInfo& out);

/* Human-readable attribute names for the well-known IDs (pure table;
 * the win header re-exports). */
const char* smart_attr_name(u8 id);

} // namespace health
} // namespace cr

#endif // CR_SMART_ATA_H_
