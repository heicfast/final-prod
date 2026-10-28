# MAJOR M-067 — the S-039 convergence residue: the caps fix landed without its ABI half — `cr_ui_user_sign` still `mime[64]`/`bytes[16]`, so a 73-char MIME surfaces non-NUL-terminated (a host string read runs past the field) and only 16 of a 44-byte sign surface

**Severity:** MAJOR
**Module / Surface:** carve / the user-signature ABI surface (modules/carve.md ABI notes; ABI-CONTRACT §7d)
**Parameters affected:** P42 (the ABI struct contract), C20-adjacent (the caps' consumer widths)
**Corpus references:** the og's shipped `EUUserData` (measured: max sign 44 B, max MIME 73 chars — the pptx office family); `artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` (the grammar)
**Core references:** `b07ee9d` (the S-039 caps fix — landed without touching `include/cr/cr_ui.h`); `include/cr/cr_ui.h` post-`b07ee9d` (`cr_ui_user_sign { char mime[64]; ... uint32_t sign_len; /* 1..16 */ uint8_t bytes[16]; }`); `src/ui/cr_ui.cpp` `cr_ui_user_sign_info` (`memcpy(out->mime, s.mime, sizeof(out->mime))` — exactly 64 bytes copied)
**Status:** filed and fixed in the same convergence landing (this session's M-067 commit)

## Summary

The S-039 caps fix (`b07ee9d`) raised the core's parse caps to the og's measured
maxima (48-byte signs, 80-char MIMEs) but did not touch the ABI struct that
surfaces them. `cr_ui_user_sign` still carried `mime[64]` and `bytes[16]`:
`cr_ui_user_sign_info` memcpy's exactly 64 bytes of a now-legal 73-char MIME
into `mime[64]` — **no NUL inside the field** — so a host reading `s.mime` as a
C string runs past the field into `offset`/`sign_len`/`bytes` (undefined
content). Likewise only 16 of a legal 44-byte sign's bytes surface
(`sign_len` says 44 while `bytes` is 16 — the host reads 28 bytes out of
bounds if it trusts `sign_len`).

## What the reference does

The og's engine consults EUUserData whole-file; its UI lists user formats
(the og's own surface renders the full MIME/type strings — 73-char office
MIMEs included, they are the shipped values).

## What our implementation does (at `b07ee9d`)

The parse layer accepts the og's extremes; the ABI layer truncates them and
can emit a non-NUL-terminated `mime[]` — the two halves of the same feature
diverged inside one commit.

## Gap

Falsifiable claim: register `{"img": {"signs": ["0,<44-B sign>"], "mime":
"<the og's 73-char pptx MIME>"}}` via `cr_ui_register_user_signs`, then
`cr_ui_user_sign_info(0, &s)` — at `b07ee9d` the field `s.mime` has no
terminator within its 64 bytes (`strlen(s.mime)` reads past the field) and
`s.sign_len == 44` while `sizeof(s.bytes) == 16`.

## Impact

- A host rendering the registered set (the documented enumeration use, cr_ui.h
  §7d) reads past `mime[64]` — UB at the consumer, on the feature's own
  advertised path with the og's own shipped data.
- The truncation is silent: no error, no cap rejection — wrong data out.

## Fix plan

1. Resize the struct fields to the v3 caps: `mime[81]`, `bytes[48]`,
   `sign_len` 1..48, `offset` bounded 0..1024 (the memcpy sizes then always
   NUL-terminate: `s.mime` holds ≤ 80 chars + NUL).
2. Bump `CR_UI_ABI_VERSION` 2 → 3 per the stability policy's struct-FIELD
   -change rule (the first such change; hosts must recompile — a v2 host's
   smaller allocation would be overrun by `cr_ui_user_sign_info`'s
   `sizeof(*out)` writes).
3. ABI-CONTRACT §9 (the v3 entry) + §7d (the widths note).
4. Test: `CrUi.UserSignAbiCarriesTheFullS039Widths` — the og's 73-char MIME +
   44-B sign round-trip whole and NUL-terminated (fail-before: RED at
   `b07ee9d`).

## Tests to add

- `CrUi.UserSignAbiCarriesTheFullS039Widths` (fails before).

## Related findings

- **S-039** (FIXED at `b07ee9d`) — the caps half this completes; the convergent
  landing's reconciliation record rides the worklog and RESOLVED-INDEX.
- **M-048** (FIXED) — the feature's original landing.

## Open questions

None — the widths are the measured og maxima (S-039's census); the
NUL-termination defect is mechanical (memcpy of a fixed field size from a
longer legal source).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-067 commit — the S-039 convergence landing)
**Verified by:** `CrUi.UserSignAbiCarriesTheFullS039Widths` (the og's 73-char
pptx MIME + 44-B img sign round-trip whole through
`cr_ui_register_user_signs` → `cr_ui_user_sign_info`: the MIME NUL-terminated
inside the field at [73], `sign_len` 44, `bytes[43]` surfaces). **Fail-before
PROVEN compile-RED**: the same test against the reverted v2 struct fails to
build — `-Werror=array-bounds: array subscript 127 is outside array bounds of
'cr_ui_user_sign [1]'` (the compiler's own model of the host-side OOB read
the finding describes) and `array subscript 43 is above array bounds of
'uint8_t [16]'`; GREEN on the v3 struct. Full suite 955/955 (+1 test; was
954 at `5e7390b`).
**Resolution notes:** Implementation per the fix plan verbatim: the struct
fields resized to the v3 caps (`mime[81]` — always NUL-terminated since the
core's `s.mime` holds ≤ 80 chars + NUL and the memcpy copies all 81;
`bytes[48]`; `sign_len` 1..48; `offset` 0..1024), `CR_UI_ABI_VERSION`
2 → 3 per the stability policy's struct-FIELD-change rule (the FIRST such
bump; ABI-CONTRACT §9 + §7d carry the v3 entry; the generic ABI tests track
the macro and stay green). The caps half (`b07ee9d`) and this widths half
converged this session — the reconciliation record rides the worklog and
RESOLVED-INDEX.
