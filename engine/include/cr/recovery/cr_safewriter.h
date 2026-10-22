/*
 * cr_safewriter.h - the safe recovery writer (R7).
 *
 * Pure policy + queue + verification. The actual file IO is injected as a
 * DestIo interface (Win32 implementation in the platform layer).
 *
 * R7.2 (fix F12): HARD refusal when the destination volume's disk extent
 * intersects any queued node's source extents - code-level, not UI.
 * R7.1: collision naming (numeric suffix), atomic temp+rename, hashes.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SAFEWRITER_H_
#define CR_SAFEWRITER_H_

#include "../crypto/cr_sha256.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace recovery {

/* Destination IO abstraction (Win32 file backend in the platform layer). */
class DestIo {
public:
    virtual ~DestIo() = default;
    /* create the temp file (unique name per item); returns handle id */
    virtual u32 create_temp(u64 item_id, const std::u16string &final_path_utf16) = 0;
    virtual u32 write(u64 item_id, u64 offset, const void *data, u32 len) = 0;
    /* fsync-equivalent + atomic rename temp -> final */
    virtual u32 commit(u64 item_id) = 0;
    virtual void close(u64 item_id) = 0;
    /* check existing final path (resume detection) */
    virtual bool exists(const std::u16string &path_utf16) = 0;
};

enum class ItemState : u8 { Pending = 0, Reading, Writing, Verifying, Done, Error, Refused };

struct QueueItem {
    tree::FileNode *node = nullptr;
    std::u16string dest_dir;    /* utf16 directory (with trailing sep) */
    std::u16string final_name;  /* collision-resolved */
    std::u16string temp_suffix; /* ".part-" + id */
    u64 item_id = 0;
    ItemState state = ItemState::Pending;
    u32 error = 0;
    u64 bytes_done = 0;
    u64 bytes_total = 0;
    bool hash_verified = false;
};

struct WriterConfig {
    u64 read_chunk = 1 << 20;         /* source read granularity */
    u32 rename_attempts = 64;         /* collision suffix cap */
    bool verify_hash = true;         /* refuse-after-write mismatch is an error */
    bool skip_hashless = false;      /* items without a known hash still recover */
};

struct WriterStats {
    u64 queued = 0;
    u64 done = 0;
    u64 refused = 0;
    u64 errors = 0;
    u64 hash_mismatch = 0;
    u64 resumed = 0; /* detected complete .part files finalized */
};

class SafeWriter {
public:
    SafeWriter(DestIo &io, IStorageDevice *source, WriterConfig cfg);

    /*
     * set_destination - records the destination volume's disk extent.
     * Source slices report their absolute placement via slice_offset param.
     */
    void set_destination(u64 dest_disk_start, u64 dest_disk_bytes);

    /* enqueue one node; computes the collision-resolved name. Returns
     * ERR_DEST_OVERLAP (refusal) when the node's extents hit the dest. */
    u32 enqueue(tree::FileNode *node, const std::u16string &dest_dir);

    /* process the queue: read source extents -> temp -> verify -> commit. */
    u32 process(CancelToken *cancel, bool (*progress)(u64 done, u64 total, void *),
                void *progress_user, WriterStats &stats);

    const std::vector<QueueItem> &queue() const { return queue_; }

    /* collision resolution policy: "name" -> "name (2)", "name (3)"... */
    std::u16string resolve_collision(const std::u16string &dir,
                                     const std::u16string &base_name) const;

    /* sanitize a node display name for the destination FS (corpus sanitizer
     * replaces / * : ? " < > | with _). */
    static std::u16string sanitize_name(const std::u16string &in);

private:
    u32 recover_item(QueueItem &it, CancelToken *cancel, WriterStats &stats);
    DestIo &io_;
    IStorageDevice *source_;
    WriterConfig cfg_;
    u64 dest_start_ = 0;
    u64 dest_bytes_ = 0;
    bool have_dest_ = false;
    u64 next_id_ = 1;
    std::vector<QueueItem> queue_;
};

} // namespace recovery
} // namespace cr

#endif // CR_SAFEWRITER_H_
