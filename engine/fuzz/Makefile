# fuzz/ - libFuzzer harnesses for the core_recovery parsers.
#
# Targets:
#   fuzz_pure      pure primitives: USA fixup, runlist codec, checksums,
#                  LFN, attributes, undo-log container
#   fuzz_engines   device-driven engines: boot validators, all FS scans,
#                  lost-partition discovery, rawfixer analyze, staged scan
#   fuzz_carve     carving pipeline (all grammar validators + dedup)
#   fuzz_container CRSF reader + record codecs
#
# Build:  make CXX=clang++ (requires clang with libFuzzer)
# Run:    make run       (45 s smoke round per target)
#         make run-long  (2-minute campaign per target)

CXX ?= clang++
CXXFLAGS := -std=c++20 -g -O1 -fsanitize=fuzzer,address,undefined \
            -fno-omit-frame-pointer -I../include -Wall -Wextra
LDLIBS := -lz

BUILD := build
CORE_SRCS := $(shell find ../src/core -name '*.cpp')
CORE_DIRS := $(sort $(dir $(CORE_SRCS)))
CORE_OBJS := $(patsubst ../src/core/%.cpp,$(BUILD)/%.o,$(CORE_SRCS))
LIB := $(BUILD)/libcrfuzz.a

TARGETS := $(BUILD)/fuzz_pure $(BUILD)/fuzz_engines $(BUILD)/fuzz_carve \
           $(BUILD)/fuzz_container

all: $(TARGETS)

$(BUILD)/%.o: ../src/core/%.cpp | dirs
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

dirs:
	@mkdir -p $(BUILD) $(CORE_DIRS:../src/core/%=$(BUILD)/%)

$(LIB): $(CORE_OBJS)
	ar rcs $@ $^

$(BUILD)/fuzz_%: fuzz_%.cpp $(LIB)
	$(CXX) $(CXXFLAGS) $< $(LIB) $(LDLIBS) -o $@

# corpora live under fuzz/corpus/<target> (committed seeds welcome)
$(TARGETS): | dirs

corpus:
	mkdir -p corpus/pure corpus/engines corpus/carve corpus/container

# -timeout=25: a single input taking >25s fails fast instead of eating
# the libFuzzer default 1200s alarm; -rss_limit_mb bounds heap.
run: $(TARGETS) corpus
	$(BUILD)/fuzz_pure      corpus/pure      -max_total_time=45 -max_len=65536 -timeout=25 -rss_limit_mb=2560
	$(BUILD)/fuzz_engines   corpus/engines   -max_total_time=45 -max_len=2097152 -timeout=25 -rss_limit_mb=2560
	$(BUILD)/fuzz_carve     corpus/carve     -max_total_time=45 -max_len=1048576 -timeout=25 -rss_limit_mb=2560
	$(BUILD)/fuzz_container corpus/container -max_total_time=45 -max_len=1048576 -timeout=25 -rss_limit_mb=2560

run-long: $(TARGETS) corpus
	$(BUILD)/fuzz_pure      corpus/pure      -max_total_time=120 -max_len=65536 -timeout=25 -rss_limit_mb=2560
	$(BUILD)/fuzz_engines   corpus/engines   -max_total_time=120 -max_len=2097152 -timeout=25 -rss_limit_mb=2560
	$(BUILD)/fuzz_carve     corpus/carve     -max_total_time=120 -max_len=1048576 -timeout=25 -rss_limit_mb=2560
	$(BUILD)/fuzz_container corpus/container -max_total_time=120 -max_len=1048576 -timeout=25 -rss_limit_mb=2560

clean:
	rm -rf $(BUILD)

.PHONY: all run run-long clean dirs corpus
