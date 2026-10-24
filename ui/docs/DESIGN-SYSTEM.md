# DESIGN-SYSTEM

Living design-system document for the Recovery Wizard UI. Token values live in
`src/styles/tokens.css` (single source of truth consumed by Tailwind v4
`@theme inline`); this doc explains the system, usage rules, and evolution.

## 1. Principles

1. **One focal point per surface** — a single primary CTA (brand gradient) per view;
   everything else is secondary (outline/ghost) or tertiary (text/link).
2. **Trust through restraint** — destructive actions gated and colored danger;
   warnings amber; capability claims always backed by connector data.
3. **4px grid** — all spacing/size tokens are multiples of 4 (8 for macro rhythm).
4. **Tabular data discipline** — numerics always tabular; sizes/dates via Intl.
5. **Non-color-only status** — every semantic color pairs with an icon or label.
6. **Dark/HC parity is a gate, not a variant** — tokens are triads (light/dark/HC).

## 2. Token architecture

```
tokens.css
  :root            — light values (oklch where stable, hex from VISUAL-SPEC)
  .dark            — dark overrides
  .hc / .hc.dark   — high-contrast overrides (no gradients, hard borders)
index.css
  @import "tailwindcss"; @import "tw-animate-css";
  @custom-variant dark (&:is(.dark *));
  @theme inline — maps --color-* → --background/--surface/… (shadcn v4 pattern)
  @layer base    — border-border, bg-background, text-foreground, font stack
```

Component classes use Tailwind utilities + `@variant` only; no ad-hoc hexes.
New tokens require a VISUAL-SPEC row with a source.

## 3. Color roles (summary — full table in VISUAL-SPEC §1)

- Canvas: blue-tinted gradient (light) / near-black (dark)
- Surface: white / #202428; tint fills for selection/hover
- Brand: `#0560fd` light, `#5094ff` dark; CTA gradient horizontal
- Semantic: success `#30b373`, warning `#ffc10e`/dark `#ff7342`, danger `#f72c2c`/dark `#ff5a5a`
- Neutral ladder for borders/dividers: 4 steps light / desaturated steel dark
- Shadows: `rgba(4,5,43,*)` family light / `rgba(31,31,31,*)` dark

## 4. Typography

Segoe UI first with Microsoft YaHei/Meiryo/Malgun fallbacks via `:lang()`.
Scale: 12 caption / 13 table+secondary / 14 body / 16 section / 20 page / 28 hero;
weights 400 body, 500 emphasis, 600 headings+buttons. Line-heights 1.3 (titles) →
1.45 (body). No font below 12px. All-numeric columns `tabular-nums`.

## 5. Component inventory (owned shadcn/ui v4 base, restyled)

button (primary gradient / secondary outline / ghost / danger / icon sizes sm–lg) ·
input · label · select · checkbox · radio-group · switch · slider · tabs · tooltip ·
popover · dropdown-menu · context-menu · dialog · alert · badge (incl. confidence) ·
progress (linear + circular gauge) · skeleton · scroll-area · separator · toast
(sonner) · table primitives · tree (custom) · virtualized list (custom) · stepper ·
segmented control · empty-state · stat-tile.

State coverage per component: default / hover / active / focus-visible / disabled /
loading / selected / (indeterminate where applicable) — verified by unit tests +
storybook-like state matrix in VISUAL-ACCEPTANCE.

## 6. Motion

`--dur-fast 120ms` micro, `--dur-base 200ms` panels, `--dur-slow 320ms` surfaces;
standard decelerating cubic-bezier. Progress and live counters animate linearly.
`prefers-reduced-motion` collapses to ≤100ms fades; never removes information.

## 7. Iconography

Lucide interim base (24px, 2px stroke, rounded joins) tinted via `currentColor`.
Filetype glyphs: original two-tone SVG set generated at 24/48 with type-tinted
accents. Interim assets tracked in INTERIM-ASSETS.md.

## 8. Layout system

- Shell: title bar 36px / sidebar 200px / content fluid; window 1240×840 target,
  800×600 floor.
- Results: 200 / 1fr / 300px three-pane with 4px gutters, collapsible preview.
- Density: comfortable (row 40px, pane padding 16) / compact (row 32, padding 12)
  via `[data-density]` attribute + token swap; no layout breakage at either.

## 9. Change process

1. Proposal → VISUAL-SPEC row (with source) → tokens.css change.
2. Component changes require updated state matrix + a11y check.
3. Breaking token renames logged in this file's changelog section below.

### Changelog

- v0.1.0 (this session): initial token set from corpus hard evidence + VLM census.
