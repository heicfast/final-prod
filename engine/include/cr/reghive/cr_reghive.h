/*
 * cr_reghive.h - read-only Windows registry hive (regf) parser + BitLocker
 * FVEAutoUnlock key harvest (diff-to-fix S-014).
 *
 * Purpose: the offline (dead-system) BitLocker auto-unlock path. The og's
 * bitlockerhelp mounts the target volume's own SYSTEM hive
 * ("<root>\\Windows\\System32\\config\\SYSTEM" - bitlockerhelp_decompiled.c
 * :3128-3155) and reads \\ControlSet001\\Control\\FVEAutoUnlock\\<volume-GUID>\\Data
 * binary blobs, feeding them to the same unlock flow as the live registry
 * (DRWService_decompiled.c:104-140). This module implements the equivalent
 * read-only walk as a pure, portable parser (no OS API - the og's RegLib is
 * an in-house parser too; corpus citations in the .cpp).
 *
 * Safety: strictly read-only, bounds-checked at every offset, size-capped
 * (records <= kFveMaxRecords, blobs <= kFveMaxBlobBytes), no allocation
 * beyond the caller's vectors.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_REGHIVE_H_
#define CR_REGHIVE_H_

#include "../cr_types.h"
#include <cstddef>
#include <string>
#include <vector>

namespace cr {
namespace reghive {

/* ---------- regf constants (corpus-decoded; see cr_reghive.cpp) ---------- */

constexpr u32 REGF_HEADER_SIZE = 0x1000;
constexpr u32 REGF_CHECKSUM_OFF = 0x1FC;
constexpr u32 REGF_ROOT_CELL_OFF = 0x24; /* relative to first hbin */
constexpr u32 REGF_BINS_SIZE_OFF = 0x28; /* total hbin bytes */
constexpr u32 REGF_CELL_ALIGN = 8;
constexpr u32 HBIN_HEADER_SIZE = 0x20;

/* ---------- FVE auto-unlock model (corpus caps) ---------- */

/* DRWService pipe record: guid[50] + pad[10] + keyBlob[256], <=100 records
 * (artifacts/DRWService-exe--bitlocker-service.md :56-74, :1461-1593). */
constexpr size_t kFveMaxRecords = 100;
constexpr size_t kFveMaxBlobBytes = 256;
constexpr size_t kFveGuidMaxChars = 50;

struct AutoUnlockKey {
    std::string volume_guid; /* subkey name ("{...}" volume GUID), ASCII */
    std::vector<u8> blob;    /* the "Data" REG_BINARY payload (capped) */
};

/* Parsed hive view. The buffer is not copied: the caller keeps `data`
 * alive for the hive's lifetime. */
class RegistryHive {
public:
    /* Validates the regf magic, the XOR header checksum and the root cell,
     * exactly like the og's RegAPI::OpenHive path (RegLib_decompiled.c
     * :2376-2377: magic 'regf' + FUN_180007670 checksum == header[0x1FC]).
     * Returns false on any validation failure or truncated buffer. */
    bool open(const u8* data, size_t size);

    /* Case-insensitive key walk from the root, '\' separated, leading
     * separators allowed ("\\ControlSet001\\Control\\FVEAutoUnlock").
     * Names are ASCII-folded (registry semantics). Returns a key handle
     * or 0 when absent. */
    u64 find_key(const char* path) const;

    /* One-level case-insensitive child lookup (registry semantics). */
    u64 find_subkey(u64 parent, const char* name) const;

    /* Enumerate the subkey NAMES of a key handle (nk-based; both name
     * encodings). Returns false when the handle is invalid. */
    bool enum_subkeys(u64 key, std::vector<std::string>& out) const;

    /* Read a named value's data. out_type: 1 REG_SZ, 3 REG_BINARY, ...
     * (REG_BINARY == 3 is the only type the FVE harvest selects, matching
     * the og's "Data" reads). Returns false when absent / oversized
     * (> kFveMaxHiveValueBytes) / unreadable. */
    bool read_value(u64 key, const char* name, u32& out_type, std::vector<u8>& out_data) const;

    /* Value data larger than this is refused (the og routes such values
     * through its BigData class; values that big are never FVE keys, whose
     * blob cap is 256 bytes). */
    static constexpr size_t kFveMaxHiveValueBytes = 16344;

private:
    const u8* base_ = nullptr;
    size_t size_ = 0;
    u32 bins_end_ = 0;  /* total hbin bytes (header+0x28), bounds offsets */
    u32 root_cell_ = 0; /* root cell offset (header+0x24) */
    u32 cell_stride_ = 4;

    bool cell_at(u32 offset, const u8*& rec, i32& cell_size) const;
    bool nk_subkeys_list(u32 nk_off, u32& list_off) const;
    bool nk_values_list(u32 nk_off, u32& list_off, u32& count) const;
    bool collect_list_offsets(u32 list_off, std::vector<u32>& out, u32 depth) const;
    std::string nk_name(const u8* nk) const;
};

/* Harvest FVEAutoUnlock keys from an open hive, walking the og's hardcoded
 * \\ControlSet001\\Control\\FVEAutoUnlock family (bitlockerhelp_decompiled.c
 * :3140-3146 - the og does NOT consult the Select key; ControlSet001 only).
 * Applies the corpus caps: at most kFveMaxRecords, blobs capped at
 * kFveMaxBlobBytes, subkey names at kFveGuidMaxChars. Returns the number of
 * harvested keys. */
size_t harvest_fve_auto_unlock(const RegistryHive& hive, std::vector<AutoUnlockKey>& out);

/* Selection order for unlock attempts against a target volume GUID:
 * the og associates the record's GUID with the target volume; the matching
 * GUID is attempted first, the remaining records follow in enumeration
 * order (S-014 fix plan: prefer the match, fall back to the others).
 * target_guid may be null/empty (no association possible - all records in
 * enumeration order). Caps are applied by the harvester, not here. */
std::vector<const AutoUnlockKey*>
select_auto_unlock_attempts(const std::vector<AutoUnlockKey>& keys, const char* target_guid);

/* Case-insensitive ASCII compare (either may be null). */
bool guid_equals_ci(const std::string& a, const char* b);

} // namespace reghive
} // namespace cr

#endif // CR_REGHIVE_H_
