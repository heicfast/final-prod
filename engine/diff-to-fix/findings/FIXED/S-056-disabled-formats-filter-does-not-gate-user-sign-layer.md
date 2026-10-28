# SEVERE S-056 — the disabled-formats filter does not gate the user-sign layer: turning a format OFF does not stop it being carved via the default user-sign set (D-104's own registration claim is false in code)

**Severity:** SEVERE
**Module:** carve (format-toggle gating)
**Parameters affected:** filter model (16), carving validator logic (28), detection correctness (1), API contract truthfulness
**Corpus references:** the og's only sign sources are all inside the RawConfig.ini `Filter` gate — `FUN_180006b30`'s analyser-removal loop (RAWSearchDLL, registered as M-024: "the og's filter removes the analyser from the working registry so its signs never fire"); RawConfig.ini read at RAWSearchDLL :13133
**Core references:** `src/core/carve/cr_carve.cpp:738-808` (the ungated user-sign block), `:836-844` (the raw-sign gate it lacks), `:1118-1131` (the disable-map build for the other two layers), `:438-439` (the grammar-layer gate); `docs/PARITY-DELTAS.md:3206-3207` (D-104's false claim); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

`disabled_formats` gates the grammar layer (trial rows, :438-439) and the built-in
raw-sign layer (:836-844), but the user-sign block (:738-808) has no disabled-format
check. With the default-on user-sign set (userdata-defaults.json), a host that disables
"cpl" (or "jpg", "exe", ...) still gets those formats carved through the user-sign
channel, tagged 0xFFFE0000. D-104's registration explicitly claims "the carve pass
remains gated by the same format-toggle/occupancy/bitmap rules as every other carve
source" — the format-toggle half is not implemented.

## What the reference does

The og's RawConfig.ini `Filter` removes analysers from the working registry
(`FUN_180006b30` removal loop, the M-024 decode) — every sign source the og has
(raw_signs.json table rows, grammar analysers) is inside that gate; a disabled format
produces nothing anywhere. The og has no user-sign channel (E-A/E-B), so "every carve
source" in og terms is exhaustive.

## What our core does

`cr_carve.cpp` — the disable maps are built once for the two stock layers:

```c
1118  if (flags.disabled_formats != nullptr && *flags.disabled_formats != 0) {
...
1124      for (u32 ti = 0; ti < kTrialCount; ++ti)
1125          ctx.trial_disabled[ti] =                    /* grammar layer */
1126              carve_format_listed(flags.disabled_formats, ctx.reg[kTrialOrder[ti]]);
1127      const rawsigns::SignIndex& idx = rawsigns::SignIndex::get();
1128      ctx.rawsign_disabled.assign(rawsigns::kFormatCount, false);
1129      for (u32 fi = 0; fi < rawsigns::kFormatCount; ++fi)
1130          ctx.rawsign_disabled[fi] =                  /* built-in raw-sign layer */
1131                  carve_token_listed(flags.disabled_formats, idx.format(fi).ext, nullptr);
1132  }
```

The raw-sign loop applies the filter to its matches:

```c
836  if (!ctx.rawsign_disabled.empty()) {
837      size_t w = 0;
838      for (size_t r = 0; r < matches.size(); ++r)
839          if (!ctx.rawsign_disabled[matches[r]->fmt])
840              matches[w++] = matches[r];
```

The user-sign block (:738-808) references neither map — occupancy/bitmap/free-map gates
apply (they precede it), the format toggle does not.

## Gap

For any format present in the default user-sign set, `disabled_formats` does not suppress
user-sign emission. Falsifiable: scan with `disabled_formats="cpl"` on a fixture with a
cpl magic → a 0xFFFE0000-tagged cpl record appears in the tree.

## Impact

User-observable wrong behavior on a documented control: the host's format toggle is
silently bypassed by the default-on channel (78 extensions in the shipped set are carried
by no stock source — the entire bypass surface). The registry's own truth is violated
(D-104:3206-3207), which misleads every future parity session that trusts the
registration. Restore pipelines honoring the toggle for filtering also see inconsistent
emission shapes.

## Reproduction / verification

Verified first-hand this session (lead auditor): read cr_carve.cpp:738-808 (no
disabled-format reference in the block), :836-844 (the gate shape for the built-in
layer), :1118-1131 (the two-map build with no user-sign map). D-104 text read at
docs/PARITY-DELTAS.md:3206-3207 ("the carve pass remains gated by the same
format-toggle/occupancy/bitmap rules as every other carve source"). Og contrast:
M-024's removal-loop decode (the corpus reference above).

## Fix plan (for the engineering team)

1. Build a third disable map at setup: `ctx.usersign_disabled` — but note user signs are
   keyed by extension string, not format id, so the gate belongs at match time:
   `usersigns::match_at` gains a predicate (or returns the ext and the caller checks
   `carve_token_listed(flags.disabled_formats, us->ext, nullptr)`) — the same token
   matcher the raw-sign layer uses (:1131).
2. Files to modify: `src/core/carve/cr_carve.cpp` (setup block :1118-1132 + the user-sign
   block :738-808), possibly `include/cr/carve/cr_usersigns.h` (match_at signature or an
   `is_disabled(ext)` helper).
3. Wire the same occupancy/bitmap semantics (already applied) — only the format toggle is
   missing.
4. Tests: below.
5. Contract/doc: after the fix, D-104's claim is true; no registration change needed
   (the registration was the intended truth, the code was the defect).
6. Acceptance criteria: with the default set loaded and `disabled_formats` naming any
   user-sign extension, zero records with that extension (tag 0xFFFE0000 or otherwise)
   appear; the same scan with the toggle empty is unchanged.

## Tests to add

- `Carve.DisabledFormatsGatesUserSignLayer` (default set + disable "cpl"; assert no cpl
  record) — fails today
- `Carve.DisabledFormatsGatesUserSignLayerCaseInsensitive` (the token matcher's case
  semantics pinned)
- `Carve.DisabledOtherFormatsLeaveUserSignsOn` (no over-blocking)

## Related findings

- S-055 (the same block's ordering inversion — fix in the same patch)
- N-171 (the same block's stats undercount)
- M-024 lineage (the og's filter semantics this must match)
- D-104 (the registration whose text this finding vindicates)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S47 lane, one carve patch with S-055 + N-171)
**Verified by:** `CarveEngine.DisabledFormatGatesUserSignLayer` (FAILED pre-fix: the disabled-ext record still carved; PASSES post-fix: sign_hits==0/validated==0/no 0xFFFE0000 records with the ext disabled, and the control leg with an unrelated ext disabled still fires) + full suite 1209/1167/0-fail.
**Resolution notes:** The user-sign hit path now checks `carve_token_listed(flags.disabled_formats, us->ext, us->ext)` before emission - the same toggle that gates the grammar layer (trial_disabled) and the raw-sign layer (rawsign_disabled), making D-104's "same format-toggle rules" registration true in code. The check rides the HIT path only (a magic actually matched - rare by construction), so the per-stride hot path pays nothing. The docs/PARITY-DELTAS.md D-104 row needs no text change (its claim was aspirational; the code now honors it). No og-side citation change: the og's Filter removes analysers from the working registry (FUN_180006b30, the M-024 decode) and the core now removes the user-sign channel's per-ext reach the same way.
