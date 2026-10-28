# MAJOR M-071 — rda-driver FileVersion pinned at 1.0.0.0 while releases tag v0.2.x: the M-064 versioned-coexistence scheme is functionally inert

**Severity:** MAJOR
**Module / Surface:** driver-install (the rda-driver deployer's versioned layout + the binary's version resource)
**Parameters affected:** P-inst-15 (the `%s%d` versioned coexistence contract — the og's setupdrvx64 scheme), P-inst-16 (in-place upgrade), the release/version-key contract
**Corpus references:** the og's `%s%d` versioned coexistence: `artifacts/PartitionRecovery/per-binary/setupdrvx64.md` (the `-Layout Versioned` design's reference); `architecture/02-kernel-driver-architecture.md` §5 (driver install lifecycle); the epmntdrv-trio artifact's `%s%d` numbered-variant enabling (`artifacts/PartitionRecovery/epmntdrv-trio--naming-pid-handshake.md` :34-39 — runtime-composed names let `eprdtdrv2` etc. coexist); prior trail: M-064 (FIXED) fix plan + rda-driver worklog M064-REMAINDER
**Core references (rda-driver @ 323e75a, v0.2.4):** `src/driver/rda_version.h:5-8` (MAJOR 1 / MINOR 0 / "1.0.0.0"), `src/driver/rda.rc:20-21,38` (FILEVERSION/PRODUCTVERSION 1,0,0,0 + FileVersion string), `src/driver/rda.inf:22` (DriverVer 01/01/2026,1.0.0.0), `scripts/install.ps1:99-148` (the Versioned layout + upgrade branch); the GET_VERSION/GET_INFO wire values (`src/shared/inc/rda_ioctl.h` — 0x604 @+8 / (1<<32)|0 / session 0x00010100)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** expanded on prior finding M-064 (FIXED) — its resolution assumed a per-release version resource; the rda-driver worklog recorded the pin as an "installer note for the packaging lane" (same-build reinstalls) but the cross-version consequence (the scheme never triggering between v0.x releases) was not owned by a finding

## Summary

The M-064 fix shipped `-Layout Versioned` (the default): the deployer copies
the driver to `<ServiceName><MajorMinor>.sys`, taking MajorMinor from the
binary's **version resource**, and treats a different computed destination as
an in-place upgrade. But the binary's version resource is pinned at
**1.0.0.0** across all v0.x releases (`rda_version.h` MAJOR=1/MINOR=0), while
the release tags advance (v0.2.3 → v0.2.4). Every v0.x build therefore
computes the same destination `<name>10.sys`, the upgrade condition
(`$imagePath -ne $destPath`) is never true, and **every reinstall over an
existing service exits 2** demanding `-Uninstall` first. The versioned
coexistence and in-place upgrade — the feature M-064 shipped — cannot ever
trigger between our own releases until the version resource is un-pinned.
The wire version surfaces (GET_VERSION, GET_INFO, the session protocol
version 0x00010100) are pinned to the same 1.0 values, so the wire cannot
distinguish releases either.

## What the reference does

The og's setupdrvx64 deploys the EPMNtDrv family with `%s%d`-composed
versioned image names (the mechanism the trio artifact documents as enabling
`eprdtdrv2`-style coexistence: an in-use image is never overwritten, a new
version installs beside/replaces by repointing the service). The og's
binaries carry per-release version resources (the corpus's per-binary
identity cards record FileVersion values that track the product's releases),
so the scheme's "different version → different image name → in-place
repoint" path is live in the reference deployment.

## What our implementation does

- `rda_version.h:5-8`: `RDA_VERSION_MAJOR 1`, `RDA_VERSION_MINOR 0`,
  `RDA_VERSION_FILE_STRING "1.0.0.0"` — unchanged across v0.2.1..v0.2.4
  (verified: the tags move, the header does not — `git log src/driver/
  rda_version.h` shows no release-coupled bumps).
- `rda.rc:20-21` `FILEVERSION/PRODUCTVERSION RDA_VERSION_MAJOR,
  RDA_VERSION_MINOR, 0, 0`; `:38` the FileVersion string; `rda.inf:22`
  `DriverVer = 01/01/2026,1.0.0.0`.
- `install.ps1:104-115`: `$tag = "$($vi.FileMajorPart)$($vi.FileMinorPart)"`
  → always `"10"` → `$destName = "<ServiceName>10.sys"`.
- `install.ps1:132-135`: the upgrade branch requires
  `($imagePath -ne $destPath)`; with every release computing the same
  destPath, an existing service with the same path is not an upgrade →
  `exit 2` ("Service already exists. Remove it first (-Uninstall …)").
- Wire side: GET_VERSION returns `(1<<32)|0`, GET_INFO version fields and
  the session protocol version `0x00010100` are all 1.0-pinned.

## Gap

Falsifiable claim: for any two releases vX and vY of rda-driver with
X,Y ≥ 0.2 and X ≠ Y, `install.ps1 -Layout Versioned` computes the identical
destination file name for both, and the in-place upgrade branch is
unreachable; installing Y over a service installed from X exits 2. The
M-064 acceptance criterion ("installing a different version over an
existing service reconfigures it in place") therefore fails for every
actual pair of our releases.

## Impact

- **Deploy workflow:** every driver upgrade requires a manual
  `-Uninstall`/uninstall.ps1 pass first — the exact friction M-064 was
  filed to remove. The core repo's installer lane (S-038's internal driver
  step) and the documented exit-code contract inherit the same behavior.
- **Coexistence:** the "an in-use image is never overwritten" guarantee is
  trivially satisfied (nothing ever installs a second name), so the
  guarantee is untested in practice — no release has ever exercised the
  versioned path.
- **Diagnostics:** ARP/version-key and wire version probes all report
  1.0.0.0 regardless of the running release; correlating a deployed
  driver with a GitHub release tag requires the file's timestamp or hash.

## Reproduction / verification

1. `rda_version.h` + `rda.rc` + `rda.inf` read directly (values above).
2. `install.ps1:104-148` read directly; the tag-derivation and the
   `-ne $destPath` upgrade condition quoted in the finding.
3. Logical proof: tag = f(MajorMinor of version resource) = "10" for every
   v0.x build; destPath is a pure function of ServiceName + tag; therefore
   destPath(release X) == destPath(release Y) for all v0.x X,Y; therefore
   `$isUpgrade` is false whenever the existing service points at the
   versioned name; therefore exit 2.
4. The rda-driver worklog's own M064-REMAINDER entry records the pin as a
   packaging-lane note (same-build case only) — this finding extends it to
   the cross-version case, which is the scheme's entire purpose.

## Fix plan (for the engineering team)

1. Decide the versioning contract: the release tag (v0.2.4 → FILEVERSION
   0,2,4,0 with FileVersion "0.2.4.0") is the natural mapping — but note
   the og-family convention uses the *driver's own* resource, and our
   service-name compat targets (`eprdtdrv` drop-ins) do not depend on the
   version value. Either mapping works; what matters is that the value
   changes when the release changes.
2. Files to modify (one commit, both repos coordinated):
   - `rda-driver/src/driver/rda_version.h` — derive from a single
     release-pinned define (e.g. `RDA_VERSION_RELEASE 0,2,4` triplet),
     documented as the tag mirror;
   - `rda-driver/src/driver/rda.rc` + `rda.inf` — consume the same
     defines (rda.inf's DriverVer date+version);
   - the wire surfaces in `rda_ioctl.h`/`rda_ident.c`(GET_VERSION)/
     `rda_ioctl.c` (GET_INFO + session 0x00010100) — decide whether they
     stay 1.0-pinned (documented: wire-stable by design, matching the og's
     EUDCPEPM protocol-version semantics) or follow the release; if they
     stay, register the decision in `docs/DRIVER-CONTRACT.md` and the
     ABI notes so the pin is deliberate, not accidental;
   - `rda-driver/scripts/install.ps1` — no change needed (the scheme is
     correct once the resource moves); optionally extend the no-resource
     fallback warning to name the expected define.
3. CI: add a gate asserting `rda_version.h`'s triplet equals the tag
   (the release workflow knows the tag; a mismatch fails the release) so
   the pin cannot silently regress.
4. Tests: `test_installer_versioned_upgrade.ps1` (or the existing
   integration lane): install vA (stub a build with MajorMinor A), install
   vB over it → no exit 2, service repointed to `<name>BA-tag.sys`, old
   image still on disk; same-version reinstall → exit 2 (the documented
   refuse). Execute on the windows installer lane (the S-038 lane's
   pattern).
5. Contract/doc updates: README's exit-code contract gains the
   cross-version upgrade behavior; `docs/DRIVER-RELEASE.md` documents the
   tag→resource mapping rule; the core repo's
   `diff-to-fix/modules/driver-install.md` row flips when landed.

## Tests to add

- `DriverInstall.VersionedUpgradeCrossRelease` — vA→vB in-place upgrade
  (repoint + old image retained), fails before the un-pin.
- `DriverInstall.SameVersionReinstallRefused` — exit 2 preserved (the
  deliberate behavior).
- `DriverInstall.WireVersionStability` (if wire stays 1.0) — GET_VERSION/
  GET_INFO/session version unchanged across the resource bump (pins the
  deliberate decision).
- CI: `Release.TagMatchesVersionResource` gate.

## Related findings

- M-064 (FIXED) — the feature this finding renders inert; its resolution
  addendum should link here.
- M-072 — the same deploy surface's WinPE profile defects.
- S-036/M-062 (FIXED) — the install.ps1 exit-code contract this interacts
  with (exit 2 remains truthful: the message already tells the user to
  uninstall or use a new-version image — the new-version image is what
  never arrives).

## Open questions

None blocking. The wire-version decision (follow the release vs stay
1.0-by-design) is an engineering choice to register, not an unknown.
