/*
 * cr_decrypt_device.cpp - the in-stack XTS decrypting device (M-033).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_decrypt_device.h"
#include <cstring>

namespace cr {

std::unique_ptr<DecryptingDevice> DecryptingDevice::create(IStorageDevice* inner,
                                                           const u8* fvek,
                                                           size_t fvek_len) {
    if (!inner || !fvek)
        return nullptr;
    if (fvek_len != 32 && fvek_len != 64)
        return nullptr;
    std::unique_ptr<DecryptingDevice> d(new DecryptingDevice());
    if (!d->xts_.init(fvek, fvek_len))
        return nullptr;
    d->inner_ = inner;
    /* staging cap: whole-sector reads only; the largest staging need is
     * two sectors when a read straddles a boundary (inner-sector reads
     * are decomposed so staging never exceeds 2 sectors - see read_at). */
    d->scratch_.reserve(size_t(inner->geometry().logical_sector) * 2);
    return d;
}

const Geometry& DecryptingDevice::geometry() const {
    return inner_->geometry();
}

u32 DecryptingDevice::read_at(u64 offset, void* buffer, u32 length) {
    if (!inner_)
        return u32(ERR_BAD_PARAMETER);
    if (length == 0)
        return OK;
    if (!buffer)
        return u32(ERR_BAD_PARAMETER);
    const u64 total = inner_->size();
    if (offset >= total || length > total - offset)
        return u32(ERR_RANGE_OUT_OF_BOUNDS);

    const u32 ss = inner_->geometry().logical_sector;
    if (ss == 0 || (ss % 16) != 0)
        return u32(ERR_BAD_PARAMETER); /* XTS data units must be 16-byte
                                          multiples (SP 800-38E) */

    const u64 first_sec = offset / ss;
    const u64 last_sec = (offset + length - 1) / ss;
    const size_t span_secs = size_t(last_sec - first_sec + 1);
    const u32 span_bytes = u32(span_secs * ss);

    /* read the whole spanned sector range once (the inner device sees the
     * sector-aligned request; transports favor aligned reads anyway),
     * decrypt, then copy the requested window. */
    std::vector<u8> stage(span_bytes);
    const u32 rc = inner_->read_at(first_sec * ss, stage.data(), span_bytes);
    if (rc != OK)
        return rc; /* transport errors propagate verbatim - the error map
                      stays truthful; decryption never masks a failure */

    /* every sector is its own XTS data unit (BitLocker's convention:
     * tweak = the sector's own index) - decrypt sector by sector. */
    for (size_t i = 0; i < span_secs; i++) {
        u8 dun[16];
        crypto::sector_dun(first_sec + i, dun);
        xts_.decrypt(dun, stage.data() + i * ss, ss);
    }

    const u32 head = u32(offset - first_sec * ss);
    std::memcpy(buffer, stage.data() + head, length);
    return OK;
}

} // namespace cr
