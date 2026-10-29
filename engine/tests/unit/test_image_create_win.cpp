/* test_image_create_win.cpp - S-040: the flat-raw image producer's file
 * disposition contract (Windows lane only).
 *
 * Proves the truth contract the finding demands: the final path appears
 * ONLY for a complete image (atomic temp+rename commit), the SHA-256
 * sidecar rides the commit, and a cancelled or failed create keeps the
 * partial under the explicit '<out>.partial' name - reported, never
 * silently passed off as complete.
 *
 * The class-(a) driver path (open_disk + the RDA transport) is exercised
 * by the staged-scenarios lane (driver-install + VHD attach); this suite
 * pins the disposition layer with an in-memory source device.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include <windows.h>
#include "cr/crypto/cr_sha256.h"
#include "cr/storage/cr_image.h"
#include "cr/win/cr_image_create.h"
#include "test_tmpdir.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::win;

namespace {

std::u16string u16_from(const std::string& s) {
    std::u16string out;
    for (char c : s)
        out.push_back(char16_t(unsigned char(c)));
    return out;
}

/* deterministic in-memory source device (2.5 chunks: 64 KiB + 64 KiB + tail) */
class SrcDevice final : public IStorageDevice {
public:
    explicit SrcDevice(u64 bytes) : data_(size_t(bytes)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = bytes;
        g_.sector_count = bytes / 512 + (bytes % 512 ? 1 : 0);
        for (u64 i = 0; i < bytes; ++i)
            data_[size_t(i)] = u8((i * 11 + 0xA7) & 0xFF);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }
    const std::vector<u8>& bytes() const { return data_; }

private:
    std::vector<u8> data_;
    Geometry g_{};
};

class FlagToken final : public CancelToken {
public:
    bool cancelled() const override { return on_; }
    void flip() { on_ = true; }

private:
    bool on_ = false;
};

std::vector<u8> read_file_bytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string hex32(const u8 d[32]) {
    return crypto::to_hex(d, 32);
}

} // namespace

TEST(ImageCreateFile, CommitIsAtomicWithSidecar) {
    /* the happy path: temp streamed, flushed, renamed; the final path
     * appears ONLY on completion; the sidecar carries the digest */
    SrcDevice src(2 * 64 * 1024 + 4096);
    const std::string out = cr_test::tmp_path("imgcreate", ".commit.img");
    const std::u16string out16 = u16_from(out);

    storage::ImageCreateStats st{};
    ASSERT_EQ(OK, create_image_file(src, out16, nullptr, nullptr, nullptr, &st));

    EXPECT_TRUE(std::filesystem::exists(out)) << "the committed image exists";
    EXPECT_FALSE(std::filesystem::exists(out + ".crimg-tmp")) << "no temp left behind";
    EXPECT_FALSE(std::filesystem::exists(out + ".partial")) << "no partial left behind";

    /* byte-identical to the source device */
    const std::vector<u8> got = read_file_bytes(out);
    ASSERT_EQ(src.bytes().size(), got.size());
    EXPECT_EQ(0, std::memcmp(src.bytes().data(), got.data(), got.size()));
    EXPECT_EQ(src.bytes().size(), st.bytes_written);

    /* the sidecar: '<hex>  <basename>' */
    const std::string sidecar_path = out + ".sha256";
    ASSERT_TRUE(std::filesystem::exists(sidecar_path));
    const std::vector<u8> sidecar = read_file_bytes(sidecar_path);
    const std::string want = hex32(st.sha256) + "  .commit.img\n";
    const std::string got_line(sidecar.begin(), sidecar.end());
    EXPECT_EQ(want, got_line);
    /* and the digest equals an independent SHA-256 of the source bytes */
    u8 want_d[32];
    crypto::Sha256::digest(src.bytes().data(), src.bytes().size(), want_d);
    EXPECT_EQ(hex32(want_d), hex32(st.sha256));
}

TEST(ImageCreateFile, CancelKeepsExplicitPartialNeverComplete) {
    /* cancel after the first chunk: '<out>.partial' holds the committed
     * prefix, '<out>' itself NEVER appears, and the rc is ERR_CANCELLED */
    SrcDevice src(3 * 64 * 1024);
    const std::string out = cr_test::tmp_path("imgcreate", ".cancel.img");
    FlagToken tok;
    auto progress = +[](void* user, u64, u64) { static_cast<FlagToken*>(user)->flip(); };
    storage::ImageCreateStats st{};
    const u32 rc = create_image_file(src, u16_from(out), &tok, progress, &tok, &st);
    ASSERT_EQ(ERR_CANCELLED, rc);
    EXPECT_TRUE(st.cancelled);
    EXPECT_EQ(u64(64 * 1024), st.bytes_written);

    EXPECT_FALSE(std::filesystem::exists(out)) << "a cancelled create never "
                                                  "claims completeness";
    EXPECT_FALSE(std::filesystem::exists(out + ".sha256")) << "no sidecar without "
                                                              "a commit";
    EXPECT_TRUE(std::filesystem::exists(out + ".partial")) << "the partial is kept";
    const std::vector<u8> part = read_file_bytes(out + ".partial");
    ASSERT_EQ(size_t(64 * 1024), part.size());
    EXPECT_EQ(0, std::memcmp(src.bytes().data(), part.data(), 64 * 1024));
}

TEST(ImageCreateFile, ReCreateReplacesOldImageAndPartial) {
    /* resume-after-cancel semantics: a second create over the same path
     * commits completely and sweeps the stale .partial (CREATE_ALWAYS on
     * the temp + MOVEFILE_REPLACE_EXISTING on both renames) */
    SrcDevice src(2 * 64 * 1024);
    const std::string out = cr_test::tmp_path("imgcreate", ".recreate.img");

    /* first: cancel to leave a .partial */
    FlagToken tok;
    auto progress = +[](void* user, u64, u64) { static_cast<FlagToken*>(user)->flip(); };
    ASSERT_EQ(ERR_CANCELLED, create_image_file(src, u16_from(out), &tok, progress, &tok, nullptr));
    ASSERT_TRUE(std::filesystem::exists(out + ".partial"));

    /* second: completes; the stale .partial is gone, the image is whole */
    storage::ImageCreateStats st{};
    ASSERT_EQ(OK, create_image_file(src, u16_from(out), nullptr, nullptr, nullptr, &st));
    EXPECT_FALSE(std::filesystem::exists(out + ".partial"));
    const std::vector<u8> got = read_file_bytes(out);
    ASSERT_EQ(src.bytes().size(), got.size());
    EXPECT_EQ(0, std::memcmp(src.bytes().data(), got.data(), got.size()));
}

TEST(ImageCreateFile, EmptyPathRefused) {
    SrcDevice src(64 * 1024);
    storage::ImageCreateStats st{};
    EXPECT_EQ(ERR_BAD_PARAMETER,
              create_image_file(src, u16_from(""), nullptr, nullptr, nullptr, &st));
}
