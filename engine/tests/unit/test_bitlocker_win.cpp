/* test_bitlocker_win.cpp - BitLocker auto-unlock contract tests (Windows
 * lane only; diff-to-fix S-014).
 *
 * The live-registry enumeration is exercised through the detail seam with
 * an HKCU fixture (HKLM writes need admin); the path constant itself is
 * pinned against the og's DRWService string. The offline path is exercised
 * end-to-end with a synthetic regf hive file and a recorder unlock
 * function - a real BitLocker volume is a Windows-lab artifact, exactly as
 * the finding's own reproduction section records.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/cr_error.h" /* Windows-lane unblock (core-api session): the S-014
                          * leg's test uses ERR_FVE_LOCKED/OK unqualified; on the
                          * Linux lane cr_bitlocker.h's include chain reaches
                          * cr_error.h transitively, on MSVC it does not - the
                          * Windows build of ed00020 failed on exactly this
                          * (run 40222295117). One explicit include, both lanes
                          * deterministic. */
#include "cr/win/cr_bitlocker.h"
#include "cr/reghive/cr_reghive.h"
#include <windows.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::win::bitlocker;
using cr::reghive::AutoUnlockKey;

namespace {

/* ---------------- synthetic hive bytes (same builder as test_reghive) --- */

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32(std::vector<u8>& v, size_t at, u32 x) {
    for (int i = 0; i < 4; ++i)
        v[at + i] = u8(x >> (8 * i));
}
u32 get32(const std::vector<u8>& v, size_t at) {
    return u32(v[at]) | (u32(v[at + 1]) << 8) | (u32(v[at + 2]) << 16) | (u32(v[at + 3]) << 24);
}

class HiveBuilder {
public:
    HiveBuilder() {
        bins_.resize(0x1000, 0);
        std::memcpy(bins_.data(), "hbin", 4);
        put32(bins_, 4, 0);
        put32(bins_, 8, 0x1000);
        root_off_ = 0x20;
        add_nk(0, "ROOTKEY", 0);
    }
    u32 add_nk(u32 parent, const std::string& name, u16 flags) {
        std::vector<u8> rec(0x4C + (flags & 0x20 ? name.size() : name.size() * 2), 0);
        rec[0] = 'n';
        rec[1] = 'k';
        put16(rec, 0x02, flags);
        const bool ascii = (flags & 0x20) != 0;
        if (ascii) {
            std::memcpy(rec.data() + 0x4C, name.data(), name.size());
            put16(rec, 0x48, u16(name.size()));
        } else {
            for (size_t i = 0; i < name.size(); ++i)
                put16(rec, 0x4C + i * 2, name[i]);
            put16(rec, 0x48, u16(name.size() * 2));
        }
        const u32 off = append_cell(rec);
        if (parent != 0)
            children_[parent].push_back(off);
        return off;
    }
    void add_vk(u32 nk, const std::string& name, u32 type, const std::vector<u8>& data) {
        std::vector<u8> rec(0x12 + name.size(), 0);
        rec[0] = 'v';
        rec[1] = 'k';
        put16(rec, 0x02, u16(name.size()));
        put16(rec, 0x10, 1);
        std::memcpy(rec.data() + 0x12, name.data(), name.size());
        put32(rec, 0x0C, type);
        if (data.size() <= 4) {
            put32(rec, 0x04, u32(data.size()) | 0x80000000u);
            for (size_t i = 0; i < data.size(); ++i)
                rec[0x08 + i] = data[i];
        } else {
            put32(rec, 0x04, u32(data.size()));
            put32(rec, 0x08, append_cell(data));
        }
        values_[nk].push_back(append_cell(rec));
    }
    std::vector<u8> build() {
        for (const auto& [parent, kids] : children_) {
            std::vector<u8> rec(4 + kids.size() * 8, 0);
            rec[0] = 'l';
            rec[1] = 'f';
            put16(rec, 0x02, u16(kids.size()));
            for (size_t i = 0; i < kids.size(); ++i) {
                put32(rec, 4 + i * 8, kids[i]);
                put32(rec, 4 + i * 8 + 4, 0);
            }
            patch_nk(parent, 0x14, u32(kids.size()));
            patch_nk(parent, 0x1C, append_cell(rec));
        }
        for (const auto& [nk, vks] : values_) {
            std::vector<u8> rec(vks.size() * 4, 0);
            for (size_t i = 0; i < vks.size(); ++i)
                put32(rec, i * 4, vks[i]);
            patch_nk(nk, 0x24, u32(vks.size()));
            patch_nk(nk, 0x28, append_cell(rec));
        }
        std::vector<u8> hive(0x1000 + bins_.size(), 0);
        std::memcpy(hive.data(), "regf", 4);
        put32(hive, 0x18, 6);
        put32(hive, 0x24, root_off_);
        put32(hive, 0x28, u32(bins_.size()));
        std::memcpy(hive.data() + 0x1000, bins_.data(), bins_.size());
        u32 acc = 0;
        for (u32 off = 0; off < 0x1FC; off += 4)
            acc ^= get32(hive, off);
        put32(hive, 0x1FC, acc);
        return hive;
    }
    u32 root() const { return root_off_; }

private:
    u32 append_cell(const std::vector<u8>& rec) {
        u32 off = next_off_;
        const size_t need = 4 + rec.size();
        const size_t aligned = (need + 7) & ~size_t(7);
        while (off + aligned > bins_.size())
            bins_.resize(bins_.size() + 0x1000, 0);
        put32(bins_, off, u32(-i64(aligned)));
        std::memcpy(bins_.data() + off + 4, rec.data(), rec.size());
        put32(bins_, 8, u32(bins_.size()));
        next_off_ = off + u32(aligned);
        return off;
    }
    void patch_nk(u32 nk_off, u32 field, u32 value) { put32(bins_, nk_off + 4 + field, value); }
    std::vector<u8> bins_;
    u32 root_off_ = 0;
    std::map<u32, std::vector<u32>> children_;
    std::map<u32, std::vector<u32>> values_;
    u32 next_off_ = 0x20;
};

std::wstring temp_hive_file(const std::vector<u8>& bytes) {
    static int n = 0;
    wchar_t dir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, dir);
    wchar_t path[MAX_PATH] = {};
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%scr_s14_%d.system", dir, ++n);
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY,
                           nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return {};
    DWORD wrote = 0;
    WriteFile(h, bytes.data(), DWORD(bytes.size()), &wrote, nullptr);
    CloseHandle(h);
    return path;
}

/* recorder for apply_auto_unlock_keys */
struct Recorder {
    std::vector<std::pair<std::string, std::vector<u8>>> attempts;
    u32 rc = ERR_FVE_LOCKED;
    static u32 thunk(wchar_t letter, const wchar_t* bek_path, void* user) {
        (void)letter;
        auto* self = static_cast<Recorder*>(user);
        std::vector<u8> bytes;
        HANDLE h = CreateFileW(bek_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER sz = {};
            if (GetFileSizeEx(h, &sz) && sz.QuadPart > 0 && sz.QuadPart < (1 << 20)) {
                bytes.resize(size_t(sz.QuadPart));
                DWORD got = 0;
                ReadFile(h, bytes.data(), DWORD(bytes.size()), &got, nullptr);
                bytes.resize(got);
            }
            CloseHandle(h);
        }
        std::wstring w = bek_path;
        /* MSVC C4244: a wchar_t->char narrowing via the string iterator ctor;
         * narrow explicitly (the paths are ASCII GUID/BEK names in this test) */
        std::string narrow;
        narrow.reserve(w.size());
        for (wchar_t c : w)
            narrow.push_back(char(u8(c)));
        self->attempts.emplace_back(std::move(narrow), std::move(bytes));
        return self->rc;
    }
};

const char kGuidA[] = "{aaaaaaaa-1111-2222-3333-444444444444}";
const char kGuidB[] = "{bbbbbbbb-1111-2222-3333-444444444444}";

std::vector<u8> blob_of(u8 fill, size_t n) {
    return std::vector<u8>(n, fill);
}

} // namespace

TEST(Bitlocker, AutoUnlockLivePathMatchesOgDrwService) {
    /* S-014: the og DRWService opens
     * HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock
     * (DRWService_decompiled.c:104). The pre-fix core opened
     * ...\Services\FVEAutoUnlock - the live unlock could never succeed. */
    EXPECT_STREQ("SYSTEM\\CurrentControlSet\\Control\\FVEAutoUnlock",
                 detail::fve_live_registry_path_a());
}

TEST(Bitlocker, AutoUnlockEnumeratesPerGuidDataValues) {
    /* HKCU fixture (the enumeration seam takes any opened root): two
     * volume-GUID subkeys with Data blobs + decoys that must be ignored. */
    HKEY root = nullptr;
    ASSERT_EQ(ERROR_SUCCESS,
              RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\cr_s14_test\\FVEAutoUnlock", 0,
                              nullptr, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &root,
                              nullptr));
    auto cleanup = [&]() {
        RegCloseKey(root);
        RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\cr_s14_test");
    };

    HKEY a = nullptr;
    ASSERT_EQ(ERROR_SUCCESS, RegCreateKeyExW(root, L"{aaaa-1111}", 0, nullptr, REG_OPTION_VOLATILE,
                                             KEY_ALL_ACCESS, nullptr, &a, nullptr));
    const std::vector<u8> blobA = blob_of(0x11, 64);
    ASSERT_EQ(ERROR_SUCCESS,
              RegSetValueExW(a, L"Data", 0, REG_BINARY, blobA.data(), DWORD(blobA.size())));
    RegCloseKey(a);

    HKEY b = nullptr;
    ASSERT_EQ(ERROR_SUCCESS, RegCreateKeyExW(root, L"{bbbb-2222}", 0, nullptr, REG_OPTION_VOLATILE,
                                             KEY_ALL_ACCESS, nullptr, &b, nullptr));
    const std::vector<u8> blobB(512, 0x22); /* > cap: truncated to 256 */
    ASSERT_EQ(ERROR_SUCCESS,
              RegSetValueExW(b, L"Data", 0, REG_BINARY, blobB.data(), DWORD(blobB.size())));
    /* decoy: REG_SZ "Data" ignored */
    const wchar_t str[] = L"notbinary";
    ASSERT_EQ(ERROR_SUCCESS, RegSetValueExW(b, L"Other", 0, REG_SZ,
                                            reinterpret_cast<const BYTE*>(str), sizeof(str)));
    /* decoy: tiny REG_BINARY ignored (BEK-class floor) */
    const BYTE tiny[] = {1, 2, 3};
    ASSERT_EQ(ERROR_SUCCESS, RegSetValueExW(b, L"Small", 0, REG_BINARY, tiny, sizeof(tiny)));
    RegCloseKey(b);

    std::vector<AutoUnlockKey> keys;
    const size_t n = detail::enum_auto_unlock_keys(root, keys);
    cleanup();
    ASSERT_EQ(size_t(2), n);
    EXPECT_EQ("{AAAA-1111}", keys[0].volume_guid); /* og's _strupr_s shape */
    EXPECT_EQ(blobA, keys[0].blob);
    EXPECT_EQ("{BBBB-2222}", keys[1].volume_guid);
    EXPECT_EQ(size_t(256), keys[1].blob.size()); /* the 256-byte corpus cap */
    EXPECT_EQ(0x22, keys[1].blob[0]);
}

TEST(Bitlocker, AutoUnlockPrefersMatchingVolumeGuid) {
    /* Two records; the target GUID matches the first. Only the matching
     * blob is attempted, in its recorded order (S-014 fix plan). */
    std::vector<AutoUnlockKey> keys;
    AutoUnlockKey ka;
    ka.volume_guid = kGuidA;
    ka.blob = blob_of(0xAA, 48);
    AutoUnlockKey kb;
    kb.volume_guid = kGuidB;
    kb.blob = blob_of(0xBB, 48);
    keys.push_back(ka);
    keys.push_back(kb);

    Recorder rec;
    rec.rc = OK;
    const u32 rc = detail::apply_auto_unlock_keys(L'Z', keys, kGuidA, &Recorder::thunk, &rec);
    ASSERT_EQ(OK, rc);
    ASSERT_EQ(size_t(1), rec.attempts.size()); /* match-only before success */
    EXPECT_EQ(std::vector<u8>(48, 0xAA), rec.attempts[0].second);
}

TEST(Bitlocker, AutoUnlockOfflineHive) {
    /* Dead-system scenario: the volume's key lives in another volume's
     * SYSTEM hive. The synthetic hive carries the FVE family under
     * ControlSet001; the recorder stands in for the real unlock. */
    HiveBuilder hb;
    const u32 cs = hb.add_nk(hb.root(), "ControlSet001", 0);
    const u32 control = hb.add_nk(cs, "Control", 0);
    const u32 fve = hb.add_nk(control, "FVEAutoUnlock", 0);
    const u32 sub = hb.add_nk(fve, kGuidA, 0);
    const std::vector<u8> blob = blob_of(0x42, 96);
    hb.add_vk(sub, "Data", 3, blob);
    const std::wstring path = temp_hive_file(hb.build());
    ASSERT_FALSE(path.empty());

    /* harvest through the offline entry point: probe with a recorder via
     * the seam by harvesting first, then applying */
    cr::reghive::RegistryHive hive;
    {
        std::vector<u8> bytes;
        HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
        ASSERT_NE(HANDLE(INVALID_HANDLE_VALUE), h);
        LARGE_INTEGER sz = {};
        ASSERT_TRUE(GetFileSizeEx(h, &sz));
        bytes.resize(size_t(sz.QuadPart));
        DWORD got = 0;
        ASSERT_TRUE(ReadFile(h, bytes.data(), DWORD(bytes.size()), &got, nullptr));
        CloseHandle(h);
        bytes.resize(got);
        ASSERT_TRUE(hive.open(bytes.data(), bytes.size()));
    }
    std::vector<AutoUnlockKey> keys;
    ASSERT_EQ(size_t(1), cr::reghive::harvest_fve_auto_unlock(hive, keys));
    ASSERT_EQ(kGuidA, keys[0].volume_guid);
    EXPECT_EQ(blob, keys[0].blob);

    Recorder rec;
    rec.rc = OK;
    ASSERT_EQ(OK, detail::apply_auto_unlock_keys(L'Z', keys, kGuidA, &Recorder::thunk, &rec));
    ASSERT_EQ(size_t(1), rec.attempts.size());
    EXPECT_EQ(blob, rec.attempts[0].second);

    /* the full offline API path: with no real BitLocker volume present the
     * unlock attempts fail (recorder not injectable there) - prove the
     * harvest ran by the ERR_FVE_LOCKED result, not a crash, and that a
     * garbage hive file is refused the same way */
    EXPECT_EQ(ERR_FVE_LOCKED, try_auto_unlock_offline(L'Z', path.c_str()));
    const std::wstring junk = temp_hive_file(std::vector<u8>(0x2000, 0xAB));
    EXPECT_EQ(ERR_FVE_LOCKED, try_auto_unlock_offline(L'Z', junk.c_str()));
    EXPECT_EQ(ERR_FVE_LOCKED, try_auto_unlock_offline(L'Z', nullptr));

    DeleteFileW(path.c_str());
    DeleteFileW(junk.c_str());
}

TEST(Bitlocker, AutoUnlockSelectionNoGuidFallsBackInOrder) {
    std::vector<AutoUnlockKey> keys;
    AutoUnlockKey ka;
    ka.volume_guid = kGuidA;
    ka.blob = blob_of(0x01, 40);
    AutoUnlockKey kb;
    kb.volume_guid = kGuidB;
    kb.blob = blob_of(0x02, 40);
    keys.push_back(ka);
    keys.push_back(kb);

    /* no volume GUID available (target GUID empty): all records in
     * enumeration order until one succeeds */
    Recorder rec;
    rec.rc = ERR_FVE_LOCKED;
    EXPECT_EQ(ERR_FVE_LOCKED,
              detail::apply_auto_unlock_keys(L'Z', keys, "", &Recorder::thunk, &rec));
    ASSERT_EQ(size_t(2), rec.attempts.size());
    EXPECT_EQ(std::vector<u8>(40, 0x01), rec.attempts[0].second);
    EXPECT_EQ(std::vector<u8>(40, 0x02), rec.attempts[1].second);
}

/* ---------------------------------------------------------------- */
/* N-064: the password-dictionary unlock aid                         */
/* ---------------------------------------------------------------- */

namespace {

/* recorder for the dictionary seam: candidate order + stop-on-success */
struct DictRecorder {
    std::vector<std::wstring> seen;
    u32 ok_at = 0; /* 1-based candidate position that unlocks */
    static u32 thunk(wchar_t /*letter*/, const wchar_t* password, void* user) {
        auto* self = static_cast<DictRecorder*>(user);
        self->seen.emplace_back(password);
        return self->ok_at != 0 && self->seen.size() >= self->ok_at ? OK : ERR_FVE_LOCKED;
    }
};

} // namespace

TEST(Bitlocker, DictionaryAttemptUsesManageBde) {
    /* The plumbing test (the finding's own spec): every dictionary attempt
     * rides the SAME command grammar as a user-typed password - the
     * PowerShell -Password primary and the manage-bde -Password legacy
     * fallback (corpus: BitLockerLib's manage-bde string-built command,
     * bitlockerlib--bitlocker-implementation.md :17/:44). */
    const std::wstring ps = detail::build_unlock_command(L'X', 0, L"hunter2", false);
    EXPECT_NE(std::wstring::npos, ps.find(L"Unlock-BitLocker -MountPoint \"X:\""));
    EXPECT_NE(std::wstring::npos, ps.find(L"-Password \"hunter2\""));
    const std::wstring mbde = detail::build_unlock_command(L'X', 0, L"hunter2", true);
    EXPECT_NE(std::wstring::npos, mbde.find(L"manage-bde -unlock X: -Password \"hunter2\""));

    /* the other key forms keep their grammar (regression pin) */
    EXPECT_NE(std::wstring::npos,
              detail::build_unlock_command(L'X', 1, L"48-digits", false)
                  .find(L"-RecoveryPassword"));
    EXPECT_NE(std::wstring::npos,
              detail::build_unlock_command(L'X', 2, L"C:\\k.beK", true).find(L"-KeyFilePath"));
}

TEST(Bitlocker, DictionaryUnlockConsentGated) {
    /* No attempts without the explicit action: the AUTO unlock surfaces
     * wired into the scan/open path (try_auto_unlock_offline here - the
     * same family try_auto_unlock drives live) attempt ONLY harvested
     * .bek keys, never a password candidate; the dictionary loop is a
     * separate entry point (unlock_with_dictionary) that nothing in the
     * open path calls - the CLI gates it behind --bitlocker-dict and
     * cr_ui_open_volume never invokes it (a locked volume stays
     * ERR_FVE_LOCKED until the host's explicit consent action). */
    std::vector<AutoUnlockKey> keys;
    AutoUnlockKey ka;
    ka.volume_guid = kGuidA;
    ka.blob = blob_of(0x01, 40);
    keys.push_back(ka);
    Recorder rec;
    rec.rc = ERR_FVE_LOCKED;
    EXPECT_EQ(ERR_FVE_LOCKED,
              detail::apply_auto_unlock_keys(L'Z', keys, kGuidA, &Recorder::thunk, &rec));
    ASSERT_EQ(size_t(1), rec.attempts.size()); /* bek attempts only */
    /* the dictionary loop, when explicitly driven, DOES run through its
     * own seam - the consent boundary is the call site, pinned here by
     * contrast: the auto path above never touched it. */
    DictRecorder dr;
    dr.ok_at = 2;
    size_t made = 0;
    const u32 rc = detail::dictionary_unlock_seam({}, true, &DictRecorder::thunk, L'Z', &dr,
                                                  &made);
    EXPECT_EQ(OK, rc);
    EXPECT_EQ(size_t(2), made);
    ASSERT_EQ(size_t(2), dr.seen.size());
    EXPECT_EQ(std::wstring(L"123456"), dr.seen[0]); /* the seed list's first entry widened */
}

TEST(Bitlocker, DictionarySeamOrderStopAndWideConversion) {
    /* the pwddict union: the seed list runs first (D-78 authored set),
     * then the user candidates, stop on first success, every candidate
     * widened for the -Password command grammar */
    const auto& seed = pwddict::seed_dictionary();
    DictRecorder dr;
    dr.ok_at = seed.size() + 1; /* first user candidate wins */
    const std::vector<std::string> user{"my own secret"};
    size_t made = 0;
    const u32 rc = detail::dictionary_unlock_seam(user, true, &DictRecorder::thunk, L'Z', &dr,
                                                  &made);
    EXPECT_EQ(OK, rc);
    EXPECT_EQ(seed.size() + 1, made);
    ASSERT_EQ(seed.size() + 1, dr.seen.size());
    /* shipped set in table order (spot: first + last), then the user set */
    EXPECT_EQ(L"123456", dr.seen[0]);
    EXPECT_EQ(std::wstring(seed.back().begin(), seed.back().end()),
              dr.seen[seed.size() - 1]);
    EXPECT_EQ(L"my own secret", dr.seen[seed.size()]);
}

TEST(Bitlocker, DictionaryUserOnlySkipsSeed) {
    /* include_seed=false: only the user candidates run (the
     * try_password_unlock file-dictionary form - the host supplied its
     * own list, the seed must not silently ride along) */
    DictRecorder dr;
    dr.ok_at = 1;
    const std::vector<std::string> user{"only mine"};
    size_t made = 0;
    const u32 rc = detail::dictionary_unlock_seam(user, false, &DictRecorder::thunk, L'Z', &dr,
                                                  &made);
    EXPECT_EQ(OK, rc);
    ASSERT_EQ(size_t(1), dr.seen.size());
    EXPECT_EQ(L"only mine", dr.seen[0]);
}
