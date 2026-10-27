# BUILD-SUPPRESSIONS — every warning/error suppression, with rationale and scope

> Policy (mandate §7): no suppression may hide a real issue. Every entry is a
> deferred fix with an owner and a scope — never a permanent silencing. A
> suppression that is not in this table is a bug.

**This file intentionally carries ZERO entries.** The tree builds warning-clean
under gcc `-Wall -Wextra -Werror -Wshadow` and MSVC `/W4 /WX` (global flags in
`CMakeLists.txt`; every preset build is the Werror build). The historical
warnings-as-errors fixes (the MSVC `/W4` red waves, the clang fuzz lane's
48-warning backlog zeroing, C4244/C4459-class repairs) are recorded in the
repo worklog and `docs/error-fix-registry.md` — fixed, not suppressed.

When a suppression becomes genuinely necessary (e.g. a third-party header
warning under a new toolchain), it lands here with:

| ID | Suppression (file:line / flag) | Rationale | Scope | Deferred-fix tracking |
|---|---|---|---|---|
| — | — | — | — | — |
