/*
 * cr_surgery.cpp - the recovered-OS boot-environment surgery (M-113).
 * The header carries the corpus contract; this file is the operations.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/reghive/cr_surgery.h"
#include "cr/cr_error.h"

#include <cstring>
#include <string>

namespace cr {
namespace reghive {
namespace surgery {

namespace {

/* "\DosDevices\X:" - the value-name shape the offlineos read decodes
 * (DeviceManager :62137-62195: the literal + SystemRoot[0..1]); the
 * write mirrors it. */
std::string dos_devices_value_name(char letter) {
    std::string n = "\\DosDevices\\";
    n.push_back(letter);
    n.push_back(':');
    return n;
}

/* u16 string -> the raw little-endian bytes (the og writes wcslen*2) */
std::vector<u8> wide_bytes(const char16_t* s) {
    std::vector<u8> b;
    for (const char16_t* p = s; *p != 0; ++p) {
        b.push_back(u8(u16(*p) & 0xFF));
        b.push_back(u8(u16(*p) >> 8));
    }
    return b;
}

/* the 23000003 element's referenced object GUID, as the ASCII-folded
 * form the parser's u16-name decode yields (the element value data is a
 * UTF-16 string; the og wcslen-compares it against the boot-manager
 * literal). Returns false when absent/unreadable. */
bool read_referenced_object(const HiveEditor& ed, std::string& guid_out) {
    const std::string mgr_key =
        std::string("\\Objects\\") + kBcdBootManagerGuid + "\\Elements\\23000003";
    u32 type = 0;
    std::vector<u8> data;
    if (!ed.read_value(mgr_key.c_str(), "Element", type, data))
        return false;
    if (data.size() < 2 || (data.size() & 1) != 0)
        return false;
    guid_out.clear();
    for (size_t i = 0; i < data.size(); i += 2) {
        const u16 c = u16(data[i]) | (u16(data[i + 1]) << 8);
        if (c == 0)
            break; /* the string's terminator: wcslen semantics */
        guid_out.push_back(char(c & 0x7F));
    }
    return !guid_out.empty();
}

} // namespace

u32 rewrite_mounted_devices(std::vector<u8>& system_hive, const MountedDeviceMapping* mappings,
                            size_t count) {
    if (count != 0 && mappings == nullptr)
        return ERR_BAD_PARAMETER;
    HiveEditor ed;
    if (!ed.open(system_hive.data(), system_hive.size()))
        return ERR_SESSION_FORMAT; /* not a valid hive - the honest refuse */
    /* locate EVERY value first: a partial rewrite must never happen */
    for (size_t i = 0; i < count; ++i) {
        const char letter = mappings[i].letter;
        if (letter < 'A' || letter > 'Z')
            return ERR_BAD_PARAMETER; /* the drive-letter band the og's
                                       * SystemRoot gate accepts */
        if (!ed.has_value("\\MountedDevices", dos_devices_value_name(letter).c_str()))
            return ERR_BAD_PARAMETER;
    }
    /* the writes: 12-byte REG_BINARY, u32 signature + u64 offset LE */
    for (size_t i = 0; i < count; ++i) {
        u8 v[12] = {};
        const MountedDeviceMapping& m = mappings[i];
        for (int k = 0; k < 4; ++k)
            v[k] = u8(m.disk_signature >> (8 * k));
        for (int k = 0; k < 8; ++k)
            v[4 + k] = u8(m.partition_offset >> (8 * k));
        const u32 rc = ed.set_value("\\MountedDevices", dos_devices_value_name(m.letter).c_str(),
                                    REG_TYPE_BINARY, v, sizeof(v));
        if (rc != OK)
            return rc;
    }
    system_hive = ed.bytes();
    return OK;
}

u32 repoint_bcd_device_element(std::vector<u8>& bcd_hive, const char16_t* new_device_utf16) {
    if (new_device_utf16 == nullptr || *new_device_utf16 == 0)
        return ERR_BAD_PARAMETER;
    HiveEditor ed;
    if (!ed.open(bcd_hive.data(), bcd_hive.size()))
        return ERR_SESSION_FORMAT;
    /* the og's walk: the boot manager's 23000003 -> the referenced
     * application's 12000004 */
    std::string ref;
    if (!read_referenced_object(ed, ref))
        return ERR_BAD_PARAMETER; /* no boot-manager element: not a BCD
                                   * (never a guess) */
    if (ref == kBcdBootManagerGuid)
        return ERR_BAD_PARAMETER; /* the self-reference case the og's
                                   * wcslen compare guards - never
                                   * self-repoint */
    const std::string app_key = "\\Objects\\" + ref + "\\Elements\\12000004";
    if (!ed.has_value(app_key.c_str(), "Element"))
        return ERR_BAD_PARAMETER; /* the application device element absent */
    /* the og's write: type 1 (REG_SZ), the wide-string bytes, length
     * wcslen*2 (the decoded call at :74875-74878 - the finding's
     * "REG_BINARY" annotation corrected by the decode) */
    const std::vector<u8> bytes = wide_bytes(new_device_utf16);
    const u32 rc =
        ed.set_value(app_key.c_str(), "Element", REG_TYPE_SZ, bytes.data(), bytes.size());
    if (rc != OK)
        return rc;
    bcd_hive = ed.bytes();
    return OK;
}

} // namespace surgery
} // namespace reghive
} // namespace cr
