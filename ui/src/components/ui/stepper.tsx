import * as React from 'react';
import { cn } from '@/lib/utils';

/** Horizontal wizard stepper (used in header to show flow position). */
function Stepper({
  steps,
  current,
  className,
}: {
  steps: readonly string[];
  current: number;
  className?: string;
}): React.JSX.Element {
  return (
    <ol className={cn('flex items-center gap-2', className)} aria-label="progress">
      {steps.map((label, i) => {
        const state = i < current ? 'done' : i === current ? 'current' : 'todo';
        return (
          <li
            key={label}
            className="flex items-center gap-2"
            aria-current={state === 'current' ? 'step' : undefined}
          >
            <span
              className={cn(
                'flex size-5 items-center justify-center rounded-full text-2xs font-bold tabular',
                state === 'done' && 'bg-success text-white',
                state === 'current' && 'bg-primary text-primary-foreground',
                state === 'todo' && 'border border-border-strong text-foreground-disabled',
              )}
            >
              {i + 1}
            </span>
            <span
              className={cn(
                'hidden text-xs font-medium md:inline',
                state === 'todo' ? 'text-foreground-disabled' : 'text-foreground-secondary',
              )}
            >
              {label}
            </span>
            {i < steps.length - 1 ? (
              <span
                className={cn('h-px w-6', i < current ? 'bg-success' : 'bg-border')}
                aria-hidden
              />
            ) : null}
          </li>
        );
      })}
    </ol>
  );
}

/** Small two-option segmented control (view toggle, path/type). */
function Segmented<T extends string>({
  options,
  value,
  onChange,
  ariaLabel,
  className,
}: {
  options: readonly {
    value: T;
    label: string;
    icon?: React.ComponentType<{ className?: string }>;
  }[];
  value: T;
  onChange: (v: T) => void;
  ariaLabel: string;
  className?: string;
}): React.JSX.Element {
  return (
    <div
      role="radiogroup"
      aria-label={ariaLabel}
      className={cn(
        'inline-flex h-8 items-center gap-0.5 rounded-md bg-surface-muted p-0.5',
        className,
      )}
    >
      {options.map((opt) => {
        const Icon = opt.icon;
        const active = opt.value === value;
        return (
          <button
            key={opt.value}
            type="button"
            role="radio"
            aria-checked={active}
            onClick={() => onChange(opt.value)}
            className={cn(
              'inline-flex h-7 items-center gap-1.5 rounded-sm px-2.5 text-xs font-medium transition-colors outline-none',
              'focus-visible:ring-2 focus-visible:ring-ring',
              active
                ? 'bg-surface text-foreground shadow-sm'
                : 'text-foreground-secondary hover:text-foreground',
            )}
          >
            {Icon ? <Icon className="size-4" aria-hidden /> : null}
            {opt.label ? <span>{opt.label}</span> : null}
          </button>
        );
      })}
    </div>
  );
}

export { Segmented, Stepper };
