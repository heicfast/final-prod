/*
 * rrecov - the core_recovery CLI (Windows).
 *
 * Commands:
 *   list                                  enumerate disks/volumes
 *   list-carve-formats                    the two-layer carve registry
 *   sessions [--dir D] [--json]          the saved-session list (light
 *                                         summary per file; the og's
 *                                         LastScanRecordView surface)
 *   scan --disk N | --volume C: | --image file [--whole-disk] [--deep]
 *         [--quick-only] [--no-carve] [--threads N] [--out file.crsf]
 *         [--resume file.crsf] [--autosave-dir D] [--label s] [--json]
 *         [--log file] [--csv]
 *         [--filter s] [--deleted-only] [--bitlocker-fvek hex]
 *         [--bitlocker-dict [file]] [--user-signs file] [--repair]
 *         [--repair-apply] [--repair-undo file] [--check]
 *   tree --in file.crsf [--filter substr] [--deleted-only] [--csv] [--json]
 *   recover --in file.crsf (--disk N | --image file) --to DIR
 *         [--deleted-only] [--filter s] [--report file.json]
 *   health --disk N
 *   fix --disk N [--part P] [--apply] [--undo file] [--undo-apply file]
 *   apply --disk N --session file.crsf [--candidate N] [--apply]
 *         [--undo file] [--boot-fix lba] [--boot-fix-bcd file]
 *   bench --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]
 *         [--iterations N] [--seed N] [--json]
 *   driver [--service NAME] [--json]      the driver-status surface
 *
 * Exit codes: scan 0 ok / 1 failure / 3 cancelled (session saved when --out
 * was given); list 1 on enumeration failure; list-carve-formats 1 on the
 * empty registry; sessions 1 on enumeration failure (unreadable files are
 * per-row data, never a failure); health 3 when every health source is
 * unavailable;
 * recover 2 partial (item errors / hash mismatch); apply / fix 2 on
 * open/plan/execute/undo failure; bench 2 on open/read failure or zero
 * hits;
 * 4 = the driver prerequisite is not met (any driver-requiring verb; the
 * refusal names the exact ERR_DRIVER_* class on stderr - see
 * docs/DRIVER-REQUIRED.md).
 *
 * Everything is logged in detail (stage timings, per-stage counts, error
 * maps) for testing, validation and benchmarking workflows.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_log.h"
#include "cr/carve/cr_signatures.h" /* N-033 CLI half: the carve-format registry */
#include "cr/carve/cr_rawsigns.h"
#include "cr/carve/cr_usersigns.h" /* N-064: --user-signs installs through the same core path the cr_ui ABI wraps */
#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_dict.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/query/cr_query.h"
#include "cr/apply/cr_apply.h"
#include "cr/repair/cr_rawfixer.h"
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/storage/cr_slice.h"
#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_parse.h"
#include "cr/win/cr_bitlocker.h"
#include "cr/win/cr_destio.h"
#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_health.h"
#include "cr/win/cr_transport.h"
#include "cr/win/cr_elevate.h"
#include "cr/win/cr_bootfix.h"
#include "cr/storage/cr_decrypt_device.h"
#include "cr/crypto/cr_aes.h"
#include "cr/cr_text.h" /* N-009c: --filter UTF-8 encoding */
#include <windows.h>
#include <map>
#include <chrono>
#include <atomic>
#include <cstdarg>
#include <cwctype> /* W1a: towlower (the sessions *.crsf tail check) */
#include <ctime>   /* W1a: localtime_s (the sessions save-time display) */
#include <filesystem>
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

/* N-048: the CLI's --log FILE* is the host sink for the core logging seam
 * (the og's EPMLOG.log role - the persistent run-log artifact). The sink
 * runs on any thread (engine workers); vfprintf on a FILE* is CRT-locked,
 * and the seam serializes deliveries on top. */
void log_sink(cr::log::Level level, const char* line, void*) {
    const char* tag = level == cr::log::Level::Error  ? "E"
                      : level == cr::log::Level::Warn ? "W"
                                                      : "I";
    fprintf(g_log, "[%s] %s\n", tag, line);
    fflush(g_log);
}

void log_open(const wchar_t* path) {
    if (g_log)
        fclose(g_log);
    g_log = nullptr;
    if (path)
        _wfopen_s(&g_log, path, L"w");
    /* the seam follows the file: open = sink on, close/reopen = swap */
    cr::log::set_sink(g_log ? &log_sink : nullptr, nullptr);
}

void log_close() {
    cr::log::set_sink(nullptr, nullptr); /* detach before the FILE* dies */
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

/* OQ-S4-3's TTFR probe (the acceptance's scan-level time-to-first-result
 * half; probe-target correction 97cc3b2 = ride the staged-scenarios lane):
 * the typed event stream's FIRST FilesFound event with a positive count is
 * the earliest observable "first result" (counters coalesce at stage
 * boundaries - the event's timestamp_ms is run-elapsed, pause-compensated).
 * One log line, once; the staged-scenarios evidence JSON extracts it
 * (verify-scenario.ps1). Events may fire from engine worker threads - the
 * once-flag is atomic. */
struct TtfrProbe {
    std::atomic<bool> fired{false};
};

static void ttfr_event_fn(const orchestrator::event::Event& evt, void* user) {
    auto* probe = static_cast<TtfrProbe*>(user);
    if (evt.type == orchestrator::event::Type::FilesFound && evt.files_found > 0 &&
        !probe->fired.exchange(true)) {
        log_line("ttfr_ms=%llu first_files=%llu stage=%u",
                 static_cast<unsigned long long>(evt.timestamp_ms),
                 static_cast<unsigned long long>(evt.files_found), evt.stage);
    }
}

/* ---------------------------------------------------------------------------
 * Device opening (shared by scan/recover/bench).
 * ------------------------------------------------------------------------- */

struct OpenResult {
    std::unique_ptr<IStorageDevice> device;     /* the view handed to engines */
    std::unique_ptr<IStorageDevice> hold;       /* parent disk (slices reference it) */
    std::unique_ptr<IStorageDevice> slice_hold; /* M-033: the slice under the
                                                 * decrypting device (wrapper is
                                                 * non-owning; kept alive here) */
    u32 disk = 0;
    u64 volume_start = 0; /* 0 = whole-disk view */
    u64 volume_len = 0;
    wchar_t volume_letter = 0; /* M-079: the --volume target's letter (the
                                * label derivation's source; 0 = not a
                                * letter-addressed volume) */
    bool is_image = false;     /* S-030: the target is a disk-image file */
    std::wstring image_path;   /* the --image argument (identity + resume) */
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
    /* driver-only (PLAN-DRIVER-ONLY R9): the direct RW retry is gone; an
     * RW-open failure with a Ready driver is a truthful target error and
     * the repair stays unavailable for this scan (orchestrator contract,
     * cr_scan.h:212-216) */
    auto w = win::open_disk_rw(c->disk, L"RDA", &e);
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

/* M-079: the volume label for the session record + the auto-save name
 * (the og's GetVolumeName - rsf-format:167; the auto_<vol>_ naming
 * :443-451 with every corpus sample labeled). The og derives it from
 * the scanned volume itself; the CLI asks the win layer by letter. An
 * explicit --label always wins (checked by the caller: empty = derive,
 * nonempty = the user's). Whole-disk targets have no volume name (the
 * og has no whole-disk scan; the timestamp-only shape is the documented
 * fallback). Image targets derive the label half from the image
 * filename stem (the og has no image-scan naming semantics - the
 * S-030-registered divergence's naming half, documented here). */
void fill_volume_label(OpenResult& target, orchestrator::ScanConfig& cfg) {
    if (cfg.volume_label[0] != 0)
        return; /* the explicit --label override */
    if (target.is_image) {
        /* the image filename's stem: "disk.img" -> "disk" */
        const wchar_t* path = target.image_path.c_str();
        const wchar_t* base = wcsrchr(path, L'\\');
        base = base ? base + 1 : path;
        const wchar_t* dot = wcsrchr(base, L'.');
        size_t n = dot ? size_t(dot - base) : wcslen(base);
        if (n == 0)
            return;
        if (n > sizeof(cfg.volume_label) - 1)
            n = sizeof(cfg.volume_label) - 1;
        for (size_t k = 0; k < n && base[k] < 0x80; ++k)
            cfg.volume_label[k] = char(base[k]);
        cfg.volume_label[n] = 0;
        return;
    }
    if (target.volume_letter == 0)
        return; /* whole-disk: no volume name (documented fallback) */
    wchar_t root[4] = {target.volume_letter, L':', L'\\', 0};
    wchar_t label[64] = {};
    if (!GetVolumeInformationW(root, label, 64, nullptr, nullptr, nullptr, nullptr, 0))
        return; /* unformatted/locked/label-less: the timestamp-only shape */
    size_t k = 0;
    for (; k + 1 < sizeof(cfg.volume_label) && label[k] && label[k] < 0x80; ++k)
        cfg.volume_label[k] = char(label[k]);
    cfg.volume_label[k] = 0;
}

/* S-030: open a disk-image file as the scan target (the og's "Import a
 * disk image" flow - DRWUI's extension factory with the flat fallback).
 * Prints the format + the LBA0 layout classification exactly like the
 * og's construction-time classify (DRWUI:275877-275880). */
bool open_image_view(const std::wstring& path, OpenResult& out) {
    u32 err = 0;
    std::u16string p16(path.begin(), path.end());
    auto dev = storage::open_image(win::image_file_io(), p16, &err);
    if (!dev) {
        const wchar_t* hint = err == ERR_UNSUPPORTED
                                  ? L" (proprietary/unsupported container family - see "
                                    L"docs/PARITY-DELTAS.md)"
                                  : L"";
        wprintf(L"[image] cannot open %s: %hs (0x%08x)%s\n", path.c_str(), err_text(err), err,
                hint);
        return false;
    }
    const storage::ImageFormat fmt = storage::classify_extension(p16);
    const wchar_t* fmt_name = fmt == storage::ImageFormat::Vmdk   ? L"VMDK"
                              : fmt == storage::ImageFormat::Vhd  ? L"VHD"
                              : fmt == storage::ImageFormat::Vhdx ? L"VHDX"
                                                                  : L"raw";
    /* LBA0 classification (the og reads 0x1000 bytes and classifies
     * MBR/GPT at construction; a failed read prints unknown) */
    u8 lba0[512] = {};
    const wchar_t* layout = L"unknown layout";
    if (dev->read_at(0, lba0, sizeof(lba0)) == OK) {
        const storage::Lba0Layout lay = storage::classify_lba0(lba0, sizeof(lba0));
        layout = lay == storage::Lba0Layout::Mbr   ? L"MBR"
                 : lay == storage::Lba0Layout::Gpt ? L"GPT"
                                                   : L"unknown layout";
    }
    wprintf(L"[image] %s (%s, %s, %llu MiB)\n", path.c_str(), fmt_name, layout,
            static_cast<unsigned long long>(dev->size() >> 20));
    out.device = std::move(dev);
    out.is_image = true;
    out.image_path = path;
    return true;
}

/* ---- Driver precondition (docs/DRIVER-REQUIRED.md; PLAN-DRIVER-ONLY
 * section 5): the data path is driver-only. require_driver() runs BEFORE
 * any other output in every driver-requiring verb - the refusal is the
 * whole response (no "starting scan", no partial progress, exit 4).
 * Returns 0 when the driver is Ready; the verb's exit code (4) otherwise. */
int require_driver() {
    win::DriverStatus st;
    const u32 qrc = win::query_driver_status(win::kDefaultDriverService, &st);
    if (qrc != OK) {
        fwprintf(stderr, L"rrecov: driver status query failed (%hs 0x%08x)\n", err_text(qrc), qrc);
        return 4;
    }
    const win::DriverState ds = win::driver_state(st);
    if (ds == win::DriverState::Ready)
        return 0;
    const u32 rc = win::driver_state_error(ds);
    switch (ds) {
    case win::DriverState::Absent:
        fwprintf(stderr,
                 L"rrecov: driver required - the RDA driver is not installed (service \"%s\" "
                 L"not found).\nrrecov: install it from the product installer or the rda-driver "
                 L"release (install.ps1), then retry.\n",
                 win::kDefaultDriverService);
        break;
    case win::DriverState::NotStarted:
        fwprintf(stderr,
                 L"rrecov: driver required - service \"%s\" is installed but not running.\n"
                 L"rrecov: start it (elevated: sc start %s) or re-run the installer, then "
                 L"retry.\n",
                 win::kDefaultDriverService, win::kDefaultDriverService);
        break;
    case win::DriverState::Unauthorized:
        fwprintf(stderr,
                 L"rrecov: driver required - the driver denied access (its security descriptor "
                 L"allows administrators and SYSTEM only).\nrrecov: run elevated / from an "
                 L"administrator account, then retry.\n");
        break;
    case win::DriverState::VersionMismatch:
        fwprintf(stderr,
                 L"rrecov: driver required - driver version %u.%u is incompatible with this "
                 L"core (requires major %u).\nrrecov: update the driver via the installer, then "
                 L"retry.\n",
                 st.major, st.minor, win::kDriverWireMajorRequired);
        break;
    default:
        fwprintf(stderr,
                 L"rrecov: driver required - service \"%s\" is running but its control device "
                 L"is not usable (CreateFile/GET_INFO error %lu).\nrrecov: reinstall the "
                 L"driver, then retry.\n",
                 win::kDefaultDriverService, st.last_gle);
        break;
    }
    fwprintf(stderr, L"rrecov: [%hs 0x%08x] - no operation was attempted\n", err_text(rc), rc);
    return 4;
}

/* argv scan: does the tail carry a driver-independent target (--image)?
 * The scan/recover preconditions skip the driver gate for image targets
 * (classified (b), docs/DRIVER-REQUIRED.md - images never use the driver). */
bool argv_has_image_target(int argc, wchar_t** argv, int from) {
    for (int k = from; k < argc; ++k)
        if (wcscmp(argv[k], L"--image") == 0)
            return true;
    return false;
}

bool open_disk_view(u32 n, OpenResult& out) {
    u32 err = 0;
    /* Driver-only data path (PLAN-DRIVER-ONLY R1): the grammar path is the
     * ONLY leg - the probe->direct fallback is gone. The verb-level
     * require_driver() gate owns the exit-code contract; this in-path check
     * is the truth layer for direct callers (and the TOCTOU window). */
    win::DriverStatus st;
    if (win::query_driver_status(win::kDefaultDriverService, &st) == OK) {
        const win::DriverState ds = win::driver_state(st);
        if (ds != win::DriverState::Ready) {
            const u32 rc = win::driver_state_error(ds);
            wprintf(L"[driver] RDA driver not usable: %hs (0x%08x, last error %lu) - the data "
                    L"path requires the driver (docs/DRIVER-REQUIRED.md)\n",
                    err_text(rc), rc, st.last_gle);
            return false;
        }
    }
    if (auto dev = win::open_rda_partition(L"RDA", n, 0, &err)) {
        wprintf(L"[transport] RDA driver path (\\\\.\\RDA)\n");
        out.device = std::move(dev);
        out.disk = n;
        return true;
    }
    /* driver Ready but the TARGET would not open: a target error, never a
     * silent transport switch (%ls: the guidance literals are wchar_t) */
    wprintf(L"[transport] cannot open disk %u through RDA (rc=%08x %hs): %ls\n", n, err,
            err_text(err),
            cr::win::is_elevated() ? L"write/device access denied even elevated - device busy, "
                                     L"locked, protected or the disk number is out of range"
                                   : L"administrator rights may be required (elevate and retry)");
    return false;
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

/* N-064 (absorbed from the convergent DFX34 landing): load user password
 * candidates from a dictionary file in the og's exact Dict format
 * (comma-terminated lines, CRLF/LF, pure ASCII - cr::dict::parse is the
 * grammar). Honest failures: unreadable file, over-cap blob, grammar
 * violation, or zero valid entries each abort with a diagnostic (a
 * silent fallback to the shipped-only set would mask the user's intent).
 * The 256 KiB read cap bounds hostile inputs. */
bool load_dict_candidates(const std::wstring& path, std::vector<std::string>& out) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || f == nullptr) {
        wprintf(L"[bitlocker] --bitlocker-dict: cannot open %ls\n", path.c_str());
        return false;
    }
    constexpr size_t kMaxDictFileBytes = 256 * 1024;
    std::vector<char> bytes(kMaxDictFileBytes + 1);
    const size_t got = fread(bytes.data(), 1, bytes.size(), f);
    const bool over_cap = got > kMaxDictFileBytes || (!feof(f) && got == bytes.size());
    fclose(f);
    if (over_cap) {
        wprintf(L"[bitlocker] --bitlocker-dict: %ls exceeds the %zu KiB cap\n", path.c_str(),
                kMaxDictFileBytes / 1024);
        return false;
    }
    bytes.resize(got);
    if (!dict::parse(bytes.data(), bytes.size(), out)) {
        wprintf(L"[bitlocker] --bitlocker-dict: %ls is not a valid dictionary "
                L"(ASCII, one entry per line, entries 1..%zu chars)\n",
                path.c_str(), dict::kMaxPasswordLen);
        return false;
    }
    if (out.empty()) {
        wprintf(L"[bitlocker] --bitlocker-dict: %ls holds no valid entries\n", path.c_str());
        return false;
    }
    return true;
}

/* N-064: open_volume_view carries the ONE consented dictionary surface:
 * --bitlocker-dict (bare = the shipped 228-entry table; with a file
 * value = the user's og-format dictionary) - the og's user-initiated
 * unlock action. It never fires without the flag (F61). */
bool open_volume_view(wchar_t letter, OpenResult& out, const std::wstring& fvek_hex,
                      bool dict_consent, const std::vector<std::string>& dict_user) {
    std::vector<u8> fvek;
    const bool have_key = !fvek_hex.empty() && parse_fvek_hex(fvek_hex, fvek);
    if (!fvek_hex.empty() && !have_key) {
        wprintf(L"[bitlocker] --bitlocker-fvek: expected 64 hex chars (AES-128-XTS) "
                L"or 128 hex chars (AES-256-XTS)\n");
        return false;
    }
    const win::bitlocker::Status bst = win::bitlocker::status(letter);
    /* N-064's Unknown-state design point (the convergent landing's, kept):
     * a boot-unreadable volume (typical: a LOCKED volume denying \\.\X:
     * reads) with the explicit dictionary consent gets the attempt BEFORE
     * the FB-B01 refusal - a locked volume denying reads is exactly the
     * dictionary aid's target population. Without consent/key: refuse. */
    if (bst == win::bitlocker::Status::Unknown && !have_key) {
        if (dict_consent) {
            wprintf(L"[bitlocker] %c: boot sector unreadable - trying the password "
                    L"dictionary...\n",
                    letter);
            if (win::bitlocker::unlock_with_dictionary(letter, dict_user) != OK) {
                wprintf(L"[bitlocker] still locked. Unlock with:\n"
                        L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                        L"or re-run with --bitlocker-fvek <hex> (the in-stack decrypt path),\n"
                        L"or --bitlocker-dict [file] (the consented password-dictionary unlock).\n",
                        letter);
                return false;
            }
            wprintf(L"[bitlocker] dictionary-unlocked\n");
        } else {
            /* Boot sector unreadable: the BitLocker state cannot be determined.
             * Never scan blind - refuse with the unlock path (FB-B01). With a
             * user key the in-stack decrypt path may proceed (M-033): the user
             * has supplied key material, which is the og's own opt-in. */
            wprintf(L"[bitlocker] %c: boot sector unreadable - cannot determine BitLocker state\n"
                    L"  (typical for a locked volume). Unlock with:\n"
                    L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                    L"or re-run with --bitlocker-fvek <hex> (the in-stack decrypt path),\n"
                    L"or --bitlocker-dict [file] to try a password dictionary.\n",
                    letter, letter);
            return false;
        }
    }
    if (bst == win::bitlocker::Status::Locked && !have_key) {
        wprintf(L"[bitlocker] %c: locked - trying auto-unlock...\n", letter);
        bool unlocked = win::bitlocker::try_auto_unlock(letter) == OK;
        if (!unlocked && dict_consent) {
            /* N-064: the dictionary unlock aid runs ONLY on explicit user
             * consent (--bitlocker-dict on the command line = the og's
             * user-initiated password dialog). Without the flag the volume
             * stays refused - legitimate unlock only, never an unattended
             * password sweep. */
            wprintf(L"[bitlocker] trying the password dictionary...\n");
            unlocked = win::bitlocker::unlock_with_dictionary(letter, dict_user) == OK;
            if (unlocked) {
                wprintf(L"[bitlocker] dictionary-unlocked\n");
            } else {
                wprintf(L"[bitlocker] dictionary exhausted - still locked. Unlock with:\n"
                        L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                        L"or re-run with --bitlocker-fvek <hex> (the in-stack decrypt path),\n"
                        L"or --bitlocker-dict [file] with a different dictionary.\n",
                        letter);
                return false;
            }
        } else if (!unlocked) {
            wprintf(L"[bitlocker] still locked. Unlock with:\n"
                    L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                    L"or re-run with --bitlocker-fvek <hex> (the in-stack decrypt path),\n"
                    L"or --bitlocker-dict [file] to try a password dictionary.\n",
                    letter);
            return false;
        } else {
            wprintf(L"[bitlocker] auto-unlocked\n");
        }
    }
    if (have_key &&
        (bst == win::bitlocker::Status::Locked || bst == win::bitlocker::Status::Unknown ||
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
                d.table_ok
                    ? (d.table.layout == table::TableLayout::Gpt
                           ? L"GPT"
                           : (d.table.layout == table::TableLayout::Mbr
                                  ? L"MBR"
                                  : (d.table.layout == table::TableLayout::Ebr ? L"EBR" : L"-")))
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
 * list-carve-formats (N-033's CLI half: the og host enumerates carvers
 * through EUGetAnalyserList/Map/Multimap to build its "which formats can
 * we carve" UI; the ABI counterpart cr_ui_carve_format_count/info is the
 * same surface - this is the CLI projection of it).
 * ------------------------------------------------------------------------- */

int print_carve_formats() {
    /* Same projection the cr_ui ABI enumerates (cr_ui_carve_format_count/
     * info are thin wrappers over this exact walk), consumed through the
     * core registry directly: the CLI stays a standalone cr_core+cr_win
     * binary - linking cr_ui.dll here would give the process TWO copies
     * of the engine (the static core in the exe + the one inside the
     * DLL), and a --user-signs install would land in the copy the exe's
     * scan never reads. The cr_ui wrapper surface itself stays pinned
     * by the cr_ui ABI tests. */
    const uint32_t grammar_n = uint32_t(carve::registry_size());
    const uint32_t raw_n = carve::rawsigns::kFormatCount;
    const uint32_t n = grammar_n + raw_n;
    if (n == 0) {
        wprintf(L"carve registry is empty\n");
        return 1; /* never exit-0 on an empty registry - scripts must not
                     mistake a broken table for "nothing to carve" */
    }
    wprintf(L"%u carve format(s)\n", n);
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t layer;
        const char* ext;
        uint64_t min_size, max_size;
        uint16_t conf;
        uint32_t magic_len = 0;
        const uint8_t* magic = nullptr;
        if (i < grammar_n) {
            const carve::Signature& s = carve::registry()[i];
            layer = 0;
            ext = s.ext;
            min_size = s.min_size;
            max_size = s.max_size;
            conf = s.base_confidence;
            magic_len = s.header_len;
            magic = s.header;
        } else {
            const uint32_t fmt = i - grammar_n;
            const carve::rawsigns::RawSignFormat& f = carve::rawsigns::kFormats[fmt];
            layer = 1;
            ext = f.ext;
            min_size = f.min_size;
            max_size = f.max_size;
            /* the format's strongest header-sign confidence (the table
             * stores confidence per sign, not per format) */
            uint16_t best = 0;
            for (uint32_t k = 0; k < carve::rawsigns::kHeaderSignCount; ++k)
                if (carve::rawsigns::kHeaderSigns[k].fmt == fmt &&
                    carve::rawsigns::kHeaderSigns[k].conf > best)
                    best = carve::rawsigns::kHeaderSigns[k].conf;
            conf = best;
        }
        wchar_t magic_hex[16 * 2 + 1] = L"";
        if (magic_len > 0) {
            for (uint32_t k = 0; k < magic_len && k < 16; ++k)
                swprintf(magic_hex + k * 2, 3, L"%02x", magic[k]);
        }
        wprintf(L"  [%2u] %-7s %-8hs min %10llu B  max %-10llu  conf %4u  %s%s\n", i,
                layer == 0 ? L"grammar" : L"rawsign", ext,
                static_cast<unsigned long long>(min_size),
                static_cast<unsigned long long>(max_size), conf, magic_len ? L"magic " : L"",
                magic_len ? magic_hex : L"-");
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

/* N-009c: ASCII-case-insensitive UTF-8 substring match - the shared
 * filter contract (query::Filter::name_substring is "UTF-8, ASCII-
 * case-insensitive"; the tree command routes through it, so the scan
 * print and recover paths must agree byte-for-byte with that matcher). */
bool name_contains_ci(const std::string& name, const std::string& needle) {
    if (needle.empty())
        return true;
    if (name.size() < needle.size())
        return false;
    for (size_t i = 0; i + needle.size() <= name.size(); ++i) {
        size_t k = 0;
        for (; k < needle.size(); ++k) {
            char a = name[i + k], b = needle[k];
            if (a >= 'A' && a <= 'Z')
                a += 32;
            if (b >= 'A' && b <= 'Z')
                b += 32;
            if (a != b)
                break;
        }
        if (k == needle.size())
            return true;
    }
    return false;
}

bool name_matches(const tree::FileNode* n, const TreePrintFlags& f) {
    if (f.filter.empty())
        return true;
    return name_contains_ci(n->name_utf8(), f.filter);
}

/* S-053 display-layer placeholder for name-destroyed rows (ReFS unnamed
 * $DATA-only records): the stored name is EMPTY - the og's engine emits
 * them nameless and its sessions persist empty - so the human-readable
 * tree print gives each one a stable, per-print numbered placeholder.
 * Data exports (csv/json) and the session never see this string. */
std::string unnamed_display_name(u32 counter) {
    char buf[16];
    snprintf(buf, sizeof(buf), "File%06u", counter);
    return buf;
}

void print_tree(tree::ResultTree& tree, const TreePrintFlags& f) {
    /* S-053: name-destroyed ReFS rows carry an EMPTY stored name (the og's
     * own engine emits them nameless; the session persists empty). The
     * human-readable print gives them a stable display placeholder so the
     * row is visible and addressable - DISPLAY LAYER ONLY: the csv/json
     * exports and the session keep the true empty name (never a synthetic
     * one baked into data). */
    u32 unnamed_display_counter = 0;
    std::function<void(tree::FileNode*, const std::string&, int)> walk =
        [&](tree::FileNode* n, const std::string& path, int depth) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                if (!c->props())
                    continue;
                if ((c->props()->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0)
                    continue; /* M-018/M-071: the og's FilterFolder hidden
                               * set never prints (the gate the N-064
                               * reconstruction dropped) */
                const std::string name = c->name_utf8();
                const std::string shown =
                    name.empty() ? unnamed_display_name(++unnamed_display_counter) : name;
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
                    wprintf(L"%hs (%llu)%hs%hs\n", shown.c_str(),
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
    return _fseeki64(static_cast<FILE*>(user), static_cast<long long>(absolute), SEEK_SET) == 0
               ? OK
               : ERR_SESSION_IO;
}
u32 file_read_cb(void* user, void* buf, u32 len) {
    FILE* f = static_cast<FILE*>(user);
    return fread(buf, 1, len, f) == len ? OK : ERR_SESSION_IO;
}
u64 file_size_cb(void* user) {
    FILE* f = static_cast<FILE*>(user);
    const long long cur = _ftelli64(f);
    _fseeki64(f, 0, SEEK_END);
    const long long end = _ftelli64(f);
    _fseeki64(f, cur, SEEK_SET);
    return u64(end);
}

/* ---------------------------------------------------------------------------
 * sessions (W1a / R-56: the og's LastScanRecordView surface, corpus
 * UIUX/07-home-phase-deep-dive.md:21 + rsf-format--scan-session.md 5.2):
 * list every *.crsf in a directory with the per-file light summary -
 * SESSION_STATE + VOLUME_INFO only; the heavy tree record is
 * header-skipped (ScanController::read_summary), so a listing costs
 * milliseconds even against 100 MB sessions. Unreadable files are
 * per-row data (never a failure); only enumeration failure exits 1.
 * ------------------------------------------------------------------------- */

int print_sessions(int argc, wchar_t** argv, int& i) {
    std::wstring dir = L"."; /* default: the current directory */
    std::wstring v;
    bool json = false;
    while (i < argc) {
        if (get_opt(argc, argv, i, L"--dir", &v)) {
            dir = v;
            continue;
        }
        if (wcscmp(argv[i], L"--json") == 0) {
            json = true;
            ++i;
            continue;
        }
        wprintf(L"unknown sessions option: %s\n", argv[i]);
        return 1;
    }
    std::wstring pattern = dir;
    /* accept both "C:\x\y" and "C:\x\y\" - exactly one separator */
    if (!pattern.empty() && pattern.back() == L'\\')
        pattern += L"*.crsf";
    else
        pattern += L"\\*.crsf";
    std::vector<std::wstring> files;
    WIN32_FIND_DATAW fd;
    HANDLE h =
        FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0);
    const std::wstring sep = (!dir.empty() && dir.back() == L'\\') ? L"" : L"\\";
    if (h == INVALID_HANDLE_VALUE) {
        /* the empty-vs-missing discriminator (the list_crsf_files seam,
         * runs 40694592245 + 40700058960): the FindFirstFileExW error
         * code is NOT the reliable axis on real windows (the observed
         * matrix inverted from the docs), so the DIRECTORY-EXISTS probe
         * decides - an existing directory with zero matches prints the
         * zero-row list and exits 0 (the ABI surface's own contract),
         * a missing path (or a plain FILE handed as a dir) stays the
         * genuine enumeration failure: FB-K01, exit 1. */
        const DWORD attr = GetFileAttributesW(dir.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            wprintf(L"cannot enumerate session directory %s\n", dir.c_str());
            return 1; /* FB-K01: enumeration failure is never exit-0 */
        }
    } else {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            /* the 8.3 short-name quirk: the pattern can match on the SHORT
             * name - verify the LONG name's tail too (case-insensitive) */
            const std::wstring name(fd.cFileName);
            if (name.size() < 5)
                continue;
            const wchar_t* t = name.c_str() + name.size() - 5;
            if (t[0] != L'.' || towlower(t[1]) != L'c' || towlower(t[2]) != L'r' ||
                towlower(t[3]) != L's' || towlower(t[4]) != L'f')
                continue;
            files.push_back(dir + sep + name);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(files.begin(), files.end()); /* the deterministic enumeration INPUT (the ROW order is
                                              the ABI's time key below) */

    struct SRow {
        std::wstring path;
        bool opened = false;
        u32 rc = 0;
        session::SessionSummary s;
    };
    std::vector<SRow> rows;
    rows.reserve(files.size());
    for (const std::wstring& f : files) {
        SRow row;
        row.path = f;
        FILE* fin = nullptr;
        if (_wfopen_s(&fin, f.c_str(), L"rb") == 0 && fin) {
            row.opened = true;
            session::Reader r(file_read_cb, file_seek_cb, file_size_cb, fin);
            row.rc = orchestrator::ScanController::read_summary(r, row.s);
            fclose(fin);
        } else {
            row.rc = ERR_OPEN_FAILED;
        }
        rows.push_back(std::move(row));
    }
    /* N-116: rows re-sorted by the ABI's time key (save_unix DESC, path
     * tie-break) - the og's LastScanRecordView order, so the verb's first
     * row is the most recent session exactly like cr_ui_session_list's */
    std::sort(rows.begin(), rows.end(), [](const SRow& a, const SRow& b) {
        const u64 ta = (a.opened && a.rc == OK) ? a.s.volume.save_unix : 0;
        const u64 tb = (b.opened && b.rc == OK) ? b.s.volume.save_unix : 0;
        if (ta != tb)
            return ta > tb;
        return a.path < b.path;
    });

    if (json) {
        /* FROZEN schema (GAP-TREE-JSON lineage): path/label UTF-8,
         * save_unix = the embedded save time (0 = pre-v5 session). */
        std::string doc = "[";
        bool first = true;
        for (const SRow& row : rows) {
            if (!first)
                doc += ',';
            first = false;
            doc += "{\"path\":\"";
            doc += recovery::report::escape_json(text::wide_to_utf8(row.path));
            doc += "\",\"valid\":";
            doc += std::to_string(row.opened && row.rc == OK ? 1 : 0);
            doc += ",\"error\":";
            doc += std::to_string(row.rc);
            doc += ",\"fs_family\":";
            doc += std::to_string(row.s.have_volume ? row.s.volume.fs_kind : 0u);
            doc += ",\"stage_done_mask\":";
            doc += std::to_string(row.s.state.stage_done_mask);
            doc += ",\"configured_mask\":";
            doc += std::to_string(row.s.configured_mask);
            doc += ",\"files_found\":";
            doc += std::to_string(row.s.state.files_found);
            doc += ",\"deleted_found\":";
            doc += std::to_string(row.s.state.deleted_found);
            doc += ",\"carved_found\":";
            doc += std::to_string(row.s.state.carved_found);
            doc += ",\"bytes_scanned\":";
            doc += std::to_string(row.s.state.bytes_scanned);
            doc += ",\"elapsed_ms\":";
            doc += std::to_string(row.s.volume.elapsed_ms);
            doc += ",\"save_unix\":";
            doc += std::to_string(row.s.volume.save_unix);
            doc += ",\"label\":\"";
            doc += recovery::report::escape_json(row.s.volume.label);
            doc += "\"}";
        }
        doc += "]";
        wprintf(L"%hs\n", doc.c_str());
        return 0;
    }

    wprintf(L"%zu session(s) in %s\n", rows.size(), dir.c_str());
    for (const SRow& row : rows) {
        const wchar_t* base = row.path.c_str();
        if (const wchar_t* slash = wcsrchr(base, L'\\'))
            base = slash + 1;
        if (!row.opened || row.rc != OK) {
            wprintf(L"  %s  [unreadable: %hs (0x%08x)]\n", base, err_text(row.rc), row.rc);
            continue;
        }
        wchar_t stages[32] = L"(none recorded)";
        {
            const u32 m = row.s.state.stage_done_mask;
            if (m != 0) {
                wchar_t* p = stages;
                *p = 0;
                if (m & 1u)
                    p += swprintf(p, 4, L"quick");
                if (m & 2u)
                    p += swprintf(p, 4, p != stages ? L"+deep" : L"deep");
                if (m & 4u)
                    p += swprintf(p, 6, p != stages ? L"+carve" : L"carve");
            }
        }
        wchar_t when[32] = L"(save time not recorded)";
        if (row.s.volume.save_unix != 0) {
            /* the platform seam pattern (cr_query.cpp: localtime_s on
             * MSVC, localtime_r elsewhere - rrecov is Windows-only at
             * link time but the parse gate compiles this file on gcc) */
            std::tm lt{};
            std::time_t tt = std::time_t(row.s.volume.save_unix);
#ifdef _WIN32
            localtime_s(&lt, &tt);
#else
            localtime_r(&tt, &lt);
#endif
            swprintf(when, 32, L"%04d-%02d-%02d %02d:%02d:%02d", lt.tm_year + 1900, lt.tm_mon + 1,
                     lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
        }
        wprintf(L"  %s  %hs  %s  %llu file(s) (%llu deleted, %llu carved)  %s\n", base,
                row.s.volume.label, stages, row.s.state.files_found, row.s.state.deleted_found,
                row.s.state.carved_found, when);
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * scan
 * ------------------------------------------------------------------------- */

int cmd_scan(int argc, wchar_t** argv, int& i) {
    /* Driver precondition FIRST (docs/DRIVER-REQUIRED.md): the disk/volume
     * data path is driver-only - the refusal (exit 4, stderr) is the whole
     * response. Image targets are classified (b) and skip the gate. */
    if (!argv_has_image_target(argc, argv, i)) {
        const int drc = require_driver();
        if (drc != 0)
            return drc;
    }
    /* M-033 pre-pass: --bitlocker-fvek may appear anywhere on the line
     * (the target --volume is opened in the main loop below). N-064 adds
     * --bitlocker-dict the same way: the explicit consent flag, with an
     * OPTIONAL value - bare `--bitlocker-dict` tries the shipped
     * dictionary; `--bitlocker-dict <file>` additionally loads user
     * candidates from an og-format dictionary file (the absorbed
     * convergent-landing capability). */
    std::wstring fvek_hex;
    bool dict_consent = false;
    std::wstring dict_path;
    for (int k = i; k < argc; ++k) {
        if (wcscmp(argv[k], L"--bitlocker-fvek") == 0) {
            if (k + 1 < argc)
                fvek_hex = argv[k + 1];
            break;
        }
    }
    for (int k = i; k < argc; ++k) {
        if (wcscmp(argv[k], L"--bitlocker-dict") == 0) {
            dict_consent = true;
            /* optional value: the next argument when it does not look
             * like another flag (the valueless consent form) */
            if (k + 1 < argc && argv[k + 1][0] != L'-')
                dict_path = argv[k + 1];
            break;
        }
    }
    /* M-048: --user-signs <file> - the EUUserData-shaped signature
     * database, loaded and registered before the scan (the og re-reads
     * its install-tree file per scan; the flag is the same posture).
     * Honest failure: an unreadable or invalid file aborts the scan -
     * never a silent no-signatures run the user did not ask for. */
    {
        std::wstring user_signs_path;
        for (int k = i; k + 1 < argc; ++k) {
            if (wcscmp(argv[k], L"--user-signs") == 0 && argv[k + 1][0] != L'-') {
                user_signs_path = argv[k + 1];
                break;
            }
        }
        /* M-065: the shipped default set - the og's install-tree
         * EUUserData posture (111 entries; the release zip ships the
         * same file as data/userdata-defaults.json). Precedence: an
         * explicit --user-signs file replaces the default entirely
         * (install_set's replace semantics); with no flag, the
         * executable-adjacent default loads when present; absent
         * default + absent flag = the empty registry + one log line
         * (the og's degraded install posture, never an error). */
        bool default_loaded = false;
        if (user_signs_path.empty()) {
            wchar_t exe[MAX_PATH] = {};
            if (GetModuleFileNameW(nullptr, exe, MAX_PATH) != 0) {
                std::wstring dir(exe);
                const size_t slash = dir.find_last_of(L"\\/");
                if (slash != std::wstring::npos)
                    dir.resize(slash + 1);
                std::wstring def = dir + L"userdata-defaults.json";
                FILE* f = nullptr;
                if (_wfopen_s(&f, def.c_str(), L"rb") == 0 && f != nullptr) {
                    std::vector<char> json;
                    char buf[4096];
                    size_t got = 0;
                    while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
                        json.insert(json.end(), buf, buf + got);
                    fclose(f);
                    carve::usersigns::UserSignSet set;
                    u32 rc = carve::usersigns::parse(json.data(), size_t(json.size()), set);
                    if (rc == 0)
                        rc = carve::usersigns::install_set(std::move(set));
                    if (rc == 0) {
                        default_loaded = true;
                        wprintf(L"[user-signs] %u default user signature(s) loaded\n",
                                carve::usersigns::active_count());
                    } else {
                        /* the shipped default is corrupt: loud, but the
                         * scan proceeds empty (the og treats a bad
                         * config file as absent - degrade, never abort) */
                        wprintf(L"[user-signs] userdata-defaults.json is not a valid "
                                L"signature database; proceeding with none\n");
                    }
                }
            }
            if (!default_loaded)
                wprintf(L"[user-signs] no default signature database; proceeding "
                        L"with none\n");
        }
        if (!user_signs_path.empty()) {
            FILE* f = nullptr;
            if (_wfopen_s(&f, user_signs_path.c_str(), L"rb") != 0 || f == nullptr) {
                wprintf(L"[user-signs] cannot open %ls\n", user_signs_path.c_str());
                return 1;
            }
            std::vector<char> json;
            char buf[4096];
            size_t got = 0;
            while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
                json.insert(json.end(), buf, buf + got);
            fclose(f);
            /* the same install the cr_ui ABI performs (cr_ui_register_user_
             * signs wraps parse + install_set): through the core directly so
             * the registration lands in THIS process's engine registry -
             * the copy the scan below actually reads */
            carve::usersigns::UserSignSet set;
            u32 rc = carve::usersigns::parse(json.data(), size_t(json.size()), set);
            if (rc == 0)
                rc = carve::usersigns::install_set(std::move(set));
            if (rc != 0) {
                wprintf(L"[user-signs] %ls is not a valid signature database "
                        L"(the og's EUUserData JSON shape)\n",
                        user_signs_path.c_str());
                return 1;
            }
            wprintf(L"[user-signs] %u user signature(s) registered\n",
                    carve::usersigns::active_count());
        }
    }
    std::vector<std::string> dict_user;
    if (dict_consent && !dict_path.empty() && !load_dict_candidates(dict_path, dict_user))
        return 1; /* honest failure: the user named a dictionary we cannot use */
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
            if (!open_volume_view(v[0], target, fvek_hex, dict_consent, dict_user))
                return 1;
            target.volume_letter = v[0]; /* M-079: the label source */
            have_target = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--bitlocker-fvek", &v)) {
            /* consumed by the pre-pass; not a positional */
            continue;
        }
        if (wcscmp(argv[i], L"--bitlocker-dict") == 0) {
            /* consumed by the pre-pass (value, if any, too); not a positional */
            ++i;
            if (i < argc && argv[i][0] != L'-')
                ++i;
            continue;
        }
        if (wcscmp(argv[i], L"--user-signs") == 0) {
            /* M-048: consumed by the pre-pass (with its value) */
            ++i;
            if (i < argc && argv[i][0] != L'-')
                ++i;
            continue;
        }
        if (get_opt(argc, argv, i, L"--image", &v)) {
            if (v.empty())
                return 1;
            if (!open_image_view(v, target))
                return 1;
            have_target = true;
            continue;
        }
        if (has_flag(argc, argv, i, L"--whole-disk")) {
            /* N-044: the honest name. The og's "Deep Scan" toggle means the
             * complete-searcher engine pass - which this CLI ALWAYS runs;
             * this flag instead enables whole-disk lost-partition discovery.
             * The deep stage is always on; --quick-only disables it.
             * S-051: discovery is the ADDITIONAL flow - a whole-disk
             * target ALWAYS scans the partition table's volumes first
             * (the og's CDoScanPartition partition-list read); this flag
             * only adds the lost-partition candidate sweep on top. */
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
        if (get_opt(argc, argv, i, L"--autosave-dir", &v)) {
            /* W1c (R-52): the completion auto-save directory (the og's
             * auto_<vol>_<ts> SaveScan flow, rsf-format section 5.5).
             * Set on the CORE config (the shared host trigger) and honored
             * below on run() completion only - never a cancelled run. */
            const std::string d8 = text::wide_to_utf8(v);
            for (size_t k = 0; k + 1 < sizeof(cfg.autosave_dir) && k < d8.size(); ++k)
                cfg.autosave_dir[k] = d8[k];
            continue;
        }
        if (get_opt(argc, argv, i, L"--label", &v)) {
            /* M-079: explicit label override (the ABI card's
             * volume_label_utf8 seam, CLI-side). The derived label only
             * fills an EMPTY field, so caller text always wins. */
            const std::string l8 = text::wide_to_utf8(v);
            for (size_t k = 0; k + 1 < sizeof(cfg.volume_label) && k < l8.size(); ++k)
                cfg.volume_label[k] = l8[k];
            continue;
        }
        if (get_opt(argc, argv, i, L"--resume", &resume_path))
            continue;
        if (get_opt(argc, argv, i, L"--log", &log_path))
            continue;
        if (get_opt(argc, argv, i, L"--label", &v)) {
            /* M-079: the explicit volume-label override (the auto-save
             * name's <vol> half + the session record); empty = derive from
             * the target (volume letter / image stem) */
            size_t k = 0;
            for (; k + 1 < sizeof(cfg.volume_label) && k < v.size() && v[k] < 0x80; ++k)
                cfg.volume_label[k] = char(v[k]);
            cfg.volume_label[k] = 0;
            continue;
        }
        if (get_opt(argc, argv, i, L"--filter", &v)) {
            print_flags.filter = text::wide_to_utf8(v); /* N-009c: UTF-8, not '?' */
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
        if (has_flag(argc, argv, i, L"--check")) {
            /* M-041: the read-only damage-audit pre-scan pass (og
             * FileSystemCheck parity, 0x1001..0x100e taxonomy; never
             * aborts the scan - the report informs, the scan proceeds) */
            cfg.pre_scan_check = true;
            continue;
        }
        wprintf(L"unknown scan option: %s\n", argv[i]);
        return 1;
    }
    if (!have_target) {
        wprintf(L"scan needs --disk N, --volume C:, or --image file%s\n",
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
    fill_volume_label(target, cfg);       /* M-079: the <vol> half of the
                                           * auto-save name + the session
                                           * record (derive unless --label) */
    if (target.is_image) {
        /* S-030: the image identity - format as model, path into the v3
         * tail (the resume path warns when the path changes) */
        const storage::ImageFormat fmt = storage::classify_extension(
            std::u16string(target.image_path.begin(), target.image_path.end()));
        const char* model = fmt == storage::ImageFormat::Vmdk   ? "Disk image (VMDK)"
                            : fmt == storage::ImageFormat::Vhd  ? "Disk image (VHD)"
                            : fmt == storage::ImageFormat::Vhdx ? "Disk image (VHDX)"
                                                                : "Disk image (raw)";
        for (size_t k = 0; k < sizeof(cfg.volume_model) - 1 && model[k]; ++k)
            cfg.volume_model[k] = model[k];
        std::string narrow;
        for (wchar_t c : target.image_path)
            if (c < 0x80)
                narrow.push_back(char(c));
        for (size_t k = 0; k < sizeof(cfg.volume_image) - 1 && k < narrow.size(); ++k)
            cfg.volume_image[k] = narrow[k];
    }

    /* S-008 pre-scan repair hooks: --repair analyzes + reports; the
     * --repair-apply FLAG is the consent (non-interactive CLI posture);
     * the RW handle opens lazily at consent time (never grabbed for a
     * dry-run). Volume scans map writes through the volume slice. */
    RepairCliCtx repair_ctx;
    if (repair) {
        cfg.pre_scan_repair = true;
        repair_ctx.apply = repair_apply;
        if (target.is_image) {
            /* S-030: repairs target PHYSICAL disks (the write path opens a
             * disk RW handle). On an image scan the analysis still runs
             * and surfaces its plan; the null writer = "repair unavailable
             * this scan" (the orchestrator's documented contract), so
             * nothing is ever written into the evidence image. */
            repair_ctx.disk = 0xFFFFFFFF;
            cfg.repair.consent = scan_repair_consent;
            cfg.repair.user = &repair_ctx;
        } else {
            repair_ctx.disk = target.disk; /* the parent basis for the undo log */
            cfg.repair.consent = scan_repair_consent;
            cfg.repair.open_writer = scan_repair_open_writer;
            cfg.repair.close_writer = scan_repair_close_writer;
            cfg.repair.user = &repair_ctx;
        }
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
                    /* driver-only (PLAN-DRIVER-ONLY R7): the direct retry is
                     * gone - a member that will not open through RDA simply
                     * does not resolve (non-LDM disks never did) */
                    auto dev = win::open_disk(pd.number, L"RDA", &oerr);
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
    /* OQ-S4-3's TTFR probe: consume the typed event stream for the
     * first-result timestamp - set BEFORE the controller construction
     * (the controller copies the config); the once-flag lives in scan's
     * frame so the probe outlives run(); events fire from worker threads */
    TtfrProbe ttfr_probe;
    cfg.event_fn = &ttfr_event_fn;
    cfg.event_user = &ttfr_probe;
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
        /* M-042: the current target disk is the re-validation arbiter -
         * stale candidates the disk no longer confirms are dropped
         * at load (the og's PRD:20748-20842 search-start semantics). */
        const u32 rrc = orchestrator::ScanController::resume(r, tree, cfg, stats, info, ctrl,
                                                             target.device.get());
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
        /* S-030: the session's image-path identity (v3 tail; empty on
         * older sessions = skip). Compare with the CURRENT --image. */
        if (info.image[0] != '\0') {
            std::string cur;
            for (wchar_t c : target.image_path)
                if (c < 0x80)
                    cur.push_back(char(c));
            if (!target.is_image || cur != info.image)
                wprintf(L"[resume] WARNING: this session was saved over image %hs - "
                        L"re-run scan with --image of that file for exact results\n",
                        info.image);
        } else if (target.is_image) {
            wprintf(L"[resume] note: resuming a non-image session over an image target "
                    L"- results may differ from the original run\n");
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
    /* M-042: the og's search-start disk-truth signals (surfaced, never
     * enforced - the drop of stale records already happened at load/run) */
    if (stats.layout_changed)
        wprintf(L"[resume] WARNING: the disk layout changed since the session was saved\n");
    if (stats.dropped_stale_candidates > 0)
        wprintf(L"[resume] %llu stale lost-partition candidate(s) dropped "
                L"(the disk no longer confirms them)\n",
                static_cast<unsigned long long>(stats.dropped_stale_candidates));

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

    /* M-041 damage-audit summary: the full per-finding report (the og's
     * 0x1001..0x100e taxonomy, cluster/LCN/version as param, path hints
     * where the og logs "Name: %s\%s"). The scan already ran regardless -
     * the audit informs, never aborts. */
    if (cfg.pre_scan_check && stats.check_ran) {
        wprintf(L"[check] family=%hs status=%u issues=%zu%s\n", stats.check_report.family,
                stats.check_status, stats.check_report.errors.size(),
                stats.check_detected ? L"" : L" (clean)");
        for (const auto& is : stats.check_report.errors) {
            if (is.path_hint.empty())
                wprintf(L"[check]   0x%04x param=%llu\n", is.code,
                        static_cast<unsigned long long>(is.param));
            else
                wprintf(L"[check]   0x%04x param=%llu  %hs\n", is.code,
                        static_cast<unsigned long long>(is.param), is.path_hint.c_str());
        }
    }
    log_close();

    /* save helper: full truthful session from the (now quiesced) tree.
     * N-060: atomic temp-then-rename (the N-014 checkpoint discipline) -
     * a torn save (cancel/failure/crash between truncate and finalize)
     * never destroys the previous good session file. */
    auto save_session_to = [&](const std::wstring& path) -> bool {
        const std::wstring tmp = path + L".tmp";
        FILE* f = nullptr;
        if (_wfopen_s(&f, tmp.c_str(), L"wb") != 0 || !f) {
            wprintf(L"cannot create session file %s\n", tmp.c_str());
            return false;
        }
        session::Writer w(file_write_cb, file_tell_cb, file_seek_cb, f);
        w.set_flags(session::CRSF_FLAG_ABS_EXTENTS); /* F-1: absolute basis */
        bool ok =
            w.open(0x52434F56) == OK && ctrl->save_session(w, stats) == OK && w.finalize() == OK;
        fclose(f);
        if (ok) {
            std::error_code ec;
            std::filesystem::rename(tmp, path, ec);
            ok = !ec;
        }
        if (!ok) {
            /* the staging file is a torn never-committed save - remove it,
             * never trust it (the final path keeps the previous good one) */
            std::error_code rm_ec;
            std::filesystem::remove(tmp, rm_ec);
            wprintf(L"session save failed\n");
            return false;
        }
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
        /* read-truth refusal (the fake-scan fix): the volume was (essentially)
         * fully unreadable - the scan did NOT scan. The specific text names
         * the coverage; the generic branch keeps the code. */
        if (rc == ERR_READ_FAILED && stats.bytes_unreadable > 0) {
            wprintf(L"scan REFUSED: %llu of %llu byte(s) on this target were unreadable "
                    L"(%u error range(s)) - nothing could actually be read, so nothing was "
                    L"scanned\n",
                    static_cast<unsigned long long>(stats.bytes_unreadable),
                    static_cast<unsigned long long>(
                        target.volume_len != 0 ? target.volume_len
                                               : (target.device ? target.device->size() : 0)),
                    u32(stats.errors.ranges().size()));
            wprintf(L"check the transport before retrying: `rrecov driver` reports the RDA "
                    L"driver prerequisite; a green driver + these errors means the media "
                    L"itself (cabling, health, encryption) is refusing reads\n");
        } else {
            wprintf(L"scan failed: %hs (0x%08x)\n", cr::err_text(rc), rc);
        }
        return 1;
    }
    /* read-truth summary: ALWAYS visible on completion (the error map used
     * to live only in the log - a scan with unreadable regions looked
     * perfectly healthy). The zero-fill salvage semantics are unchanged;
     * the truth is no longer optional output. */
    if (stats.probe_unreadable)
        wprintf(L"[scan] volume header unreadable - filesystem family unknown because "
                L"sector 0 could not be read (raw/deep recovery mode)\n");
    if (!stats.errors.ranges().empty())
        wprintf(L"[scan] read errors: %u range(s), %.1f MiB unreadable - those regions were "
                L"NOT scanned; recovered content may be incomplete\n",
                u32(stats.errors.ranges().size()),
                double(stats.bytes_unreadable) / (1024.0 * 1024.0));
    prog.done();
    if (json) {
        wprintf(L"{\"files\":%llu,\"deleted\":%llu,\"carved\":%llu,\"lost_partitions\":%llu,"
                L"\"bytes_scanned\":%llu,\"bytes_unreadable\":%llu,\"error_ranges\":%u,"
                L"\"probe_unreadable\":%hs,\"elapsed_ms\":%llu}\n",
                stats.files_found, stats.deleted_found, stats.carved_found, stats.lost_partitions,
                stats.bytes_scanned, stats.bytes_unreadable, u32(stats.errors.ranges().size()),
                stats.probe_unreadable ? "true" : "false", stats.elapsed_ms);
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
    /* W1c (R-52): the completion auto-save (the og's SaveScan window;
     * cancelled runs return before this point, so ONLY completed runs
     * auto-save). The N-061 corpus naming + sanitizer; silent write,
     * one printed outcome line - never silent failure.
     * S-044: the og's trigger-site gates (FUN_1402857c0's call sites,
     * evidence s8-autosave-and-stamps.md): results-found (an empty
     * scan never auto-saves) and not-already-saved (--out/--resume is
     * the explicit save - effective_out non-empty means the session was
     * saved, which also covers the not-import case for resumed runs). */
    if (cfg.autosave_dir[0] != '\0' && effective_out.empty() &&
        (stats.files_found != 0 || stats.lost_partitions != 0)) {
        const std::string name8 =
            session::auto_session_name(text::utf8_to_u16(cfg.volume_label)) + ".crsf";
        std::wstring dir;
        for (char c : std::string(cfg.autosave_dir))
            dir.push_back((c & 0x80) ? L'?' : wchar_t(c)); /* ASCII dirs; the
                                                            * corpus SaveScan folder is ASCII */
        if (!dir.empty() && dir.back() != L'\\' && dir.back() != L'/')
            dir += L'\\';
        std::wstring wide_name = dir;
        for (char c : name8)
            wide_name.push_back((c & 0x80) ? L'?' : wchar_t(c)); /* ASCII names
                                                                  * (the sanitizer guarantees it) */
        if (save_session_to(wide_name))
            wprintf(L"session auto-saved: %s\n", wide_name.c_str());
        else
            wprintf(L"[autosave] FAILED: %s\n", wide_name.c_str());
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
            f.filter = text::wide_to_utf8(v); /* N-009c: UTF-8, not '?' */
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
    /* Driver precondition FIRST (docs/DRIVER-REQUIRED.md): recovering a
     * --disk target re-opens the data path - driver-only; --image targets
     * are classified (b) and skip the gate. */
    if (!argv_has_image_target(argc, argv, i)) {
        const int drc = require_driver();
        if (drc != 0)
            return drc;
    }
    std::wstring in_path, to_dir, filter, report_path, image_path;
    bool deleted_only = false;
    u32 disk = 0;
    bool have_disk = false, have_image = false;
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
            if (v.empty())
                return 1;
            image_path = v;
            have_image = true;
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
    if (in_path.empty() || to_dir.empty() || (!have_disk && !have_image)) {
        wprintf(L"recover needs --in file.crsf (--disk N | --image file) --to DIR\n");
        return 1;
    }
    /* N-009c: encode the filter ONCE as UTF-8 (the old per-item narrow()
     * mapped every non-ASCII unit to '?', so a CJK/accented --filter never
     * matched); matching is the shared ASCII-case-insensitive contract. */
    const std::string filter8 = text::wide_to_utf8(filter);

    /* load the session tree */
    FILE* fin = nullptr;
    if (_wfopen_s(&fin, in_path.c_str(), L"rb") != 0 || !fin) {
        wprintf(L"cannot open session %s\n", in_path.c_str());
        return 1;
    }
    session::Reader r(file_read_cb, file_seek_cb, file_size_cb, fin);
    tree::ResultTree tree;
    orchestrator::ScanStats stats;
    /* M-042: open the SOURCE (extents in sessions are absolute) BEFORE the
     * load so the restored lost candidates are re-validated against the
     * current disk (the og's search-start drop of stale records); the
     * recover content reads use the same disk the session is now bound to.
     * The disk the session ran on, or the image file it ran over (--image). */
    OpenResult src;
    if (have_image) {
        if (!open_image_view(image_path, src))
            return 1;
    } else if (!open_disk_view(disk, src)) {
        return 1;
    }
    u32 rc = orchestrator::ScanController::load_session(r, tree, stats, src.device.get());
    fclose(fin);
    if (rc != OK) {
        wprintf(L"session load failed: %hs\n", err_text(rc));
        return 1;
    }

    /* destination validation + overlap refusal (F12): the destination
     * volume extent must not intersect the source disk */
    if (to_dir.size() >= 2 && to_dir[1] == L':') {
        u32 ddisk = 0;
        u64 dstart = 0, dlen = 0;
        if (win::volume_extent(to_dir[0], &ddisk, &dstart, &dlen)) {
            if (have_image)
                wprintf(L"[dest] disk %u @%llu +%llu (source is an image file - no overlap)\n",
                        ddisk, static_cast<unsigned long long>(dstart),
                        static_cast<unsigned long long>(dlen));
            else if (ddisk == disk)
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
                if ((c->props()->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0)
                    continue; /* M-018/M-071: recover-all never writes the
                               * og's hidden junk set (the gate the N-064
                               * reconstruction dropped) */
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
                if (!filter8.empty() && !name_contains_ci(c->name_utf8(), filter8))
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
    /* Driver precondition FIRST (docs/DRIVER-REQUIRED.md): bench measures
     * the disk data path - driver-only. The --no-rda flag is REMOVED
     * (PLAN-DRIVER-ONLY R10): there is no driverless measurement mode. */
    {
        const int drc = require_driver();
        if (drc != 0)
            return drc;
    }
    std::wstring v;
    u32 disk = 0;
    bool have_disk = false;
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

    /* driver-only transport (PLAN-DRIVER-ONLY R10): the grammar path is
     * the only leg; the label is the truth, not a choice */
    u32 err = 0;
    auto dev = win::open_disk(disk, L"RDA", &err);
    if (!dev) {
        wprintf(L"cannot open disk %u (rc=%08x): %hs\n", disk, err,
                cr::win::is_elevated() ? "access denied even elevated (device busy or locked)"
                                       : "administrator rights required (elevate and retry)");
        return 2;
    }
    const Geometry& g = dev->geometry();
    const char* transport = "rda";

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
            auto d2 = win::open_disk(disk, L"RDA", &e2);
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

/* ---- S-029 lost-partition apply --------------------------------------
 * The og's two-step product flow (discover, then APPLY the recovered
 * entry into the user's MBR - RecoveryModule CreateRecoveryPartitionObject
 * -> PartitionRecovery FUN_1800566d0 "rebuild disk layout"). The CLI
 * posture mirrors fix: default is a DRY-RUN that prints the plan;
 * --apply is the explicit consent; the undo log (the shared CRUNDO01
 * container) is saved beside the session for 'fix --undo-apply'.
 * Candidates come from a whole-disk scan's saved session (--whole-disk
 * discovery persists them; 'tree --in' lists them). */
struct ApplyCliCtx {
    bool apply = false;
};
bool apply_cli_consent(const apply::ApplyPlan& plan, void* user) {
    auto* c = static_cast<ApplyCliCtx*>(user);
    if (!c->apply) {
        wprintf(L"[apply] consent NOT given: rerun with --apply to write the entry\n");
        return false;
    }
    wprintf(L"[apply] consent given (--apply): writing the entry now (volumes on the disk "
            L"are locked and dismounted for the write)\n");
    (void)plan;
    return true;
}

/* M-040: the bootfix consent (bcdfix::ConsentFn shape) - the operator's
 * --boot-fix + --apply flags ARE the explicit gate; the callback prints
 * the action line and allows. Refusal (no --apply) never reaches the
 * write path at all. */
bool bootfix_cli_consent(void* user) {
    ApplyCliCtx* ctx = static_cast<ApplyCliCtx*>(user);
    wprintf(L"[boot-fix] consent given: rewriting the boot entries now\n");
    return ctx->apply != 0;
}

int cmd_apply(int argc, wchar_t** argv, int& i) {
    /* Driver precondition FIRST (docs/DRIVER-REQUIRED.md): the apply path
     * reads and writes the disk - driver-only. */
    {
        const int drc = require_driver();
        if (drc != 0)
            return drc;
    }
    std::wstring v;
    u32 disk = 0;
    bool have_disk = false;
    std::wstring in_path;
    int candidate = -1; /* 1-based, the session's lost-candidate index */
    bool do_apply = false;
    std::wstring undo_path;
    /* M-040: the post-apply boot-chain repair (the og's MainModule
     * apply chain FUN_180075580: apply -> Bcd unlock/newpos). Opt-in:
     * --boot-fix <old-lba> states what the boot configuration (BCD /
     * NVRAM) currently references; --boot-fix-bcd overrides the two
     * standard hive locations. Every write under the bootfix consent
     * gate below. */
    u64 boot_fix_old_lba = 0;
    bool boot_fix = false;
    std::wstring boot_fix_bcd;
    while (i < argc) {
        if (get_opt(argc, argv, i, L"--disk", &v)) {
            disk = u32(wcstoul(v.c_str(), nullptr, 10));
            have_disk = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--session", &v) || get_opt(argc, argv, i, L"--in", &v)) {
            in_path = v;
            continue;
        }
        if (get_opt(argc, argv, i, L"--candidate", &v)) {
            candidate = int(wcstol(v.c_str(), nullptr, 10));
            continue;
        }
        if (get_opt(argc, argv, i, L"--undo", &v)) {
            undo_path = v;
            continue;
        }
        if (wcscmp(argv[i], L"--apply") == 0) {
            do_apply = true;
            ++i;
            continue;
        }
        if (get_opt(argc, argv, i, L"--boot-fix", &v)) {
            boot_fix_old_lba = wcstoull(v.c_str(), nullptr, 10);
            boot_fix = true;
            continue;
        }
        if (get_opt(argc, argv, i, L"--boot-fix-bcd", &v)) {
            boot_fix_bcd = v;
            continue;
        }
        wprintf(L"unknown apply option: %s\n", argv[i]);
        return 1;
    }
    if (!have_disk || in_path.empty()) {
        wprintf(L"apply needs --disk N --session file.crsf [--candidate N] [--apply]\n");
        return 1;
    }

    /* open the target disk FIRST: the candidate listing below is
     * re-validated against it (M-042 - the og re-reads every persisted
     * lostpart record before re-applying, PRD:20748-20842, so the listing
     * shows only what the CURRENT disk still confirms) and the same view
     * serves plan_apply below. */
    u32 open_err = 0;
    auto apply_view = win::open_disk(disk, L"RDA", &open_err);
    if (!apply_view) {
        wprintf(L"cannot open disk %u (rc=%08x)\n", disk, open_err);
        return 2;
    }

    /* load the session's lost candidates (the discovery results the scan
     * scored - apply never invents geometry, it transcribes a candidate) */
    FILE* fin = nullptr;
    if (_wfopen_s(&fin, in_path.c_str(), L"rb") != 0 || fin == nullptr) {
        wprintf(L"cannot open session %s\n", in_path.c_str());
        return 1;
    }
    tree::ResultTree tree;
    orchestrator::ScanStats stats;
    session::VolumeSessionInfo info;
    session::Reader r(file_read_cb, file_seek_cb, file_size_cb, fin);
    u32 rc =
        orchestrator::ScanController::load_session_summary(r, tree, stats, info, apply_view.get());
    if (stats.dropped_stale_candidates > 0) {
        wprintf(L"[apply] %llu stale candidate(s) dropped - the disk no longer "
                L"confirms them (re-scan to rediscover)\n",
                static_cast<unsigned long long>(stats.dropped_stale_candidates));
    }
    fclose(fin);
    if (rc != OK) {
        wprintf(L"session load failed (rc=%08x %hs)\n", rc, err_text(rc));
        return 2;
    }
    if (stats.lost_candidates.empty()) {
        wprintf(L"the session carries no lost-partition candidates "
                L"(run: rrecov scan --disk %u --whole-disk)\n",
                disk);
        return 1;
    }
    wprintf(L"session candidates:\n");
    for (size_t k = 0; k < stats.lost_candidates.size(); ++k) {
        const auto& c = stats.lost_candidates[k];
        wprintf(L"  %zu: LBA %llu, %llu sectors, score %d%s\n", k + 1,
                static_cast<unsigned long long>(c.start_lba),
                static_cast<unsigned long long>(c.sector_count), c.score,
                int(k) == candidate - 1 ? L"  <-- selected" : L"");
    }
    if (candidate < 1 || size_t(candidate) > stats.lost_candidates.size()) {
        wprintf(L"select a candidate with --candidate N (1..%zu)\n", stats.lost_candidates.size());
        return 1;
    }
    const lost::Candidate& c = stats.lost_candidates[size_t(candidate) - 1];

    /* plan (read-only) - the SAME view the listing was re-validated on */
    apply::ApplyPlan plan;
    rc = apply::plan_apply(apply_view.get(), c, plan);
    if (rc != OK) {
        wprintf(L"plan failed: rc=%08x %hs\n", rc, err_text(rc));
        return 2;
    }
    if (plan.refused) {
        wprintf(L"REFUSED: %hs\n", plan.refusal_reason.c_str());
        wprintf(L"no writes performed (dry-run is the default; nothing is ever written "
                L"without --apply)\n");
        return 0;
    }
    wprintf(L"apply plan: entry slot %u, type 0x%02X, LBA %llu, %llu sectors%s\n", plan.entry_index,
            plan.mbr_type, static_cast<unsigned long long>(plan.start_lba),
            static_cast<unsigned long long>(plan.sector_count),
            plan.restores_signature ? L" + 0x55AA signature restore" : L"");
    for (const auto& a : plan.actions)
        wprintf(L"  [apply] offset=%llu len=%zu : %hs\n", static_cast<unsigned long long>(a.offset),
                a.after.size(), a.reason.c_str());
    /* M-040: the boot-chain repair family now exists (bcdfix +
     * nvramfix + the win runtime); the CORE plan never performs it
     * (the og's Fixup runs host-side after the layout write too) - the
     * CLI offers it as the explicit --boot-fix step after a successful
     * apply. */
    if (!plan.boot_repair_available)
        wprintf(L"  [apply] boot-chain repair: separate opt-in step (re-run with "
                L"--boot-fix <old-lba> to rewrite the BCD offsets + NVRAM entry after "
                L"the apply lands)\n");

    if (!do_apply) {
        wprintf(L"dry-run only - re-run with --apply to execute (undo log saved automatically)\n");
        return 0;
    }

    /* execute (consent-gated transactional write) */
    /* the union's apply-capable view: the RW transport carrying the corpus
     * device-control chain (0x7C100/0x7C010/0x7C058/0x70140 + the 3x
     * settle tail) - execute_apply drives it inside the transaction */
    u32 e = 0; /* M-057: was referenced (&e) but never declared - the apply
                * path could not compile on MSVC since it landed (the app
                * layer had no parse gate until this session) */
    auto wdev = win::open_disk_apply_rw(disk, L"RDA", &e);
    if (!wdev) {
        wprintf(L"cannot open disk %u for write (rc=%08x): %hs\n", disk, e,
                cr::win::is_elevated()
                    ? "write access denied even elevated (device busy, locked or protected)"
                    : "administrator rights required (elevate and retry)");
        return 2;
    }
    ApplyCliCtx ctx;
    ctx.apply = do_apply;
    repair::UndoLog log;
    rc = apply::execute_apply(wdev.get(), plan, apply_cli_consent, &ctx, log);
    if (rc != OK) {
        wprintf(L"execute FAILED: rc=%08x %hs (any written bytes were rolled back)\n", rc,
                err_text(rc));
        return 2;
    }
    wprintf(L"apply OK: %zu write(s) + verified; the partition entry is in the table\n",
            log.entries.size());

    /* the OS must re-read the table (the og's IOCTL_DISK_UPDATE_PROPERTIES
     * + settle, PR:46873-46880 - the win layer's refresh helper) */
    if (u32 rrc = win::refresh_disk_properties(disk))
        wprintf(L"[apply] table refresh refused (rc=%08x) - replug/reboot to see the volume\n",
                rrc);
    else
        wprintf(L"[apply] disk layout refreshed - the volume should mount on next rescan\n");

    /* save the undo log (the shared CRUNDO01 container) */
    if (undo_path.empty())
        undo_path = L"rrecov-apply-disk" + std::to_wstring(disk) + L".undo";
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

    /* ---- M-040: the post-apply boot-chain repair (opt-in) ----
     * The og's chain (MainModule FUN_180075580: apply -> Bcd_Init ->
     * Bcd_UnlockBcdFile/Bcd_UnlockBcdNewPos; PR:47290-47302) patches the
     * BCD device elements + the UEFI NVRAM load options after the table
     * write. Consent model: the operator ran --boot-fix with the LBA
     * the boot config currently references; every write still rides the
     * bcdfix consent gate (the shared apply consent - an explicit
     * --apply was required to get here). */
    if (boot_fix) {
        wprintf(L"[boot-fix] running the post-apply boot-chain repair\n");
        size_t applied = 0;
        const u32 sig = plan.mbr_signature;
        const u64 new_lba = plan.start_lba;
        /* the two standard hive locations (Fixup's own CBcd/CEFIFirstBcd
         * pair: the legacy boot partition's \Boot\BCD + the ESP's
         * \EFI\MICROSOFT\BOOT\BCD); --boot-fix-bcd overrides with an
         * explicit path. The C: system-root guess matches the og's
         * GetSystemDirectoryA-based probe (FUN_1800017a0). Plan-only
         * first (no writes), then the consent-gated execute + flush. */
        const wchar_t* paths[2] = {L"C:\\Boot\\BCD", L"C:\\EFI\\MICROSOFT\\BOOT\\BCD"};
        bool any = false;
        for (const wchar_t* bcd_path : paths) {
            if (!boot_fix_bcd.empty()) {
                if (wcscmp(bcd_path, paths[0]) != 0)
                    continue; /* an explicit path replaces both guesses */
                bcd_path = boot_fix_bcd.c_str();
            }
            std::vector<cr::bcdfix::BcdRewrite> bcd_plan;
            u32 brc =
                win::bootfix::bcd_file_plan(bcd_path, sig, boot_fix_old_lba, new_lba, &bcd_plan);
            if (brc != OK || bcd_plan.empty()) {
                if (brc != OK)
                    wprintf(L"[boot-fix] %ls: rc=%08x %hs\n", bcd_path, brc, err_text(brc));
                continue;
            }
            applied = 0;
            brc = win::bootfix::bcd_file_execute(bcd_path, bcd_plan, bootfix_cli_consent, &ctx,
                                                 &applied);
            if (brc == OK && applied > 0) {
                wprintf(L"[boot-fix] %zu device element(s) rewritten in %ls "
                        L"(LBA %llu -> %llu)\n",
                        applied, bcd_path, static_cast<unsigned long long>(boot_fix_old_lba),
                        static_cast<unsigned long long>(new_lba));
                any = true;
            } else if (brc == ERR_REPAIR_REFUSED) {
                wprintf(L"[boot-fix] %ls: consent refused - nothing written\n", bcd_path);
            } else if (brc != OK) {
                wprintf(L"[boot-fix] %ls: rc=%08x %hs\n", bcd_path, brc, err_text(brc));
            }
        }
        /* the NVRAM full-position pass (EFIBoot's FUN_180004b40 update;
         * the new partition number = the MBR slot + 1, the new start =
         * the applied candidate's LBA). ERR_UNSUPPORTED = BIOS system -
         * reported, never fatal (the table write already succeeded; the
         * og's boot-repair failures are advisory too). */
        nvramfix::TargetPartition nt;
        nt.gpt_mode = false;
        nt.old_mbr_signature = sig;
        nt.old_start = boot_fix_old_lba;
        nt.old_size = plan.sector_count;
        nt.new_partition_number = plan.entry_index + 1;
        nt.new_start = plan.start_lba;
        nt.new_size = plan.sector_count;
        nt.new_mbr_signature = sig;
        u32 nrc = OK;
        bool stopped_early = false;
        const auto nplan = win::bootfix::nvram_plan(nt, &nrc, &stopped_early);
        if (nrc == OK && !nplan.empty()) {
            size_t napplied = 0;
            nrc = win::bootfix::nvram_execute(nplan, bootfix_cli_consent, &ctx, &napplied);
            if (nrc == OK && napplied > 0) {
                wprintf(L"[boot-fix] NVRAM: %zu Boot entr%s repositioned to partition %u "
                        L"LBA %llu%s\n",
                        napplied, napplied == 1 ? L"y" : L"ies", nt.new_partition_number,
                        static_cast<unsigned long long>(nt.new_start),
                        stopped_early ? L" (walk stopped at a stale BootOrder id)" : L"");
                any = true;
            } else if (nrc == ERR_REPAIR_REFUSED) {
                wprintf(L"[boot-fix] NVRAM: consent refused - nothing written\n");
            } else if (nrc != OK) {
                wprintf(L"[boot-fix] NVRAM: rc=%08x %hs\n", nrc, err_text(nrc));
            }
        } else if (nrc == ERR_UNSUPPORTED) {
            wprintf(L"[boot-fix] NVRAM: not a UEFI system (skipped)\n");
        } else if (nrc != OK) {
            wprintf(L"[boot-fix] NVRAM: rc=%08x %hs\n", nrc, err_text(nrc));
        }
        if (!any)
            wprintf(L"[boot-fix] nothing needed rewriting (no boot entry referenced the "
                    L"old position)\n");
    }
    return 0;
}

int cmd_fix(int argc, wchar_t** argv, int& i) {
    /* Driver precondition FIRST (docs/DRIVER-REQUIRED.md): the repair path
     * reads and writes the disk - driver-only (--no-rda is REMOVED,
     * PLAN-DRIVER-ONLY R10). */
    {
        const int drc = require_driver();
        if (drc != 0)
            return drc;
    }
    std::wstring v;
    u32 disk = 0;
    bool have_disk = false;
    int part = -1; /* -1 = whole disk (LBA0 view, superfloppy/lost FS) */
    bool apply = false;
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
        auto wdev = win::open_disk_rw(disk, L"RDA", &e);
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
    auto view = win::open_disk(disk, L"RDA", &e);
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
    auto wdev = win::open_disk_rw(disk, L"RDA", &e);
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

/* ---------------------------------------------------------------------------
 * driver (the driver-status surface; docs/DRIVER-REQUIRED.md)
 * ------------------------------------------------------------------------- */

int cmd_driver(int argc, wchar_t** argv, int& i) {
    std::wstring v;
    std::wstring service = win::kDefaultDriverService;
    bool json = false;
    while (i < argc) {
        if (get_opt(argc, argv, i, L"--service", &v)) {
            service = v;
            continue;
        }
        if (wcscmp(argv[i], L"--json") == 0) {
            json = true;
            ++i;
            continue;
        }
        wprintf(L"unknown driver option: %s\n", argv[i]);
        return 1;
    }
    win::DriverStatus st;
    const u32 qrc = win::query_driver_status(service.c_str(), &st);
    if (qrc != OK) {
        wprintf(L"driver status query failed: %hs (0x%08x)\n", err_text(qrc), qrc);
        return 1;
    }
    const win::DriverState ds = win::driver_state(st);
    const u32 rc = win::driver_state_error(ds);
    const auto yn = [](bool b) { return b ? "yes" : "no"; };
    if (json) {
        wprintf(L"{\"service\":\"%s\",\"state\":\"%hs\",\"present\":\"%hs\","
                L"\"loaded\":\"%hs\",\"started\":\"%hs\",\"authorized\":\"%hs\","
                L"\"healthy\":\"%hs\",\"major\":%u,\"minor\":%u,\"required_major\":%u,"
                L"\"flags\":%u,\"max_disks\":%u,\"ioctl_base\":%u,\"last_gle\":%lu,"
                L"\"error\":\"%hs\",\"error_code\":%u}\n",
                service.c_str(), win::driver_state_name(ds), yn(st.service_present),
                yn(st.service_running), yn(st.device_openable), yn(st.authorized),
                yn(st.info_valid), st.major, st.minor, win::kDriverWireMajorRequired, st.flags,
                st.max_disks, st.ioctl_base, st.last_gle, err_text(rc), rc);
        return ds == win::DriverState::Ready ? 0 : 4;
    }
    wprintf(L"driver service \"%s\" (data-path prerequisite, docs/DRIVER-REQUIRED.md):\n",
            service.c_str());
    wprintf(L"  present:     %hs   (service installed)\n", yn(st.service_present));
    wprintf(L"  loaded:      %hs   (service running)\n", yn(st.service_running));
    wprintf(L"  started:     %hs   (control device opens)\n", yn(st.device_openable));
    wprintf(L"  authorized:  %hs   (open not denied by the device DACL)\n", yn(st.authorized));
    wprintf(L"  healthy:     %hs   (GET_INFO + GET_VERSION consistent)\n", yn(st.info_valid));
    wprintf(L"  version:     %u.%u (core requires major %u)\n", st.major, st.minor,
            win::kDriverWireMajorRequired);
    wprintf(L"  capabilities: flags=%u max_disks=%u ioctl_base=0x%x\n", st.flags, st.max_disks,
            st.ioctl_base);
    if (ds == win::DriverState::Ready) {
        wprintf(L"  state:       ready\n");
        return 0;
    }
    wprintf(L"  state:       %hs - %hs (0x%08x, last error %lu)\n", win::driver_state_name(ds),
            err_text(rc), rc, st.last_gle);
    const int drc = require_driver();
    return drc != 0 ? drc : 1;
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
            L"  rrecov list-carve-formats\n"
            L"             (the carve-capability projection of the ABI registry -\n"
            L"              the og host's EUGetAnalyserList/Map/Multimap surface)\n"
            L"  rrecov sessions [--dir D] [--json]\n"
            L"             (the saved-session list: per-file light summaries,\n"
            L"              the og's LastScanRecordView surface; unreadable files\n"
            L"              are per-row data, only enumeration failure exits 1)\n"
            L"  rrecov scan --disk N | --volume C: | --image file.vmdk|.vhd|.vhdx|.img [.dmg raw]\n"
            L"             [--whole-disk] [--quick-only] [--no-carve]\n"
            L"             (deep stage always on; --quick-only disables it; --whole-disk = "
            L"lost-partition discovery)\n"
            L"             [--threads N] [--out file.crsf] [--resume file.crsf] [--log file]\n"
            L"             [--json] [--csv] [--filter s] [--deleted-only]\n"
            L"             [--repair] [--repair-apply] [--repair-undo file]\n"
            L"             [--bitlocker-fvek hex]  (locked volume: user-supplied FVEK,\n"
            L"                                     64/128 hex chars - in-stack XTS-AES\n"
            L"                                     decrypt, engines see plaintext)\n"
            L"  rrecov tree --in file.crsf [--filter s] [--deleted-only] [--csv]\n"
            L"  rrecov recover --in file.crsf (--disk N | --image file) --to DIR [--deleted-only] "
            L"[--filter s]\n"
            L"             [--report file.json]\n"
            L"  rrecov health --disk N\n"
            L"  rrecov driver [--service NAME] [--json]\n"
            L"             (the driver-status surface: present/loaded/started/"
            L"authorized/healthy/version facets; exit 0 = ready, 4 = the\n"
            L"              driver prerequisite is not met - docs/DRIVER-REQUIRED.md)\n"
            L"  rrecov fix --disk N [--part P] [--apply] [--undo file] [--undo-apply file]\n"
            L"  rrecov apply --disk N --session file.crsf --candidate N [--apply] [--undo file]\n"
            L"             [--boot-fix old-lba [--boot-fix-bcd path]] (writes a lost-partition\n"
            L"             candidate's MBR entry; dry-run by default; --boot-fix additionally\n"
            L"             rewrites the BCD offsets + UEFI NVRAM entry after the apply)\n"
            L"  rrecov bench --disk N [--mode seq|rand|open|read|info] [--mb N] [--block-kb N]\n"
            L"              [--iterations N] [--seed N] [--json]\n"
            L"\n"
            L"driver prerequisite: scan/recover/fix/apply/bench (disk targets) refuse with\n"
            L"exit 4 unless the RDA driver is present, loaded, started, authorized and\n"
            L"version-compatible (rrecov driver reports each facet). --image targets are\n"
            L"driver-independent by classification (docs/DRIVER-REQUIRED.md).\n");
        return 0;
    }
    const std::wstring cmd = argv[1];
    int i = 2;
    if (cmd == L"list")
        return print_disks();
    if (cmd == L"list-carve-formats")
        return print_carve_formats();
    if (cmd == L"sessions")
        return print_sessions(argc, argv, i);
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
    if (cmd == L"apply")
        return cmd_apply(argc, argv, i);
    if (cmd == L"bench")
        return cmd_bench(argc, argv, i);
    if (cmd == L"driver")
        return cmd_driver(argc, argv, i);
    wprintf(L"unknown command: %s\n", cmd.c_str());
    return 1;
}
