# S8 evidence — the og's thumbnail pipeline (first-hand reads)

Read directly this session (session-8 wave 2) from
`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (corpus `be0cfe9`).

## 1. EXIF orientation applied BEFORE the 256-px thumbnail

`FUN_1402fcc30` (:521694-521725) — reads GDI+ property 0x112 (EXIF
Orientation) and applies RotateFlip (`FUN_1402fbfb0`), verbatim:

```c
uVar1 = FUN_1402fbff0(param_1,0x112);          /* GetPropertyItemSize(0x112) */
if (uVar1 != 0) {
  _Memory = malloc((ulonglong)uVar1);
  FUN_1402fc040(param_1,0x112,uVar1,_Memory);  /* GetPropertyItem */
  if (*(longlong *)((longlong)_Memory + 0x10) != 0) {
    switch(**(undefined1 **)((longlong)_Memory + 0x10)) {  /* orientation value */
    case 2: FUN_1402fbfb0(param_1,4); break;   /* RotateFlipTypes mapping   */
    case 3: FUN_1402fbfb0(param_1,2); break;
    case 4: FUN_1402fbfb0(param_1,6); break;
    case 5: FUN_1402fbfb0(param_1,5); break;
    case 6: FUN_1402fbfb0(param_1,1); break;
    case 7: FUN_1402fbfb0(param_1,7); break;
    case 8: FUN_1402fbfb0(param_1,3); break;
    ...
```

Call order in the still-image work item (`FUN_1403d75b0` region,
:652300-652315):

```c
pFVar4 = ...Create(this,0);        /* GDI+ bitmap from HGLOBAL stream */
FUN_1402fcc30(pFVar4);            /* :652307 - orientation RotateFlip  */
if ((pFVar4 != 0) && (iVar1 = FUN_1403d71a0(pFVar4), iVar1 != 0)) {
  FUN_1403d7840(param_1,pFVar4);  /* :652309 - the 256-px thumbnail    */
}
```

Orientation precedes thumbnailing — the og's thumbnails are rotated.

## 2. Width-derived 256-px scale, never-upscale below 257, white flatten

`FUN_1403d7840` (:652383-652412), verbatim:

```c
FUN_1402fbda0(local_58,0xff,0xff,0xff);       /* white background color  */
uVar1 = FUN_1403d71a0(param_2);               /* GetImageWidth           */
if (uVar1 < 0x101) {                          /* width < 257: NO scaling */
  local_2c = FUN_1402fc210(param_2,local_58,param_1 + 0x58); /* HBITMAP */
  FUN_1403d77c0(param_1, ...);
}
else {
  uVar1 = FUN_1403d71a0(param_2);
  local_38 = (double)uVar1 / 256.0;           /* scale = width/256.0     */
  uVar1 = FUN_1403d71a0(param_2);
  local_44 = (int)((double)uVar1 / local_38); /* out_w = w/scale = 256   */
  uVar1 = FUN_1403d71e0(param_2);             /* GetImageHeight          */
  local_48 = (int)((double)uVar1 / local_38); /* out_h = h/scale         */
  local_50 = FUN_1403d7220(param_2,local_44,local_48,0,0); /* GetImageThumbnail */
  ...
```

The og's scale is **width-derived**: a 1000×5000 portrait yields
256×1280 (height unbounded). The core's both-axis ≤256 box (registered
deliberate correction, preview.md) diverges by design.

## 3. Thumbnail success promotes the node to grade 2

`FUN_1402636e0` region (:422498-422518), verbatim:

```c
iVar2 = FUN_140089670(*(undefined8 *)(param_1 + 0x40));   /* get grade  */
if ((iVar2 != 2) && (...)) {
  local_f8 = *(undefined8 *)(param_1 + 0x40);
  FUN_140089630(local_f8,2);                               /* set grade 2 */
}
```

`FUN_140089630(node, grade)` is the grade setter identified in
`artifacts/DRWUI-subsystems--assistant-grading-preview.md:46`. A successful
thumbnail decode promotes the node's grade to 2 ("high") — a grading
side-effect the core's thumbnail engine does not have.

## 4. Still-image eligibility: ≤ 200 MB; media: ≥ 64 KiB

The work item `FUN_1402636e0` (:422375-422560): still images with
`size > 0xc800000` (200 MB) are skipped (:422428); media (video/audio
categories 8/9) with `size < 0x10000` (64 KiB) are skipped (:422464);
media requests carry the extent list (28-byte entries) in a 602-byte
socket header to the EaseUS_VideoViewer agent (ports 20171/20172,
:653821-653975); the agent answers `[602-byte header][≤1 MiB BMP]`
(:654551-654630).

## 5. Cache layer (UI-side, not engine)

`QPixmapCache::setCacheLimit(0x19000)` = 100 MiB (:1664946);
ThumbnailFactory three-map caches + `ClearCache` with a 600×1 s drain wait
(:420996-421290); runtime cascade `PreviewDataSource::ClearCache →
OfficePreviewStub::doKillAllOpenProcess → ThumbnailFactory::ClearCache`
(`artifacts/UIUX/30-interaction-driver-ground-truth.md:59-62`).

## Cross-checks against the core (read this session)

- `src/core/preview/cr_thumbnail.cpp` has no orientation/rotate/flip code
  (grep zero); `extract_metadata` reports orientation as a string only
  (cr_preview.h:137).
- `ThumbnailCaps` defaults (cr_thumbnail.h:59-65): `max_read = 4 MiB` —
  borrowed from the M-028 PreviewReader cost doctrine, NOT the og's 200 MB
  still-image eligibility gate.
- Grade promotion: `cr_thumbnail` is grade-agnostic (no call into
  grade_tree / S-023 surfaces).
