/*
 * rrecov - the core_recovery CLI (Windows).
 *
 * Commands:
 *   list                                  enumerate disks/volumes
 *   scan --disk N | --volume C: [--deep] [--quick-only] [--no-carve]
 *         [--threads N] [--out file.crsf] [--json] [--log file]
 *   tree --in file.crsf [--filter substr] [--deleted-only] [--csv]
 *   recover --in file.crsf --disk N --to DIR [--deleted-only] [--filter s]
 *   health --disk N
 *   fix --disk N [--part P] [--apply] [--undo file] [--undo-apply file]
 *   bench --disk N --mode seq|rand|open|read --mb N --block-kb N [--json]
 *
 * Everything is logged in detail (stage timings, per-stage counts, error
 * maps) for testing, validation and benchmarking workflows.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/repair/cr_rawfixer.h"
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/storage/cr_slice.h"
#include "cr/table/cr_table_parse.h"
#include "cr/win/cr_bitlocker.h"
#include "cr/win/cr_destio.h"
#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_health.h"
#include "cr/win/cr_transport.h"
#include <windows.h>
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
    void done() { wprintf(L"\r  100.0%%  done          \n"); }
};

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
    u32 disk = 0;
    u64 volume_start = 0; /* 0 = whole-disk view */
    u64 volume_len = 0;
};

bool open_disk_view(u32 n, OpenResult& out) {
    u32 err = 0;
    /* RDA first when present (locked volumes, damaged media), direct
     * fallback - the reference Device.mo duality. */
    if (win::rda_available(L"RDA")) {
        if (auto dev = win::open_rda_partition(L"RDA", n, 0, &err)) {
            wprintf(L"[transport] RDA driver path (\\\\.\\RDA)\n");
            out.device = std::move(dev);
            out.disk = n;
            return true;
        }
        wprintf(L"[transport] RDA open failed (%hs) - direct path\n", err_text(err));
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

bool open_volume_view(wchar_t letter, OpenResult& out) {
    if (win::bitlocker::is_fve_volume(letter)) {
        if (win::bitlocker::status(letter) == win::bitlocker::Status::Locked) {
            wprintf(L"[bitlocker] %c: locked - trying auto-unlock...\n", letter);
            if (win::bitlocker::try_auto_unlock(letter) != OK) {
                wprintf(L"[bitlocker] still locked. Unlock with:\n"
                        L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                        L"then re-run.\n",
                        letter);
                return false;
            }
            wprintf(L"[bitlocker] auto-unlocked\n");
        }
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
    out.device = std::move(slice);
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

void print_disks() {
    win::PhysicalDiskList list;
    if (win::enumerate_disks(200, list) != OK) {
        wprintf(L"enumeration failed\n");
        return;
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
}

/* ---------------------------------------------------------------------------
 * tree printing (shared by scan + tree commands)
 * ------------------------------------------------------------------------- */

struct TreePrintFlags {
    bool deleted_only = false;
    bool csv = false;
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
    OpenResult target;
    bool have_target = false;
    orchestrator::ScanConfig cfg;
    cfg.whole_disk = false;
    std::wstring out_path, log_path;
    bool json = false;
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
            if (!open_volume_view(v[0], target))
                return 1;
            have_target = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--deep")) {
            cfg.whole_disk = true;
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
        wprintf(L"unknown scan option: %s\n", argv[i]);
        return 1;
    }
    if (!have_target) {
        wprintf(L"scan needs --disk N or --volume C:\n");
        return 1;
    }

    /* lost-partition discovery only makes sense over a whole-disk view */
    if (cfg.whole_disk && target.volume_len != 0) {
        wprintf(L"[note] --deep on a volume: lost-partition discovery needs --disk (skipping "
                L"that stage)\n");
        cfg.whole_disk = false;
    }
    log_open(log_path.empty() ? nullptr : log_path.c_str());
    log_line("rrecov scan: threads=%u quick=%d deep=%d carve=%d whole_disk=%d", cfg.threads,
             int(cfg.run_quick), int(cfg.run_deep), int(cfg.run_carve), int(cfg.whole_disk));

    tree::ResultTree tree;
    orchestrator::ScanController ctrl(tree, cfg);
    orchestrator::ScanStats stats;
    ProgressDisplay prog;
    log_line("scan start");
    const u32 rc = ctrl.run(
        target.device.get(), nullptr,
        [](u32 pm, void* user) -> bool {
            return static_cast<ProgressDisplay*>(user)->operator()(pm, nullptr);
        },
        &prog, stats);
    prog.done();
    log_line("scan end rc=0x%08x files=%llu deleted=%llu carved=%llu lost_parts=%llu "
             "bytes=%llu elapsed_ms=%llu errors=%zu",
             rc, stats.files_found, stats.deleted_found, stats.carved_found, stats.lost_partitions,
             stats.bytes_scanned, stats.elapsed_ms, stats.errors.ranges().size());
    for (const auto& er : stats.errors.ranges())
        log_line("  read-error range: @%llu +%llu (attempts %u)",
                 static_cast<unsigned long long>(er.offset),
                 static_cast<unsigned long long>(er.length), er.attempts);
    log_close();

    if (rc != OK) {
        wprintf(L"scan failed: %hs (0x%08x)\n", cr::err_text(rc), rc);
        return 1;
    }
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

    if (!out_path.empty()) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, out_path.c_str(), L"wb") != 0 || !f) {
            wprintf(L"cannot create session file %s\n", out_path.c_str());
            return 1;
        }
        session::Writer w(file_write_cb, file_tell_cb, file_seek_cb, f);
        if (w.open(0x52434F56) != OK || ctrl.save_session(w, stats) != OK || w.finalize() != OK) {
            wprintf(L"session save failed\n");
            fclose(f);
            return 1;
        }
        fclose(f);
        wprintf(L"session saved: %s\n", out_path.c_str());
    }
    return 0;
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
    print_tree(tree, f);
    return 0;
}

/* ---------------------------------------------------------------------------
 * recover (session -> files)
 * ------------------------------------------------------------------------- */

int cmd_recover(int argc, wchar_t** argv, int& i) {
    std::wstring in_path, to_dir, filter;
    bool deleted_only = false;
    u32 disk = 0;
    bool have_disk = false;
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
        if (get_opt(argc, argv, i, L"--filter", &filter))
            continue;
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

    /* open the SOURCE disk (extents in sessions are absolute) */
    OpenResult src;
    if (!open_disk_view(disk, src))
        return 1;

    /* destination validation + overlap refusal (F12): the destination
     * volume extent must not intersect the source disk */
    if (to_dir.size() >= 2 && to_dir[1] == L':') {
        u32 ddisk = 0;
        u64 dstart = 0, dlen = 0;
        if (win::volume_extent(to_dir[0], &ddisk, &dstart, &dlen)) {
            if (ddisk == disk)
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

    /* enqueue every matching file node */
    size_t queued = 0, refused = 0;
    std::function<void(tree::FileNode*, const std::u16string&)> plan =
        [&](tree::FileNode* n, const std::u16string& path) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                if (!c->props())
                    continue;
                const bool is_dir = (c->props()->attributes & 0x10) != 0;
                const bool deleted = c->deleted_state() != tree::DeletedState::Active;
                if (is_dir) {
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
    ProgressDisplay prog;
    rc = writer.process(
        nullptr,
        [](u64 done, u64 total, void* user) -> bool {
            auto* p = static_cast<ProgressDisplay*>(user);
            const u32 pm = total ? u32(done * 1000 / total) : 1000;
            return p->operator()(pm, nullptr);
        },
        &prog, wstats);
    prog.done();
    if (rc != OK) {
        wprintf(L"recovery failed: %hs (0x%08x)\n", err_text(rc), rc);
        return 1;
    }
    wprintf(L"recovered: %llu done, %llu refused, %llu error(s), %llu hash mismatch\n", wstats.done,
            wstats.refused, wstats.errors, wstats.hash_mismatch);
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
        wprintf(L"cannot open disk %u (admin?)\n", disk);
        return 1;
    }

    /* SMART via ATA passthrough */
    health::SmartReport sm;
    u32 rc = health::read_smart(h, sm);
    if (rc == OK && sm.available) {
        wprintf(L"SMART (%hs): ATA version %u, %zu attribute(s)\n", sm.transport.c_str(),
                sm.ata_version, sm.attributes.size());
        wprintf(L"  %-4s %-6s %-6s %-12s %hs\n", L"ID", L"value", L"worst", L"raw", "name");
        for (const auto& a : sm.attributes) {
            u64 raw = u64(a.raw[0]) | (u64(a.raw[1]) << 8) | (u64(a.raw[2]) << 16);
            wprintf(L"  %-4u %-6u %-6u %-12llu %hs\n", a.id, a.value, a.worst,
                    static_cast<unsigned long long>(raw), a.name.c_str());
        }
        wprintf(L"  reallocated=%llu pending=%llu uncorrectable=%llu hours=%llu temp=%lluC\n",
                static_cast<unsigned long long>(sm.reallocated_sectors),
                static_cast<unsigned long long>(sm.pending_sectors),
                static_cast<unsigned long long>(sm.uncorrectable),
                static_cast<unsigned long long>(sm.power_on_hours),
                static_cast<unsigned long long>(sm.temperature_c));
        if (sm.failing_now)
            wprintf(L"  WARNING: failing attribute present\n");
        if (sm.pending_sectors || sm.uncorrectable)
            wprintf(L"  WARNING: unstable media - recover data FIRST, image before any scan\n");
    } else {
        wprintf(L"SMART unavailable (rc=%08x) - trying NVMe...\n", rc);
    }

    /* NVMe health */
    health::NvmeHealth nv;
    rc = health::read_nvme_health(h, nv);
    if (rc == OK && nv.available) {
        wprintf(L"NVMe health: temp=%u.%uC spare=%u%%/threshold=%u%% used=%llu%%\n",
                u32(nv.temperature_k) - 273, (u32(nv.temperature_k) % 10), nv.available_spare,
                nv.available_spare_threshold, static_cast<unsigned long long>(nv.percentage_used));
        wprintf(L"  read=%llu units written=%llu units power-on=%lluh unsafe-shutdowns=%llu "
                L"media-errors=%llu\n",
                static_cast<unsigned long long>(nv.data_units_read),
                static_cast<unsigned long long>(nv.data_units_written),
                static_cast<unsigned long long>(nv.power_on_hours),
                static_cast<unsigned long long>(nv.unsafe_shutdowns),
                static_cast<unsigned long long>(nv.media_errors));
        if (nv.critical_warning)
            wprintf(L"  WARNING: critical warning bits 0x%02x\n", nv.critical_warning);
    }

    /* media class + TRIM */
    wprintf(L"seek-penalty (SSD heuristics): %hs\n",
            health::looks_like_ssd(h) ? "none (SSD-like)" : "present (rotational?)");
    bool trim = false;
    if (health::query_trim_state(trim) == OK)
        wprintf(L"TRIM: %hs%s\n", trim ? "enabled" : "disabled",
                trim ? " (WARNING: TRIM erases deleted data on SSDs - disable before recovery)"
                     : "");
    CloseHandle(h);
    return 0;
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
    if (!have_disk && undo_apply_path.empty()) {
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
            wprintf(L"cannot open disk %u for write (rc=%08x, admin?)\n", disk, e);
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
        part_slice =
            cr::PartitionSlice::create(view.get(), p.start_lba * view->geometry().logical_sector,
                                       u64(p.sector_count) * view->geometry().logical_sector, &prc);
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

    wprintf(L"repair plan for %hs volume: %zu action(s)\n", plan.fs_family.c_str(),
            plan.actions.size());
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
        wprintf(L"cannot open disk %u for write (rc=%08x, admin?)\n", disk, e);
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
    if (argc < 2) {
        wprintf(L"rrecov - core_recovery CLI\n"
                L"usage:\n"
                L"  rrecov list\n"
                L"  rrecov scan --disk N | --volume C: [--deep] [--quick-only] [--no-carve]\n"
                L"             [--threads N] [--out file.crsf] [--log file] [--json] [--csv]\n"
                L"  rrecov tree --in file.crsf [--filter s] [--deleted-only] [--csv]\n"
                L"  rrecov recover --in file.crsf --disk N --to DIR [--deleted-only] [--filter s]\n"
                L"  rrecov health --disk N\n"
                L"  rrecov fix --disk N [--part P] [--apply] [--undo file] [--undo-apply file]\n");
        return 0;
    }
    const std::wstring cmd = argv[1];
    int i = 2;
    if (cmd == L"list") {
        print_disks();
        return 0;
    }
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
    wprintf(L"unknown command: %s\n", cmd.c_str());
    return 1;
}
