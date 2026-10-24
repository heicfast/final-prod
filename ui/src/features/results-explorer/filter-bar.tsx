import * as React from 'react';
import { useTranslation } from 'react-i18next';
import type { FileCategoryId } from '@/connector/contract';
import { CalendarIcon, FilterXIcon, RulerIcon, TagIcon } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Popover, PopoverContent, PopoverTrigger } from '@/components/ui/menus';
import { CategoryIcon } from '@/components/ui/file-icon';
import { useFilterStore } from '@/stores/filter-store';
import { cn } from '@/lib/utils';

/**
 * Filter bar — Path/Type/Date/Size dropdowns + deleted-only toggle + Reset,
 * mirroring the corpus filter row. All filters resolve into a connector
 * ResultsFilter (server-side in the real backend; typed-array scan in the
 * mock — never client-side over paged data).
 */

export function FilterBar(): React.JSX.Element {
  const { t } = useTranslation();
  const f = useFilterStore();

  return (
    <div className="flex flex-wrap items-center gap-2 border-b border-border bg-surface-muted/60 px-3 py-1.5 text-sm">
      <FilterPopover icon={TagIcon} label={t('results.filterType')}>
        <CategoryPicker />
      </FilterPopover>

      <FilterPopover icon={CalendarIcon} label={t('results.filterDate')}>
        <DatePicker />
      </FilterPopover>

      <FilterPopover icon={RulerIcon} label={t('results.filterSize')}>
        <SizePicker />
      </FilterPopover>

      <label className="flex cursor-pointer select-none items-center gap-1.5 text-xs text-foreground-secondary">
        <input
          type="checkbox"
          className="accent-[var(--accent)]"
          checked={f.showDeletedOnly}
          onChange={(e) => f.setShowDeletedOnly(e.target.checked)}
        />
        {t('results.tree.deletedFiles')}
      </label>

      {f.hasActiveFilters() ? (
        <Button variant="ghost" size="sm" onClick={() => f.reset()} className="ml-auto">
          <FilterXIcon aria-hidden />
          {t('results.clearFilter')}
        </Button>
      ) : null}
    </div>
  );
}

function FilterPopover({
  icon: Icon,
  label,
  children,
}: {
  icon: React.ComponentType<{ className?: string }>;
  label: string;
  children: React.ReactNode;
}): React.JSX.Element {
  return (
    <Popover>
      <PopoverTrigger asChild>
        <Button variant="ghost" size="sm" className="gap-1.5">
          <Icon className="size-4" aria-hidden />
          {label}
        </Button>
      </PopoverTrigger>
      <PopoverContent className="w-64">{children}</PopoverContent>
    </Popover>
  );
}

const CATEGORIES: readonly FileCategoryId[] = [
  'graphics',
  'video',
  'audio',
  'document',
  'email',
  'archive',
  'bookmarks',
  'other',
];

function CategoryPicker(): React.JSX.Element {
  const { t } = useTranslation();
  const f = useFilterStore();
  return (
    <div className="space-y-1" role="group" aria-label={t('results.filterType')}>
      {CATEGORIES.map((cat) => {
        const checked = f.categories.includes(cat);
        return (
          <label
            key={cat}
            className="flex cursor-pointer items-center gap-2.5 rounded-xs px-2 py-1.5 text-sm hover:bg-surface-hover"
          >
            <input
              type="checkbox"
              className="sr-only"
              checked={checked}
              onChange={() => f.toggleCategory(cat)}
            />
            <span
              className={cn(
                'flex size-4 items-center justify-center rounded-xs border text-2xs',
                checked
                  ? 'border-primary bg-primary text-primary-foreground'
                  : 'border-border-strong',
              )}
              aria-hidden
            >
              {checked ? '✓' : ''}
            </span>
            <CategoryIcon category={cat} />
            <span className="flex-1">{t(`scanConfig.categories.${cat}`)}</span>
          </label>
        );
      })}
    </div>
  );
}

function DatePicker(): React.JSX.Element {
  const { t } = useTranslation();
  const f = useFilterStore();
  return (
    <div className="space-y-2">
      {(['any', 'yesterday', 'week', 'month'] as const).map((preset) => (
        <label key={preset} className="flex cursor-pointer items-center gap-2 text-sm">
          <input
            type="radio"
            name="date-preset"
            className="accent-[var(--accent)]"
            checked={f.datePreset === preset}
            onChange={() => f.setDatePreset(preset)}
          />
          {t(
            `results.date${preset === 'any' ? 'Any' : preset === 'yesterday' ? 'Yesterday' : preset === 'week' ? 'Week' : 'Month'}`,
          )}
        </label>
      ))}
      <div className="rounded-sm border border-border p-2">
        <p className="mb-1.5 text-xs font-medium text-foreground-secondary">
          {t('results.dateCustom')}
        </p>
        <div className="grid grid-cols-2 gap-2">
          <div>
            <Label htmlFor="date-from" className="mb-1 text-2xs text-foreground-disabled">
              {t('results.dateFrom')}
            </Label>
            <Input
              id="date-from"
              type="date"
              value={f.dateFrom ?? ''}
              onChange={(e) => f.setDatePreset('custom', e.target.value || null, undefined)}
              className="h-7 text-xs"
            />
          </div>
          <div>
            <Label htmlFor="date-to" className="mb-1 text-2xs text-foreground-disabled">
              {t('results.dateTo')}
            </Label>
            <Input
              id="date-to"
              type="date"
              value={f.dateTo ?? ''}
              onChange={(e) => f.setDatePreset('custom', undefined, e.target.value || null)}
              className="h-7 text-xs"
            />
          </div>
        </div>
      </div>
    </div>
  );
}

function SizePicker(): React.JSX.Element {
  const { t } = useTranslation();
  const f = useFilterStore();
  return (
    <div className="space-y-2">
      {(['any', 'tiny', 'small', 'medium', 'large', 'huge'] as const).map((preset) => (
        <label key={preset} className="flex cursor-pointer items-center gap-2 text-sm">
          <input
            type="radio"
            name="size-preset"
            className="accent-[var(--accent)]"
            checked={f.sizePreset === preset}
            onChange={() => f.setSizePreset(preset)}
          />
          {t(
            `results.size${preset === 'any' ? 'Any' : preset === 'tiny' ? 'Tiny' : preset === 'small' ? 'Small' : preset === 'medium' ? 'Medium' : preset === 'large' ? 'Large' : 'Huge'}`,
          )}
        </label>
      ))}
      <div className="rounded-sm border border-border p-2">
        <p className="mb-1.5 text-xs font-medium text-foreground-secondary">
          {t('results.sizeCustom')}
        </p>
        <div className="flex items-center gap-2">
          <Input
            aria-label={t('results.dateFrom')}
            type="number"
            min={0}
            placeholder="0"
            className="h-7 w-20 text-xs tabular"
            value={f.sizeMin ?? ''}
            onChange={(e) => f.setSizePreset('custom', e.target.value, undefined)}
          />
          <span className="text-xs text-foreground-disabled">MB —</span>
          <Input
            aria-label={t('results.dateTo')}
            type="number"
            min={0}
            placeholder="∞"
            className="h-7 w-20 text-xs tabular"
            value={f.sizeMax ?? ''}
            onChange={(e) => f.setSizePreset('custom', undefined, e.target.value)}
          />
          <span className="text-xs text-foreground-disabled">MB</span>
        </div>
      </div>
    </div>
  );
}
