/*
 * cr_decrypt_device.h - the in-stack BitLocker decrypting device
 * (diff-to-fix M-033).
 *
 * Reference layering (corpus: architecture/05-scan-engine-architecture.md
 * §8 "Volume IO layers"):
 *
 *   Engine tree reader
 *     └─ CPartitionVirtualVolumeDevice
 *          ├─ normal file API / raw IOCTL path
 *          ├─ BitLocker: FVE keys -> XTS-AES decrypt   <-- THIS LAYER
 *          └─ ...
 *
 * The og decrypts INSIDE the volume-IO stack: engines are BitLocker-agnostic
 * and always see plaintext. DecryptingDevice is the core's equivalent: an
 * IStorageDevice wrapper that decrypts every read with AES-XTS using the
 * per-sector data-unit number (LE sector index) - BitLocker's volume-content
 * tweak convention. It wraps the volume slice (the bounded engine view), so
 * the layer order is: physical transport -> slice -> [XTS decrypt] ->
 * engine, matching the og's "below the engines" placement.
 *
 * Key policy (fix F13 / S61 preserved): the wrapper is OPT-IN and requires
 * the user-supplied full-volume encryption key (the FVEK - the reference's
 * BitLockerManager::GetFullVolumeEncryptionKey output shape, 32 bytes for
 * AES-128-XTS / 64 for AES-256-XTS, corpus decompile :27075-27107).
 * Nothing is bypassed: without the key the locked-volume refusal stands
 * (cr_bitlocker.h policy). Deriving the FVEK from a recovery password or
 * BEK inside our stack is tracked open (OQ-S7-1) - the corpus pins the
 * derivation EXISTENCE (stretch -> VMK -> FVEK per the BitLockerLib
 * artifact) but not its byte interop (the finding's own open question).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_DECRYPT_DEVICE_H_
#define CR_DECRYPT_DEVICE_H_

#include "cr_device.h"
#include "../crypto/cr_aes.h"
#include <memory>

namespace cr {

/* AES-XTS decrypting wrapper over another IStorageDevice (non-owning:
 * the caller keeps the inner device alive, exactly like PartitionSlice
 * borrows its parent). Thread safety: same contract as the inner device;
 * the wrapper itself is immutable after create(). */
class DecryptingDevice final : public IStorageDevice {
public:
    /*
     * inner       - the wrapped device view (typically the volume slice).
     * fvek        - 32 bytes (AES-128-XTS) or 64 bytes (AES-256-XTS),
     *               key || tweak-key concatenated (the FVEK shapes).
     * fvek_len    - 32 or 64; anything else (or an XTS-weak key) fails
     *               create() with ERR_BAD_PARAMETER.
     *
     * Reads are decrypted with the tweak derived from the ABSOLUTE
     * inner-device offset: sector index = offset / inner.geometry().
     * logical_sector, little-endian in the first 8 tweak bytes (BitLocker
     * convention). Unaligned reads decrypt whole spanned sectors and copy
     * the requested window - byte-exact for any offset/length.
     */
    static std::unique_ptr<DecryptingDevice> create(IStorageDevice* inner,
                                                    const u8* fvek,
                                                    size_t fvek_len);

    u32 read_at(u64 offset, void* buffer, u32 length) override;
    const Geometry& geometry() const override;
    u64 size() const override { return inner_->size(); }
    u64 absolute_base() const override { return inner_->absolute_base(); }
    bool is_static_media() const override { return inner_->is_static_media(); }

private:
    DecryptingDevice() = default;
    IStorageDevice* inner_ = nullptr;
    crypto::AesXts xts_;
    std::vector<u8> scratch_; /* sector staging, reused per read */
};

} // namespace cr

#endif // CR_DECRYPT_DEVICE_H_
