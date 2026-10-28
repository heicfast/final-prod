/*
 * cr_usersigns.h - the user-extensible signature database (M-048).
 *
 * Corpus: the og's og-codebase/EUUserData - a 16,891-byte JSON map of
 * 111 extensions, each carrying user-defined file signatures
 * ("signs": ["<offset>,<hex bytes>"]) and a MIME type
 * (config-data-reference.md :95; byte-verified in the finding). N-170
 * (CORRECTIONS.md #9 = E-A/E-B, re-read first-hand): the og's RAW
 * carving engine NEVER consults this file - its sole og consumer is
 * DRWUI's CHKRepairFileSign (the CHK repair chain's
 * MatchedFile/NotMatchedFile signature checker, DRWUI :1005387/:1005818/
 * :721520-721680; zero EUUserData references in RAWSearchDLL's 80,614
 * lines). The CORE's carve-side consumption of this format is the
 * registered deliberate superset D-104 (the user-taught channel the og
 * never built engine-side).
 *
 * Schema (the og's shape verbatim):
 *   { "<ext>": { "signs": ["<offset>,<HEX>"], "mime": "<type>" }, ... }
 * One entry may carry several signs; a candidate matches when ANY sign's
 * bytes appear at the sign's offset from the candidate start.
 *
 * Source-of-truth posture: EUUserData is an INSTALL-TREE CONFIG FILE the
 * og re-reads per scan - not session state. The core matches that: the
 * file-load path re-reads per run (CLI --user-signs <file> / the cr_ui
 * registration surface); sessions do not carry the set (re-registration
 * is the caller's one flag, exactly like the og's config file).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_USERSIGNS_H_
#define CR_USERSIGNS_H_

#include "../cr_types.h"
#include <cstddef>
#include <vector>

namespace cr {
namespace carve {
namespace usersigns {

/* S-039: the caps are the og's own shipped EUUserData measured maxima
 * (16,891-byte file, 111 entries, census in the finding) plus honest
 * hostile-blob headroom - the og's DB must parse clean; anything beyond
 * these bounds is hostile input, rejected atomically. */
constexpr u32 kMaxSignBytes =
    48; /* the og's longest sign is 44 bytes (hqx/img/eps); 48 = 44 + headroom */
constexpr u32 kMaxEntries = 256;      /* N-170: the cap is enforced against TOTAL SIGNS
                                         * (the loader's out.signs.size() check, cr_usersigns.cpp)
                                         * - the og's shipped set carries 190 signs deduped (191 raw,
                                         * the duplicate-asf merge; N-169), so 256 = 1.34x the raw
                                         * count, the hostile-blob bound */
constexpr u32 kMaxSignsPerEntry = 12; /* the og's max is 11 signs (mpeg); 12 = 11 + headroom */
constexpr u32 kMaxOffset = 1024; /* the og's max offset is 526 (zip "526,504B537058"); the og has no
                                  * probe-stride bound - 1024 keeps a hostile-input bound */
constexpr u32 kMaxExtLen = 8;    /* N-179: the og's longest ext is 8 ("manifest"; "class" is 5 - the
                                  * >4 set measured over og-codebase/EUUserData's 111 keys) - 8 = exact
                                  * fit, no headroom: a 9-char ext is a legitimate reject, and manifest
                                  * at exactly 8 is the boundary case
                                  * UserSigns.ManifestEightCharExtParses    pins */
constexpr u32 kMaxMimeLen = 80;  /* the og's longest mime is 73 chars (the OOXML pptx string) */

struct UserSign {
    char ext[kMaxExtLen + 1] = {};   /* lowercased */
    char mime[kMaxMimeLen + 1] = {}; /* may be empty (the og's entries all carry one) */
    u32 offset = 0;                  /* from the candidate start */
    u16 len = 0;
    u8 bytes[kMaxSignBytes] = {};
};

struct UserSignSet {
    std::vector<UserSign> signs;
    bool valid = false; /* true after a successful parse/registration */
};

/* Parse the og's EUUserData JSON shape into `out`.
 * Returns OK, or:
 *   ERR_BAD_PARAMETER  - null bytes / empty
 *   ERR_BAD_FORMAT     - grammar violation (the honest rejection: the
 *                        caller surfaces it, never a silent partial load)
 * Entries validate as they parse: offset <= kMaxOffset, hex length
 * 2..2*kMaxSignBytes and even, extension charset [A-Za-z0-9]{1,8}
 * (folded lowercase - the og's keys are lowercase), mime length bound,
 * entry/sign caps. The caps are the measured maxima of the og's own
 * shipped database (S-039) - the og's EUUserData parses clean. The
 * parser accepts the og's pretty-printed form
 * (4-space indent, multi-line arrays) and compact JSON alike - it reads
 * the token stream, not the layout. */
u32 parse(const char* bytes, size_t len, UserSignSet& out);

/* ---- the process-global registry (the cr_ui registration consumer) ----
 * Thread-safe (mutex-guarded); installed sets replace the previous one
 * (re-registration is the og's config-reload posture). */
u32 install_set(UserSignSet&& set); /* returns the install error or OK */
u32 active_count();                 /* installed sign count (0 = none) */
u32 snapshot(UserSignSet& out);     /* value copy under the lock (ABI info) */

/* Match `p` (the stride base, at least kMaxOffset+1 bytes valid) against
 * the active set; returns the matching sign or nullptr. First match in
 * registration order wins (deterministic). S-055: the scan probes this
 * layer ONLY after the built-in raw-sign table did not match at the
 * same stride (built-in-first, the M-048 collision rule); S-056:
 * `disabled_exts` (nullable) is the host's disabled-formats list - a
 * listed extension's signs never fire (the og's removal-loop
 * semantics), other extensions' signs still can. */
const UserSign* match_at(const u8* p, u64 avail, const char* disabled_exts = nullptr);

} // namespace usersigns
} // namespace carve
} // namespace cr

#endif // CR_USERSIGNS_H_
