/* fuzz_smoke_standalone.cpp - libFuzzer-less deterministic smoke driver.
 *
 * Local stand-in for the CI libFuzzer smoke when clang is unavailable on
 * this host: compiles the same harness entry points with g++ ASan+UBSan
 * and feeds them deterministic structured seeds (existing fuzz corpora +
 * fixtures generated from the test suite's images). Not a replacement for
 * the coverage-guided CI campaign - only a local regression smoke.
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

/* harness entry (one translation unit, picked by FUZZ_ENTRY define) */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

static std::vector<uint8_t> load(const std::string& path) {
    std::vector<uint8_t> out;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f)
        return out;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n > 0) {
        out.resize(size_t(n));
        if (fread(out.data(), 1, size_t(n), f) != size_t(n))
            out.clear();
    }
    fclose(f);
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <seed-file>...\n", argv[0]);
        return 2;
    }
    int fed = 0;
    for (int i = 1; i < argc; ++i) {
        auto bytes = load(argv[i]);
        if (bytes.empty()) {
            fprintf(stderr, "seed %s: unreadable/empty\n", argv[i]);
            return 2;
        }
        LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
        ++fed;
    }
    printf("fuzz-smoke: %d seeds executed clean\n", fed);
    return 0;
}
