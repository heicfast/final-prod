/*
 * cr_rda.h - user-mode wire contract for the RDA kernel driver (rda-driver).
 *
 * Clean-room restatement of the public contract documented in
 * rda-driver/src/shared/inc/rda_ioctl.h + docs/ground-truth-contracts.md:
 * the recovery core is a first-class RDA client. Everything the driver
 * exposes is available here: identity IOCTLs, extended capability IOCTLs
 * (GET_INFO / OPEN_TARGET / GET_VERSION), the legacy path-grammar opens,
 * the PID-token handshake and the copy-session protocol (used by the
 * imaging / verification paths).
 *
 * Critical wire fact (F11): on success the driver reports
 * Information = Length * 512 regardless of the true byte count - every
 * ReadFile/WriteFile result through an RDA handle must be divided by
 * kLegacyInformationScale.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RDA_H_
#define CR_RDA_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace win {
namespace rda {

/* ---------------------------------------------------------------------------
 * Device type + custom IOCTL codes (CTL_CODE(0x600, fn, METHOD_BUFFERED,
 * FILE_ANY_ACCESS)).
 * ------------------------------------------------------------------------- */
constexpr u32 kDeviceType = 0x600;

/* Reference identity IOCTLs (EPMNtDrv contract - exact codes). */
constexpr u32 kIoctlGetProcName = 0x60002400u;
constexpr u32 kIoctlGetFileObjName = 0x60002404u;
constexpr u32 kIoctlGetDiskContextMode = 0x60002408u;

/* RDA extended IOCTLs. */
constexpr u32 kIoctlGetInfo = 0x60002440u;    /* fn 0x910 */
constexpr u32 kIoctlOpenTarget = 0x60002444u; /* fn 0x911 */
constexpr u32 kIoctlGetVersion = 0x60002448u; /* fn 0x912 */

/* Identity IOCTL length gates. */
constexpr u32 kIdentInLen = 4u;
constexpr u32 kIdentMinOutLen = 0x208u;

/* READ/WRITE legacy Information scaling (F11): every byte count coming back
 * from ReadFile/WriteFile on an RDA handle is 512x the true value. */
constexpr u32 kLegacyInformationScale = 512u;

/* ---------------------------------------------------------------------------
 * IOCTL_RDA_GET_INFO output (mirror of RDA_DRIVER_INFO). Size-frozen.
 * ------------------------------------------------------------------------- */
struct DriverInfo {
    u32 struct_size;   /* sizeof == 24 */
    u32 major_version; /* 1 */
    u32 minor_version; /* 0 */
    u32 flags;         /* kInfoFlagXxx */
    u32 max_disks;     /* 200 by default */
    u32 ioctl_base;    /* session-protocol code base in effect */
};

constexpr u32 kInfoFlagFilterMode = 0x1u; /* PnP filter role active */
constexpr u32 kInfoFlagSessions = 0x2u;   /* copy-session engine active */

static_assert(sizeof(DriverInfo) == 24, "DriverInfo wire size");

/* ---------------------------------------------------------------------------
 * IOCTL_RDA_OPEN_TARGET input (mirror of RDA_OPEN_TARGET_IN). Size-frozen.
 * The structured open: no kernel string parsing, no PID token - preferred
 * over the legacy grammar when the driver advertises it.
 * ------------------------------------------------------------------------- */
struct OpenTargetIn {
    u32 struct_size;   /* sizeof == 24 */
    u32 kind;          /* kTargetKindXxx */
    u32 disk_number;   /* kinds: Disk, DiskPart */
    u32 part_number;   /* kind: DiskPart (DR-fallback selector) */
    u32 volume_number; /* kind: Volume */
    u32 reserved;
};

constexpr u32 kTargetKindVolume = 1;   /* \\.\SYM\...\Volume\{n} equivalent */
constexpr u32 kTargetKindDisk = 2;     /* EUEDKEPM whole-disk variant */
constexpr u32 kTargetKindDiskPart = 3; /* EPMNtDrv 3-segment variant */

static_assert(sizeof(OpenTargetIn) == 24, "OpenTargetIn wire size");

/* ---------------------------------------------------------------------------
 * PID-token handshake (exact transform, driver + client sides):
 *   b0..b3 = little-endian bytes of the 32-bit process id
 *   token  = b0 | (b0+b1)<<8 | (b1+b2)<<16 | (b2+b3)<<24  (8-bit wraps)
 * ------------------------------------------------------------------------- */
u32 compute_pid_token(u32 pid);

/* ---------------------------------------------------------------------------
 * Copy-session protocol (EUDCPEPM contract). The wire code is
 * normalized_code + ioctl_base (learned via GET_INFO).
 * ------------------------------------------------------------------------- */
constexpr u32 kSessDiscovery = 0x800C2AC0u; /* filter stacks only */
constexpr u32 kSessGetVersion = 0x800C2AC8u;
constexpr u32 kSessOpenSession = 0x800C2AD0u;
constexpr u32 kSessSessionOp = 0x800C2AD8u;  /* flush protected file */
constexpr u32 kSessSessionOp2 = 0x800C2AE0u; /* StartBackup: pump registration */
constexpr u32 kSessGetNextBlock = 0x800C2AE8u;
constexpr u32 kSessSessionStatus = 0x800C2AF0u;
constexpr u32 kSessGetProgress = 0x800C2AF8u; /* FINALIZE */
constexpr u32 kSessCloseSession = 0x800C2B00u;
constexpr u32 kSessDirectRead = 0x800C2B0Au;
constexpr u32 kSessDirectWrite = 0x800C2B12u;
constexpr u32 kSessMode4 = 0x800C2B18u; /* raw paging write by file handle */
constexpr u32 kSessMode3 = 0x800C2B20u; /* raw paging read by file handle */
constexpr u32 kSessSizedXfer = 0x800C2B2Au;
constexpr u32 kSessSizedXfer2 = 0x800C2B2Eu;
constexpr u32 kSessCancel = 0x800C2B38u;
constexpr u32 kSessRrwrWrite = 0x800C2B3Eu;   /* filter only */
constexpr u32 kSessRrwrVersion = 0x800C2B40u; /* filter only */
constexpr u32 kSessRrwrRead = 0x800C2B42u;    /* filter only */

/* Session state machine (driver-side): 0 created, 1 flushing, 3 standby,
 * 4 copying, 5 done. CANCEL forces 3; StartBackup requires 3. */
constexpr u32 kSessionStateCreated = 0;
constexpr u32 kSessionStateFlushing = 1;
constexpr u32 kSessionStateStandby = 3;
constexpr u32 kSessionStateCopying = 4;
constexpr u32 kSessionStateDone = 5;

/* OPEN_SESSION overlaid input/output buffer (0x840 bytes, pack(4) - the
 * reference wire format places 64-bit fields at 4-mod-8 offsets).
 * On success the driver writes: session handle @+0x834, effective
 * sectors-per-block @+0x30 (input slot), sector size @+0x83C. */
#pragma pack(push, 4)
struct OpenSession {
    u32 disk_id;            /* +0x000 */
    u64 data_first_sec;     /* +0x004 copy range start sector */
    u64 total_secs;         /* +0x00C must be nonzero */
    u64 extent_param1;      /* +0x014 block-map builder parameter */
    u64 extent_param2;      /* +0x01C block-map builder parameter */
    u64 bitmap_size;        /* +0x024 must be nonzero */
    u32 bitmap_count;       /* +0x02C must be nonzero */
    u32 spb_in_out;         /* +0x030 in: 0 = auto; out: effective spb */
    u8 block_bitmap[0x800]; /* +0x034 copy-plan bitmap, NUL pad @0x7FE */
    u64 session_handle;     /* +0x834 out: opaque handle (RDA: ID, not ptr) */
    u32 sector_size;        /* +0x83C out: probed sector size */
};
#pragma pack(pop)

static_assert(sizeof(OpenSession) == 0x840, "OpenSession wire size");
static_assert(offsetof(OpenSession, data_first_sec) == 0x004, "ofs data_first_sec");
static_assert(offsetof(OpenSession, spb_in_out) == 0x030, "ofs spb_in_out");
static_assert(offsetof(OpenSession, block_bitmap) == 0x034, "ofs block_bitmap");
static_assert(offsetof(OpenSession, session_handle) == 0x834, "ofs session_handle");
static_assert(offsetof(OpenSession, sector_size) == 0x83C, "ofs sector_size");

/* StartBackup (SESSION_OP2) input, pack(4): 0x28 bytes. */
#pragma pack(push, 4)
struct StartBackupIn {
    u64 handle;      /* +0x00 */
    u64 pump_buffer; /* +0x08 user pointer, driver locks it with an MDL */
    u32 pump_bytes;  /* +0x10 multiple of spb*sector_size */
    u64 user_bitmap; /* +0x14 optional exclusion bitmap (unaligned-8) */
    u32 start_sec;   /* +0x1C */
    u32 block_count; /* +0x20 */
    u32 spb;         /* +0x24 must match session spb when bitmap supplied */
};
#pragma pack(pop)

static_assert(sizeof(StartBackupIn) == 0x28, "StartBackupIn wire size");
static_assert(offsetof(StartBackupIn, pump_buffer) == 0x08, "ofs pump_buffer");
static_assert(offsetof(StartBackupIn, user_bitmap) == 0x14, "ofs user_bitmap");
static_assert(offsetof(StartBackupIn, start_sec) == 0x1C, "ofs start_sec");
static_assert(offsetof(StartBackupIn, spb) == 0x24, "ofs spb");

/* GETNEXTBLOCK wire record (in == out == 0x1E, overlaid, pack(1)).
 * Input: handle, cursor LBA, max sectors this round. Output: source diskId,
 * copy progress percent, block-map progress + ok. The pumped data lands in
 * the StartBackup-registered buffer, NOT in this record. */
#pragma pack(push, 1)
struct GetNextBlock {
    u64 handle;      /* +0x00 in */
    u64 cursor_sec;  /* +0x08 in */
    u32 max_sec;     /* +0x10 in */
    u32 disk_id;     /* +0x14 out */
    u32 progress;    /* +0x18 out: copy percent */
    u8 map_progress; /* +0x1C out */
    u8 map_ok;       /* +0x1D out */
};
#pragma pack(pop)

static_assert(sizeof(GetNextBlock) == 0x1E, "GetNextBlock wire size");
static_assert(offsetof(GetNextBlock, cursor_sec) == 0x08, "ofs cursor_sec");
static_assert(offsetof(GetNextBlock, max_sec) == 0x10, "ofs max_sec");
static_assert(offsetof(GetNextBlock, disk_id) == 0x14, "ofs disk_id");
static_assert(offsetof(GetNextBlock, map_ok) == 0x1D, "ofs map_ok");

/* Session-handle input (8 bytes) for SESSION_OP / STATUS / CANCEL /
 * GET_PROGRESS / CLOSE_SESSION. */
struct SessionHandleIn {
    u64 handle;
};

/* GET_PROGRESS (FINALIZE) output: percent u32 @+8; needs 0x0C bytes. */
constexpr u32 kGetProgressOutLen = 0x0Cu;

/* Driver-reported version (GET_VERSION: u64 = (Major<<32)|Minor). */
constexpr u32 kVersionMajor = 1u;
constexpr u32 kVersionMinor = 0u;

/* ---------------------------------------------------------------------------
 * RdaClient - the full driver client.
 *
 * One control-device handle (bare open, global mode) serves all IOCTLs;
 * target handles (opened via OPEN_TARGET or the legacy grammar) serve
 * sector IO with the F11 unscale. Thread-safe: control-handle IOCTLs
 * serialize on a critical section; target handles are independent
 * OVERLAPPED readers (one OVERLAPPED per call, per-thread stack).
 * ------------------------------------------------------------------------- */
class RdaClient {
public:
    /* Opens \\.\{service} (bare). Returns null + err if the driver is not
     * running (caller falls back to direct handles). */
    static std::unique_ptr<RdaClient> open(const wchar_t* service_name, u32* err);

    ~RdaClient();
    RdaClient(const RdaClient&) = delete;
    RdaClient& operator=(const RdaClient&) = delete;

    /* Capability query (IOCTL_RDA_GET_INFO). False = driver too old /
     * IOCTL unsupported. */
    bool query_info(DriverInfo* out);

    /* Version query (IOCTL_RDA_GET_VERSION): (major<<32)|minor. */
    bool query_version(u64* out);

    /* Structured target open (IOCTL_RDA_OPEN_TARGET) - the preferred path.
     * The returned device implements IStorageDevice with the F11 unscale. */
    std::unique_ptr<IStorageDevice> open_target(u32 kind, u32 disk, u32 part, u32 volume, u32* err);

    /* Legacy grammar opens (EPMNtDrv/EUEDKEPM wire compat, PID token). */
    std::unique_ptr<IStorageDevice> open_disk_grammar(u32 disk, u32 part, u32* err);
    std::unique_ptr<IStorageDevice> open_volume_grammar(u32 volume, u32* err);

    /* Identity IOCTLs (admin-gated by device SDDL). */
    bool get_proc_name(u32 pid, wchar_t* out, u32 out_chars);

    /* Copy-session protocol (imaging / verification). A session is an
     * exclusive range claim on a disk; overlap fails with a sharing error. */
    u64 session_open(u32 disk_id, u64 data_first_sec, u64 total_secs, u32* spb_out,
                     u32* sector_size_out, u32* err);
    bool session_flush(u64 handle); /* SESSION_OP: flush protected file */
    bool session_start_backup(u64 handle, void* pump_buffer, u32 pump_bytes, u64 start_sec,
                              u32 block_count, u32 spb, const u8* user_bitmap);
    /* Pumps up to max_sec sectors into the registered buffer; returns the
     * sector cursor to use next (0 + err when the stream is complete). */
    bool session_get_next_block(u64 handle, u64 cursor_sec, u32 max_sec, u64* next_cursor,
                                u8* progress, bool* stream_done, u32* err);
    bool session_finalize(u64 handle, u32* percent_out); /* GET_PROGRESS */
    bool session_cancel(u64 handle);
    bool session_close(u64 handle);

    /* Learned capability cache. */
    const DriverInfo& info() const { return info_; }
    bool has_info() const { return have_info_; }
    u32 ioctl_base() const { return ioctl_base_; }

private:
    RdaClient();
    bool ctl_ioctl(u32 code, void* in, u32 in_len, void* out, u32 out_len, u32* info_out);
    /* Opens a fresh bare (global-mode) handle on the same service. */
    void* open_bare_handle_(bool need_write);

    void* ctrl_ = nullptr;       /* HANDLE to \\.\{service} */
    void* info_mutex_ = nullptr; /* CRITICAL_SECTION guarding control IOCTLs */
    u32 ioctl_base_ = 0;
    DriverInfo info_ = {};
    bool have_info_ = false;
    wchar_t service_[64] = {0}; /* service this client talks to */
    /* pump bookkeeping for session_get_next_block cursor math */
    u32 pump_bytes_ = 0;
    u32 session_sector_size_ = 0;
};

/* Service-name helpers. */
std::wstring control_device_path(const wchar_t* service_name);

} // namespace rda
} // namespace win
} // namespace cr

#endif // CR_RDA_H_
