/*
 * cr_destio.h - Win32 DestIo: atomic temp+rename destination writes (R7.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_DESTIO_H_
#define CR_DESTIO_H_

#include "cr/recovery/cr_safewriter.h"
#include <string>

namespace cr {
namespace win {

class WinDestIo final : public recovery::DestIo {
public:
    u32 create_temp(u64 item_id, const std::u16string& final_path_utf16) override;
    u32 write(u64 item_id, u64 offset, const void* data, u32 len) override;
    u32 commit(u64 item_id) override;
    void close(u64 item_id) override;
    bool exists(const std::u16string& path_utf16) override;

private:
    struct Open {
        void* handle = nullptr; /* HANDLE */
        std::u16string temp_path;
        std::u16string final_path;
    };
    std::map<u64, Open> open_;
};

} // namespace win
} // namespace cr

#endif // CR_DESTIO_H_
