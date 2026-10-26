/*
 * cr_apply.cpp - consent-gated lost-partition apply (S-029, diff-to-fix).
 * See cr_apply.h for the corpus contract and the A1-A6 safety standard.
 *
 * The convergent-landing union (two legs, one module): the session-10 leg
 * (plan/action model, CHS math, containment refusals, the CLI posture)
 * and the DFX16 leg (the corpus dual-path device-control sequence, the
 * GPT rebuild variant with the 32-bit-signature correction, the
 * sector-granular write coalescing, the refresh-tail honesty). The union
 * order satisfies both legs' proof sets: revalidate -> consent -> lock ->
 * revalidate -> transaction -> device chain -> refresh tail.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/apply/cr_apply.h"
#include "cr/cr_error.h"
#include <atomic>
#include <cstring>

namespace cr {
namespace apply {

namespace {

constexpr size_t kSector = 512;
constexpr size_t kTableOff = 446;             /* the MBR entry table base */
constexpr size_t kEntryBytes = 16;            /* one partition entry */
constexpr u8 kMaxChs[3] = {0xFE, 0xFF, 0xFF}; /* beyond-cylinder-1023 */

/* LBA -> CHS bytes (255 heads / 63 sectors-per-track, the universal DOS
 * geometry a BIOS/firmware reports for large disks). Beyond cylinder
 * 1023 the maximum CHS is written (the convention the kernel derives for
 * DRIVE_LAYOUT_INFORMATION MBR writes; nothing boot-relevant reads CHS
 * for LBA-booted partitions). */
void lba_to_chs(u64 lba, u8 out[3]) {
    constexpr u64 kHeads = 255, kSpt = 63;
    const u64 cyl = lba / (kHeads * kSpt);
    if (cyl > 1023) {
        out[0] = kMaxChs[0];
        out[1] = kMaxChs[1];
        out[2] = kMaxChs[2];
        return;
    }
    const u64 rem = lba % (kHeads * kSpt);
    const u64 head = rem / kSpt;
    const u64 sect = (rem % kSpt) + 1;
    out[0] = u8(head);
    out[1] = u8(((cyl >> 8) & 0x03) << 6) | u8(sect & 0x3F);
    out[2] = u8(cyl & 0xFF);
}

void put32le(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}

u32 get32le(const u8* p) {
    return u32(p[0]) | u32(p[1]) << 8 | u32(p[2]) << 16 | u32(p[3]) << 24;
}

bool entry_is_zero(const u8* e) {
    for (size_t i = 0; i < kEntryBytes; ++i)
        if (e[i] != 0)
            return false;
    return true;
}

void refuse(ApplyPlan& p, const char* reason) {
    p.refused = true;
    p.refusal_reason = reason;
    p.actions.clear();
    p.entry_bytes.clear();
}

/* The corpus 3x refresh tail (PR:47015-47025): settle, attempt, break on
 * success, at most 3 attempts. The og IGNORES tail failure; the core
 * reports ERR_APPLY_REFRESH_FAILED with the table written and verified
 * (D-72c - a stale OS mount view must be visible to the operator, never
 * silently absorbed). */
u32 refresh_tail(IApplyDevice& dev) {
    for (u32 attempt = 0; attempt < 3; ++attempt) {
        dev.settle_refresh();
        if (dev.update_properties() == OK)
            return OK;
    }
    return ERR_APPLY_REFRESH_FAILED;
}

} // namespace

/* Distinct-per-call default signature generator: FNV-1a over the current
 * sector 0 mixed with a process-unique counter (two consecutive rebuilds
 * on the same image must not stamp the same signature - mount-manager
 * confusion). Transports with real entropy and deterministic tests
 * override. */
u32 IApplyDevice::generate_disk_signature() {
    static std::atomic<u32> counter{0x9E3779B9u};
    u8 sec[kSector];
    u32 h = 0x811C9DC5u;
    if (read_at(0, sec, sizeof(sec)) == OK) {
        for (size_t i = 0; i < kSector; ++i) {
            h ^= sec[i];
            h *= 0x01000193u;
        }
    }
    h ^= counter.fetch_add(0x9E3779B9u);
    h *= 0x01000193u;
    return h ? h : 1u; /* 0 is a legal-but-weird signature; keep nonzero */
}

u8 mbr_type_for_family(lost::CandidateFs fs) {
    switch (fs) {
    case lost::CandidateFs::Ntfs:
    case lost::CandidateFs::Exfat:
    case lost::CandidateFs::Refs:
    case lost::CandidateFs::BitLocker:
        return 0x07; /* HPFS/NTFS/exFAT - the Microsoft data family */
    case lost::CandidateFs::Fat:
        return 0x06; /* FAT12/16 (big) */
    case lost::CandidateFs::GptTable:
        return 0xEE; /* protective (never planned by apply - refused) */
    case lost::CandidateFs::Ext2:
    case lost::CandidateFs::Btrfs:
        return 0x83; /* Linux native */
    case lost::CandidateFs::HfsPlus:
        return 0xAF; /* Apple HFS/HFS+ */
    case lost::CandidateFs::Zfs:
        return 0xBF; /* Solaris/ZFS */
    case lost::CandidateFs::Unknown:
    default:
        return 0x07; /* the generic data type - the honest default for
                      * an unclassified-but-scored candidate */
    }
}

u32 plan_apply(IStorageDevice* disk, const lost::Candidate& c, ApplyPlan& out,
               const std::vector<table::PartitionInfo>& known, ApplyScope scope) {
    out = ApplyPlan{};
    out.scope = scope;
    out.start_lba = c.start_lba;
    out.sector_count = c.sector_count;
    out.fs = c.fs;
    out.mbr_type = mbr_type_for_family(c.fs);

    if (disk == nullptr)
        return ERR_BAD_PARAMETER;
    if (c.sector_count == 0) {
        refuse(out, "candidate has no size (sector_count 0)");
        return OK;
    }

    /* read + snapshot sector 0 (the og re-reads sector 0 raw before its
     * rewrite, PR:46944 - the same truth, captured at plan time so
     * execute can detect a table that moved underneath) */
    u8 sec[kSector];
    if (disk->read_at(0, sec, sizeof(sec)) != OK) {
        refuse(out, "cannot read sector 0 (the disk's table is not readable)");
        return OK;
    }
    out.sector0_original.assign(sec, sec + kSector);

    /* the current 32-bit disk signature (MBR +440) - the preserve policy
     * keeps it byte-for-byte (the corpus passes a caller-chosen signature
     * through the same slot; preservation is the safe default). */
    out.mbr_signature = get32le(sec + 440);

    const bool protective = sec[kTableOff + 4] == 0xEE;
    const bool has_mbr = sec[510] == 0x55 && sec[511] == 0xAA;
    (void)has_mbr; /* a missing 0x55AA is restorable by the plan's
                    * signature-restore action, not a refusal (the og's
                    * rewrite covers it, PR:46957-46962) */

    if (scope == ApplyScope::SingleEntry) {
        /* GPT refusal: a protective entry means the layout rebuild is the
         * GPT variant (FUN_1800392f0) - entry writes on a GPT layout are
         * v2 scope, refused cleanly, never a half-mode write. A MISSING
         * 0x55AA is NOT a refusal: the signature-restore action covers it
         * (the og's rebuild rewrites it, PR:46957-46962). */
        if (protective) {
            refuse(out, "GPT layout (protective MBR) - GPT apply is not supported yet");
            return OK;
        }

        /* A3 containment: LBA 0 is the MBR itself */
        if (c.start_lba == 0) {
            refuse(out, "candidate starts at LBA 0 (the MBR sector itself)");
            return OK;
        }
        /* A3 containment: the candidate must fit the disk */
        const u64 disk_sectors =
            disk->size() /
            (disk->geometry().logical_sector ? disk->geometry().logical_sector : 512);
        if (disk_sectors != 0 && c.start_lba + c.sector_count > disk_sectors) {
            refuse(out, "candidate extends past the end of the disk");
            return OK;
        }
        /* A3 containment: MBR entries are 32-bit LBA/count fields - an
         * extent that does not fit would be silently truncated into a
         * wrong entry (data-loss class), so it is refused (GPT territory). */
        if (c.start_lba + c.sector_count > 0x100000000ull) {
            refuse(out, "candidate extends beyond the 32-bit MBR LBA range");
            return OK;
        }
        /* A3 containment: no overlap with any live entry (the og's recovery
         * flow ADDS the recovered partition; live entries are never
         * overwritten by the single-partition apply). The parsed table is
         * authoritative when the caller has it; the raw entries are the
         * fallback (the cr_ui path passes the parsed table). */
        if (!known.empty()) {
            for (const auto& p : known) {
                if (!p.used || p.mbr_type == 0x05 || p.mbr_type == 0x0F)
                    continue; /* containers never block a data candidate */
                const u64 live_end = p.start_lba + p.sector_count;
                const u64 cand_end = c.start_lba + c.sector_count;
                if (c.start_lba < live_end && p.start_lba < cand_end) {
                    refuse(out, "candidate overlaps an existing partition entry "
                                "(live entries are never overwritten)");
                    return OK;
                }
            }
        } else {
            for (u32 i = 0; i < 4; ++i) {
                const u8* e = sec + kTableOff + i * kEntryBytes;
                if (entry_is_zero(e))
                    continue;
                const u32 lba = get32le(e + 8);
                const u32 cnt = get32le(e + 12);
                if (cnt != 0 && c.start_lba < u64(lba) + cnt &&
                    u64(lba) < c.start_lba + c.sector_count) {
                    refuse(out, "candidate overlaps an existing partition entry "
                                "(live entries are never overwritten)");
                    return OK;
                }
            }
        }

        /* slot pick: the first all-zero entry (the og's 4-entry
         * DRIVE_LAYOUT_INFORMATION rebuild fills free slots; a full table
         * has nowhere to land) */
        out.entry_index = 0xFFFFFFFFu;
        for (u32 i = 0; i < 4; ++i) {
            if (entry_is_zero(sec + kTableOff + i * kEntryBytes)) {
                out.entry_index = i;
                break;
            }
        }
        if (out.entry_index > 3) {
            refuse(out, "the MBR table is full (no free entry slot)");
            return OK;
        }
    } else {
        /* RebuildTable (the og's rebuild semantics): all four entries are
         * rewritten (zeroed, then the candidate at slot 0). GPT disks
         * force the signature-regeneration policy (FUN_1800392f0:
         * GPT_GenerateSignature is unconditional in the corpus GPT
         * variant); the GPT headers themselves stay untouched (corpus
         * truth - only the protective MBR is rewritten; see the header's
         * correction note). */
        out.entry_index = 0;
        if (protective)
            out.regenerate_signature = true;
    }

    /* the entry: status 0x00 (never auto-bootable), CHS pair, type,
     * LBA + count. The candidate's evidence (the discovery score)
     * stands behind every field - apply transcribes, never invents. */
    u8 entry[kEntryBytes] = {};
    u8 chs0[3], chs1[3];
    lba_to_chs(c.start_lba, chs0);
    lba_to_chs(c.start_lba + c.sector_count - 1, chs1);
    entry[0] = 0x00; /* not bootable - the og's flow does not decode a
                      * boot-flag decision for the recovered entry */
    entry[1] = chs0[0];
    entry[2] = chs0[1];
    entry[3] = chs0[2];
    entry[4] = out.mbr_type;
    entry[5] = chs1[0];
    entry[6] = chs1[1];
    entry[7] = chs1[2];
    put32le(entry + 8, u32(c.start_lba));
    put32le(entry + 12, u32(c.sector_count));
    out.entry_bytes.assign(entry, entry + kEntryBytes);

    /* the modern-path record (corpus 0x18-byte 0x7C058 shape) for the
     * consent surface and the device chain */
    out.record.partition_number = out.entry_index + 1;
    u64 le = c.start_lba, cnt = c.sector_count;
    std::memcpy(out.record.identity + 0, &le, 8);
    std::memcpy(out.record.identity + 8, &cnt, 8);
    out.record.boot_flag = 0x00;

    if (scope == ApplyScope::RebuildTable) {
        /* one full-sector action: the rebuilt table (zero entries, the
         * candidate at slot 0, 0x55AA, the effective signature policy
         * applied at execute time when regenerating). The signature bytes
         * in the action are the preserve-policy value; the regenerate
         * policy patches them at execute (generate_disk_signature runs
         * under the lock, at transaction time). */
        std::vector<u8> rebuilt(out.sector0_original);
        std::memset(rebuilt.data() + kTableOff, 0, 64); /* 4 entries + pad */
        std::memcpy(rebuilt.data() + 440, &out.mbr_signature, 4);
        std::memcpy(rebuilt.data() + kTableOff, entry, kEntryBytes);
        rebuilt[510] = 0x55;
        rebuilt[511] = 0xAA;
        out.restores_signature = true; /* the rebuild always restamps */
        ApplyAction a;
        a.offset = 0;
        a.after = std::move(rebuilt);
        a.reason = "rebuild the MBR table (zero entries, the recovered partition at slot 0, "
                   "0x55AA" +
                   std::string(out.regenerate_signature ? ", regenerated disk signature"
                                                        : ", preserved disk signature") +
                   ")";
        out.actions.push_back(std::move(a));
        return OK;
    }

    /* the signature restore (only when absent - the og's rebuild
     * rewrites 0x55AA, PR:46957-46962) */
    if (sec[510] != 0x55 || sec[511] != 0xAA) {
        out.restores_signature = true;
        ApplyAction sig;
        sig.offset = 510;
        sig.after = {0x55, 0xAA};
        sig.reason = "restore the 0x55AA MBR signature (absent on the disk)";
        out.actions.push_back(std::move(sig));
    }

    char type_hex[4];
    type_hex[0] = '0';
    type_hex[1] = 'x';
    type_hex[2] = "0123456789ABCDEF"[out.mbr_type >> 4];
    type_hex[3] = "0123456789ABCDEF"[out.mbr_type & 0xF];
    ApplyAction a;
    a.offset = kTableOff + out.entry_index * kEntryBytes;
    a.after = out.entry_bytes;
    a.reason = "write the recovered partition entry (slot " + std::to_string(out.entry_index) +
               ", LBA " + std::to_string(c.start_lba) + ", " + std::to_string(c.sector_count) +
               " sectors, type " + std::string(type_hex, type_hex + 4) + ")";
    out.actions.push_back(std::move(a));
    return OK;
}

u32 execute_apply(IApplyDevice* dev, const ApplyPlan& plan, ApplyConsentFn consent,
                  void* consent_user, repair::UndoLog& undo) {
    if (dev == nullptr || plan.refused || plan.actions.empty())
        return ERR_BAD_PARAMETER;

    /* A2 revalidation BEFORE consent: a stale plan never reaches the
     * consent surface (why ask the user to approve a write that will be
     * refused?) - the union of both convergent legs' orders. This first
     * check is a plain read; the lock has not been taken. */
    {
        std::vector<u8> now(kSector, 0);
        if (dev->read_at(0, now.data(), u32(kSector)) != OK)
            return ERR_READ_FAILED;
        if (plan.sector0_original.size() != kSector ||
            std::memcmp(now.data(), plan.sector0_original.data(), kSector) != 0)
            return ERR_APPLY_STALE;
    }

    /* A1 consent BEFORE the lock: the caller deliberates without the
     * volume being held (a denial takes no lock, holds nothing). */
    if (consent == nullptr || !consent(plan, consent_user))
        return ERR_APPLY_CONSENT_DENIED;

    /* A4 order: quiesce (the S-010 sequence in the platform writer) - a
     * busy volume is a clean refusal before any write */
    if (u32 rc = dev->lock_volume())
        return rc;
    struct UnlockGuard {
        IApplyDevice* dev;
        ~UnlockGuard() { dev->unlock_volume(); }
    } unlock_guard{dev};

    /* A2 revalidation again (the lock-window race): the live sector 0
     * must still match the plan's snapshot - a third-party table change
     * between the first check and the lock is never overwritten. */
    const u32 sbytes = dev->sector_bytes() ? dev->sector_bytes() : 512;
    if (sbytes < kSector)
        return ERR_BAD_PARAMETER; /* a sub-512 "sector" device cannot hold
                                   * an MBR - refuse, never a torn write */
    {
        std::vector<u8> now(size_t(sbytes), 0);
        if (dev->read_at(0, now.data(), u32(kSector)) != OK)
            return ERR_READ_FAILED;
        if (plan.sector0_original.size() != kSector ||
            std::memcmp(now.data(), plan.sector0_original.data(), kSector) != 0)
            return ERR_APPLY_STALE;
    }

    /* The legacy path's device control BEFORE the writes (corpus
     * PR:46953: DELETE_DRIVE_LAYOUT precedes the sector-0 rewrite; the
     * og's FUN_180038b10 bails on its failure before writing). */
    const bool legacy = dev->legacy_os_path();
    if (legacy) {
        if (u32 rc = dev->delete_drive_layout())
            return rc;
    }

    /* the effective signature for the rebuild-regenerate policy: decided
     * under the lock, at transaction time (the signature generator may
     * consult the live sector 0). The SingleEntry scope preserves the
     * on-disk signature; the RebuildTable regenerate policy stamps the
     * fresh value into the action bytes before the write. */
    std::vector<u8> patched;
    bool use_patched = false;
    if (plan.scope == ApplyScope::RebuildTable && plan.regenerate_signature) {
        const u32 fresh = dev->generate_disk_signature();
        patched = plan.actions[0].after; /* the one full-sector action */
        std::memcpy(patched.data() + 440, &fresh, 4);
        use_patched = true;
    }

    /* A4 transaction: sector-granular snapshot -> write -> verify; any
     * failure rolls back everything applied (reverse order, verified).
     * Raw-disk writes must be sector-granular (a 16-byte entry write at
     * offset 462 fails on real direct handles) - every action is
     * coalesced up to its containing sector span, exactly like
     * rawfixer's execute; the undo log therefore always holds full
     * sectors. ONE snapshot per span: co-located actions (the signature
     * restore + the entry, both inside sector 0) share the ORIGINAL
     * bytes - the undo's job is restoring the pre-transaction state, and
     * the shared forward-replaying repair::undo must end on exactly
     * that. */
    const size_t applied_start = undo.entries.size();
    const auto rollback_all = [&]() {
        u32 undo_rc = OK;
        std::vector<size_t> unrestored;
        for (size_t k = undo.entries.size(); k-- > applied_start;) {
            const auto& e = undo.entries[k];
            std::vector<u8> check(e.length);
            if (dev->write_at(e.offset, e.original.data(), e.length) != OK ||
                dev->read_at(e.offset, check.data(), e.length) != OK ||
                std::memcmp(check.data(), e.original.data(), e.length) != 0)
                unrestored.push_back(k);
        }
        if (unrestored.empty()) {
            undo.entries.resize(applied_start);
            (void)refresh_tail(*dev); /* best-effort after a restore */
        } else {
            std::vector<repair::UndoLog::Entry> saved;
            for (size_t k = unrestored.size(); k-- > 0;)
                saved.push_back(std::move(undo.entries[unrestored[k]]));
            undo.entries.resize(applied_start);
            for (auto& e : saved)
                undo.entries.push_back(std::move(e));
            undo_rc = ERR_REPAIR_UNDO_FAILED;
        }
        return undo_rc;
    };

    /* the spans to write: either the plan's actions or the patched
     * rebuild span. Each span is coalesced to sector granularity. */
    struct Span {
        u64 offset;
        std::vector<u8> bytes;
    };
    std::vector<Span> spans;
    if (use_patched) {
        spans.push_back(Span{0, patched});
    } else {
        for (const ApplyAction& a : plan.actions) {
            if (a.after.empty())
                continue;
            spans.push_back(Span{a.offset, a.after});
        }
    }

    std::vector<u64> recorded_spans;
    const auto span_recorded = [&recorded_spans](u64 start) {
        return std::find(recorded_spans.begin(), recorded_spans.end(), start) !=
               recorded_spans.end();
    };

    for (const Span& sp : spans) {
        const u64 span_start = sp.offset / sbytes * sbytes;
        const u64 span_end = (sp.offset + sp.bytes.size() + sbytes - 1) / sbytes * sbytes;
        const u32 span_len = u32(span_end - span_start);
        if (span_end > dev->size()) {
            (void)rollback_all();
            return ERR_RANGE_OUT_OF_BOUNDS;
        }
        std::vector<u8> span(span_len, 0);
        if (dev->read_at(span_start, span.data(), span_len) != OK) {
            (void)rollback_all();
            return ERR_READ_FAILED;
        }
        if (!span_recorded(span_start)) {
            undo.record(span_start, span_len, span.data());
            recorded_spans.push_back(span_start);
        }
        std::memcpy(span.data() + size_t(sp.offset - span_start), sp.bytes.data(), sp.bytes.size());
        if (dev->write_at(span_start, span.data(), span_len) != OK) {
            /* the write never landed; the snapshot is restored with the
             * rest (a no-op write of identical bytes - correct even when
             * earlier actions share this span, unlike a naive pop). */
            const u32 rc = rollback_all();
            return rc != OK ? rc : ERR_WRITE_FAILED;
        }
        std::vector<u8> check(span_len, 0);
        if (dev->read_at(span_start, check.data(), span_len) != OK ||
            std::memcmp(check.data(), span.data(), span_len) != 0) {
            const u32 rc = rollback_all();
            return rc != OK ? rc : ERR_REPAIR_REFUSED; /* clean rollback = the
                                                        * write landed wrong */
        }
    }

    /* Occupied-entry count after the apply (the legacy SET_DRIVE_LAYOUT
     * report value). */
    u32 used_after = 0;
    {
        std::vector<u8> now(kSector, 0);
        if (dev->read_at(0, now.data(), u32(kSector)) == OK) {
            for (u32 i = 0; i < 4; ++i)
                if (!entry_is_zero(now.data() + kTableOff + i * kEntryBytes))
                    ++used_after;
        }
    }
    const u32 effective_signature = [&] {
        std::vector<u8> now(kSector, 0);
        if (dev->read_at(0, now.data(), u32(kSector)) == OK)
            return get32le(now.data() + 440);
        return plan.mbr_signature;
    }();

    /* the path's device-control sequence (the corpus order):
     *  legacy: single UPDATE_PROPERTIES (failure = bail WITH rollback -
     *          the og bails without one; the core is transactional, A4)
     *          then SET_DRIVE_LAYOUT with the effective signature
     *  modern: the 0x18-byte 0x7C058 record (PR:46995-47002 region) */
    if (legacy) {
        if (u32 rc = dev->update_properties()) {
            const u32 rb = rollback_all();
            return rb != OK ? rb : rc;
        }
        if (u32 rc = dev->set_drive_layout_mbr(effective_signature, used_after)) {
            const u32 rb = rollback_all();
            return rb != OK ? rb : rc;
        }
    } else {
        if (u32 rc = dev->set_partition_info_ex(plan.record)) {
            const u32 rb = rollback_all();
            return rb != OK ? rb : rc;
        }
    }

    /* the shared 3x refresh tail (PR:47015-47025). Failure here does NOT
     * roll back: the table is written and verified (A4); a stale OS view
     * is an environment problem the operator must see, and the undo log
     * remains valid (the og swallows this silently - D-72c). */
    return refresh_tail(*dev);
}

u32 undo_apply(IApplyDevice& dev, const repair::UndoLog& undo) {
    if (u32 rc = dev.lock_volume())
        return rc;
    struct UnlockGuard {
        IApplyDevice* dev;
        ~UnlockGuard() { dev->unlock_volume(); }
    } unlock_guard{&dev};

    for (const auto& e : undo.entries) {
        if (e.offset + e.length > dev.size())
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (dev.write_at(e.offset, e.original.data(), e.length) != OK)
            return ERR_WRITE_FAILED;
        std::vector<u8> check(e.length);
        if (dev.read_at(e.offset, check.data(), e.length) != OK ||
            std::memcmp(check.data(), e.original.data(), e.length) != 0)
            return ERR_REPAIR_UNDO_FAILED;
    }

    /* the corpus chains UPDATE_PROPERTIES after every layout mutation;
     * the undo path replays the same 3x tail. A failing refresh is
     * reported (the bytes ARE restored and verified - the distinct code
     * exists so bytes-truth and OS-view-truth are never conflated). */
    return refresh_tail(dev);
}

} // namespace apply
} // namespace cr
