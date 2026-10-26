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

} // namespace

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
        return "Current pending sectors";
    case 198:
        return "Offline uncorrectable";
    case 199:
        return "UDMA CRC errors";
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
    out.smart_version = u16(table[0]) | (u16(table[1]) << 8);

    /* 2b. SMART READ_THRESHOLDS (feature 0xD1, S-024): vendor
     * thresholds for the Bad rule; failure is non-fatal. */
    u8 thr[512];
    const bool got_thr = active->issue(0xD1, 0, 0xB0, thr);

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
        if (got_thr) {
            const u8* tp = thr + 2 + e * 12;
            if (tp[0] == a.id)
                a.vendor_threshold = tp[1];
        }
        /* flags bit0 = pre-failure (advisory) attribute; bit3 = FAILED NOW.
         * A pre-failure attribute that has decayed to zero is failing. */
        a.failing = (flags & 0x0008) != 0 || ((flags & 0x0001) != 0 && a.value == 0);
        a.name = smart_attr_name(a.id);
        out.attributes.push_back(a);
        if (a.failing)
            out.failing_now = true;
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

    /* drive model string: IDENTIFY words 27-46 (byte-swapped pairs);
     * the og's is-SSD flag and vendor family derive from it. */
    for (u32 w = 27; w <= 46; ++w) {
        const char c1 = char(identify[w * 2 + 1]);
        const char c2 = char(identify[w * 2]);
        if (c1 != ' ')
            out.model.push_back(c1);
        if (c2 != ' ')
            out.model.push_back(c2);
    }
    out.model_says_ssd = model_says_ssd(out.model);

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
