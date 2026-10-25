/*
 * rrecov - the core_recovery CLI (Windows).
 *
 * Commands:
 *   list                                  enumerate disks/volumes
 *   scan --disk N | --volume C: | --image FILE [--whole-disk] [--quick-only] [--no-carve]
 *         [--threads N] [--out file.crsf] [--resume file.crsf] [--json]
 *         [--log file] [--csv] [--filter s] [--deleted-only]
 *   tree --in file.crsf [--filter substr] [--deleted-only] [--csv] [--json]
 *   recover --in file.crsf --disk N --to DIR [--deleted-only] [--filter s]
 *         [--report file.json]
 *   health --disk N
 *   fix --disk N [--part P] [--apply] [--undo file] [--undo-apply file]
 *   bench --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]
 *         [--iterations N] [--seed N] [--json] [--no-rda]
 *
 * Exit codes: scan 0 ok / 1 failure / 3 cancelled (session saved when --out
 * was given); list 1 on enumeration failure; health 3 when every health
 * source is unavailable; recover 2 partial (item errors / hash mismatch).
 *
 * Everything is logged in detail (stage timings, per-stage counts, error
 * maps) for testing, validation and benchmarking workflows.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/query/cr_query.h"
#include "cr/repair/cr_rawfixer.h"
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/storage/cr_slice.h"
#include "cr/storage/cr_image.h"
#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_parse.h"
#include "cr/win/cr_bitlocker.h"
#include "cr/win/cr_destio.h"
#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_health.h"
#include "cr/win/cr_transport.h"
#include "cr/win/cr_elevate.h"
#include "cr/storage/cr_decrypt_device.h"
#include "cr/crypto/cr_aes.h"
#include <windows.h>
#include <map>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

using namespace cr;

namespace {

/* ---------------------------------------------------------------------------
 * Small utilities.
 * ------------------------------------------------------------------------- */

std::wstring widen(const std::string& s) {
    std::wstring w;
    for (unsigned char c : s) {
        if (c < 0x80)
            w.push_back(wchar_t(c));
        else
            w.push_back(L'?'); /* CLI display only; node names are u16 exact */
    }
    return w;
}

std::string narrow(const std::wstring& s) {
    std::string n;
    for (wchar_t c : s)
        n.push_back(c < 0x80 ? char(c) : '?');
    return n;
}

/* stdout progress line: "\r  42.1% | quick 100% deep 12% carve 0%" */
struct ProgressDisplay {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    u32 last_permille = 0xFFFFFFFFu;

    bool operator()(u32 permille, void*) {
        if (permille == last_permille)
            return true;
        last_permille = permille;
        const double pct = permille / 10.0;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0)
                            .count();
        wprintf(L"\r  %5.1f%%  (%lld s)   ", pct, static_cast<long long>(ms) / 1000);
        return true;
    }
    /* Success summary only - a failed/cancelled run must never print
     * "100.0% done" (AUDIT-ARCH F-B-15). */
    void done() { wprintf(L"\r  100.0%%  done          \n"); }
    double pct() const { return last_permille == 0xFFFFFFFFu ? 0.0 : last_permille / 10.0; }
};

/* ---------------------------------------------------------------------------
 * Cancellation (ARCH-2 wiring): Ctrl-C cancels the in-flight scan/recover
 * through a pause-capable token (the orchestrator/writer poll it at loop
 * heads; a UI host additionally uses pause()). Cooperative: the scan
 * thread observes the cancel, run() returns ERR_CANCELLED, the session is
 * saved when --out was given, and the process exits 3.
 * ------------------------------------------------------------------------- */
PausableToken g_cancel_token;

BOOL WINAPI ctrl_c_handler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) {
        g_cancel_token.cancel();
        return TRUE;
    }
    return FALSE;
}

/* ---------------------------------------------------------------------------
 * Logging (detailed, for test/bench workflows).
 * ------------------------------------------------------------------------- */

FILE* g_log = nullptr;

void log_open(const wchar_t* path) {
    if (g_log)
        fclose(g_log);
    g_log = nullptr;
    if (path)
        _wfopen_s(&g_log, path, L"w");
}

void log_close() {
    if (g_log)
        fclose(g_log);
    g_log = nullptr;
}

void log_line(const char* fmt, ...) {
    if (!g_log)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

/* ---------------------------------------------------------------------------
 * Device opening (shared by scan/recover/bench).
 * ------------------------------------------------------------------------- */

struct OpenResult {
    std::unique_ptr<IStorageDevice> device; /* the view handed to engines */
    std::unique_ptr<IStorageDevice> hold;   /* parent disk (slices reference it) */
    std::unique_ptr<IStorageDevice> slice_hold; /* M-033: the slice under the
                                                 * decrypting device (wrapper is
                                                 * non-owning; kept alive here) */
    u32 disk = 0;
    u64 volume_start = 0; /* 0 = whole-disk view */
    u64 volume_len = 0;
    /* S-030: the image path when the target is a disk image file (kept
     * for the session identity card; empty for --disk/--volume) */
    std::string image_path;
};

/* ---- S-008 pre-scan repair (scan --repair / --repair-apply) ----
 * The CLI posture the finding specifies: default OFF; --repair analyzes
 * and reports (dry-run), --repair-apply is the explicit consent that
 * lets the orchestrator execute the transactional plan pre-scan; the
 * undo log is persisted beside the session for rrecov fix --undo-apply. */

/* S-008 pre-scan repair write device (A-002 design note): the hooks
 * return the UNMAPPED parent writer (the whole RW disk); the
 * orchestrator rebases the slice-relative plan onto the parent basis
 * before execute (repair::rebase_plan), so the undo log - and
 * 'fix --undo-apply' - always carries disk-absolute offsets. */

struct RepairCliCtx {
    bool apply = false;
    int consent_calls = 0;
    u32 disk = 0;
    std::unique_ptr<repair::IWriteDevice> writer;
};

bool scan_repair_consent(const repair::RepairPlan& plan, void* user) {
    auto* c = static_cast<RepairCliCtx*>(user);
    ++c->consent_calls;
    wprintf(L"[repair] %hs volume: %zu action(s) planned\n", plan.fs_family.c_str(),
            plan.actions.size());
    for (const auto& a : plan.actions)
        wprintf(L"  [repair] offset=%llu len=%u : %hs\n", static_cast<unsigned long long>(a.offset),
                a.length, a.reason.c_str());
    for (const auto& adv : plan.advisories)
        wprintf(L"  [repair][advisory] %hs\n", adv.c_str());
    if (c->apply) {
        wprintf(L"[repair] applying - mounted volumes on the disk are locked and their drive "
                L"letters removed for the repair (corpus behavior; restored on next mount)\n");
        return true;
    }
    wprintf(L"[repair] dry-run only - re-run with --repair-apply to execute\n");
    return false;
}

repair::IWriteDevice* scan_repair_open_writer(void* user) {
    auto* c = static_cast<RepairCliCtx*>(user);
    u32 e = 0;
    auto w = win::open_disk_rw(c->disk, true, L"RDA", &e);
    if (!w)
        w = win::open_disk_rw(c->disk, false, nullptr, &e);
    if (!w) {
        wprintf(L"[repair] cannot open disk %u for write (rc=%08x): %hs\n", c->disk, e,
                cr::win::is_elevated()
                    ? "write access denied even elevated (device busy, locked or protected)"
                    : "administrator rights required for repairs (elevate and retry)");
        return nullptr;
    }
    /* UNMAPPED parent writer (A-002): the orchestrator rebases the plan
     * onto the parent basis - volume scans included - so no slice mapping
     * belongs here. */
    c->writer = std::move(w);
    return c->writer.get();
}

void scan_repair_close_writer(repair::IWriteDevice* /*dev*/, void* user) {
    static_cast<RepairCliCtx*>(user)->writer.reset();
}

/* SESS-2 (GAP-W3-9a): fill the session identity card from the disk
 * enumeration - model/serial, UTF-8, informational. The volume identity
 * proper (boot-sector CRC32) is computed by the orchestrator at run
 * start. Quiet on failure: an empty card is honest, never fabricated. */
void fill_identity_card(u32 disk, orchestrator::ScanConfig& cfg) {
    win::PhysicalDiskList disks;
    if (win::enumerate_disks(disk + 1, disks) != OK || disks.size() <= disk)
        return;
    const auto narrow = [](const std::wstring& w, char* out, size_t cap) {
        size_t i = 0;
        for (; i + 1 < cap && i < w.size() && w[i] < 0x80; ++i)
            out[i] = char(w[i]);
        out[i] = 0;
    };
    narrow(disks[disk].model, cfg.volume_model, sizeof(cfg.volume_model));
    narrow(disks[disk].serial, cfg.volume_serial, sizeof(cfg.volume_serial));
}

bool open_disk_view(u32 n, OpenResult& out) {
    u32 err = 0;
    /* RDA first when present (locked volumes, damaged media), direct
     * fallback - the reference Device.mo duality. */
    u32 rda_gle = 0;
    if (win::rda_available(L"RDA", &rda_gle)) {
        if (auto dev = win::open_rda_partition(L"RDA", n, 0, &err)) {
            wprintf(L"[transport] RDA driver path (\\\\.\\RDA)\n");
            out.device = std::move(dev);
            out.disk = n;
            return true;
        }
        wprintf(L"[transport] RDA open failed (%hs) - direct path\n", err_text(err));
    } else {
        /* evidence line for the runner mystery: the service runs but the
         * control device open fails - surface the exact CreateFile error */
        wprintf(L"[transport] RDA not available (\\\\.\\RDA CreateFile error %lu) - direct path\n",
                rda_gle);
    }
    out.device = win::open_physical_drive(n, &err);
    if (!out.device) {
        wprintf(L"[transport] cannot open disk %u: %hs (elevate?)\n", n, err_text(err));
        return false;
    }
    wprintf(L"[transport] direct path (\\\\.\\PhysicalDrive%u)\n", n);
    out.disk = n;
    return true;
}

bool parse_fvek_hex(const std::wstring& hex, std::vector<u8>& out) {
    /* M-033: the user-supplied FVEK (the og's
     * BitLockerManager::GetFullVolumeEncryptionKey output shape):
     * 64 hex chars = 32 bytes (AES-128-XTS), 128 = 64 bytes (AES-256-XTS). */
    out.clear();
    if (hex.size() != 64 && hex.size() != 128)
        return false;
    out.reserve(hex.size() / 2);
    for (size_t k = 0; k < hex.size(); k += 2) {
        int b = 0;
        for (int j = 0; j < 2; j++) {
            wchar_t c = hex[k + j];
            int v;
            if (c >= L'0' && c <= L'9')
                v = int(c - L'0');
            else if (c >= L'a' && c <= L'f')
                v = int(c - L'a' + 10);
            else if (c >= L'A' && c <= L'F')
                v = int(c - L'A' + 10);
            else
                return false;
            b = (b << 4) | v;
        }
        out.push_back(u8(b));
    }
    return true;
}

bool open_volume_view(wchar_t letter, OpenResult& out, const std::wstring& fvek_hex) {
    std::vector<u8> fvek;
    const bool have_key = !fvek_hex.empty() && parse_fvek_hex(fvek_hex, fvek);
    if (!fvek_hex.empty() && !have_key) {
        wprintf(L"[bitlocker] --bitlocker-fvek: expected 64 hex chars (AES-128-XTS) "
                L"or 128 hex chars (AES-256-XTS)\n");
        return false;
    }
    const win::bitlocker::Status bst = win::bitlocker::status(letter);
    if (bst == win::bitlocker::Status::Unknown && !have_key) {
        /* Boot sector unreadable: the BitLocker state cannot be determined
         * (typical for a locked volume denying \\.\X: reads). Never scan
         * blind - refuse with the unlock path (FB-B01). With a user key
         * the in-stack decrypt path may proceed (M-033): the user has
         * supplied key material, which is the og's own opt-in. */
        wprintf(L"[bitlocker] %c: boot sector unreadable - cannot determine BitLocker state\n"
                L"  (typical for a locked volume). Unlock with:\n"
                L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                L"or re-run with --bitlocker-fvek <hex> (the in-stack decrypt path).\n",
                letter, letter);
        return false;
    }
    if (bst == win::bitlocker::Status::Locked && !have_key) {
        wprintf(L"[bitlocker] %c: locked - trying auto-unlock...\n", letter);
        if (win::bitlocker::try_auto_unlock(letter) != OK) {
            wprintf(L"[bitlocker] still locked. Unlock with:\n"
                    L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                    L"or re-run with --bitlocker-fvek <hex> (the in-stack decrypt path).\n",
                    letter);
            return false;
        }
        wprintf(L"[bitlocker] auto-unlocked\n");
    }
    if (have_key && (bst == win::bitlocker::Status::Locked ||
                     bst == win::bitlocker::Status::Unknown ||
                     bst == win::bitlocker::Status::UnlockedEncrypted)) {
        wprintf(L"[bitlocker] %c: in-stack XTS-AES decrypt active (%hs key, "
                L"engines see plaintext)\n",
                letter, fvek.size() == 32 ? "128-bit" : "256-bit");
    }
    u32 disk = 0;
    u64 start = 0, len = 0;
    if (!win::volume_extent(letter, &disk, &start, &len)) {
        wprintf(L"cannot map %c: to a disk extent\n", letter);
        return false;
    }
    OpenResult whole;
    if (!open_disk_view(disk, whole))
        return false;
    auto slice = PartitionSlice::create(whole.device.get(), start, len, nullptr);
    if (!slice) {
        wprintf(L"slice creation failed for %c: (@%llu +%llu)\n", letter,
                static_cast<unsigned long long>(start), static_cast<unsigned long long>(len));
        return false;
    }
    out.disk = disk;
    out.volume_start = start;
    out.volume_len = len;
    /* the slice references the parent by raw pointer - keep it alive */
    out.hold = std::move(whole.device);
    if (have_key) {
        /* M-033: wrap the volume slice in the in-stack XTS decrypting
         * device - the og's layer order (physical -> slice -> [BitLocker
         * XTS] -> engine; corpus architecture doc §8). The wrapped slice
         * must outlive the wrapper; both are held by OpenResult. */
        auto dec = DecryptingDevice::create(slice.get(), fvek.data(), fvek.size());
        if (!dec) {
            wprintf(L"[bitlocker] key material rejected (weak or malformed FVEK)\n");
            return false;
        }
        wprintf(L"[bitlocker] decrypting device wraps the volume slice\n");
        out.slice_hold = std::move(slice);
        out.device = std::move(dec);
    } else {
        out.device = std::move(slice);
    }
    wprintf(L"[volume] %c: disk %u @%llu +%llu\n", letter, disk,
            static_cast<unsigned long long>(start), static_cast<unsigned long long>(len));
    return true;
}

/* argument helpers */
bool get_opt(int argc, wchar_t** argv, int& i, const wchar_t* name, std::wstring* val) {
    if (i >= argc || wcscmp(argv[i], name) != 0)
        return false;
    ++i;
    if (i >= argc) {
        wprintf(L"missing value for %s\n", name);
        return false;
    }
    if (val)
        *val = argv[i];
    ++i;
    return true;
}

bool has_flag(int argc, wchar_t** argv, int& i, const wchar_t* name) {
    if (i >= argc || wcscmp(argv[i], name) != 0)
        return false;
    ++i;
    return true;
}

/* ---------------------------------------------------------------------------
 * list
 * ------------------------------------------------------------------------- */

int print_disks() {
    win::PhysicalDiskList list;
    if (win::enumerate_disks(200, list) != OK) {
        wprintf(L"enumeration failed\n");
        return 1; /* FB-K01: enumeration failure is never exit-0 - scripts
                     must not mistake total failure for "no disks" */
    }
    win::VolumeEntryList vols;
    win::enumerate_volumes(vols);
    wprintf(L"%zu disk(s), %zu volume(s)\n", list.size(), vols.size());
    for (const auto& d : list) {
        wprintf(L"  Disk %u: %llu MiB (%s) %s [%s] %s\n", d.number, d.size_bytes >> 20,
                d.model.c_str(), d.serial.c_str(), d.removable ? L"removable" : L"fixed",
                d.table_ok ? (d.table.layout == table::TableLayout::Gpt
                                  ? L"GPT"
                                  : (d.table.layout == table::TableLayout::Mbr ? L"MBR" : L"-"))
                           : L"no-table");
        for (const auto& v : vols) {
            if (v.disk == d.number) {
                wprintf(L"    %c: %s %s %llu MiB %s\n", v.letter, v.fs_name.c_str(),
                        v.label.c_str(), v.bytes >> 20, v.bitlocker ? L"[BitLocker]" : L"");
            }
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * tree printing (shared by scan + tree commands)
 * ------------------------------------------------------------------------- */

struct TreePrintFlags {
    bool deleted_only = false;
    bool csv = false;
    bool json = false;  /* G-12 projection via query::run (frozen schema
                         * below); --csv grammar stays byte-identical */
    std::string filter; /* substring, case-insensitive */
};

bool name_matches(const tree::FileNode* n, const TreePrintFlags& f) {
    if (f.filter.empty())
        return true;
    const std::string name = n->name_utf8();
    if (name.size() < f.filter.size())
        return false;
    for (size_t i = 0; i + f.filter.size() <= name.size(); ++i) {
        size_t k = 0;
        for (; k < f.filter.size(); ++k) {
            char a = name[i + k], b = f.filter[k];
            if (a >= 'A' && a <= 'Z')
                a += 32;
            if (b >= 'A' && b <= 'Z')
                b += 32;
            if (a != b)
                break;
        }
        if (k == f.filter.size())
            return true;
    }
    return false;
}

void print_tree(tree::ResultTree& tree, const TreePrintFlags& f) {
    std::function<void(tree::FileNode*, const std::string&, int)> walk =
        [&](tree::FileNode* n, const std::string& path, int depth) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                if (!c->props())
                    continue;
                const std::string name = c->name_utf8();
                const std::string full = path.empty() ? name : path + "\\" + name;
                const bool is_dir = (c->props()->attributes & 0x10) != 0;
                const bool deleted = c->deleted_state() != tree::DeletedState::Active;
                if (!is_dir && f.deleted_only && !deleted)
                    continue;
                if (!is_dir && !name_matches(c, f))
                    continue;
                if (f.csv) {
                    wprintf(L"\"%hs\",%llu,%u,%u,%u,%llu\n", full.c_str(),
                            static_cast<unsigned long long>(c->props()->logical_size),
                            u32(c->origin()), u32(c->deleted_state()), u32(c->confidence()),
                            static_cast<unsigned long long>(
                                c->props()->extents.empty() ? 0 : c->props()->extents[0].offset));
                } else {
                    for (int k = 0; k < depth; ++k)
                        putwchar(L' ');
                    const char* st = deleted ? " [deleted]"
                                     : c->confidence() == tree::Confidence::PartialRisk ? " [risk]"
                                                                                        : "";
                    const char* org = c->origin() == tree::Origin::Carved
                                          ? " [carved]"
                                          : (c->origin() == tree::Origin::Lost ? " [lost]" : "");
                    wprintf(L"%hs (%llu)%hs%hs\n", name.c_str(),
                            static_cast<unsigned long long>(c->props()->logical_size), st, org);
                }
                if (is_dir)
                    walk(c, full, depth + 1);
            }
        };
    for (auto* r : tree.roots()) {
        if (!f.csv)
            wprintf(L"== %hs\n", r->name_utf8().c_str());
        walk(r, "", 1);
    }
}

/* ---------------------------------------------------------------------------
 * CRSF file I/O adapters (sink-based container -> Win32 files).
 * ------------------------------------------------------------------------- */

u32 file_write_cb(void* user, const void* bytes, u32 len) {
    FILE* f = static_cast<FILE*>(user);
    return fwrite(bytes, 1, len, f) == len ? OK : ERR_SESSION_IO;
}
u64 file_tell_cb(void* user) {
    return u64(_ftelli64(static_cast<FILE*>(user)));
}
u32 file_seek_cb(void* user, u64 absolute) {
    return _fseeki64(static_cast<FILE*>(user), __int64(absolute), SEEK_SET) == 0 ? OK
                                                                                 : ERR_SESSION_IO;
}
/* ---- S-030: disk-image scan targets --------------------------------------
 * The og's "Import a disk image" flow: vmdk/vhd/vhdx/edisk dispatch with a
 * flat-file fallback for raw/.img (DRWUI FUN_1405b3410 + FUN_140178250).
 * The CLI provides the FILE byte source (the core image layer stays
 * OS-header-free; every byte flows through the injected seam). */
class CliImageSource final : public storage::ImageByteSource {
public:
    static std::unique_ptr<CliImageSource> open(const wchar_t* wpath, u32* err) {
        *err = OK;
        FILE* f = nullptr;
        if (_wfopen_s(&f, wpath, L"rb") != 0 || f == nullptr) {
            *err = ERR_OPEN_FAILED;
            return nullptr;
        }
        return std::unique_ptr<CliImageSource>(new CliImageSource(f));
    }
    ~CliImageSource() override {
        if (f_)
            std::fclose(f_);
    }
    u32 read(u64 at, void* buf, u32 len) override {
        if (at > size_ || u64(len) > size_ - at)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        if (_fseeki64(f_, __int64(at), SEEK_SET) != 0)
            return ERR_READ_FAILED;
        if (std::fread(buf, 1, len, f_) != len)
            return ERR_READ_FAILED;
        return OK;
    }
    u64 size() const override { return size_; }

private:
    explicit CliImageSource(FILE* f) : f_(f) {
        const __int64 cur = _ftelli64(f);
        _fseeki64(f, 0, SEEK_END);
        size_ = u64(_ftelli64(f));
        _fseeki64(f, cur, SEEK_SET);
    }
    FILE* f_;
    u64 size_ = 0;
};

u32 cli_open_image_source(void*, const char* utf8_path, std::unique_ptr<storage::ImageByteSource>& out) {
    /* extent files: the descriptor directory is ASCII-safe in practice;
     * non-ASCII sibling names widen losslessly only for BMP chars - the
     * scan still works, the name display is the only lossy edge */
    std::wstring w;
    for (const char* p = utf8_path; *p; ++p) {
        const unsigned char c = unsigned char(*p);
        if (c < 0x80)
            w.push_back(wchar_t(c));
        else
            w.push_back(L'?');
    }
    u32 err = OK;
    auto f = CliImageSource::open(w.c_str(), &err);
    if (!f)
        return err;
    out = std::move(f);
    return OK;
}

bool open_image_view(const std::wstring& wpath, OpenResult& out) {
    u32 err = OK;
    auto src = CliImageSource::open(wpath.c_str(), &err);
    if (!src) {
        wprintf(L"[image] cannot open %s: %hs\n", wpath.c_str(), err_text(err));
        return false;
    }
    /* the descriptor base directory (sibling extent files) */
    std::string base;
    const std::string n8 = narrow(wpath);
    if (const size_t slash = n8.find_last_of("/\\"); slash != std::string::npos)
        base = n8.substr(0, slash + 1);
    storage::ImageFileOps ops;
    ops.open = &cli_open_image_source;
    std::unique_ptr<IStorageDevice> dev;
    const u32 rc = storage::open_image(std::move(src), ops, base.c_str(), dev);
    if (rc != OK) {
        wprintf(L"[image] %s: %hs\n", wpath.c_str(), err_text(rc));
        return false;
    }
    out.device = std::move(dev);
    out.image_path = n8;
    wprintf(L"[image] %s opened (%llu bytes, layout %hs)\n", wpath.c_str(),
            static_cast<unsigned long long>(out.device->size()),
            storage::classify_boot_layout(out.device.get()) == storage::BootLayout::Gpt  ? "GPT"
            : storage::classify_boot_layout(out.device.get()) == storage::BootLayout::Mbr ? "MBR"
                                                                                          : "unknown");
    return true;
}

u32 file_read_cb(void* user, void* buf, u32 len) {
    FILE* f = static_cast<FILE*>(user);
    return fread(buf, 1, len, f) == len ? OK : ERR_SESSION_IO;
}
u64 file_size_cb(void* user) {
    FILE* f = static_cast<FILE*>(user);
    const __int64 cur = _ftelli64(f);
    _fseeki64(f, 0, SEEK_END);
    const __int64 end = _ftelli64(f);
    _fseeki64(f, cur, SEEK_SET);
    return u64(end);
}

/* ---------------------------------------------------------------------------
 * scan
 * ------------------------------------------------------------------------- */

int cmd_scan(int argc, wchar_t** argv, int& i) {
    /* M-033 pre-pass: --bitlocker-fvek may appear anywhere on the line
     * (the target --volume is opened in the main loop below). */
    std::wstring fvek_hex;
    for (int k = i; k + 1 < argc; ++k) {
        if (wcscmp(argv[k], L"--bitlocker-fvek") == 0) {
            fvek_hex = argv[k + 1];
            break;
        }
    }
    OpenResult target;
    bool have_target = false;
    orchestrator::ScanConfig cfg;
    cfg.whole_disk = false;
    std::wstring out_path, log_path, resume_path, repair_undo_path;
    bool json = false;
    bool repair = false, repair_apply = false;
    TreePrintFlags print_flags;

    while (i < argc) {
        std::wstring v;
        if (get_opt(argc, argv, i, L"--disk", &v)) {
            if (!open_disk_view(u32(wcstoul(v.c_str(), nullptr, 10)), target))
                return 1;
            have_target = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--volume", &v)) {
            if (v.size() < 1)
                return 1;
            if (!open_volume_view(v[0], target, fvek_hex))
                return 1;
            have_target = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--bitlocker-fvek", &v)) {
            /* consumed by the pre-pass; not a positional */
            continue;
        }
        if (get_opt(argc, argv, i, L"--image", &v)) {
            /* S-030: disk-image scan targets - the og's import flow
             * (vmdk/vhd/flat with vhdx/edisk honestly refused) */
            if (!open_image_view(v, target))
                return 1;
            have_target = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--whole-disk")) {
            /* N-044: the honest name. The og's "Deep Scan" toggle means the
             * complete-searcher engine pass - which this CLI ALWAYS runs;
             * this flag instead enables whole-disk lost-partition discovery.
             * The deep stage is always on; --quick-only disables it. */
            cfg.whole_disk = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--deep")) {
            /* N-044: deprecated alias (kept one release with a note). Do
             * not map the og's Deep Scan toggle onto this flag: it sets
             * whole-disk lost-partition discovery, not the deep stage. */
            cfg.whole_disk = true;
            wprintf(L"[note] --deep is a deprecated alias for --whole-disk "
                    L"(lost-partition discovery; the deep engine stage is always on - "
                    L"--quick-only disables it)\n");
            continue;
        }
        if (has_flag(argc, argv, i, L"--quick-only")) {
            cfg.run_deep = false;
            cfg.run_carve = false;
            continue;
        }
        if (has_flag(argc, argv, i, L"--no-carve")) {
            cfg.run_carve = false;
            continue;
        }
        if (get_opt(argc, argv, i, L"--threads", &v)) {
            cfg.threads = u32(wcstoul(v.c_str(), nullptr, 10));
            continue;
        }
        if (get_opt(argc, argv, i, L"--out", &out_path))
            continue;
        if (get_opt(argc, argv, i, L"--resume", &resume_path))
            continue;
        if (get_opt(argc, argv, i, L"--log", &log_path))
            continue;
        if (get_opt(argc, argv, i, L"--filter", &v)) {
            print_flags.filter = narrow(v);
            continue;
        }
        if (has_flag(argc, argv, i, L"--deleted-only")) {
            print_flags.deleted_only = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--csv")) {
            print_flags.csv = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--json")) {
            json = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--repair")) {
            repair = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--repair-apply")) {
            repair = true;
            repair_apply = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--repair-undo", &repair_undo_path))
            continue;
        wprintf(L"unknown scan option: %s\n", argv[i]);
        return 1;
    }
    if (!have_target) {
        wprintf(L"scan needs --disk N or --volume C:%s\n",
                resume_path.empty() ? L"" : L" (plus the ORIGINAL target of the saved session)");
        return 1;
    }

    /* lost-partition discovery only makes sense over a whole-disk view */
    if (cfg.whole_disk && target.volume_len != 0) {
        wprintf(L"[note] --whole-disk on a volume: lost-partition discovery needs --disk (skipping "
                L"that stage)\n");
        cfg.whole_disk = false;
    }
    log_open(log_path.empty() ? nullptr : log_path.c_str());
    log_line("rrecov scan: threads=%u quick=%d deep=%d carve=%d whole_disk=%d repair=%d/%d",
             cfg.threads, int(cfg.run_quick), int(cfg.run_deep), int(cfg.run_carve),
             int(cfg.whole_disk), int(repair), int(repair_apply));
    fill_identity_card(target.disk, cfg); /* SESS-2: model/serial into the card */
    /* S-030: the image target's file path into the identity card (v3) */
    for (size_t k = 0; k + 1 < sizeof(cfg.volume_image) && k < target.image_path.size(); ++k)
        cfg.volume_image[k] = target.image_path[k];

    /* S-008 pre-scan repair hooks: --repair analyzes + reports; the
     * --repair-apply FLAG is the consent (non-interactive CLI posture);
     * the RW handle opens lazily at consent time (never grabbed for a
     * dry-run). Volume scans map writes through the volume slice. */
    RepairCliCtx repair_ctx;
    if (repair) {
        cfg.pre_scan_repair = true;
        repair_ctx.apply = repair_apply;
        repair_ctx.disk = target.disk; /* the parent basis for the undo log */
        cfg.repair.consent = scan_repair_consent;
        cfg.repair.open_writer = scan_repair_open_writer;
        cfg.repair.close_writer = scan_repair_close_writer;
        cfg.repair.user = &repair_ctx;
    }

    /* Cross-disk LDM members (audit row 30): on a whole-disk scan of an
     * LDM (dynamic) disk, spanned/striped/mirrored/RAID-5 volumes may
     * reference member disks other than this one. Enumerate every disk,
     * read its PRIVHEAD disk GUID (LBA 6 - one 512-byte read), and give
     * the orchestrator a GUID -> device resolver. The devices stay open
     * for the whole scan. Non-LDM disks simply have no GUID entry and
     * never resolve. */
    std::vector<std::unique_ptr<IStorageDevice>> ldm_members;
    if (cfg.whole_disk && target.volume_len == 0) {
        table::LdmInfo probe;
        if (table::parse_ldm_database(target.device.get(), probe) == OK && probe.found) {
            win::PhysicalDiskList list;
            if (win::enumerate_disks(32, list) == OK) {
                auto guid_map = std::make_shared<std::map<table::LdmGuid, IStorageDevice*>>();
                for (const auto& pd : list) {
                    u32 oerr = 0;
                    auto dev = win::open_disk(pd.number, true, L"RDA", &oerr);
                    if (!dev)
                        dev = win::open_disk(pd.number, false, L"RDA", &oerr);
                    if (!dev)
                        continue;
                    u8 sec[512];
                    if (dev->read_at(6 * 512, sec, sizeof(sec)) == OK &&
                        std::memcmp(sec, "PRIVHEAD", 8) == 0) {
                        table::LdmGuid g{};
                        std::memcpy(g.b, sec + 0x30, 16);
                        (*guid_map)[g] = dev.get();
                    }
                    ldm_members.push_back(std::move(dev));
                }
                if (!guid_map->empty()) {
                    log_line("ldm cross-disk registry: %zu disk(s)", guid_map->size());
                    cfg.ldm_resolver = [guid_map](const table::LdmGuid& g) -> IStorageDevice* {
                        auto it = guid_map->find(g);
                        return it != guid_map->end() ? it->second : nullptr;
                    };
                }
            }
        }
    }

    /* session output: --out explicit; --resume without --out updates the
     * resumed file in place */
    const std::wstring effective_out = out_path.empty() ? resume_path : out_path;

    /* G-13 checkpoint sink (ARCH-2/SESS-3 wiring): --out enables
     * stage-boundary auto-checkpoints. Each emit seeks to 0 and
     * overwrites, so the file always holds the newest complete
     * checkpoint (a torn one is no worse than no checkpoint - the
     * reader re-indexes forward by construction). */
    FILE* ck_file = nullptr;
    if (!effective_out.empty()) {
        if (_wfopen_s(&ck_file, effective_out.c_str(), L"wb") != 0 || !ck_file) {
            wprintf(L"cannot create session file %s\n", effective_out.c_str());
            return 1;
        }
        cfg.checkpoint.write = file_write_cb;
        cfg.checkpoint.tell = file_tell_cb;
        cfg.checkpoint.seek = file_seek_cb;
        cfg.checkpoint.user = ck_file;
    }

    tree::ResultTree tree;
    std::unique_ptr<orchestrator::ScanController> ctrl;
    orchestrator::ScanStats stats;
    if (!resume_path.empty()) {
        FILE* fin = nullptr;
        if (_wfopen_s(&fin, resume_path.c_str(), L"rb") != 0 || !fin) {
            wprintf(L"cannot open session %s\n", resume_path.c_str());
            if (ck_file)
                fclose(ck_file);
            return 1;
        }
        session::Reader r(file_read_cb, file_seek_cb, file_size_cb, fin);
        session::VolumeSessionInfo info;
        const u32 rrc = orchestrator::ScanController::resume(r, tree, cfg, stats, info, ctrl);
        fclose(fin);
        if (rrc != OK) {
            wprintf(L"resume failed: %hs (0x%08x)\n", err_text(rrc), rrc);
            if (ck_file)
                fclose(ck_file);
            return 1;
        }
        wprintf(L"[resume] session mask 0x%x - skipping completed stage(s)\n",
                info.stage_done_mask);
        /* SESS-2 verify hook: the loaded session carries a volume identity
         * (boot-sector CRC32). Recompute it over the CURRENT target and
         * warn loudly on mismatch - resuming a session against a different
         * volume would otherwise produce garbage results silently. Zero
         * hash (v1 session, or unreadable start) = unverifiable, skipped. */
        if (info.boot_hash != 0) {
            u8 boot[512] = {};
            if (target.device->read_at(0, boot, sizeof(boot)) == OK &&
                table::crc32(boot, sizeof(boot)) != info.boot_hash)
                wprintf(L"[resume] WARNING: volume identity mismatch - this session was "
                        L"saved on a different volume (boot hash differs)\n");
        }
    } else {
        ctrl.reset(new orchestrator::ScanController(tree, cfg));
    }
    ProgressDisplay prog;
    log_line("scan start");
    const u32 rc = ctrl->run(
        target.device.get(), &g_cancel_token,
        [](u32 pm, void* user) -> bool {
            return static_cast<ProgressDisplay*>(user)->operator()(pm, nullptr);
        },
        &prog, stats);
    if (ck_file)
        fclose(ck_file); /* checkpoints done; the final save reopens fresh */
    log_line("scan end rc=0x%08x files=%llu deleted=%llu carved=%llu lost_parts=%llu "
             "bytes=%llu elapsed_ms=%llu errors=%zu",
             rc, stats.files_found, stats.deleted_found, stats.carved_found, stats.lost_partitions,
             stats.bytes_scanned, stats.elapsed_ms, stats.errors.ranges().size());
    for (const auto& er : stats.errors.ranges())
        log_line("  read-error range: @%llu +%llu (attempts %u)",
                 static_cast<unsigned long long>(er.offset),
                 static_cast<unsigned long long>(er.length), er.attempts);

    /* S-008 pre-scan repair summary (visible even when the plan was empty,
     * refused, or consent-denied - the operator always sees the truth) */
    if (repair && stats.repair_ran) {
        wprintf(L"[repair] family=%hs planned=%u applied=%u%s%s%s\n", stats.repair_family,
                stats.repair_actions_planned, stats.repair_actions_applied,
                stats.repair_consent_denied ? L" (consent denied - nothing written)" : L"",
                stats.repair_writer_unavailable ? L" (no write handle available)" : L"",
                stats.repair_status != OK ? L" (see log)" : L"");
        for (const auto& adv : stats.repair_advisories)
            wprintf(L"[repair][advisory] %hs\n", adv.c_str());
        if (stats.repair_actions_applied > 0) {
            if (repair_undo_path.empty())
                repair_undo_path =
                    L"rrecov-scan-repair-disk" + std::to_wstring(target.disk) + L".undo";
            std::vector<u8> blob;
            stats.repair_undo.serialize(blob);
            FILE* uf = nullptr;
            if (_wfopen_s(&uf, repair_undo_path.c_str(), L"wb") == 0 && uf != nullptr) {
                fwrite(blob.data(), 1, blob.size(), uf);
                fclose(uf);
                wprintf(L"[repair] undo log saved: %s (%zu bytes) - restore with: "
                        L"rrecov fix --disk %u --undo-apply %s\n",
                        repair_undo_path.c_str(), blob.size(), target.disk,
                        repair_undo_path.c_str());
            } else {
                wprintf(L"[repair] WARNING: could not save the undo log to %s\n",
                        repair_undo_path.c_str());
            }
        }
    }
    log_close();

    /* save helper: full truthful session from the (now quiesced) tree */
    auto save_session_to = [&](const std::wstring& path) -> bool {
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) {
            wprintf(L"cannot create session file %s\n", path.c_str());
            return false;
        }
        session::Writer w(file_write_cb, file_tell_cb, file_seek_cb, f);
        w.set_flags(session::CRSF_FLAG_ABS_EXTENTS); /* F-1: absolute basis */
        if (w.open(0x52434F56) != OK || ctrl->save_session(w, stats) != OK || w.finalize() != OK) {
            wprintf(L"session save failed\n");
            fclose(f);
            return false;
        }
        fclose(f);
        return true;
    };

    if (rc != OK) {
        if (rc == ERR_CANCELLED) {
            /* Save-on-cancel (ARCH-2): a cancelled run persists a resumable
             * session when a path is known - exactly what the core's
             * stage_done_mask truth was built for. */
            wprintf(L"\nscan cancelled at %.1f%%\n", prog.pct());
            if (!effective_out.empty()) {
                if (save_session_to(effective_out)) {
                    wprintf(L"session saved (cancelled): %s\n"
                            L"resume with: rrecov scan --resume %s --disk N | --volume C:\n",
                            effective_out.c_str(), effective_out.c_str());
                }
            } else {
                wprintf(L"(pass --out file.crsf to save a resumable session on cancel)\n");
            }
            return 3; /* cancelled is distinct from failure (0/1/3) */
        }
        wprintf(L"scan failed: %hs (0x%08x)\n", cr::err_text(rc), rc);
        return 1;
    }
    prog.done();
    if (json) {
        wprintf(L"{\"files\":%llu,\"deleted\":%llu,\"carved\":%llu,\"lost_partitions\":%llu,"
                L"\"bytes_scanned\":%llu,\"elapsed_ms\":%llu}\n",
                stats.files_found, stats.deleted_found, stats.carved_found, stats.lost_partitions,
                stats.bytes_scanned, stats.elapsed_ms);
    } else {
        wprintf(L"scan complete: %llu file(s), %llu deleted, %llu carved, %llu lost partition(s) "
                L"in %llu.%01u s\n",
                stats.files_found, stats.deleted_found, stats.carved_found, stats.lost_partitions,
                stats.elapsed_ms / 1000, u32(stats.elapsed_ms % 1000) / 100);
    }

    print_tree(tree, print_flags);

    if (!effective_out.empty()) {
        if (!save_session_to(effective_out))
            return 1;
        wprintf(L"session saved: %s\n", effective_out.c_str());
    }
    return 0;
}

/* GAP-TREE-JSON: machine-readable tree projection via query::run (G-12).
 * Schema (FROZEN, mirrors the --csv columns):
 *   {"total":N,"rows":[{"path":"...","name":"...","size":N,
 *     "modified":N,"origin":0..3,"deleted":0..3,
 *     "confidence":0..4,"category":0..6},...]}
 * - path/name: UTF-8, '/'-joined ancestry incl. root roles
 * - modified: 100-ns units since 1601 (FileTimes wire unit)
 * - origin/deleted/confidence: the same numeric enums as --csv
 *   (cr_node.h 28-36)
 * - category: cr::query::Category (1=Image 2=Video 3=Audio 4=Document
 *   5=Archive 6=Other; 0=None)
 * --filter/--deleted-only apply with query semantics (the filter applies
 * to directories too, unlike --csv where dirs always print). */
void print_tree_json(tree::ResultTree& t, const TreePrintFlags& f) {
    query::Query q;
    q.filter.name_substring = f.filter;
    q.filter.deleted_only = f.deleted_only;
    q.limit = 0; /* all matching rows */
    const query::Result res = query::run(t, q);
    std::string doc;
    doc.reserve(size_t(128 + res.rows.size() * 192));
    doc += "{\"total\":";
    doc += std::to_string(res.total_matched);
    doc += ",\"rows\":[";
    bool first = true;
    for (const query::Row& row : res.rows) {
        if (!first)
            doc.push_back(',');
        first = false;
        doc += "{\"path\":\"";
        doc += recovery::report::escape_json(row.path_utf8);
        doc += "\",\"name\":\"";
        doc += recovery::report::escape_json(row.name_utf8);
        doc += "\",\"size\":";
        doc += std::to_string(row.size);
        doc += ",\"modified\":";
        doc += std::to_string(row.modified);
        doc += ",\"origin\":";
        doc += std::to_string(row.origin);
        doc += ",\"deleted\":";
        doc += std::to_string(row.deleted);
        doc += ",\"confidence\":";
        doc += std::to_string(row.confidence);
        doc += ",\"category\":";
        doc += std::to_string(row.category);
        doc.push_back('}');
    }
    doc += "]}";
    wprintf(L"%hs\n", doc.c_str());
}

/* ---------------------------------------------------------------------------
 * tree (load a session and print)
 * ------------------------------------------------------------------------- */

int cmd_tree(int argc, wchar_t** argv, int& i) {
    std::wstring in_path;
    TreePrintFlags f;
    while (i < argc) {
        std::wstring v;
        if (get_opt(argc, argv, i, L"--in", &in_path))
            continue;
        if (get_opt(argc, argv, i, L"--filter", &v)) {
            f.filter = narrow(v);
            continue;
        }
        if (has_flag(argc, argv, i, L"--deleted-only")) {
            f.deleted_only = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--csv")) {
            f.csv = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--json")) {
            f.json = true;
            continue;
        }
        wprintf(L"unknown tree option: %s\n", argv[i]);
        return 1;
    }
    if (in_path.empty()) {
        wprintf(L"tree needs --in file.crsf\n");
        return 1;
    }
    FILE* fin = nullptr;
    if (_wfopen_s(&fin, in_path.c_str(), L"rb") != 0 || !fin) {
        wprintf(L"cannot open session %s\n", in_path.c_str());
        return 1;
    }
    session::Reader r(file_read_cb, file_seek_cb, file_size_cb, fin);
    tree::ResultTree tree;
    orchestrator::ScanStats stats;
    const u32 rc = orchestrator::ScanController::load_session(r, tree, stats);
    fclose(fin);
    if (rc != OK) {
        wprintf(L"session load failed: %hs (0x%08x)\n", err_text(rc), rc);
        return 1;
    }
    wprintf(L"session: %llu file(s)\n", stats.files_found);
    if (f.json) {
        print_tree_json(tree, f);
        return 0;
    }
    print_tree(tree, f);
    return 0;
}

/* ---------------------------------------------------------------------------
 * recover (session -> files)
 * ------------------------------------------------------------------------- */

int cmd_recover(int argc, wchar_t** argv, int& i) {
    std::wstring in_path, to_dir, filter, report_path;
    bool deleted_only = false;
    u32 disk = 0;
    bool have_disk = false;
    std::wstring image_path; /* S-030: --image source */
    while (i < argc) {
        std::wstring v;
        if (get_opt(argc, argv, i, L"--in", &in_path))
            continue;
        if (get_opt(argc, argv, i, L"--to", &to_dir))
            continue;
        if (get_opt(argc, argv, i, L"--disk", &v)) {
            disk = u32(wcstoul(v.c_str(), nullptr, 10));
            have_disk = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--image", &v)) {
            /* S-030: recover from the image the session was scanned from */
            image_path = v;
            have_disk = true; /* a target is a target */
            continue;
        }
        if (get_opt(argc, argv, i, L"--filter", &filter))
            continue;
        if (get_opt(argc, argv, i, L"--report", &report_path))
            continue; /* G-09: machine-readable result report */
        if (has_flag(argc, argv, i, L"--deleted-only")) {
            deleted_only = true;
            continue;
        }
        wprintf(L"unknown recover option: %s\n", argv[i]);
        return 1;
    }
    if (in_path.empty() || to_dir.empty() || !have_disk) {
        wprintf(L"recover needs --in file.crsf --disk N --to DIR\n");
        return 1;
    }

    /* load the session tree */
    FILE* fin = nullptr;
    if (_wfopen_s(&fin, in_path.c_str(), L"rb") != 0 || !fin) {
        wprintf(L"cannot open session %s\n", in_path.c_str());
        return 1;
    }
    session::Reader r(file_read_cb, file_seek_cb, file_size_cb, fin);
    tree::ResultTree tree;
    orchestrator::ScanStats stats;
    u32 rc = orchestrator::ScanController::load_session(r, tree, stats);
    fclose(fin);
    if (rc != OK) {
        wprintf(L"session load failed: %hs\n", err_text(rc));
        return 1;
    }

    /* open the SOURCE (extents in sessions are absolute): --disk or the
     * --image the session was scanned from (S-030) */
    OpenResult src;
    if (!image_path.empty()) {
        if (!open_image_view(image_path, src))
            return 1;
    } else if (!open_disk_view(disk, src)) {
        return 1;
    }

    /* destination validation + overlap refusal (F12): the destination
     * volume extent must not intersect the source disk */
    if (to_dir.size() >= 2 && to_dir[1] == L':') {
        u32 ddisk = 0;
        u64 dstart = 0, dlen = 0;
        if (win::volume_extent(to_dir[0], &ddisk, &dstart, &dlen)) {
            if (ddisk == disk && image_path.empty())
                wprintf(L"[F12] WARNING: destination is on the SOURCE disk %u\n", ddisk);
            else
                wprintf(L"[dest] disk %u @%llu +%llu\n", ddisk,
                        static_cast<unsigned long long>(dstart),
                        static_cast<unsigned long long>(dlen));
        }
    }

    win::WinDestIo io;
    recovery::WriterConfig wcfg;
    recovery::SafeWriter writer(io, src.device.get(), wcfg);
    if (to_dir.size() >= 2 && to_dir[1] == L':') {
        u64 dstart = 0, dlen = 0;
        u32 ddisk = 0;
        if (win::volume_extent(to_dir[0], &ddisk, &dstart, &dlen))
            writer.set_destination(dstart, dlen);
    }

    /* normalize the destination dir (trailing backslash) */
    if (!to_dir.empty() && to_dir.back() != L'\\')
        to_dir.push_back(L'\\');
    std::u16string dest16(to_dir.begin(), to_dir.end());

    /* enqueue every matching file node; directory nodes enqueue as
     * structure items too (G-08) so empty leaves materialize */
    size_t queued = 0, refused = 0;
    std::function<void(tree::FileNode*, const std::u16string&)> plan =
        [&](tree::FileNode* n, const std::u16string& path) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                if (!c->props())
                    continue;
                const bool is_dir = (c->props()->attributes & 0x10) != 0;
                const bool deleted = c->deleted_state() != tree::DeletedState::Active;
                if (is_dir) {
                    /* the directory itself becomes a queue item (mkdir on
                     * process); descend regardless so deep leaves survive */
                    if (writer.enqueue(c, path) == OK)
                        ++queued;
                    plan(c, path + c->props()->name + u"\\");
                    continue;
                }
                if (deleted_only && !deleted)
                    continue;
                if (!filter.empty() && c->name_utf8().find(narrow(filter)) == std::string::npos)
                    continue;
                const u32 eq = writer.enqueue(c, path);
                if (eq == ERR_DEST_OVERLAP)
                    ++refused;
                else if (eq == OK)
                    ++queued;
            }
        };
    for (auto* root : tree.roots())
        plan(root, dest16);
    wprintf(
        L"queued %zu item(s)%s\n", queued,
        refused
            ? (L" (" + std::to_wstring(refused) + L" refused: destination overlaps source)").c_str()
            : L"");

    recovery::WriterStats wstats;
    const auto t_recover0 = std::chrono::steady_clock::now();
    ProgressDisplay prog;
    rc = writer.process(
        &g_cancel_token,
        [](u64 done, u64 total, void* user) -> bool {
            auto* p = static_cast<ProgressDisplay*>(user);
            const u32 pm = total ? u32(done * 1000 / total) : 1000;
            return p->operator()(pm, nullptr);
        },
        &prog, wstats);
    prog.done();
    if (rc == ERR_DEST_FULL) {
        /* N-015: destination-full is a completed-with-failures pass, not
         * an abort: the report below still emits (per-item outcomes carry
         * the truth, including "pending" for the never-attempted tail). */
        wprintf(L"[dest] destination full: %hs (0x%08x) - stopped early, "
                L"remaining items stay pending\n",
                err_text(rc), rc);
    } else if (rc != OK) {
        wprintf(L"recovery failed: %hs (0x%08x)\n", err_text(rc), rc);
        return 1;
    }
    wprintf(L"recovered: %llu done, %llu refused, %llu error(s), %llu hash mismatch"
            L", %llu salvaged (zero-filled spans), %llu nothing-to-recover (metadata-only)\n",
            wstats.done, wstats.refused, wstats.errors, wstats.hash_mismatch, wstats.salvaged,
            wstats.nothing_to_recover);

    /* G-09: machine-readable report (schema: docs/recovery-report-schema.md).
     * Emitted even when items failed - the per-item outcomes carry the
     * truth, including "pending" for an aborted pass. */
    if (!report_path.empty()) {
        /* S-023: the scan-analysis block from the loaded session (the
         * grading pass's RecoveryAssistant statistics). */
        recovery::report::ScanAnalysis analysis;
        analysis.total_file_count = stats.assistant.total_file_count;
        analysis.raw_count = stats.assistant.raw_count;
        analysis.lost_partition_exist_file_count = stats.assistant.lost_partition_exist_file_count;
        analysis.lost_partition_delete_file_count =
            stats.assistant.lost_partition_delete_file_count;
        analysis.quick_scan_exist_file_count = stats.assistant.quick_scan_exist_file_count;
        analysis.quick_scan_delete_file_count = stats.assistant.quick_scan_delete_file_count;
        analysis.deep_scan_file_count = stats.assistant.deep_scan_file_count;
        analysis.quick_scan_delete_extend = stats.assistant.quick_scan_delete_extend;
        for (size_t k = 0; k < 8; ++k)
            analysis.category_counts[k] = stats.assistant.category_counts[k];
        analysis.matched = stats.assistant.matched;
        analysis.total = stats.assistant.total;
        analysis.small_result = stats.assistant.small_result;
        const std::string doc = recovery::report::build_report(
            writer.report_items(),
            writer.report_totals(narrow(in_path), narrow(to_dir),
                                 u64(std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - t_recover0)
                                         .count()),
                                 wstats),
            stats.assistant.graded_count ? &analysis : nullptr);
        FILE* rf = nullptr;
        if (_wfopen_s(&rf, report_path.c_str(), L"wb") != 0 || !rf) {
            wprintf(L"cannot write report %s\n", report_path.c_str());
            return 1;
        }
        const size_t wn = fwrite(doc.data(), 1, doc.size(), rf);
        fclose(rf);
        if (wn != doc.size()) {
            wprintf(L"report write truncated: %s\n", report_path.c_str());
            return 1;
        }
        wprintf(L"report: %s\n", report_path.c_str());
    }
    return wstats.errors || wstats.hash_mismatch ? 2 : 0;
}

/* ---------------------------------------------------------------------------
 * health - SMART + NVMe + TRIM report (read-only)
 * ------------------------------------------------------------------------- */
int cmd_health(int argc, wchar_t** argv, int& i) {
    std::wstring v;
    u32 disk = 0;
    bool have_disk = false;
    while (i < argc) {
        if (get_opt(argc, argv, i, L"--disk", &v)) {
            disk = u32(wcstoul(v.c_str(), nullptr, 10));
            have_disk = true;
            continue;
        }
        wprintf(L"unknown health option: %s\n", argv[i]);
        return 1;
    }
    if (!have_disk) {
        wprintf(L"health needs --disk N\n");
        return 1;
    }

    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        wprintf(L"cannot open disk %u: %hs\n", disk,
                cr::win::is_elevated()
                    ? "access denied even elevated (device busy or locked by another process)"
                    : "administrator rights required (elevate and retry)");
        return 1;
    }

    /* SMART via ATA passthrough */
    win::health::SmartReport sm;
    u32 rc = win::health::read_smart(h, sm);
    bool any_health_source = false;
    if (rc == OK && sm.available) {
        any_health_source = true;
        wprintf(L"SMART (%hs): ATA version %u, %zu attribute(s)\n", sm.transport.c_str(),
                sm.ata_version, sm.attributes.size());
        wprintf(L"  %-4s %-6s %-6s %-6s %-12s %-9s %hs\n", L"ID", L"value", L"worst", L"thr",
                L"raw", L"grade", "name");
        for (const auto& a : sm.attributes) {
            u64 raw = u64(a.raw[0]) | (u64(a.raw[1]) << 8) | (u64(a.raw[2]) << 16);
            wprintf(L"  %-4u %-6u %-6u %-6u %-12llu %-9hs %hs\n", a.id, a.value, a.worst,
                    a.vendor_threshold, static_cast<unsigned long long>(raw),
                    cr::health::health_grade_name(a.grade), a.name.c_str());
        }
        wprintf(L"  reallocated=%llu pending=%llu uncorrectable=%llu hours=%llu temp=%lluC\n",
                static_cast<unsigned long long>(sm.reallocated_sectors),
                static_cast<unsigned long long>(sm.pending_sectors),
                static_cast<unsigned long long>(sm.uncorrectable),
                static_cast<unsigned long long>(sm.power_on_hours),
                static_cast<unsigned long long>(sm.temperature_c));
        /* S-024: the og DiskHealth 4-status verdict */
        wprintf(L"  health: %hs (%hs)\n", cr::health::health_grade_name(sm.grade),
                sm.is_ssd ? "SSD" : "HDD/unknown");
        if (sm.grade == cr::health::HealthGrade::Bad)
            wprintf(L"  WARNING: BAD - back up data immediately\n");
        else if (sm.grade == cr::health::HealthGrade::Caution)
            wprintf(L"  WARNING: Caution - recover data FIRST, image before any scan\n");
        if (sm.failing_now)
            wprintf(L"  WARNING: failing attribute present\n");
        if (sm.pending_sectors || sm.uncorrectable)
            wprintf(L"  WARNING: unstable media - recover data FIRST, image before any scan\n");
    } else {
        wprintf(L"SMART unavailable (rc=%08x) - trying NVMe...\n", rc);
    }

    /* NVMe health */
    win::health::NvmeHealth nv;
    rc = win::health::read_nvme_health(h, nv);
    if (rc == OK && nv.available) {
        any_health_source = true;
        /* N-042: decidegrees from the shared accessor - the pre-fix
         * fractional digit was the KELVIN remainder (k % 10), wrong by up
         * to ~0.55 C; now identical to the ABI spelling to the digit. */
        const u32 dec_c = nv.decidegrees_c();
        wprintf(L"NVMe health: temp=%u.%uC spare=%u%%/threshold=%u%% used=%llu%% - %hs\n",
                dec_c / 10, dec_c % 10, nv.available_spare, nv.available_spare_threshold,
                static_cast<unsigned long long>(nv.percentage_used),
                cr::health::health_grade_name(cr::health::grade_nvme(nv)));
        wprintf(L"  read=%llu bytes written=%llu bytes (M-036: units x 512,000)\n",
                static_cast<unsigned long long>(nv.data_bytes_read),
                static_cast<unsigned long long>(nv.data_bytes_written));
        wprintf(L"  host-reads=%llu host-writes=%llu busy=%llumin power-on=%lluh\n",
                static_cast<unsigned long long>(nv.host_reads),
                static_cast<unsigned long long>(nv.host_writes),
                static_cast<unsigned long long>(nv.controller_busy_time),
                static_cast<unsigned long long>(nv.power_on_hours));
        wprintf(L"  unsafe-shutdowns=%llu media-errors=%llu error-log=%llu\n",
                static_cast<unsigned long long>(nv.unsafe_shutdowns),
                static_cast<unsigned long long>(nv.media_errors),
                static_cast<unsigned long long>(nv.num_error_log_entries));
        if (nv.critical_warning)
            wprintf(L"  WARNING: critical warning bits 0x%02x\n", nv.critical_warning);
    }

    /* media class + TRIM */
    wprintf(L"seek-penalty (SSD heuristics): %hs\n",
            win::health::looks_like_ssd(h) ? "none (SSD-like)" : "present (rotational?)");
    bool trim = false;
    if (win::health::query_trim_state(trim) == OK)
        wprintf(L"TRIM: %hs%hs\n", trim ? "enabled" : "disabled",
                trim ? " (WARNING: TRIM erases deleted data on SSDs - disable before recovery)"
                     : "");
    CloseHandle(h);
    /* FB-H04: when EVERY health source is unavailable the exit code must
     * say so (3) - scripts must not mistake total unavailability for a
     * healthy report. */
    return any_health_source ? 0 : 3;
}

int cmd_bench(int argc, wchar_t** argv, int& i) {
    std::wstring v;
    u32 disk = 0;
    bool have_disk = false;
    bool use_rda = true;
    std::wstring mode = L"seq";
    u32 mb = 256;
    u32 block_kb = 1024;
    u32 iterations = 1000;
    u32 seed = 4242;
    bool json = false;
    while (i < argc) {
        if (get_opt(argc, argv, i, L"--disk", &v)) {
            disk = u32(wcstoul(v.c_str(), nullptr, 10));
            have_disk = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--mode", &v)) {
            mode = v;
            continue;
        }
        if (get_opt(argc, argv, i, L"--mb", &v)) {
            mb = u32(wcstoul(v.c_str(), nullptr, 10));
            continue;
        }
        if (get_opt(argc, argv, i, L"--block-kb", &v)) {
            block_kb = u32(wcstoul(v.c_str(), nullptr, 10));
            continue;
        }
        if (get_opt(argc, argv, i, L"--iterations", &v)) {
            iterations = u32(wcstoul(v.c_str(), nullptr, 10));
            continue;
        }
        if (get_opt(argc, argv, i, L"--seed", &v)) {
            seed = u32(wcstoul(v.c_str(), nullptr, 10));
            continue;
        }
        if (wcscmp(argv[i], L"--no-rda") == 0) {
            use_rda = false;
            ++i;
            continue;
        }
        if (wcscmp(argv[i], L"--json") == 0) {
            json = true;
            ++i;
            continue;
        }
        wprintf(L"unknown bench option: %s\n", argv[i]);
        return 1;
    }
    if (!have_disk) {
        wprintf(L"bench needs --disk N\n");
        return 1;
    }

    /* transport: RDA when available (the full stack), direct fallback */
    const bool rda = use_rda && win::rda_available(L"RDA");
    u32 err = 0;
    auto dev = win::open_disk(disk, use_rda, L"RDA", &err);
    if (!dev) {
        wprintf(L"cannot open disk %u (rc=%08x): %hs\n", disk, err,
                cr::win::is_elevated() ? "access denied even elevated (device busy or locked)"
                                       : "administrator rights required (elevate and retry)");
        return 2;
    }
    const Geometry& g = dev->geometry();
    const char* transport = rda && win::rda_available(L"RDA") ? "rda" : "direct";

    auto qpc = []() -> double {
        LARGE_INTEGER c;
        QueryPerformanceCounter(&c);
        return double(c.QuadPart);
    };
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    const double freq = double(f.QuadPart);

    if (mode == L"info") {
        if (json) {
            wprintf(L"{\"mode\":\"info\",\"disk\":%u,\"transport\":\"%hs\",\"bytes\":%llu,"
                    L"\"logical_sector\":%u,\"physical_sector\":%u,\"is_4kn\":%hs}\n",
                    disk, transport, static_cast<unsigned long long>(g.total_bytes),
                    g.logical_sector, g.physical_sector, g.is_4kn() ? "true" : "false");
        } else {
            wprintf(L"disk %u via %hs: %llu bytes, logical %u / physical %u%s\n", disk, transport,
                    static_cast<unsigned long long>(g.total_bytes), g.logical_sector,
                    g.physical_sector, g.is_4kn() ? L" (4Kn)" : L"");
        }
        return 0;
    }

    if (mode == L"read") {
        /* contract probe: read LBA 0 through the active transport */
        const u32 ssec = g.logical_sector ? g.logical_sector : 512;
        void* buf = _aligned_malloc(ssec, ssec);
        if (buf == nullptr)
            return 2;
        const u32 rc = dev->read_at(0, buf, ssec);
        if (json) {
            wprintf(L"{\"mode\":\"read\",\"rc\":%u,\"first16\":[", rc);
        } else {
            wprintf(L"read LBA0 rc=%08x:", rc);
        }
        const u8* p = static_cast<const u8*>(buf);
        for (u32 k = 0; k < 16; ++k) {
            if (json)
                wprintf(L"%s%u", k ? L"," : L"", unsigned(p[k]));
            else
                wprintf(L" %02X", p[k]);
        }
        wprintf(json ? L"]}\n" : L"\n");
        _aligned_free(buf);
        return rc == OK ? 0 : 2;
    }

    if (mode == L"open") {
        /* open/close churn: measures the device-open cost (RDA grammar
         * path vs CreateFile) without touching platters */
        const u32 n = iterations < 1 ? 1 : (iterations > 10000 ? 10000 : iterations);
        const double t0 = qpc();
        for (u32 k = 0; k < n; ++k) {
            u32 e2 = 0;
            auto d2 = win::open_disk(disk, use_rda, L"RDA", &e2);
        }
        const double dt = (qpc() - t0) / freq;
        const double mean_ns = dt > 0 ? dt * 1e9 / n : 0;
        if (json) {
            wprintf(L"{\"mode\":\"open\",\"transport\":\"%hs\",\"iterations\":%u,"
                    L"\"mean_ns\":%.0f,\"ops\":%.0f}\n",
                    transport, n, mean_ns, dt > 0 ? double(n) / dt : 0);
        } else {
            wprintf(L"open: %u iterations, mean %.0f ns, %.0f ops/s (%hs)\n", n, mean_ns,
                    dt > 0 ? double(n) / dt : 0, transport);
        }
        return 0;
    }

    /* seq / rand: read workloads with a sector-aligned buffer */
    const u32 block = (block_kb < 4 ? 4 : block_kb > 16384 ? 16384 : block_kb) * 1024;
    const u64 span = u64(mb < 1 ? 1 : mb > 65536 ? 65536 : mb) << 20;
    void* buf = _aligned_malloc(block, g.logical_sector ? g.logical_sector : 512);
    if (buf == nullptr)
        return 2;

    if (mode == L"seq") {
        u64 done = 0;
        const double t0 = qpc();
        while (done + block <= span && done + block <= g.total_bytes) {
            if (dev->read_at(done, buf, block) != OK)
                break;
            done += block;
        }
        const double dt = (qpc() - t0) / freq;
        const double mibs = dt > 0 ? double(done) / (1024.0 * 1024.0) / dt : 0;
        if (json) {
            wprintf(L"{\"mode\":\"seq\",\"transport\":\"%hs\",\"block\":%u,\"bytes\":%llu,"
                    L"\"mib_per_s\":%.2f}\n",
                    transport, block, static_cast<unsigned long long>(done), mibs);
        } else {
            wprintf(L"seq: %u KiB blocks, %llu MiB via %hs: %.2f MiB/s\n", block / 1024,
                    static_cast<unsigned long long>(done >> 20), transport, mibs);
        }
        _aligned_free(buf);
        return done > 0 ? 0 : 2;
    }

    if (mode == L"rand") {
        /* LCG random offsets inside the span (rdabench-compatible) */
        u64 state = seed ? seed : 1;
        const u32 n = iterations < 1 ? 1 : (iterations > 100000 ? 100000 : iterations);
        const u64 limit = span < g.total_bytes ? span : g.total_bytes;
        double lat_total = 0;
        u32 hits = 0;
        for (u32 k = 0; k < n; ++k) {
            state = state * 6364136223846793005ull + 1442695040888963407ull;
            const u64 off = (state >> 16) % (limit > block ? limit - block : 1);
            const double t0 = qpc();
            if (dev->read_at(off, buf, block) == OK) {
                lat_total += (qpc() - t0) / freq;
                ++hits;
            }
        }
        const double dt = lat_total;
        const double mean_us = hits ? dt * 1e6 / hits : 0;
        const double iops = dt > 0 ? double(hits) / dt : 0;
        if (json) {
            wprintf(L"{\"mode\":\"rand\",\"transport\":\"%hs\",\"block\":%u,\"iterations\":%u,"
                    L"\"hits\":%u,\"mean_us\":%.2f,\"iops\":%.0f}\n",
                    transport, block, n, hits, mean_us, iops);
        } else {
            wprintf(L"rand: %u KiB blocks, %u hits, mean %.2f us, %.0f IOPS (%hs)\n", block / 1024,
                    hits, mean_us, iops, transport);
        }
        _aligned_free(buf);
        return hits > 0 ? 0 : 2;
    }

    wprintf(L"unknown bench mode: %s (seq|rand|open|read|info)\n", mode.c_str());
    return 1;
}

int cmd_fix(int argc, wchar_t** argv, int& i) {
    std::wstring v;
    u32 disk = 0;
    bool have_disk = false;
    int part = -1; /* -1 = whole disk (LBA0 view, superfloppy/lost FS) */
    bool apply = false;
    bool use_rda = true;
    std::wstring undo_path, undo_apply_path;
    while (i < argc) {
        if (get_opt(argc, argv, i, L"--disk", &v)) {
            disk = u32(wcstoul(v.c_str(), nullptr, 10));
            have_disk = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--part", &v)) {
            part = int(wcstol(v.c_str(), nullptr, 10));
            continue;
        }
        if (get_opt(argc, argv, i, L"--undo", &v)) {
            undo_path = v;
            continue;
        }
        if (get_opt(argc, argv, i, L"--undo-apply", &v)) {
            undo_apply_path = v;
            continue;
        }
        if (wcscmp(argv[i], L"--apply") == 0) {
            apply = true;
            ++i;
            continue;
        }
        if (wcscmp(argv[i], L"--no-rda") == 0) {
            use_rda = false;
            ++i;
            continue;
        }
        wprintf(L"unknown fix option: %s\n", argv[i]);
        return 1;
    }
    if (!have_disk) {
        /* undo-apply also needs the disk: the undo log carries absolute
         * sector offsets for a specific disk. Without this guard,
         * 'fix --undo-apply log.undo' silently targeted PhysicalDrive0
         * (disk defaulted to 0). PARITY-AUDIT F-B-4. */
        wprintf(L"fix needs --disk N\n");
        return 1;
    }

    /* ---- undo-apply mode: replay a saved undo log ---- */
    if (!undo_apply_path.empty()) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, undo_apply_path.c_str(), L"rb") != 0 || f == nullptr) {
            wprintf(L"cannot open undo log %s\n", undo_apply_path.c_str());
            return 1;
        }
        std::vector<u8> blob;
        {
            u8 buf[65536];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                blob.insert(blob.end(), buf, buf + n);
        }
        fclose(f);
        repair::UndoLog log;
        u32 rc = repair::UndoLog::deserialize(blob.data(), blob.size(), log);
        if (rc != OK) {
            wprintf(L"undo log invalid (rc=%08x %hs)\n", rc, err_text(rc));
            return 2;
        }
        wprintf(L"undo log: %zu sector(s)\n", log.entries.size());
        u32 e = 0;
        auto wdev = win::open_disk_rw(disk, use_rda, use_rda ? L"RDA" : nullptr, &e);
        if (!wdev) {
            wprintf(L"cannot open disk %u for write (rc=%08x): %hs\n", disk, e,
                    cr::win::is_elevated()
                        ? "write access denied even elevated (device busy, locked or protected)"
                        : "administrator rights required for repairs (elevate and retry)");
            return 2;
        }
        rc = repair::undo(*wdev, log);
        if (rc != OK) {
            wprintf(L"undo FAILED (rc=%08x %hs) - verify the disk state!\n", rc, err_text(rc));
            return 2;
        }
        wprintf(L"undo complete: %zu sector(s) restored\n", log.entries.size());
        return 0;
    }

    /* ---- analyze (read-only) ---- */
    u32 e = 0;
    auto view = win::open_disk(disk, use_rda, use_rda ? L"RDA" : nullptr, &e);
    if (!view) {
        wprintf(L"cannot open disk %u (rc=%08x)\n", disk, e);
        return 2;
    }
    IStorageDevice* slice = view.get();
    std::unique_ptr<IStorageDevice> part_slice;
    u64 part_base = 0; /* A-002: partition base in DISK bytes (0 = whole disk) */
    if (part >= 1) {
        table::DiskTable t;
        u32 trc = table::parse_disk_table(view.get(), t);
        if (trc != OK || t.partitions.empty()) {
            wprintf(L"no partition table on disk %u (rc=%08x)\n", disk, trc);
            return 2;
        }
        if (size_t(part) > t.partitions.size()) {
            wprintf(L"partition %d out of range (disk has %zu)\n", part, t.partitions.size());
            return 2;
        }
        const auto& p = t.partitions[size_t(part) - 1];
        u32 prc = 0;
        part_base = p.start_lba * view->geometry().logical_sector;
        part_slice = cr::PartitionSlice::create(
            view.get(), part_base, u64(p.sector_count) * view->geometry().logical_sector, &prc);
        if (!part_slice) {
            wprintf(L"cannot slice partition %d (rc=%08x)\n", part, prc);
            return 2;
        }
        slice = part_slice.get();
        wprintf(L"analyzing partition %d @ LBA %llu (%llu sectors)\n", part,
                static_cast<unsigned long long>(p.start_lba),
                static_cast<unsigned long long>(p.sector_count));
    } else {
        wprintf(L"analyzing disk %u whole-disk view (LBA 0)\n", disk);
    }

    repair::RepairPlan plan;
    u32 rc = repair::analyze(slice, plan);
    if (rc != OK) {
        wprintf(L"analyze failed: rc=%08x %hs\n", rc, err_text(rc));
        return 2;
    }

    if (plan.refused) {
        wprintf(L"REFUSED: %hs\n", plan.refusal_reason.c_str());
        wprintf(L"no writes performed (dry-run is the default; nothing is ever written "
                L"without --apply)\n");
        return 0;
    }
    if (plan.actions.empty()) {
        wprintf(L"clean %hs volume - nothing to repair (S2: clean volumes are never touched)\n",
                plan.detected ? plan.fs_family.c_str() : "unknown");
        return 0;
    }

    /* A-002 (discovered during the S-008 session): the plan's offsets are
     * SLICE-relative, but the write device below is the whole PHYSICAL
     * DISK - without this rebase, 'fix --part N --apply' would write the
     * repair at DISK offset 0, over the MBR/GPT. The rebase happens
     * before the display so the dry-run shows the true disk-absolute
     * targets; the undo log (and --undo-apply) carries the same basis,
     * matching its documented contract. */
    if (part_base != 0)
        repair::rebase_plan(plan, part_base);

    wprintf(L"repair plan for %hs volume: %zu action(s)%s\n", plan.fs_family.c_str(),
            plan.actions.size(), part_base != 0 ? L" (disk-absolute offsets)" : L"");
    for (const auto& a : plan.actions) {
        const char* kind =
            a.kind == repair::ActionKind::RestoreBootFromBackup ? "restore-boot-from-backup"
            : a.kind == repair::ActionKind::RepairBootChecksum  ? "repair-boot-checksum"
                                                                : "repair-geometry-field";
        wprintf(L"  [%hs] offset=%llu len=%u : %hs\n", kind,
                static_cast<unsigned long long>(a.offset), a.length, a.reason.c_str());
    }
    if (!apply) {
        wprintf(L"dry-run only - re-run with --apply to execute (undo log saved automatically)\n");
        return 0;
    }

    /* ---- execute (transactional writes) ---- */
    auto wdev = win::open_disk_rw(disk, use_rda, use_rda ? L"RDA" : nullptr, &e);
    if (!wdev) {
        wprintf(L"cannot open disk %u for write (rc=%08x): %hs\n", disk, e,
                cr::win::is_elevated()
                    ? "write access denied even elevated (device busy, locked or protected)"
                    : "administrator rights required for repairs (elevate and retry)");
        return 2;
    }
    repair::UndoLog log;
    rc = repair::execute(wdev.get(), plan, log);
    if (rc != OK) {
        wprintf(L"execute FAILED: rc=%08x %hs (all writes rolled back)\n", rc, err_text(rc));
        return 2;
    }
    wprintf(L"execute OK: %zu sector(s) written + verified\n", log.entries.size());

    /* save the undo log */
    if (undo_path.empty()) {
        undo_path = L"rrecov-fix-disk" + std::to_wstring(disk) + L".undo";
    }
    std::vector<u8> blob;
    log.serialize(blob);
    FILE* f = nullptr;
    if (_wfopen_s(&f, undo_path.c_str(), L"wb") == 0 && f != nullptr) {
        fwrite(blob.data(), 1, blob.size(), f);
        fclose(f);
        wprintf(L"undo log saved: %s (%zu bytes) - restore with: rrecov fix --disk %u "
                L"--undo-apply %s\n",
                undo_path.c_str(), blob.size(), disk, undo_path.c_str());
    } else {
        wprintf(L"WARNING: could not save the undo log to %s\n", undo_path.c_str());
    }
    return 0;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    /* Ctrl-C: cooperative cancel of the in-flight scan/recover (the
     * process is NOT killed - the token polls at loop heads, the session
     * saves, the exit code is 3). */
    SetConsoleCtrlHandler(ctrl_c_handler, TRUE);
    if (argc < 2) {
        wprintf(
            L"rrecov - core_recovery CLI\n"
            L"usage:\n"
            L"  rrecov list\n"
            L"  rrecov scan --disk N | --volume C: [--whole-disk] [--quick-only] [--no-carve]\n"
            L"             (deep stage always on; --quick-only disables it; --whole-disk = "
            L"lost-partition discovery)\n"
            L"             [--threads N] [--out file.crsf] [--resume file.crsf] [--log file]\n"
            L"             [--json] [--csv] [--filter s] [--deleted-only]\n"
            L"             [--repair] [--repair-apply] [--repair-undo file]\n"
            L"             [--bitlocker-fvek hex]  (locked volume: user-supplied FVEK,\n"
            L"                                     64/128 hex chars - in-stack XTS-AES\n"
            L"                                     decrypt, engines see plaintext)\n"
            L"  rrecov tree --in file.crsf [--filter s] [--deleted-only] [--csv]\n"
            L"  rrecov recover --in file.crsf --disk N --to DIR [--deleted-only] [--filter s]\n"
            L"             [--report file.json]\n"
            L"  rrecov health --disk N\n"
            L"  rrecov fix --disk N [--part P] [--apply] [--undo file] [--undo-apply file]\n"
            L"  rrecov bench --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]\n"
            L"              [--iterations N] [--seed N] [--json] [--no-rda]\n");
        return 0;
    }
    const std::wstring cmd = argv[1];
    int i = 2;
    if (cmd == L"list")
        return print_disks();
    if (cmd == L"scan")
        return cmd_scan(argc, argv, i);
    if (cmd == L"tree")
        return cmd_tree(argc, argv, i);
    if (cmd == L"recover")
        return cmd_recover(argc, argv, i);
    if (cmd == L"health")
        return cmd_health(argc, argv, i);
    if (cmd == L"fix")
        return cmd_fix(argc, argv, i);
    if (cmd == L"bench")
        return cmd_bench(argc, argv, i);
    wprintf(L"unknown command: %s\n", cmd.c_str());
    return 1;
}
