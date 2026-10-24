import * as React from 'react';
import * as DropdownMenuPrimitive from '@radix-ui/react-dropdown-menu';
import * as ContextMenuPrimitive from '@radix-ui/react-context-menu';
import * as PopoverPrimitive from '@radix-ui/react-popover';
import { CheckIcon } from 'lucide-react';
import { cn } from '@/lib/utils';

// ---------------------------------------------------------------------------
// Dropdown menu
// ---------------------------------------------------------------------------

const DropdownMenu = DropdownMenuPrimitive.Root;
const DropdownMenuTrigger = DropdownMenuPrimitive.Trigger;

function DropdownMenuContent({
  className,
  sideOffset = 4,
  ...props
}: React.ComponentProps<typeof DropdownMenuPrimitive.Content>): React.JSX.Element {
  return (
    <DropdownMenuPrimitive.Portal>
      <DropdownMenuPrimitive.Content
        sideOffset={sideOffset}
        className={cn(
          'z-50 min-w-44 overflow-hidden rounded-md border border-border bg-surface p-1 shadow-overlay',
          'data-[state=open]:animate-fade-in',
          className,
        )}
        {...props}
      />
    </DropdownMenuPrimitive.Portal>
  );
}

function DropdownMenuItem({
  className,
  inset,
  ...props
}: React.ComponentProps<typeof DropdownMenuPrimitive.Item> & { inset?: boolean }): React.JSX.Element {
  return (
    <DropdownMenuPrimitive.Item
      className={cn(
        'relative flex cursor-default select-none items-center gap-2 rounded-xs px-2 py-1.5 text-sm outline-none transition-colors',
        'focus:bg-surface-hover data-[disabled]:pointer-events-none data-[disabled]:opacity-50',
        '[&_svg]:size-4 [&_svg]:shrink-0',
        inset && 'pl-8',
        className,
      )}
      {...props}
    />
  );
}

function DropdownMenuCheckboxItem({
  className,
  children,
  checked,
  ...props
}: React.ComponentProps<typeof DropdownMenuPrimitive.CheckboxItem>): React.JSX.Element {
  return (
    <DropdownMenuPrimitive.CheckboxItem
      className={cn(
        'relative flex cursor-default select-none items-center gap-2 rounded-xs py-1.5 pl-7 pr-2 text-sm outline-none',
        'focus:bg-surface-hover data-[state=checked]:text-foreground',
        className,
      )}
      checked={checked}
      {...props}
    >
      <span className="absolute left-2 flex size-3.5 items-center justify-center">
        <DropdownMenuPrimitive.ItemIndicator>
          <CheckIcon className="size-3.5 text-primary" aria-hidden />
        </DropdownMenuPrimitive.ItemIndicator>
      </span>
      {children}
    </DropdownMenuPrimitive.CheckboxItem>
  );
}

function DropdownMenuLabel({
  className,
  ...props
}: React.ComponentProps<typeof DropdownMenuPrimitive.Label>): React.JSX.Element {
  return (
    <DropdownMenuPrimitive.Label
      className={cn('px-2 py-1.5 text-2xs font-semibold uppercase tracking-wide text-foreground-secondary', className)}
      {...props}
    />
  );
}

function DropdownMenuSeparator({
  className,
  ...props
}: React.ComponentProps<typeof DropdownMenuPrimitive.Separator>): React.JSX.Element {
  return <DropdownMenuPrimitive.Separator className={cn('-mx-1 my-1 h-px bg-border', className)} {...props} />;
}

// ---------------------------------------------------------------------------
// Context menu (results list right-click)
// ---------------------------------------------------------------------------

const ContextMenu = ContextMenuPrimitive.Root;
const ContextMenuTrigger = ContextMenuPrimitive.Trigger;

function ContextMenuContent({
  className,
  ...props
}: React.ComponentProps<typeof ContextMenuPrimitive.Content>): React.JSX.Element {
  return (
    <ContextMenuPrimitive.Portal>
      <ContextMenuPrimitive.Content
        className={cn(
          'z-50 min-w-52 overflow-hidden rounded-md border border-border bg-surface p-1 shadow-overlay',
          'data-[state=open]:animate-fade-in',
          className,
        )}
        {...props}
      />
    </ContextMenuPrimitive.Portal>
  );
}

function ContextMenuItem({
  className,
  ...props
}: React.ComponentProps<typeof ContextMenuPrimitive.Item>): React.JSX.Element {
  return (
    <ContextMenuPrimitive.Item
      className={cn(
        'relative flex cursor-default select-none items-center gap-2 rounded-xs px-2 py-2 text-sm outline-none transition-colors',
        'focus:bg-surface-hover data-[disabled]:pointer-events-none data-[disabled]:opacity-50',
        '[&_svg]:size-4 [&_svg]:shrink-0',
        className,
      )}
      {...props}
    />
  );
}

function ContextMenuSeparator({
  className,
  ...props
}: React.ComponentProps<typeof ContextMenuPrimitive.Separator>): React.JSX.Element {
  return <ContextMenuPrimitive.Separator className={cn('-mx-1 my-1 h-px bg-border', className)} {...props} />;
}

// ---------------------------------------------------------------------------
// Popover (filter panels)
// ---------------------------------------------------------------------------

const Popover = PopoverPrimitive.Root;
const PopoverTrigger = PopoverPrimitive.Trigger;
const PopoverAnchor = PopoverPrimitive.Anchor;

function PopoverContent({
  className,
  align = 'start',
  sideOffset = 6,
  ...props
}: React.ComponentProps<typeof PopoverPrimitive.Content>): React.JSX.Element {
  return (
    <PopoverPrimitive.Portal>
      <PopoverPrimitive.Content
        align={align}
        sideOffset={sideOffset}
        className={cn(
          'z-50 w-72 rounded-md border border-border bg-surface p-3 shadow-overlay outline-none',
          'data-[state=open]:animate-fade-in',
          className,
        )}
        {...props}
      />
    </PopoverPrimitive.Portal>
  );
}

export {
  ContextMenu,
  ContextMenuContent,
  ContextMenuItem,
  ContextMenuSeparator,
  ContextMenuTrigger,
  DropdownMenu,
  DropdownMenuCheckboxItem,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuLabel,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
  Popover,
  PopoverAnchor,
  PopoverContent,
  PopoverTrigger,
};
