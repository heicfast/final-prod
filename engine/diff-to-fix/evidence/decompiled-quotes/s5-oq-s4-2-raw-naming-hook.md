# Evidence — OQ-S4-2 RESOLVED: the RAW naming hook's engine-side consumption decoded

> Session 5 (2026-10-26). Audited core `7cf6754`; corpus `03ad46c`.
> Decode: session-5 lead, first-hand, from
> `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` (RAW:),
> `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (DRWUI:), and an
> RTTI-driven PE vtable extraction of `og-codebase/RAWSearchDLL.dll`
> (CRAWSearchController type descriptor → COL → vftable dump, slots 0-17).

## 1. The registration (confirmed, extends M-029's cite)

DRWUI `FUN_140581...` at DRWUI:917271-917296: factory
`EUCreateRAWSearcher` (GetProcAddress on DAT_1425e6d78) → controller object;
builds a `std::_Func_impl_no_alloc<string __cdecl(string, string)>` on the
stack (vftable + invoker `FUN_140161fb0`); registers it via
`(**(code **)(*plVar10 + 0x48))(plVar10, &func)` — controller vtable slot
+0x48.

## 2. The setter (engine side)

RTTI extraction of `CRAWSearchController::vftable` (this session):
slot 0x48 → **FUN_18000b8d0** (RAW:8695-8705):

```c
void FUN_18000b8d0(longlong param_1,longlong *param_2,undefined8 param_3,undefined8 param_4)
{
  FUN_18000dbc0((longlong *)(param_1 + 0x30),(longlong)param_2);   // move-store the std::function
  plVar1 = (longlong *)param_2[7];                                  // destroy the source's impl
  if (plVar1 != 0) { (**(code **)(*plVar1 + 0x20))(plVar1, plVar1 != param_2); param_2[7] = 0; }
}
```

`FUN_18000dbc0` (RAW:10350+) stores the impl copy into the controller's own
std::function at **this+0x30** (impl pointer lands at **this+0x68**). A
second setter `FUN_18000ba30` (RAW:8762-8777) stores another hook at
**this+0x78** (impl at this+0xb0) — no DRWUI registration site found for it
this session (only one hook is registered by the UI).

## 3. The invocation (the single consumption site)

The invoker helper **FUN_18000e160** (RAW:10678-10692) — the ONLY
`std::_Xbad_function_call` site in the DLL:

```c
undefined8 FUN_18000e160(longlong param_1,undefined8 param_2)   // (funcbase@ctrl+0x30, sret result)
{
  if (*(longlong **)(param_1 + 0x38) == 0) { std::_Xbad_function_call(); ... }
  (**(code **)(**(longlong **)(param_1 + 0x38) + 0x10))();       // _Do_call at impl-vtable+0x10
  return param_2;
}
```

**Single call site: RAW:9090**, inside `FUN_18000beb0` (RAW:9006 —
CRAWSearchController vtable slot 13 (0x68), invoked externally through the
controller interface; no in-file callers):

```c
if (*(longlong *)(param_1 + 0x68) != 0) {            // naming hook registered?
    // build string local_1070 = ""
    // build string local_1090 = "filter_generalraw"   (17 chars, RAW:9076/9084 — the ONLY
    //                                                     filter_* literal in any SearchDLL)
    param_3 = local_1090;  param_4 = local_1070;     // args: ("filter_generalraw", "")
    FUN_18000e160(param_1 + 0x30, local_1050);       // invoke hook -> display name
    ...
    FUN_180057b80((longlong)local_1028, puVar7, param_3, param_4);  // build the filter tree
    if (*(int *)(node + 0x18) == 6) { ... }          // type-6 = filter/category node; count children
    FUN_1800579b0 / FUN_180057e80(...)               // attach the display name to the nodes
}
```

## 4. The hook implementation (DRWUI side — the semantics)

`FUN_140161fb0` (DRWUI:263190-263210) → `FUN_140160ab0`
(DRWUI:263193-263290):

1. Looks the key up in a **local map at ctx+0x70** — hit ⇒ logs
   `"local firebase %s : %s\r\n"` and returns the mapped name.
2. Miss ⇒ calls a callback at ctx+0x18 (vtable+0x28) with the key and a
   0x1000-byte result buffer (an external name-lookup service).
3. Special-cases keys containing **"url_"** (strstr, DRWUI:263245+).
4. Always assigns the resulting string to the return value — **the hook
   always returns a name; it never drops/empties** — and the engine only
   uses it to NAME a node.

The map at ctx+0x70 is loaded by `FUN_140161460` (DRWUI:263298-263340):
`fopen("<appdir>/firebase.json", "rb")`, fread 0x1000, JSON-parse into the
map.

## 5. Resolution semantics (the answer to the OQ)

- **Rename or gate?** — **Display-name mapping ONLY.** The single engine
  consumption site invokes the hook once, with the reserved key
  `"filter_generalraw"` and an empty second string, to obtain the
  user-visible name of the **general-RAW filter/category node (tree node
  type 6)** in the RAW result tree. No per-file rename, no emission gating,
  no filter push — M-029 §Gap (b)'s "server-side per-file naming/filter
  mechanism" is corrected to "category display-name mapping, single site,
  single key".
- The og's DRWUI maps "filter_generalraw" → localized display name via
  `firebase.json` (a remote-config-style local cache) with an external
  callback fallback.
- Core impact (M-029 fix plan step 1): the core's RAW family tree emits a
  family/category name directly (no hook needed for an engine-only product);
  the parity decision is (a) name the general-raw bucket consistently with
  the og's localized display name source, and (b) optionally expose a
  name-override callback on the scan API for UI consumers. The decoded
  reserved key is `filter_generalraw`.

## Reproduction

- Vtable extraction: RTTI type descriptor `.?AVCRAWSearchController@@` in
  RAWSearchDLL.dll (file off 0x91c98) → COL at RVA 0x757d8 → vftable at RVA
  0x66e38; slot dump by the session-5 script (slots 0-17 printed; slot 0x48
  = FUN_18000b8d0, slot 0x68 = FUN_18000beb0).
- Grep corroboration: `filter_` string literals across all 9 SearchDLL
  decompiles → only `"filter_generalraw"` (2 sites, both in RAWSearchDLL);
  `_Xbad_function_call` → only RAW:10685 (one invoker); `FUN_18000e160`
  callers → only RAW:9090.
