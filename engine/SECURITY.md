# Security policy

## BitLocker: legitimate unlock only

The engine's BitLocker support is deliberately narrow
(`src/win/cr_bitlocker.cpp` — "detect + legitimate unlock only"):

- The core **assists volumes the OS already unlocked or can auto-unlock**
  via the live `FVEAutoUnlock` registry store (the reference product's
  own mechanism) and the offline SYSTEM hive — reading only
  already-available external-key material, exactly like the reference.
- It **refuses** to touch volumes it cannot legitimately unlock: the
  pre-unlock gate (`bitlocker::status` / `try_auto_unlock`, else
  explicit refusal with unlock guidance) is enforced on BOTH the CLI
  and the cr_ui ABI path (M-033, `diff-to-fix/findings/FIXED/`).
- There is **no FVEK / master-key extraction**, no decryption-key
  recovery, and no attack surface against locked volumes. Unlocking a
  locked volume is out of scope by design and by policy.

## Destructive operations are consent-gated

Partition-table apply (with undo), RawFixer-class repair
(analyze → dry-run → **consent** → apply → rollback), and BCD/NVRAM
boot-chain repair all follow the default-deny consent model documented
in `docs/CONNECTION-GUIDE.md` §6. The safe recovery writer refuses
destination overlaps outright (no silent overwrite of the source
media).

## Transport boundary

The raw-disk transport (`rda-driver`) publishes its control device
with an admin/SYSTEM-only DACL — the DACL is the security boundary;
elevation is required to install the kernel service. The engine
accesses raw devices under the caller's own privileges and never
modifies the driver's ACLs.

## No persistent service (M-105, the no-service posture)

The core never installs a persistent service and never persists key
material. The og ships a transient LocalSystem SCM service
("Read FVE Server", `DRWService.exe`) so a non-elevated host can read
the SYSTEM-ACL'd FVEAutoUnlock store, and persists the harvested keys
as plaintext `auto.txt`; the core reads the same store in-process
under the caller's own token and, when that token cannot read it,
surfaces the honest diagnosis (`ERR_FVE_ELEVATION_REQUIRED`) instead
of bridging the ACL. No SCM create/start/stop/delete code exists
anywhere in the core (PARITY-DELTAS D-166).

## Supported floor

Every shipped PE loads on **Windows 7 SP1** — enforced in CI by the
PE import allowlist gate (`tools/verify_pe_um.py`). Do not add imports
without extending the allowlist with justification.
