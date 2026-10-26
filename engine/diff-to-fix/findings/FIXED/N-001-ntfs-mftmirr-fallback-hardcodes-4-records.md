
## Session-2 main-auditor verification (2026-10-25)

`d7f3c78` verified against the decompile first-hand: the og derives mirror capacity from the
**mirror's own $DATA runlist** (`NTFSLib_decompiled.c :17407-13`, `GetFileRecordCount :17185`),
with an **inclusive** read gate (`:17460 if ((-1 < rec) && (rec <= *(this+0x70)))`) and a hard
abort on resident mirror $DATA (`"$MFTMirror::$DATA is resident!"` `:17379`). The fix's
runlist-derived span matches; the inclusive-bound nuance is recorded in the main-auditor
leg's evidence (`../evidence/decompiled-quotes/ntfs-pass2-guid-dump.md` §5) for the test
that pins it.
