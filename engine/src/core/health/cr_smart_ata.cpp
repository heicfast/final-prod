/* cr_smart_ata.cpp - SMART read flow over the transport chain (pure).
 *
 * The command sequence, the 30x12 attribute parse, the model-string
 * extraction, and the grading pass moved here from the win layer's
 * read_smart (N-032): the win layer now only supplies transports.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_smart_ata.h"
#include "cr/cr_error.h"
#include <cstring>

namespace cr {
namespace health {

namespace {

u16 word_at(const u8* p, u32 idx) {
    return u16(p[idx * 2]) | (u16(p[idx * 2 + 1]) << 8);
}

u64 triple(const u8* raw) {
    return u64(raw[0]) | (u64(raw[1]) << 8) | (u64(raw[2]) << 16);
}

/* M12 (W5): case-insensitive contains/prefix (the og's CStringT::Find
 * semantics — Find is case-sensitive in ATL, but the og uppercases the
 * model for the OCZ branch; the transports' model strings arrive in
 * mixed case, so the matcher folds). */
bool ci_contains(const std::string& hay, const char* needle) {
    const size_t n = std::strlen(needle);
    if (n == 0 || hay.size() < n)
        return false;
    for (size_t i = 0; i + n <= hay.size(); ++i) {
        size_t k = 0;
        while (k < n && (hay[i + k] | 0x20) == (needle[k] | 0x20))
            ++k;
        if (k == n)
            return true;
    }
    return false;
}

bool ci_starts(const std::string& hay, const char* prefix) {
    const size_t n = std::strlen(prefix);
    if (hay.size() < n)
        return false;
    for (size_t k = 0; k < n; ++k)
        if ((hay[k] | 0x20) != (prefix[k] | 0x20))
            return false;
    return true;
}

} // namespace

/* ---- M12 (W5): the vendor display family (the og's 0x181c cascade) ----
 * Gates: s6-health-grading-decode.md §5 (the product's own decoded
 * research, committed at engine/diff-to-fix/evidence/). Stage 1 takes
 * the model branches the decode spells out, in cascade order; stage 2
 * refines with the fully-decoded attr signatures. Families whose gates
 * the decode does not spell out (Apacer/Maxiotek/Marvell/
 * SiliconMotion/Ymtc/Ssstc/IntelDc) and the Phison firmware subgate are
 * DEFERRED — documented in the header, never invented. */
SmartVendorFamily smart_vendor_family_model(const std::string& model, bool model_says_ssd) {
    if (ci_starts(model, "WDC "))
        return SmartVendorFamily::Wdc; /* :6347-6359 */
    if (ci_contains(model, "SAMSUNG") || ci_starts(model, "MZ-"))
        return SmartVendorFamily::Samsung; /* FUN_00423bd0 */
    if (ci_contains(model, "INTEL"))
        return SmartVendorFamily::Intel; /* :6438 */
    if (ci_contains(model, "CRUCIAL") || ci_starts(model, "CT"))
        return SmartVendorFamily::MicronMU02; /* FUN_00422cc0 */
    if (ci_contains(model, "KINGSTON"))
        return SmartVendorFamily::Kingston; /* FUN_00422910 */
    if (ci_contains(model, "KIOXIA"))
        return SmartVendorFamily::Kioxia; /* :6886 */
    if (ci_contains(model, "OCZ"))
        return SmartVendorFamily::Ocz; /* :6530 (the uppercased prefix) */
    /* SanDisk's model-series forms (FUN_00423e10): the decoded series
     * list, contains-matched */
    if (ci_contains(model, "SANDISK") || ci_contains(model, "SDSSDP") ||
        ci_contains(model, "SDSSDRC") || ci_contains(model, "X110") || ci_contains(model, "X300") ||
        ci_contains(model, "X400") || ci_contains(model, "X600") || ci_contains(model, "ISSD"))
        return SmartVendorFamily::SanDisk;
    /* SK hynix's branch prefixes (:6714-6772) */
    if (ci_starts(model, "SHG") || ci_starts(model, "HFS") || ci_starts(model, "TND") ||
        ci_starts(model, "MND") || ci_starts(model, "TNF") || ci_contains(model, "SC311") ||
        ci_contains(model, "SC401"))
        return SmartVendorFamily::SKhynix;
    /* the fallback pair (:6826/:6998) */
    return model_says_ssd ? SmartVendorFamily::GenericSsd : SmartVendorFamily::Generic;
}

SmartVendorFamily smart_vendor_family_refine(SmartVendorFamily model_family,
                                             const std::vector<SmartAttrInfo>& attrs) {
    if (model_family != SmartVendorFamily::Generic && model_family != SmartVendorFamily::GenericSsd)
        return model_family; /* a model gate never demotes */
    /* signature check: the attr id SET must contain every signature id
     * (the og's per-family "attr sig" branches — the exact decoded sets) */
    auto has_all = [&attrs](const u16* sig, size_t n) {
        for (size_t k = 0; k < n; ++k) {
            bool found = false;
            for (const auto& a : attrs)
                if (a.id == sig[k]) {
                    found = true;
                    break;
                }
            if (!found)
                return false;
        }
        return true;
    };
    static const u16 kIronWolf[] = {1,    5,    9,    12,   100,  0x66, 0x67, 0xAA,
                                    0xAB, 0xAC, 0xB3, 0xB5, 0xB7, 0xB9, 0xBB};      /* :24312 */
    static const u16 kSandForce[] = {1, 5, 9, 12, 13, 100, 0xC0};                   /* :6470 */
    static const u16 kPlextor[] = {1, 5, 9, 12, 0xB1, 0xB2, 0xB5, 0xB6};            /* :6640 */
    static const u16 kJMicron60x[] = {1, 9, 12, 0xC2, 0xE5, 0xE8, 0xE9};            /* :6401 */
    static const u16 kOczVector[] = {5, 9, 12, 0xAB, 0xAE, 0xC3, 0xC4, 0xC5, 0xC6}; /* :6560 */
    /* Mtron (:6363): attr[0]==187 with attr count 1 */
    if (attrs.size() == 1 && attrs[0].id == 187)
        return SmartVendorFamily::Mtron;
    if (has_all(kIronWolf, sizeof(kIronWolf) / sizeof(kIronWolf[0])))
        return SmartVendorFamily::Seagate;
    if (has_all(kSandForce, sizeof(kSandForce) / sizeof(kSandForce[0])))
        return SmartVendorFamily::SandForce;
    if (has_all(kPlextor, sizeof(kPlextor) / sizeof(kPlextor[0])))
        return SmartVendorFamily::Plextor;
    if (has_all(kOczVector, sizeof(kOczVector) / sizeof(kOczVector[0])))
        return SmartVendorFamily::OczVector;
    if (has_all(kJMicron60x, sizeof(kJMicron60x) / sizeof(kJMicron60x[0])))
        return SmartVendorFamily::JMicron60x;
    return model_family;
}

const char* smart_attr_name(u8 id) {
    switch (id) {
    case 1:
        return "Read error rate";
    case 3:
        return "Spin-up time";
    case 4:
        return "Start/stop count";
    case 5:
        return "Reallocated sectors";
    case 7:
        return "Seek error rate";
    case 9:
        return "Power-on hours";
    case 10:
        return "Spin retry count";
    case 11:
        return "Calibration retries";
    case 12:
        return "Power cycles";
    case 13:
        return "Soft read errors";
    case 170:
        return "Reserved blocks (SSD)";
    case 171:
        return "Program failures (SSD)";
    case 172:
        return "Erase failures (SSD)";
    case 175:
        return "Program failures";
    case 183:
        return "SATA downshifts";
    case 187:
        return "Uncorrectable errors";
    case 190:
        return "Airflow temperature";
    case 194:
        return "Temperature";
    case 196:
        return "Reallocated events";
    case 197:
        return "Current Pending Sector Count"; /* M12: the og-exact observed
                                                * string */
    case 198:
        return "Offline uncorrectable";
    case 199:
        return "CRC Error Count"; /* M12: the og-exact observed string */
    case 188:
        return "Command Timeout"; /* M12: observed, was missing */
    case 22:
        return "Current Helium Level"; /* M12: observed (helium drives) */
    case 231:
        return "SSD life left";
    case 232:
        return "Endurance remaining";
    case 233:
        return "Media wearout (SSD)";
    case 234:
        return "Thermal throttling";
    case 241:
        return "Total LBAs written";
    case 242:
        return "Total LBAs read";
    case 249:
        return "NAND writes (SSD)";
    default:
        return "";
    }
}

/* M12 (W5): the family-aware resolution. The generic table above is
 * the universal base; the OVERLAYS carry only the decoded vendor
 * samples (REFERENCE-SMART §3.2's observed strings, family-pinned by
 * the s6 §5 decode where the signature identifies the drive class).
 * Every family whose full table is corpus-UNKNOWN falls back to the
 * generic name — never an invented string. */
const char* smart_attr_name(SmartVendorFamily family, u8 id) {
    switch (family) {
    case SmartVendorFamily::Seagate:
        /* the IronWolf-signature class: the observed Seagate naming for
         * the raw error rate (diskhealth_English.qm sample) */
        if (id == 1)
            return "Raw data error rate";
        break;
    default:
        break;
    }
    return smart_attr_name(id);
}

u32 read_smart_ata(const std::vector<AtaChannel*>& chain, SmartInfo& out) {
    if (chain.empty())
        return ERR_BAD_PARAMETER;

    /* 1. IDENTIFY DEVICE (0xEC): the first channel that answers serves
     * the whole command set (the og's per-transport accessor contract -
     * CAtaSmart picks the accessor family per disk, then runs every
     * command through it). */
    u8 identify[512];
    AtaChannel* active = nullptr;
    for (AtaChannel* ch : chain) {
        if (ch == nullptr)
            continue;
        u8 scratch[512];
        if (ch->issue(0, 0, 0xEC, scratch)) {
            std::memcpy(identify, scratch, sizeof(identify));
            active = ch;
            break;
        }
    }
    if (active == nullptr)
        return ERR_HEALTH_UNAVAILABLE;
    out.transport = active->name();

    out.ata_version = word_at(identify, 80);

    /* 2. SMART READ DATA (command 0xB0, feature 0xD0). */
    u8 table[512];
    if (!active->issue(0xD0, 0, 0xB0, table))
        return ERR_HEALTH_UNAVAILABLE;

    /* 2b. SMART READ_THRESHOLDS (feature 0xD1, S-024): vendor
     * thresholds for the Bad rule; failure is non-fatal. */
    u8 thr[512];
    const bool got_thr = active->issue(0xD1, 0, 0xB0, thr);

    /* drive model string: IDENTIFY words 27-46 (byte-swapped pairs);
     * the og's is-SSD flag and vendor family derive from it. */
    std::string model;
    for (u32 w = 27; w <= 46; ++w) {
        const char c1 = char(identify[w * 2 + 1]);
        const char c2 = char(identify[w * 2]);
        if (c1 != ' ')
            model.push_back(c1);
        if (c2 != ' ')
            model.push_back(c2);
    }
    return read_smart_ata_buffers(table, got_thr ? thr : nullptr, model, out);
}

u32 read_smart_ata_buffers(const u8* table, const u8* thresholds_or_null, const std::string& model,
                           SmartInfo& out) {
    out.model = model;
    out.smart_version = u16(table[0]) | (u16(table[1]) << 8);

    /* The 362-byte attribute table holds 30 twelve-byte entries at +2.
     * Entry layout (ATA-ATAPI-7): id@+0, flags(u16)@+1, value@+3,
     * worst@+4, raw[6]@+5, reserved@+11. */
    for (u32 e = 0; e < 30; ++e) {
        const u8* p = table + 2 + e * 12;
        if (p[0] == 0)
            continue; /* unused slot */
        const u16 flags = u16(p[1]) | (u16(p[2]) << 8);
        SmartAttrInfo a;
        a.id = p[0];
        a.value = p[3];
        a.worst = p[4];
        std::memcpy(a.raw, p + 5, 6);
        if (thresholds_or_null != nullptr) {
            const u8* tp = thresholds_or_null + 2 + e * 12;
            if (tp[0] == a.id)
                a.vendor_threshold = tp[1];
        }
        /* flags bit0 = pre-failure (advisory) attribute; bit3 = FAILED NOW.
         * A pre-failure attribute that has decayed to zero is failing. */
        a.failing = (flags & 0x0008) != 0 || ((flags & 0x0001) != 0 && a.value == 0);
        out.attributes.push_back(a);
        switch (a.id) {
        case 5:
            out.reallocated_sectors = triple(a.raw);
            break;
        case 9:
            out.power_on_hours = triple(a.raw);
            break;
        case 10:
            out.spin_retry = triple(a.raw);
            break;
        case 190:
        case 194:
            if (a.raw[0] != 0)
                out.temperature_c = a.raw[0];
            break;
        case 197:
            out.pending_sectors = triple(a.raw);
            break;
        case 198:
            out.uncorrectable = triple(a.raw);
            break;
        case 231:
        case 233:
            out.ssd_wear_level = triple(a.raw);
            break;
        default:
            break;
        }
    }

    out.model_says_ssd = model_says_ssd(out.model);
    /* M12 (W5): the vendor display family resolves AFTER the parse (the
     * signature refinement needs the full id set), then every row takes
     * its name from the family-aware table. */
    out.family = smart_vendor_family_refine(
        smart_vendor_family_model(out.model, out.model_says_ssd), out.attributes);
    for (auto& a : out.attributes) {
        a.name = smart_attr_name(out.family, a.id);
        if (a.failing)
            out.failing_now = true;
    }

    /* S-024: the 4-status grading pass (pure rules; the og DiskHealth
     * model). */
    {
        std::vector<SmartAttrInput> ais;
        ais.reserve(out.attributes.size());
        for (const SmartAttrInfo& a : out.attributes) {
            SmartAttrInput ai;
            ai.id = a.id;
            ai.value = a.value;
            ai.worst = a.worst;
            std::memcpy(ai.raw, a.raw, 6);
            ai.vendor_threshold = a.vendor_threshold;
            ai.failing_bit = a.failing;
            ais.push_back(ai);
        }
        const SmartGradeInputs gi = make_grade_inputs(out.model, ais);
        std::vector<HealthGrade> grades;
        out.grade = grade_smart(gi, grades);
        for (size_t k = 0; k < grades.size() && k < out.attributes.size(); ++k)
            out.attributes[k].grade = grades[k];
    }
    return OK;
}

} // namespace health
} // namespace cr
