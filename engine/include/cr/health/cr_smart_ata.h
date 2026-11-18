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

/* M12 (W5): the vendor display family — the og's 0x181c cascade
 * (FUN_0040dad0, s6-health-grading-decode.md §5) as an own-code enum.
 * Numeric values mirror the decoded codes (citation-anchored, not
 * ABI — core-internal). IMPLEMENTED gates: the model-prefix branches
 * the decode spells out + the fully-decoded attr signatures (the
 * IronWolf/SandForce/Plextor/JMicron60x/OczVector/Mtron set). DEFERRED
 * with precise scope: families whose gates the decode does not spell
 * out (Apacer/Maxiotek/Marvell/SiliconMotion/Ymtc/Ssstc/IntelDc —
 * s6 §5 gives call sites only), the Phison firmware "S9" subgate
 * (firmware is not parsed today), and the per-family NAME tables
 * beyond the decoded samples (REFERENCE-SMART §3.2's observed strings
 * — the full 30-family tables are corpus-UNKNOWN, never invented). */
enum class SmartVendorFamily : u8 {
    Generic = 0,    /* Smart: the HDD fallback */
    GenericSsd = 1, /* SmartSsd: the SSD fallback */
    Mtron = 2,
    Indilinx = 3,
    JMicron60x = 4,
    Intel = 5,
    Samsung = 6,
    SandForce = 7,
    Micron = 8,
    Ocz = 9,
    Seagate = 0xa, /* the IronWolf-signature branch */
    Wdc = 0xb,
    Plextor = 0xc,
    SanDisk = 0xd,
    OczVector = 0xe,
    Kingston = 0x11,
    MicronMU02 = 0x12, /* CRUCIAL/CT class */
    SKhynix = 0x15,
    Kioxia = 0x16,
    Ssstc = 0x17,
    IntelDc = 0x18,
    Apacer = 0x19,
    SiliconMotion = 0x1a,
    Phison = 0x1b,
    Marvell = 0x1c,
    Maxiotek = 0x1d,
    Ymtc = 0x1e,
};

/* Stage 1 (model-only, pre-attr): the decoded prefix/contains gates in
 * the cascade's order. Signature-gated families return Generic here and
 * refine in stage 2. model is the IDENTIFY words-27-46 string the parser
 * already builds (space-stripped, upper-case by caller convention is NOT
 * assumed — the matcher is case-insensitive like the og's CStringT::Find). */
SmartVendorFamily smart_vendor_family_model(const std::string& model, bool model_says_ssd);

/* Stage 2 (post-attr refinement): the fully-decoded attr-id signatures
 * (the attribute id SET must contain every signature id). Only families
 * whose signature the decode spells out are refined; a model-gated
 * family is never demoted. */
SmartVendorFamily smart_vendor_family_refine(SmartVendorFamily model_family,
                                             const std::vector<SmartAttrInfo>& attrs);

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
    /* M12 (W5): the resolved vendor display family (core-internal; the
     * name overlays consume it, the ABI report does not carry it). */
    SmartVendorFamily family = SmartVendorFamily::Generic;
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

/* Parse already-fetched 512-byte SMART payloads: the READ DATA table
 * (required) and the READ THRESHOLDS table (optional, null = missing),
 * with the model string supplied by the caller. The WMI fallback's path
 * (cr_health_wmi.cpp): MSStorageDriver_FailurePredict{Data,Thresholds}
 * serve the same 512-byte payloads but no IDENTIFY, so the model comes
 * from the storage stack (STORAGE_DEVICE_DESCRIPTOR — the same source
 * the og's WMI cross-check reads via Win32_DiskDrive) and ata_version
 * stays 0 (an IDENTIFY-only datum, honestly unavailable there). The
 * attribute parse, derived verdicts and S-024 grading are IDENTICAL to
 * the chain path — one parser, two sources, no divergence. */
u32 read_smart_ata_buffers(const u8* table, const u8* thresholds_or_null, const std::string& model,
                           SmartInfo& out);

/* Human-readable attribute names for the well-known IDs (pure table;
 * the win header re-exports). */
const char* smart_attr_name(u8 id);

/* M12 (W5): the family-aware resolution — the generic table, then the
 * decoded vendor overlays (Seagate's attr-1 "Raw data error rate" et
 * al.; the family's id set from the classifier). Falls back to the
 * generic name for every family whose table is corpus-UNKNOWN. */
const char* smart_attr_name(SmartVendorFamily family, u8 id);

} // namespace health
} // namespace cr

#endif // CR_SMART_ATA_H_
