# s3-ehb-ext4-dirwalk — executed comparison (S3-EHB, session 3)

**Question settled (CORE side):** does the core's ext4 dir walk advance by the
on-disk `rec_len` (skipping delete-merged space and inode-0 holes), and does
the merged deleted entry's NAME ever surface?

**Why it matters:** the og's `EXT4SearchDLL` dir-block walk
(`EXT4SearchDLL_decompiled.c` FUN_180005820, :4085-4283) advances by the
**computed** record size `8 + align4(name_len)` (:4160-4162 first entry,
:4173-4174 subsequent) — never by the on-disk rec_len — so after any
delete-merge (`ext4_generic_delete_entry` folds the freed entry into the
previous entry's rec_len) the og lands **on the stale deleted entry's bytes**
and re-emits it with its NAME (deleted-flagged via the dtime predicate at
:6438-6449). It also **stops the whole dir-block walk on the first inode==0
entry** (:4154-4155). Both claims are decompile-side; this execution pins the
core's opposite behavior.

**Fixture:** 1 KiB-block synthetic ext volume; root dir block =
`." ".." alpha.txt[rec_len=44 merged over beta-gone.txt] beta-gone.txt(stale,
inode 7, dtime=4242, links=0) hole(inode 0) gamma.txt(live)`.

**Command:** `sh build.sh` (g++ -std=c++20; 4 TUs; no source changes anywhere).

**Output (output.txt):**
```
scan rc=0 (OK) groups=1 files=2 dirs=0 deleted_inodes=1 journal_salvaged=0
Found: "/" deleted_state=0 logical_size=0
Found: "alpha.txt" deleted_state=0 logical_size=16
Found: "gamma.txt" deleted_state=0 logical_size=16
Deleted: "Inode7" deleted_state=1 logical_size=512
RESULT: PASS - core advances by rec_len (skips merged + inode-0 space); the merged entry's NAME never surfaces
exit=0
```

**Read-out:**
1. `beta-gone.txt`'s name appears NOWHERE in core output; the freed inode
   surfaces only as the nameless sweep node `Inode7` (Deleted branch, dtime
   predicate). The og surfaces the same file WITH its name (its computed-size
   walk re-reads the stale entry). → M-020.
2. `gamma.txt` (after an inode-0 hole) IS emitted — the core continues past
   inode-0 entries; the og terminates the walk at the first one. → N-036 sub-row.
3. Bonus pin: on 1 KiB-block volumes the core reads the GDT at block
   `first_data_block+1 = 2` (byte 2048, kernel-correct); the og reads its GDT
   at `base+block_size` = the SB block (:3420-3422) and therefore cannot
   resolve group 0's inode table on 1 KiB-block volumes. → I-013 sub-row.
   (The existing suite pins the same math: `tests/unit/test_ext4_volume.cpp:17`
   "GDT at block first_data_block+1 = 2", suite 317/317 at HEAD `34b3441`.)
