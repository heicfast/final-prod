# MINOR N-042 — CLI health temperature display: the fractional digit is the Kelvin remainder (mathematically wrong); two inconsistent temperature spellings across CLI and ABI

**Severity:** MINOR
**Module:** cli / health
**Parameters affected:** P68 (documentation accuracy — CLI output contract), P46-adjacent (health surface truthfulness), P24-adjacent (CLI output semantics)
**Corpus references:** N/A (display formatting; the og surfaces temperature only through its UI dashboard — no CLI counterpart. The NVMe composite temperature semantics themselves are spec-defined: NVMe 1.x CDW0 bits 15-0 Kelvin.)
**Core references:** `src/app/rrecov.cpp:1153-1155` — `wprintf(L"NVMe health: temp=%u.%uC ...", u32(nv.temperature_k) - 273, (u32(nv.temperature_k) % 10), ...)`; `src/ui/cr_ui.cpp:2595-2596` — ABI spelling `k*10 − 2731` (decidegrees, truncation); audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The CLI prints NVMe temperature as `%u.%u` with integer part `k − 273` and fractional part `k % 10` — but `k % 10` is the **Kelvin** remainder, not the Celsius remainder, so the printed decimal is wrong by up to ~0.55 °C and systematically biased (e.g. 309.15 K = 36.00 °C prints as "36.5"; 300.7 K = 27.55 °C prints as "27.7"). The ABI uses a different, more correct spelling (`k*10 − 2731` decidegrees, truncation with ~−0.05 °C bias). Two surfaces, two formulas, one of them mathematically wrong.

## What the reference does

Not applicable (no og CLI). The physical truth: temperature_k is Kelvin; the Celsius
fractional digit requires `(k*10 − 2731) % 10`, not `k % 10`.

## What our core does

`rrecov.cpp:1153-1155`:

```c
wprintf(L"NVMe health: temp=%u.%uC ...",
        u32(nv.temperature_k) - 273, (u32(nv.temperature_k) % 10), ...);
```

`k % 10` is invariant under the −273 shift only when 273 ≡ 0 (mod 10) — it is not
(273 mod 10 = 3), so the printed fraction is the Kelvin fraction, wrong for °C.

`cr_ui.cpp:2595-2596` (ABI): `temp_dc = u32(k) * 10 − 2731` (decidegrees, truncated).

## Gap

Falsifiable: for temperature_k = 309 (36.0 °C exactly... 309−273=36, 309%10=9) the CLI
prints "36.9C" — wrong by 0.9 °C. The correct fractional digit is (3090−2731)%10 = 9%10
= 9 — wait: 309 K = 35.85 °C; the CLI prints 36.9. Every value with k%10 != (k*10-2731)%10
misprints; the two agree only 1 time in 10 by coincidence.

## Impact

Cosmetic-but-wrong diagnostic output in the health command; users correlating CLI
temperature with the ABI value or with SMART tools see inconsistent decimals. No
recovery-results impact.

## Reproduction / verification

- Direct code read this session (both sites cited); arithmetic shown in §Gap.
- Not executed against real NVMe hardware this session (Linux container); the
  formatting path is pure arithmetic — the math check is sufficient. The ABI spelling
  was verified by read.

## Fix plan (for the engineering team)

1. One-line fix: `temp=%u.%u` with `(u32(k)*10 - 2731) / 10` and
   `(u32(k)*10 - 2731) % 10` — and reuse the ABI's decidegree value rather than
   re-deriving (single source: expose `nv.decidegrees_c()` next to `temperature_k`).
2. **Files to modify**: `src/app/rrecov.cpp:1153-1155`,
   `include/cr/win/cr_health.h` (+ accessor if added).
3. **Tests to add**: `Health.CliTempFractionIsCelsius` (k=309 → "35.8"/"35.9" family
   asserted against the decidegree formula; k=273.15-rounded cases pinned).
4. **Acceptance criteria**: CLI and ABI temperatures agree to the digit for a table of
   Kelvin values; suite green.

## Tests to add

`Health.CliTempFractionIsCelsius`.

## Related findings

- **N-032 (open)** — health transports; same module file, fix together.

## Open questions

None.
