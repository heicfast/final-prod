/* cr_nvramfix.cpp - the UEFI NVRAM Boot#### updater (M-040 stage b).
 *
 * Pure logic over an injected firmware-variable seam; the win layer
 * (src/win/cr_bootfix.cpp) owns Get/SetFirmwareEnvironmentVariableW
 * resolution, the privilege enable and the hive-file side.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/nvramfix/cr_nvramfix.h"
#include "cr/cr_error.h"

namespace cr {
namespace nvramfix {

namespace {

u16 le16_at(const u8* p) {
    return u16(u16(p[0]) | (u16(p[1]) << 8));
}
u32 le32_at(const u8* p) {
    return u32(u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24));
}
u64 le64_at(const u8* p) {
    u64 v = 0;
    for (int i = 7; i >= 0; --i)
        v = (v << 8) | p[i];
    return v;
}
void put32_at(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}
void put64_at(u8* p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = u8(v >> (8 * i));
}

bool sig16_eq(const u8* a, const u8* b) {
    return memcmp(a, b, 16) == 0;
}

/* Does this HARDDRIVE node match the target's OLD identity?
 * (FUN_1800046f0's semantics, first-hand decode: MBR mode matches the
 * disk signature AND start AND size; GPT mode matches the 16-byte
 * partition GUID AND start.) */
bool hd_node_matches(const u8* node, const TargetPartition& t) {
    const u8 sig_type = node[kHdSignatureType];
    const u8 mbr_type = node[kHdMbrType];
    if (t.gpt_mode) {
        if (sig_type != 2 || mbr_type != 2)
            return false;
        if (!sig16_eq(node + kHdSignature, t.old_gpt_guid))
            return false;
        return le64_at(node + kHdPartitionStart) == t.old_start;
    }
    if (sig_type != 1 || mbr_type != 1)
        return false;
    if (le32_at(node + kHdSignature) != t.old_mbr_signature)
        return false;
    return le64_at(node + kHdPartitionStart) == t.old_start &&
           le64_at(node + kHdPartitionSize) == t.old_size;
}

/* Patch one HARDDRIVE node with the NEW position/signature
 * (FUN_180004b40's rewrite :3660-3700: PartitionNumber, PartitionStart,
 * PartitionSize, Signature - the full-position rewrite for the moved
 * partition). */
void hd_node_rewrite(u8* node, const TargetPartition& t) {
    put32_at(node + kHdPartitionNumber, t.new_partition_number);
    put64_at(node + kHdPartitionStart, t.new_start);
    put64_at(node + kHdPartitionSize, t.new_size);
    if (t.gpt_mode)
        memcpy(node + kHdSignature, t.new_gpt_guid, 16);
    else
        memcpy(node + kHdSignature, &t.new_mbr_signature, 4);
}

/* Walk one EFI_LOAD_OPTION's device path: true when a HARDDRIVE node
 * matches the target (the entry is a rewrite candidate). The walk
 * mirrors the corpus exactly: the UTF-16 description at +6 is skipped
 * to its NUL (+1), then nodes advance by their u16 Length until the
 * END_ENTIRE (7f/ff) node. A node length below its own header size or
 * a walk past the buffer = malformed (bounded, stops). */
bool entry_matches(const std::vector<u8>& value, const TargetPartition& t) {
    if (value.size() < kLoadOptDescription + 2)
        return false;
    size_t p = kLoadOptDescription;
    /* the description: UTF-16 units to the NUL (the NUL unit itself is
     * consumed by the p += 2 - the corpus's walk: while (*p) p++; p++,
     * i.e. ONE step past the NUL, not two) */
    while (p + 1 < value.size()) {
        const u16 u = le16_at(value.data() + p);
        p += 2;
        if (u == 0)
            break;
    }
    if (p + 1 >= value.size())
        return false; /* no NUL / no room for any node: malformed */
    /* the device path nodes start AT p (the corpus's psVar6++ shape) */
    while (p + kNodeHeaderSize <= value.size()) {
        const u8 type = value[p];
        const u8 subtype = value[p + 1];
        const u16 len = le16_at(value.data() + p + 2);
        if (type == kTypeEndEntire && subtype == kSubTypeEndEntire)
            break; /* END_ENTIRE: the walk is done (the corpus's stop) */
        if (len < kNodeHeaderSize || p + len > value.size())
            break; /* malformed: bounded stop, never past the buffer */
        if (type == kTypeMedia && subtype == kSubTypeHarddrive && len >= kHdNodeSize) {
            if (hd_node_matches(value.data() + p, t))
                return true;
        }
        p += len;
    }
    return false;
}

/* Patch every HARDDRIVE node in a matched entry (the og rewrites every
 * harddrive node of a matched entry - an entry's device path can carry
 * more than one; the corpus loop is node-blind after the entry match).
 * Returns true when at least one node was rewritten. */
bool rewrite_entry(u8* data, size_t size, const TargetPartition& t) {
    if (size < kLoadOptDescription + 2)
        return false;
    size_t p = kLoadOptDescription;
    while (p + 1 < size) {
        const u16 u = le16_at(data + p);
        p += 2;
        if (u == 0)
            break;
    }
    if (p + 1 >= size)
        return false; /* no NUL / no room: malformed, bounded stop */
    bool any = false;
    while (p + kNodeHeaderSize <= size) {
        const u8 type = data[p];
        const u8 subtype = data[p + 1];
        const u16 len = le16_at(data + p + 2);
        if (type == kTypeEndEntire && subtype == kSubTypeEndEntire)
            break;
        if (len < kNodeHeaderSize || p + len > size)
            break;
        if (type == kTypeMedia && subtype == kSubTypeHarddrive && len >= kHdNodeSize &&
            data[p + kHdSignatureType] != 0) { /* a real signature: the og's != 0 gate */
            hd_node_rewrite(data + p, t);
            any = true;
        }
        p += len;
    }
    return any;
}

std::u16string boot_name(u16 id) {
    /* "Boot%04X" - the corpus's FUN_180004380 format */
    char16_t buf[9];
    const char16_t* hex = u"0123456789ABCDEF";
    buf[0] = 'B';
    buf[1] = 'o';
    buf[2] = 'o';
    buf[3] = 't';
    buf[4] = hex[(id >> 12) & 0xF];
    buf[5] = hex[(id >> 8) & 0xF];
    buf[6] = hex[(id >> 4) & 0xF];
    buf[7] = hex[id & 0xF];
    buf[8] = 0;
    return std::u16string(buf, 8);
}

} // namespace

std::vector<NvramRewrite> plan_boot_entries(GetVarFn get, void* get_user,
                                             const TargetPartition& target, u32* error) {
    if (error)
        *error = OK;
    std::vector<NvramRewrite> plan;
    if (get == nullptr) {
        if (error)
            *error = ERR_BAD_PARAMETER;
        return plan;
    }
    /* BootOrder: the u16 entry-id array (the og's 0x100000 read cap) */
    std::vector<u8> order(size_t(kMaxVariableBytes), 0);
    const u32 order_len = get(u"BootOrder", order.data(), u32(order.size()), get_user);
    if (order_len == 0 || (order_len & 1) != 0) {
        if (error)
            *error = ERR_READ_FAILED; /* absent or odd-sized: no BootOrder */
        return plan;
    }
    const size_t entries = order_len / 2 > kMaxBootEntries ? kMaxBootEntries : order_len / 2;
    std::vector<u8> value(size_t(kMaxVariableBytes), 0);
    for (size_t i = 0; i < entries; ++i) {
        const u16 id = le16_at(order.data() + i * 2);
        const std::u16string name = boot_name(id);
        const u32 len = get(name.c_str(), value.data(), u32(value.size()), get_user);
        if (len == 0)
            continue; /* absent entry in BootOrder: skip (the og's rc==2) */
        const std::vector<u8> original(value.begin(), value.begin() + len);
        if (!entry_matches(original, target))
            continue; /* not our partition: untouched */
        std::vector<u8> patched = original;
        if (!rewrite_entry(patched.data(), patched.size(), target)) {
            if (error)
                *error = ERR_BAD_MAGIC; /* matched but unwalkable: malformed */
            return std::vector<NvramRewrite>();
        }
        NvramRewrite r;
        r.name = name;
        r.old_value = std::move(original);
        r.new_value = std::move(patched);
        plan.push_back(std::move(r));
    }
    return plan;
}

u32 execute_plan(const std::vector<NvramRewrite>& plan, SetVarFn set, void* set_user,
                 ConsentFn consent, void* consent_user, size_t* applied) {
    if (applied)
        *applied = 0;
    if (plan.empty())
        return OK; /* nothing to do - an empty plan is a no-op */
    if (set == nullptr)
        return ERR_BAD_PARAMETER;
    if (consent == nullptr || !consent(consent_user))
        return ERR_REPAIR_REFUSED; /* P62 default-deny */
    for (const auto& r : plan) {
        if (!set(r.name.c_str(), r.new_value.data(), u32(r.new_value.size()), set_user))
            return ERR_WRITE_FAILED; /* stops at the first failure */
        if (applied)
            ++*applied;
    }
    return OK;
}

u32 undo_plan(const std::vector<NvramRewrite>& plan, SetVarFn set, void* set_user,
              size_t* applied) {
    if (applied)
        *applied = 0;
    if (plan.empty())
        return OK;
    if (set == nullptr)
        return ERR_BAD_PARAMETER;
    for (const auto& r : plan) {
        if (!set(r.name.c_str(), r.old_value.data(), u32(r.old_value.size()), set_user))
            return ERR_WRITE_FAILED;
        if (applied)
            ++*applied;
    }
    return OK;
}

} // namespace nvramfix
} // namespace cr
