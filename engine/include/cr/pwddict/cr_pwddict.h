/*
 * cr_pwddict.h - the password-dictionary unlock aid (diff-to-fix N-064).
 *
 * Corpus lineage: the reference ships a 228-entry password dictionary at
 * install root (og-codebase/Dict, 2,742 B, byte-verified in
 * artifacts/config-files/config-data-reference.md:20) wired as a
 * legitimate unlock aid to BitLockerLib's manage-bde -Password flow
 * (config-data-reference.md:86; BitLockerLib.md:52; the UI flow is the
 * user-consented BitlockerUnlockDialog -> BitlockerUnlockThread chain,
 * UIUX artifact 15-scenario-flows:33) and to the repair line's
 * password-aware archive repair (CheckPassWord / CheckPassWordByData,
 * FileRepair/archive-repair--7z-engine.md:23). This module ports the
 * MECHANISM: the dictionary format, the candidate sequencing, and the
 * consent-gated attempt loop. The repair-line consumers stay
 * scope-excluded (I-010).
 *
 * REGISTERED DIVERGENCE (docs/PARITY-DELTAS.md D-78): the og's exact
 * 228-entry asset is NOT copied into this repo (never copy proprietary
 * data files). The core ships an authored seed list of universal
 * common-password patterns (seed_dictionary()) and a loader that accepts
 * any dictionary in the og's exact file format - a host or user can
 * point the CLI at the og's own Dict file and get byte-format-compatible
 * behavior.
 *
 * CONSENT MODEL (the finding's parameter F61 - legitimate unlock only):
 * nothing in the engine or scan path ever calls try_passwords. It runs
 * only inside an explicit host unlock action (the CLI's
 * --bitlocker-pwddict flag / the UI's unlock dialog). Attempts go
 * through the SAME Windows unlock plumbing as a typed password
 * (PowerShell Unlock-BitLocker -Password with the manage-bde -Password
 * fallback) - Windows itself applies or rejects the password; the core
 * never touches BitLocker crypto.
 *
 * Attempt policy (the finding's open question - corpus is silent on
 * ordering/rate-limiting, so this is the core's documented policy):
 * sequential in given order, one attempt per candidate, first success
 * wins, no retry within one run; duplicate candidates are attempted
 * once (order-preserving dedup - a repeat attempt is a guaranteed
 * re-reject and each attempt is a real PowerShell round trip). Every
 * attempt is logged through the N-048 logging seam at Info with
 * progress and password LENGTH only - password text is never logged.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_PWDDICT_H_
#define CR_PWDDICT_H_

#include "../cr_types.h"
#include <cstddef>
#include <string>
#include <vector>

namespace cr {
namespace pwddict {

/* ---------- caps (hostile-input guards; corpus file is far inside) ----
 * The og's Dict: 228 entries, longest 20 chars, 2,742 B. Generous caps
 * so any real-world dictionary fits while a hostile 4 GiB "dictionary"
 * cannot exhaust memory. */
constexpr size_t kMaxEntries = 4096;
constexpr size_t kMaxEntryChars = 128;
constexpr size_t kMaxFileBytes = 256 * 1024;

/* Parse the og's Dict format, first-hand decoded from the corpus file:
 * one entry per line; every line except the last ends with a comma
 * followed by CRLF; the final entry carries neither comma nor line
 * terminator; entries are printable text and MAY contain spaces
 * ("charlie brown", "mickey mouse" - corpus first-hand). This parser
 * accepts LF as well as CRLF and treats the per-line comma as optional
 * (trailing comma stripped when present) so hand-edited dictionaries
 * work too. Empty lines and over-cap entries are skipped; the first
 * kMaxEntries valid entries win; input beyond kMaxFileBytes is refused
 * (returns 0, out untouched). Returns the number of entries appended. */
size_t parse(const char* data, size_t size, std::vector<std::string>& out);

/* The shipped seed dictionary (REGISTERED DIVERGENCE D-78): authored
 * universal common-password patterns, NOT the og's 228-entry asset.
 * ASCII-only, comma/newline-free, deduplicated - ready for the
 * sequencer. Hosts that want the og-exact list load it via parse(). */
const std::vector<std::string>& seed_dictionary();

/* One unlock attempt: return OK (0) when the candidate unlocked the
 * volume, any other value = rejected. The real flow shells out to
 * Windows (never in-process crypto); tests pass a recorder. */
using AttemptFn = u32 (*)(const std::string& password, void* user);

/* The consent-gated candidate sequencer (N-064's core loop): runs the
 * order-preserving dedup, attempts each candidate via `attempt` until
 * the first OK, logs progress through the N-048 seam (Info per attempt,
 * Warn on exhaust - password text never logged), and returns OK or
 * ERR_FVE_LOCKED (cr_error.h). `attempt` must be non-null.
 * attempts_made (optional) receives the number of real attempts run.
 *
 * There is deliberately NO auto-invocation path: the scan pipeline and
 * the orchestrator hold no reference to this function. */
u32 try_passwords(const std::vector<std::string>& candidates, AttemptFn attempt,
                  void* user, size_t* attempts_made = nullptr);

} // namespace pwddict
} // namespace cr

#endif // CR_PWDDICT_H_
