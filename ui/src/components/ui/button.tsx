import * as React from 'react';
import { Slot } from '@radix-ui/react-slot';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const buttonVariants = cva(
  'inline-flex items-center justify-center gap-2 whitespace-nowrap rounded-md text-sm font-semibold transition-[background-color,border-color,color,box-shadow] duration-100 outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-1 disabled:pointer-events-none disabled:opacity-50 [&_svg]:pointer-events-none [&_svg]:shrink-0',
  {
    variants: {
      variant: {
        primary:
          'gradient-cta gradient-glow text-primary-foreground hover:brightness-110 active:brightness-95',
        secondary:
          'border border-border-strong bg-surface text-foreground hover:bg-surface-hover active:bg-surface-selected',
        ghost:
          'text-foreground-secondary hover:bg-surface-hover hover:text-foreground active:bg-surface-tint',
        danger: 'bg-danger text-white shadow-sm hover:brightness-110 active:brightness-95',
        dangerOutline:
          'border border-danger bg-transparent text-danger hover:bg-danger-muted active:brightness-95',
        link: 'text-primary underline-offset-4 hover:underline',
      },
      size: {
        sm: 'h-7 rounded-xs px-2.5 text-xs [&_svg]:size-4',
        md: 'h-8 rounded-sm px-3.5 text-sm [&_svg]:size-4',
        lg: 'h-9.5 rounded-md px-5 text-sm [&_svg]:size-5',
        xl: 'h-11 rounded-md px-6 text-base [&_svg]:size-5',
        icon: 'size-8 rounded-sm [&_svg]:size-4',
      },
    },
    defaultVariants: { variant: 'secondary', size: 'md' },
  },
);

function Button({
  className,
  variant,
  size,
  asChild = false,
  loading = false,
  children,
  disabled,
  ...props
}: React.ComponentProps<'button'> &
  VariantProps<typeof buttonVariants> & {
    asChild?: boolean;
    loading?: boolean;
  }): React.JSX.Element {
  const Comp = asChild ? Slot : 'button';
  return (
    <Comp
      data-slot="button"
      className={cn(buttonVariants({ variant, size }), className)}
      disabled={disabled || loading}
      aria-busy={loading || undefined}
      {...props}
    >
      {loading ? (
        <span
          className="size-4 animate-spin rounded-full border-2 border-current border-t-transparent"
          aria-hidden
        />
      ) : null}
      {children}
    </Comp>
  );
}

export { Button };
