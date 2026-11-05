#!/usr/bin/env python3
"""
test_installer_osgate.py - S-038 GATE 3 (InstallerOSGateMatrix, PACKAGING.md
section 11): the OS gate admits the supported envelope and REFUSES outside
it.

Two checks:

  SYNC PIN  - the OSGATE-V1 marker line (the decision-table contract) must
              be byte-identical in installer/osgate.ps1 and
              installer/core-recovery.iss - the ISS [Code] embeds the same
              table the module implements, and this pin is what keeps the
              two from drifting apart (a divergence here means the gate
              logic the matrix proves and the logic the installer runs are
              different tables - exactly the failure mode this gate exists
              to prevent).
  MATRIX    - drives installer/osgate.ps1 with injected version tuples
              across the envelope: every in-envelope OS (Win7 SP1, Win8,
              Win8.1, Win10, Win11, Server 2022, Server 2025) ADMITS with
              exit 0; every below-floor tuple (XP, Vista, Win7 RTM/SP0)
              REFUSES with exit 3; every non-x64 architecture REFUSES. The
              probes use the module's own injectable inputs so the refusal
              behavior is provable on any runner OS.

Run:  python tools/test_installer_osgate.py     (from the repo root)
      (requires pwsh on PATH - the module is PowerShell; the CI gate lane
       runs on a runner where pwsh is guaranteed)
Exit:  0 = sync pin holds + full matrix behaves, 1 = a regression.
"""

import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
MODULE = os.path.join(REPO, "installer", "osgate.ps1")
ISS = os.path.join(REPO, "installer", "core-recovery.iss")
MARKER = ("OSGATE-V1: admit (6.1.7601+ | 6.2 | 6.3 | 10.0+ on x64); "
          "refuse (below 6.1.7601, any x86/arm64)")

FAILURES = []


def expect(cond, label, detail=""):
    if cond:
        print(f"  ok: {label}")
    else:
        print(f"  FAIL: {label} {detail}")
        FAILURES.append(label)


def find_marker_line(path):
    """The marker line inside a comment: anchor on 'OSGATE-V1:' itself so
    the check is comment-style-agnostic (# PowerShell, // Pascal ISS, ; ini)."""
    with open(path, "r", encoding="utf-8-sig", errors="replace") as f:
        for line in f:
            idx = line.find("OSGATE-V1:")
            if idx >= 0:
                return line[idx:].strip()
    return None


def probe(major, minor, build, arch):
    """Run the module with an injected tuple; return (admit, reason)."""
    r = subprocess.run(
        ["pwsh", "-NoProfile", "-File", MODULE,
         "-Major", str(major), "-Minor", str(minor), "-Build", str(build),
         "-Arch", arch],
        capture_output=True, text=True, timeout=60)
    out = r.stdout.strip()
    try:
        decision = json.loads(out.splitlines()[-1])
    except Exception:
        decision = {"admit": None, "reason": f"unparseable output: {out!r} rc={r.returncode}"}
    return decision.get("admit"), decision.get("reason"), r.returncode


def main():
    print("check 1: the OSGATE-V1 sync pin (module == ISS decision table)")
    mod_marker = find_marker_line(MODULE)
    iss_marker = find_marker_line(ISS)
    expect(mod_marker is not None, "osgate.ps1 carries the OSGATE-V1 marker")
    expect(iss_marker is not None, "core-recovery.iss carries the OSGATE-V1 marker")
    expect(mod_marker == MARKER, "module marker is the canonical table", repr(mod_marker))
    expect(iss_marker == MARKER, "ISS marker is the canonical table", repr(iss_marker))

    have_pwsh = subprocess.run(["pwsh", "-NoProfile", "-Command", "exit 0"],
                               capture_output=True).returncode == 0
    if not have_pwsh:
        print("  FAIL: pwsh not on PATH - the matrix probes need it "
              "(the CI gate lane guarantees it)")
        FAILURES.append("pwsh available")
    else:
        print("check 2: the envelope matrix (in-envelope ADMITS, outside REFUSES)")

        # every in-envelope OS admits (exit 0)
        inside = [
            (6, 1, 7601, "x64", "Windows 7 SP1 (the floor)"),
            (6, 1, 7602, "x64", "Windows 7 SP1+ (post-SP1 rollup build)"),
            (6, 2, 9200, "x64", "Windows 8"),
            (6, 3, 9600, "x64", "Windows 8.1"),
            (10, 0, 10240, "x64", "Windows 10 1507"),
            (10, 0, 19045, "x64", "Windows 10 22H2"),
            (10, 0, 22000, "x64", "Windows 11 21H2"),
            (10, 0, 26100, "x64", "Windows 11 24H2 / Server 2025"),
            (10, 0, 20348, "AMD64", "Windows Server 2022 (AMD64 arch spelling)"),
        ]
        for major, minor, build, arch, label in inside:
            admit, reason, rc = probe(major, minor, build, arch)
            expect(admit is True and rc == 0,
                   f"ADMIT {label} ({major}.{minor}.{build} {arch})", f"got {admit} rc={rc}: {reason}")

        # every below-floor tuple refuses (exit 3)
        below = [
            (5, 1, 2600, "x64", "Windows XP x64"),
            (6, 0, 6001, "x64", "Windows Vista SP1"),
            (6, 0, 6002, "x64", "Windows Vista SP2"),
            (6, 1, 7600, "x64", "Windows 7 RTM (SP0 - below the SP1 floor)"),
            (6, 1, 6002, "x64", "misreported Vista tuple on the 6.1 line"),
        ]
        for major, minor, build, arch, label in below:
            admit, reason, rc = probe(major, minor, build, arch)
            expect(admit is False and rc == 3,
                   f"REFUSE {label} ({major}.{minor}.{build} {arch})", f"got {admit} rc={rc}: {reason}")

        # any non-x64 architecture refuses (the driver's PE contract is AMD64)
        wrong_arch = [
            (10, 0, 19045, "x86", "32-bit Windows 10"),
            (10, 0, 26100, "ARM64", "Windows 11 on ARM64"),
            (6, 1, 7601, "IA64", "Itanium (the reference's dead row)"),
        ]
        for major, minor, build, arch, label in wrong_arch:
            admit, reason, rc = probe(major, minor, build, arch)
            expect(admit is False and rc == 3,
                   f"REFUSE {label}", f"got {admit} rc={rc}: {reason}")

    print()
    if FAILURES:
        print(f"test_installer_osgate: {len(FAILURES)} REGRESSION(S): {FAILURES}")
        return 1
    print("test_installer_osgate: sync pin holds + full matrix behaves "
          "(InstallerOSGateMatrix GREEN)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
