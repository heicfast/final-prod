import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import { Button } from '@/components/ui/button';

type ToolId = 'crashed' | 'nas' | 'raid' | 'cloud' | 'image' | 'health';

const TOOL_ITEMS: readonly {
  id: ToolId;
  labelKey: string;
  descriptionKey: string;
  route: string | null;
}[] = [
  {
    id: 'crashed',
    labelKey: 'tools.crashedTitle',
    descriptionKey: 'tools.crashedBody',
    route: null,
  },
  {
    id: 'nas',
    labelKey: 'tools.nasTitle',
    descriptionKey: 'tools.nasBody',
    route: null,
  },
  {
    id: 'raid',
    labelKey: 'tools.raidTitle',
    descriptionKey: 'tools.raidBody',
    route: '/lost-partition',
  },
  {
    id: 'cloud',
    labelKey: 'tools.cloudTitle',
    descriptionKey: 'tools.cloudBody',
    route: null,
  },
  {
    id: 'image',
    labelKey: 'tools.imageTitle',
    descriptionKey: 'tools.imageBody',
    route: '/',
  },
  {
    id: 'health',
    labelKey: 'tools.healthTitle',
    descriptionKey: 'tools.healthBody',
    route: '/health',
  },
];

/**
 * More Recovery Tools page (reference: landing-sidebar-moretools-page_8bba1852.png
 * & landing-sidebar-moretools-confirm_69ece5af.png) — 2×3 grid of 6 advanced
 * recovery tools with top-left curved corner icon badges.
 */
export function ToolsPage(): React.JSX.Element {
  const { t } = useTranslation();
  const navigate = useNavigate();
  const [selectedTool, setSelectedTool] = React.useState<ToolId | null>(null);

  const activeItem = TOOL_ITEMS.find((item) => item.id === selectedTool) ?? null;

  return (
    <div className="h-full overflow-y-auto px-8 py-6">
      <div className="mx-auto w-full max-w-5xl">
        <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
          {t('tools.title')}
        </h1>
        <p className="mt-1.5 text-[13px] text-[#64748B] dark:text-foreground-secondary">
          {t('tools.subtitle')}
        </p>

        <div className="mt-7 grid grid-cols-1 gap-5 sm:grid-cols-2 lg:grid-cols-3">
          {TOOL_ITEMS.map((item) => (
            <button
              key={item.id}
              type="button"
              onClick={() => {
                if (item.id === 'health') {
                  void navigate('/health');
                } else {
                  setSelectedTool(item.id);
                }
              }}
              className="group relative flex min-h-[168px] flex-col overflow-hidden rounded-[12px] border border-[#DCE8FF] bg-[#F8FAFF] p-6 pt-16 text-left shadow-[0_2px_10px_rgba(22,119,255,0.04)] transition-all hover:border-[#60A5FA] hover:bg-[#EEF4FF] hover:shadow-md active:scale-[var(--press-scale)] focus-visible:ring-2 focus-visible:ring-ring dark:border-border dark:bg-surface-elevated dark:hover:border-primary/60 dark:hover:bg-surface-hover"
            >
              {/* Top-left curved corner badge */}
              <div
                className="absolute left-0 top-0 flex h-12 w-16 items-center justify-center rounded-br-[28px] bg-[#EBF2FF] transition-colors group-hover:bg-[#DCE8FF] dark:bg-surface-tint dark:group-hover:bg-primary/25"
                aria-hidden
              >
                <ToolCornerIcon id={item.id} />
              </div>

              <h2 className="text-[15px] font-bold text-[#1E293B] transition-colors group-hover:text-[#0958D9] dark:text-foreground dark:group-hover:text-primary">
                {t(item.labelKey)}
              </h2>
              <p className="mt-2 text-[12.5px] leading-relaxed text-[#475569] dark:text-foreground-secondary">
                {t(item.descriptionKey)}
              </p>
            </button>
          ))}
        </div>
      </div>

      <Dialog
        open={selectedTool !== null}
        onOpenChange={(open) => !open && setSelectedTool(null)}
      >
        <DialogContent
          className="max-w-[440px] rounded-[12px] border border-[#E2E8F0] bg-white p-6 shadow-2xl dark:border-border dark:bg-surface-elevated"
          aria-label={activeItem ? t(activeItem.labelKey) : t('tools.providerTitle')}
        >
          <DialogHeader>
            <DialogTitle className="text-[16px] font-bold text-[#1E293B] dark:text-foreground">
              {activeItem ? t(activeItem.labelKey) : t('tools.providerTitle')}
            </DialogTitle>
            <DialogDescription className="mt-1 text-[13px] text-[#64748B] dark:text-foreground-secondary">
              {activeItem ? t(activeItem.descriptionKey) : ''}
            </DialogDescription>
          </DialogHeader>
          <p className="text-[13px] leading-relaxed text-[#475569] dark:text-foreground-secondary">
            {t('tools.providerBody')}
          </p>
          <DialogFooter className="mt-4">
            <Button variant="secondary" onClick={() => setSelectedTool(null)}>
              {t('common.close')}
            </Button>
            {activeItem?.route ? (
              <Button
                variant="primary"
                onClick={() => {
                  const targetRoute = activeItem.route;
                  setSelectedTool(null);
                  if (targetRoute) void navigate(targetRoute);
                }}
              >
                {t('tools.openTool')}
              </Button>
            ) : null}
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}

function ToolCornerIcon({ id }: { id: ToolId }): React.JSX.Element {
  switch (id) {
    case 'crashed':
      return (
        <svg viewBox="0 0 28 28" fill="none" className="size-7">
          <rect
            x="3"
            y="5"
            width="22"
            height="15"
            rx="2.5"
            fill="white"
            stroke="#64748B"
            strokeWidth="1.5"
          />
          <path d="M10 20H18L19.5 23H8.5L10 20Z" fill="#94A3B8" />
          <path
            d="M11.5 10.5L16.5 15.5M16.5 10.5L11.5 15.5"
            stroke="#1677FF"
            strokeWidth="1.8"
            strokeLinecap="round"
          />
        </svg>
      );
    case 'nas':
      return (
        <svg viewBox="0 0 28 28" fill="none" className="size-7">
          <rect
            x="5"
            y="4"
            width="18"
            height="20"
            rx="2.5"
            fill="white"
            stroke="#64748B"
            strokeWidth="1.5"
          />
          <line
            x1="9"
            y1="9"
            x2="19"
            y2="9"
            stroke="#1677FF"
            strokeWidth="1.6"
            strokeLinecap="round"
          />
          <line
            x1="9"
            y1="14"
            x2="19"
            y2="14"
            stroke="#1677FF"
            strokeWidth="1.6"
            strokeLinecap="round"
          />
          <circle cx="14" cy="19" r="1.6" fill="#1677FF" />
        </svg>
      );
    case 'raid':
      return (
        <svg viewBox="0 0 28 28" fill="none" className="size-7">
          <rect
            x="4"
            y="5"
            width="20"
            height="5.5"
            rx="1.5"
            fill="white"
            stroke="#64748B"
            strokeWidth="1.4"
          />
          <rect
            x="4"
            y="11.5"
            width="20"
            height="5.5"
            rx="1.5"
            fill="white"
            stroke="#64748B"
            strokeWidth="1.4"
          />
          <rect
            x="4"
            y="18"
            width="20"
            height="5.5"
            rx="1.5"
            fill="#DBEAFE"
            stroke="#1677FF"
            strokeWidth="1.4"
          />
          <circle cx="8" cy="7.8" r="1" fill="#1677FF" />
          <circle cx="8" cy="14.3" r="1" fill="#1677FF" />
          <circle cx="8" cy="20.8" r="1" fill="#1677FF" />
        </svg>
      );
    case 'cloud':
      return (
        <svg viewBox="0 0 28 28" fill="none" className="size-7">
          <path
            d="M8.5 21H19.5C22 21 24 19 24 16.5C24 14.2 22.3 12.3 20 12.1C19.3 8.6 16.2 6 12.5 6C8.4 6 5 9.4 5 13.5C5 14 5.1 14.5 5.2 15C3.9 15.8 3 17.2 3 18.8C3 20 4 21 5.5 21H8.5Z"
            fill="white"
            stroke="#64748B"
            strokeWidth="1.5"
          />
          <path
            d="M14 11.5V17.5M14 11.5L11.5 14M14 11.5L16.5 14"
            stroke="#1677FF"
            strokeWidth="1.7"
            strokeLinecap="round"
            strokeLinejoin="round"
          />
        </svg>
      );
    case 'image':
      return (
        <svg viewBox="0 0 28 28" fill="none" className="size-7">
          <rect
            x="4"
            y="5"
            width="20"
            height="18"
            rx="2.5"
            fill="white"
            stroke="#64748B"
            strokeWidth="1.5"
          />
          <circle cx="10.5" cy="11" r="2" fill="#1677FF" />
          <path d="M6 20L12 14.5L16 18L19.5 15L22 18V20H6Z" fill="#93C5FD" />
        </svg>
      );
    case 'health':
      return (
        <svg viewBox="0 0 28 28" fill="none" className="size-7">
          <rect
            x="5"
            y="4"
            width="18"
            height="20"
            rx="2.5"
            fill="white"
            stroke="#64748B"
            strokeWidth="1.5"
          />
          <path
            d="M8 15H11L12.8 11L15.2 18.5L17 15H20"
            stroke="#1677FF"
            strokeWidth="1.7"
            strokeLinecap="round"
            strokeLinejoin="round"
          />
        </svg>
      );
  }
}
