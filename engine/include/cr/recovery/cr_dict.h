/*
 * cr_dict.h - the password-dictionary unlock aid (N-064, diff-to-fix).
 *
 * Corpus lineage: the og ships a 228-entry common-password dictionary
 * (og-codebase/Dict, 2,742 B - config-data-reference.md :20, byte-
 * verified) wired to two consumers: BitLockerLib's manage-bde -Password
 * unlock flow (the user-consented "unlock with password" dialog,
 * bitlockerlib--bitlocker-implementation.md :39-44) and EuOfficeRepairFile's
 * encrypted-HWPX repair (OUT OF SCOPE here - the repair chain, I-010).
 *
 * Capability parity, not a bypass: the dictionary is an unlock AID inside
 * the user-initiated unlock action only (parameter F61 - legitimate
 * unlock; the og's flow is user-consented). Nothing in the scan path may
 * call try_passwords(); a locked volume still refuses with ERR_FVE_LOCKED
 * until the user explicitly asks for the password attempt (the CLI's
 * --bitlocker-dict / the cr_ui dictionary unlock action). Pinned by
 * Bitlocker.DictionaryUnlockConsentGated (win lane) and the pure-core
 * attempt-loop tests (Linux lane).
 *
 * Attempt policy (documented, per the finding's open question): sequential
 * in shipped order (numeric-first, then common words), user-provided
 * candidates AFTER the shipped set, stop at the first success, no
 * parallelism, no retry of failed candidates. Attempt lines carry the
 * candidate INDEX only - password material is never logged (the og's
 * qa-logs channel carries no password text either).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_DICT_H_
#define CR_DICT_H_

#include "../cr_types.h"
#include <cstddef>
#include <string>
#include <vector>

namespace cr {
namespace dict {

/* The shipped dictionary: 228 entries in the og's file order. */
extern const char* const kDictEntries[228];
constexpr u32 kDictEntryCount = 228;

/* Accepted candidate caps (corpus observed: lengths 2..20; 32 is the
 * documented accept bound - manage-bde password prompts historically cap
 * at this class of length). */
constexpr size_t kMaxPasswordLen = 32;
constexpr size_t kMaxCandidates = 4096; /* parse bound: hostile blob guard */

/* Parse a dictionary file's bytes in the og's Dict grammar: lines
 * terminated by CRLF (LF tolerated), every line except the last is
 * "<password>," (comma-terminated), pure ASCII, empty lines skipped.
 * Non-ASCII bytes reject the whole parse (the og's file is ASCII; a
 * non-ASCII candidate could not ride the manage-bde command line
 * verbatim). Returns false (out untouched) on grammar violations or when
 * the candidate count would exceed kMaxCandidates. */
bool parse(const char* bytes, size_t len, std::vector<std::string>& out);

/* One attempt outcome through the injected attempt function (u32 error
 * code; OK (0) = the candidate unlocked the volume). */
struct AttemptResult {
    u32 attempted = 0;        /* candidates tried (shipped + user) */
    u32 success_index = 0;    /* 1-based position of the winner (0 = none) */
    bool from_user_set = false; /* winner came from the user candidates */
    u32 rc = 0;               /* the failing error code when !success */
};

/* The consent-gated attempt loop - the og's dictionary flow. Tries every
 * shipped entry in order, then the user-provided candidates, stopping at
 * the first OK. Every attempt is reported through the cr::log seam
 * (N-061) as an Info line carrying the index only
 * ("dict unlock attempt %u/%u" / the success line names the source set,
 * never the password). The attempt function is the unlock plumbing seam:
 * the Windows layer passes the real PowerShell/manage-bde unlock; tests
 * pass a recorder. */
AttemptResult try_passwords(u32 (*attempt)(const char* password, void* user), void* user,
                            const std::vector<std::string>* user_candidates = nullptr);

} // namespace dict
} // namespace cr

#endif // CR_DICT_H_
