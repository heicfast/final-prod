/* cr_lastscan.cpp - the last-scan pointer implementation (M-154).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include "cr/session/cr_lastscan.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace cr {
namespace session {

namespace {

std::FILE* ls_open(const char* path, bool write) {
#ifdef _WIN32
    /* UTF-8 -> UTF-16 for the Windows file APIs */
    const size_t n = std::strlen(path);
    std::wstring w;
    w.reserve(n);
    for (size_t i = 0; i < n;) {
        unsigned cp = 0xFFFD;
        const unsigned char c = path[i];
        if (c < 0x80 && i + 1 <= n) {
            cp = c;
            ++i;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < n) {
            cp = (unsigned(c & 0x1F) << 6) | (unsigned(path[i + 1]) & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < n) {
            cp = (unsigned(c & 0x0F) << 12) | ((unsigned(path[i + 1]) & 0x3F) << 6) |
                 (unsigned(path[i + 2]) & 0x3F);
            i += 3;
        } else if ((c & 0xF8) == 0xF0 && i + 3 < n) {
            cp = (unsigned(c & 0x07) << 18) | ((unsigned(path[i + 1]) & 0x3F) << 12) |
                 ((unsigned(path[i + 2]) & 0x3F) << 6) | (unsigned(path[i + 3]) & 0x3F);
            i += 4;
        } else {
            ++i; /* the U+FFFD fallback */
        }
        if (cp <= 0xFFFF) {
            w.push_back(wchar_t(cp));
        } else {
            cp -= 0x10000;
            w.push_back(wchar_t(0xD800 | (cp >> 10)));
            w.push_back(wchar_t(0xDC00 | (cp & 0x3FF)));
        }
    }
    std::FILE* f = nullptr;
    return _wfopen_s(&f, w.c_str(), write ? L"wb" : L"rb") == 0 ? f : nullptr;
#else
    return std::fopen(path, write ? "wb" : "rb");
#endif
}

bool ls_exists(const char* path) {
#ifdef _WIN32
    const size_t n = std::strlen(path);
    std::wstring w;
    w.reserve(n);
    for (size_t i = 0; i < n;) {
        unsigned cp = 0xFFFD;
        const unsigned char c = path[i];
        if (c < 0x80) {
            cp = c;
            ++i;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < n) {
            cp = (unsigned(c & 0x1F) << 6) | (unsigned(path[i + 1]) & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < n) {
            cp = (unsigned(c & 0x0F) << 12) | ((unsigned(path[i + 1]) & 0x3F) << 6) |
                 (unsigned(path[i + 2]) & 0x3F);
            i += 3;
        } else {
            ++i;
        }
        w.push_back(cp <= 0xFFFF ? wchar_t(cp) : L'?');
    }
    return GetFileAttributesW(w.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    return ::access(path, 0) == 0;
#endif
}

int ls_rename(const char* from, const char* to) {
#ifdef _WIN32
    /* replace semantics: the og's WritePrivateProfileStringA overwrites;
     * MoveFileExW with REPLACE is the atomic equivalent */
    const int n1 = MultiByteToWideChar(CP_UTF8, 0, from, -1, nullptr, 0);
    const int n2 = MultiByteToWideChar(CP_UTF8, 0, to, -1, nullptr, 0);
    if (n1 <= 0 || n2 <= 0)
        return -1;
    std::wstring wf(size_t(n1), L'\0'), wt(size_t(n2), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, from, -1, &wf[0], n1);
    MultiByteToWideChar(CP_UTF8, 0, to, -1, &wt[0], n2);
    return MoveFileExW(wf.c_str(), wt.c_str(), MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
#else
    return std::rename(from, to);
#endif
}

int ls_remove(const char* path) {
#ifdef _WIN32
    const size_t n = std::strlen(path);
    std::wstring w;
    w.reserve(n);
    for (size_t i = 0; i < n;) {
        unsigned cp = 0xFFFD;
        const unsigned char c = path[i];
        if (c < 0x80) {
            cp = c;
            ++i;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < n) {
            cp = (unsigned(c & 0x1F) << 6) | (unsigned(path[i + 1]) & 0x3F);
            i += 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < n) {
            cp = (unsigned(c & 0x0F) << 12) | ((unsigned(path[i + 1]) & 0x3F) << 6) |
                 (unsigned(path[i + 2]) & 0x3F);
            i += 3;
        } else {
            ++i;
        }
        w.push_back(cp <= 0xFFFF ? wchar_t(cp) : L'?');
    }
    return DeleteFileW(w.c_str()) ? 0 : -1;
#else
    return ::remove(path);
#endif
}

std::string sidecar_path(const std::string& dir) {
    std::string path = dir;
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
        path += '/';
    return path + "last_scan.info";
}

bool name_is_auto(const std::string& name) {
    /* the og's auto-list filter: names containing both "auto_" and the
     * extension (DRWUI:432256-432341, _stricmp-class); the core's own
     * writer only ever records auto_*.crsf names in the pointer.
     * S82 hardening rider: the name is also the RETENTION GATE - what it
     * admits gets REMOVED - so it must be a bare file name: separators,
     * traversal, and a missing/mismatched .crsf tail are refused (a
     * tampered sidecar cannot smuggle a path through the name field). */
    if (name.size() <= 5 || name.compare(0, 5, "auto_") != 0)
        return false;
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos)
        return false;
    if (name.find("..") != std::string::npos)
        return false;
    return name.size() >= 5 + 5 && name.compare(name.size() - 5, 5, ".crsf") == 0;
}

} // namespace

bool last_scan_info_read(const std::string& dir, LastScanInfo& out) {
    out = LastScanInfo{};
    std::FILE* f = ls_open(sidecar_path(dir).c_str(), false);
    if (!f)
        return false;
    char line[1100];
    while (std::fgets(line, sizeof(line), f)) {
        const std::string l(line);
        const auto eq = l.find('=');
        if (eq == std::string::npos)
            continue;
        const std::string key = l.substr(0, eq);
        std::string val = l.substr(eq + 1);
        while (!val.empty() && (val.back() == '\n' || val.back() == '\r'))
            val.pop_back();
        if (key == "save_path")
            out.save_path = val;
        else if (key == "name")
            out.name = val;
        else if (key == "count")
            out.count = std::strtoull(val.c_str(), nullptr, 10);
        else if (key == "time")
            out.time = std::strtoull(val.c_str(), nullptr, 10);
    }
    std::fclose(f);
    /* the og's PathFileExistsA validation (:443209-443214): a dead
     * pointer never blocks the new save - exists is informational */
    out.exists = !out.save_path.empty() && ls_exists(out.save_path.c_str());
    return !out.save_path.empty();
}

bool last_scan_info_write(const std::string& dir, const LastScanInfo& in) {
    const std::string path = sidecar_path(dir);
    const std::string tmp = path + ".tmp";
    std::FILE* f = ls_open(tmp.c_str(), true);
    if (!f)
        return false;
    std::fprintf(f, "save_path=%s\n", in.save_path.c_str());
    std::fprintf(f, "name=%s\n", in.name.c_str());
    std::fprintf(f, "count=%llu\n", static_cast<unsigned long long>(in.count));
    std::fprintf(f, "time=%llu\n", static_cast<unsigned long long>(in.time));
    std::fclose(f);
    return ls_rename(tmp.c_str(), path.c_str()) == 0;
}

bool last_scan_retention_delete_previous(const std::string& dir) {
    LastScanInfo info;
    if (!last_scan_info_read(dir, info) || !info.exists)
        return false; /* the dead-pointer no-op (the og's flag class) */
    /* the guard the og gets by construction: the pointer only ever
     * names a file the og's own writer recorded - an auto session.
     * A corrupted/hostile pointer naming a manual save must NEVER be
     * deleted: the name gate is the core's safety hardening (a
     * deliberate superset, registered with the finding). */
    if (!name_is_auto(info.name))
        return false;
    /* S82 hardening rider: the gate must govern what is actually removed -
     * the writer records save_path as dir + "/" + name, so a sidecar whose
     * two fields DISAGREE (a tampered pointer naming a valid auto_*.crsf
     * in `name` but an arbitrary target in `save_path`) must be refused,
     * not followed: the removed path's basename must BE the gated name. */
    if (info.save_path.size() < info.name.size() ||
        info.save_path.compare(info.save_path.size() - info.name.size(), info.name.size(),
                               info.name) != 0)
        return false;
    return ls_remove(info.save_path.c_str()) == 0;
}

} // namespace session
} // namespace cr
