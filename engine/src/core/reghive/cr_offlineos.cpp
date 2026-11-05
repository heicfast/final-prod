/*
 * cr_offlineos.cpp - the offline Windows OS profile (M-082).
 *
 * The og DeviceManager walk, re-implemented from the decompile (corpus:
 * re-workspace/reversed/ghidra-mo/DeviceManager_decompiled.c - the line
 * cites at each step below). Pure reads over the reghive parser; no OS
 * API, no writes, no allocation beyond the caller's record.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_bits.h"
#include "cr/reghive/cr_offlineos.h"

#include <cstdio>
#include <cstring>

namespace cr {
namespace reghive {
namespace offlineos {

namespace {

/* Read a REG_SZ value under `key` and copy min(len, cap-1) bytes into a
 * fixed char[cap] field, NUL-terminated - the og's lstrcpynA(dst, src,
 * min(value_len, 0x80)) shape (:62038-62043 et al). Non-string types are
 * skipped (the og's helper is string-typed). */
void copy_string_field(const RegistryHive& hive, u64 key, const char* name, char* dst, size_t cap) {
    u32 type = 0;
    std::vector<u8> data;
    if (!hive.read_value(key, name, type, data))
        return; /* absent: the field stays empty - never fatal */
    if (type != 1 /* REG_SZ */)
        return;
    size_t n = data.size();
    if (n > cap - 1)
        n = cap - 1;
    /* the hive stores SZ with a NUL; strip trailing NULs from the copy len */
    while (n > 0 && data[n - 1] == 0)
        --n;
    std::memcpy(dst, data.data(), n);
    dst[n] = '\0';
}

/* The og's SystemRoot gate (:62113-62116): _strlwr_s, then accept only
 * "x:" with x in a-z (the decompile's (byte)(c + 0x9F) < 0x1A check over
 * the lowercased first char, plus buf[1] == ':'). */
bool accept_system_root(const char* src, char* dst, size_t cap) {
    if (src[0] == '\0')
        return false;
    std::string lower;
    for (const char* p = src; *p != '\0'; ++p)
        lower.push_back(char((*p >= 'A' && *p <= 'Z') ? *p + 0x20 : *p));
    if (lower.size() < 2 || lower[1] != ':' || lower[0] < 'a' || lower[0] > 'z')
        return false;
    const size_t n = lower.size() < cap - 1 ? lower.size() : cap - 1;
    std::memcpy(dst, lower.data(), n);
    dst[n] = '\0';
    return true;
}

u32 le32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

u64 le64(const u8* p) {
    return u64(le32(p)) | (u64(le32(p + 4)) << 32);
}

} // namespace

std::string resolve_control_set(const RegistryHive& system) {
    /* The og: read \Select\Current; when the read fails, fall back to
     * "\ControlSet001" (:62213-62226). On success the name is
     * "\ControlSet" + the "%03d" rendering of the dword (:62226-62245;
     * the format string DAT_18007d37c decoded from the shipped
     * DeviceManager.mo first-hand). */
    const u64 select = system.find_key("\\Select");
    if (select != 0) {
        u32 type = 0;
        std::vector<u8> data;
        if (system.read_value(select, "Current", type, data) && data.size() >= 4 && type == 4) {
            const u32 n = le32(data.data());
            /* the og's rendering: DAT_18007d37c is the "%03d" format
             * string (decoded from DeviceManager.mo .rdata at file offset
             * 0x7bf7c first-hand this session) - zero-padded 3 digits,
             * the real-Windows ControlSet001/002 naming */
            char tail[8];
            std::snprintf(tail, sizeof(tail), "%03u", n);
            return "\\ControlSet" + std::string(tail);
        }
    }
    return "\\ControlSet001";
}

bool profile_os(const RegistryHive* software, const RegistryHive* system, OsProfile& out) {
    if (software == nullptr)
        return false;
    OsProfile p{};

    /* ---- the SOFTWARE identity pass (:62034-62119) ---- */
    const u64 cv = software->find_key("\\Microsoft\\Windows NT\\CurrentVersion");
    if (cv == 0) {
        /* no CurrentVersion family: the og's walk yields the empty record
         * (every read fails, every field stays zero) and the SYSTEM pass
         * never engages (no drive letter). Still a well-formed result. */
        out = p;
        return true;
    }
    copy_string_field(*software, cv, "BuildLab", p.build_lab, sizeof(p.build_lab));
    copy_string_field(*software, cv, "BuildLabEx", p.build_lab_ex, sizeof(p.build_lab_ex));
    copy_string_field(*software, cv, "CSDVersion", p.csd_version, sizeof(p.csd_version));
    copy_string_field(*software, cv, "CurrentBuildNumber", p.current_build,
                      sizeof(p.current_build));
    copy_string_field(*software, cv, "CurrentVersion", p.current_version,
                      sizeof(p.current_version));
    copy_string_field(*software, cv, "ProductName", p.product_name, sizeof(p.product_name));
    copy_string_field(*software, cv, "ProductId", p.product_id, sizeof(p.product_id));

    /* SystemRoot: lowercased, drive-letter-gated (:62108-62119) */
    char root_buf[0x80] = {0};
    copy_string_field(*software, cv, "SystemRoot", root_buf, sizeof(root_buf));
    const bool have_root = accept_system_root(root_buf, p.system_root, sizeof(p.system_root));

    /* ---- the SYSTEM pass - only with a drive letter + a hive (:62120+) ---- */
    if (system != nullptr && have_root && p.system_root[0] != '\0') {
        /* \MountedDevices\DosDevices\<letter>: - the og builds the value
         * name from SystemRoot[0] + SystemRoot[1] ("C:") (:62137-62195).
         * The value must be EXACTLY 12 bytes: u32 signature + u64 offset,
         * stored at record +0x408/+0x410. */
        const u64 mounted = system->find_key("\\MountedDevices");
        if (mounted != 0) {
            /* the og builds the VALUE NAME as "\DosDevices\C:" (the
             * "\\DosDevices\\" literal + SystemRoot[0] + SystemRoot[1])
             * and reads it under the \MountedDevices key through its
             * (key-path, value-name) helper (:62137-62195); registry value
             * names under \MountedDevices literally carry the leading
             * backslash */
            std::string vname = "\\DosDevices\\";
            vname.push_back(p.system_root[0]);
            vname.push_back(p.system_root[1]);
            u32 type = 0;
            std::vector<u8> data;
            if (system->read_value(mounted, vname.c_str(), type, data) && type == 3 &&
                data.size() == 12) {
                p.disk_signature = le32(data.data());
                p.partition_offset = le64(data.data() + 4);
                p.has_mounted_device = true;
            }
        }

        /* the ControlSet resolution (:62213-62245) + ProductOptions
         * classification (:62275-62322) */
        const std::string cs = resolve_control_set(*system);
        const std::string po = cs + "\\Control\\ProductOptions";
        const u64 po_key = system->find_key(po.c_str());
        if (po_key != 0) {
            u32 type = 0;
            std::vector<u8> data;
            if (system->read_value(po_key, "ProductType", type, data) && type == 1) {
                std::string pt;
                for (u8 b : data)
                    if (b != 0)
                        pt.push_back(char(b));
                /* lstrcmpiA semantics: ASCII case-insensitive equality */
                auto ieq = [](const std::string& a, const char* b) {
                    if (a.size() != std::strlen(b))
                        return false;
                    for (size_t i = 0; i < a.size(); ++i) {
                        char ca = a[i], cb = b[i];
                        if (ca >= 'a' && ca <= 'z')
                            ca = char(ca - 0x20);
                        if (cb >= 'a' && cb <= 'z')
                            cb = char(cb - 0x20);
                        if (ca != cb)
                            return false;
                    }
                    return true;
                };
                if (ieq(pt, "WinNT"))
                    p.product_type = OsProfile::kProductWinNt;
                else if (ieq(pt, "LanmanNT"))
                    p.product_type = OsProfile::kProductLanmanNt;
                else if (ieq(pt, "ServerNT"))
                    p.product_type = OsProfile::kProductServerNt;
            }
        }
    }

    out = p;
    return true;
}

} // namespace offlineos
} // namespace reghive
} // namespace cr
