/*
 * cr_reghive_edit.h - the offline registry-hive VALUE EDITOR (M-113,
 * diff-to-fix): the write side of the reghive module.
 *
 * Corpus: the og's recovery-side boot surgery performs offline hive
 * writes through RegLoadKey + RegSetValueEx + RegUnLoadKey (the
 * genuefisystemfile.cpp chain static-linked in six EPM DLLs -
 * RecoveryModule_decompiled.c :73731-73790 [TempSystem/MountedDevices],
 * :74837-74887 [TmpBCD, the {9dea862c-...} 23000003 walk + the 12000004
 * element rewrite], :73605-73621 [TmpSoftWare/ProductName - a READ, the
 * offlineos profile], :42845-42860 [bootstat.dat byte@10 = 1]).
 *
 * The core's form: DIRECT hive-file surgery through the same regf model
 * the read-side parser owns - no OS registry API, no elevation, works on
 * dead-system images (the recovery context; RegLoadKey needs a live
 * registry service and an elevated token). The og itself parses hives
 * in-house elsewhere (bitlockerhelp's RegLib family); the direct-editor
 * is the layer-faithful core-scale shape for the same writes.
 *
 * Editing model (Windows hive semantics):
 *   - values are REWRITTEN, never created (the og's surgeries only touch
 *     values a real Windows installation already carries);
 *   - data <= 4 bytes lives inline in the vk's data-offset dword
 *     (bit 31 set - both the core parser and Windows read it there);
 *   - larger data sits in its own cell; an in-place rewrite reuses the
 *     cell when the new length fits its allocated capacity, otherwise a
 *     fresh cell is allocated (first-fit over free cells, else a newly
 *     appended hbin - whole 0x1000 blocks, the format's own growth
 *     pattern) and the old cell is marked free;
 *   - every mutation bumps the header sequence numbers and refreshes the
 *     XOR checksum, so the edited hive still passes the parser's (and
 *     Windows') open validation.
 *
 * Safety: bounds-checked at every offset like the parser; values larger
 * than the read surface's own cap (kFveMaxHiveValueBytes) are refused;
 * absent keys/values are honest errors (ERR_* - never a silent no-op).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_REGHIVE_EDIT_H_
#define CR_REGHIVE_EDIT_H_

#include "cr_reghive.h"
#include <vector>

namespace cr {
namespace reghive {

/* registry value types (the wire set the surgeries use) */
constexpr u32 REG_TYPE_SZ = 1;
constexpr u32 REG_TYPE_BINARY = 3;

class HiveEditor {
public:
    /* Validate (exactly like RegistryHive::open) and take a MUTABLE COPY
     * of the hive bytes. Returns false on any validation failure. */
    bool open(const u8* data, size_t size);

    /* True when the key path + value name exist (case-insensitive walk,
     * the parser's own semantics). */
    bool has_value(const char* key_path, const char* value_name) const;

    /* Rewrite one existing value. The key and the value must BOTH exist
     * (the surgeries never create); returns ERR_BAD_PARAMETER for absent
     * targets, ERR_RANGE_OUT_OF_BOUNDS for data over the read surface's
     * own cap, OK on success. The type is rewritten with the data. */
    u32 set_value(const char* key_path, const char* value_name, u32 type, const u8* data,
                  size_t len);

    /* The edited hive bytes (header seq/checksum current). */
    const std::vector<u8>& bytes() const { return buf_; }

    /* Read-back helper (the editor's own view - round-trip checks
     * without re-opening a separate parser). */
    bool read_value(const char* key_path, const char* value_name, u32& out_type,
                    std::vector<u8>& out_data) const;

    /* The header's sequence numbers after the edits (monotonic; the
     * tests pin the bump). */
    u32 sequence() const { return seq_; }

private:
    /* find the vk record's bins-relative offset for (key, name); 0 = absent */
    u32 find_vk_(const char* key_path, const char* value_name) const;
    u32 allocate_cell_(size_t need_bytes);
    void free_cell_(u32 off);
    void refresh_header_();

    std::vector<u8> buf_;
    RegistryHive view_; /* re-opened after every mutation */
    u32 seq_ = 0;
};

} // namespace reghive
} // namespace cr

#endif // CR_REGHIVE_EDIT_H_
