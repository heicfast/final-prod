import * as React from 'react';
import { useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import { RefreshCcwIcon } from 'lucide-react';
import { useDevices, useRefreshDevices } from '@/queries/devices';
import { useStartScan } from '@/queries/scan';
import { formatBytes } from '@/lib/format';
import { toast } from 'sonner';

/**
 * SD Card Recovery Page (reference: landing-sidebar-sdcard-page_bb0f35e7.png) —
 * header with circular refresh button, centerpiece illustration (drone, DSLR,
 * monitor with unplugged indicator, dash cam, PC tower, robot mascot holding
 * SD card), SDR / SSR / DVR 3-column feature row, and bottom detection banner.
 */
export function SdCardPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const devicesQuery = useDevices();
  const refresh = useRefreshDevices();
  const startScan = useStartScan();
  const [showExternal, setShowExternal] = React.useState(false);

  const devices = devicesQuery.data ?? [];
  const externalDevices = devices.filter((d) => d.kind === 'sd' || d.kind === 'usb');

  const beginScan = (targetId: string): void => {
    startScan.mutate(
      { targetId },
      {
        onSuccess: ({ scanId }) => {
          void navigate(`/scan/${targetId}/live?scanId=${scanId}`);
        },
        onError: (err) => toast.error(err.message),
      },
    );
  };

  return (
    <div className="flex h-full min-h-0 flex-col justify-between overflow-y-auto px-8 py-6">
      {/* Header */}
      <div className="flex items-start justify-between gap-4">
        <div>
          <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B]">
            {t('sdCard.title')}
          </h1>
          <p className="mt-1.5 max-w-3xl text-[13px] leading-relaxed text-[#64748B]">
            {t('sdCard.subtitle')}
          </p>
        </div>
        <button
          type="button"
          onClick={() => void refresh.mutate()}
          disabled={refresh.isPending}
          aria-label={t('sdCard.refresh')}
          title={t('sdCard.refresh')}
          className="mt-1 flex size-8 shrink-0 items-center justify-center rounded-full border border-[#E2E8F0] bg-white text-[#64748B] shadow-2xs transition-colors hover:border-[#BFDBFE] hover:bg-[#F8FAFC] hover:text-[#1677FF] focus-visible:ring-2 focus-visible:ring-ring disabled:opacity-50"
        >
          <RefreshCcwIcon className="size-4" aria-hidden />
        </button>
      </div>

      {/* Centerpiece Illustration */}
      <div className="my-4 flex flex-col items-center justify-center">
        <SdCardCenterIllustration />

        {/* 3 Feature Columns: SDR / SSR / DVR */}
        <div className="mt-5 grid w-full max-w-3xl grid-cols-1 gap-6 sm:grid-cols-3">
          <FeatureBadgeItem
            badge="SDR"
            title={t('sdCard.sdrTitle')}
            body={t('sdCard.sdrBody')}
          />
          <FeatureBadgeItem
            badge="SSR"
            title={t('sdCard.ssrTitle')}
            body={t('sdCard.ssrBody')}
          />
          <FeatureBadgeItem
            badge="DVR"
            title={t('sdCard.dvrTitle')}
            body={t('sdCard.dvrBody')}
          />
        </div>

        {/* Optional expanded external devices tray when user clicks "Show all external devices" */}
        {showExternal && externalDevices.length > 0 ? (
          <div className="mt-5 w-full max-w-2xl rounded-[10px] border border-[#DCE8FF] bg-[#F8FAFD] p-3.5">
            <div className="mb-2 text-[12px] font-semibold text-[#475569]">
              {t('home.removableGroup')} ({externalDevices.length})
            </div>
            <div className="grid grid-cols-1 gap-2.5 sm:grid-cols-2">
              {externalDevices.map((dev) => (
                <div
                  key={dev.id}
                  className="flex items-center justify-between gap-3 rounded-[8px] border border-[#E2E8F0] bg-white px-3.5 py-2.5"
                >
                  <div className="min-w-0">
                    <div className="truncate text-[13px] font-semibold text-[#1E293B]">
                      {dev.label}
                    </div>
                    <div className="text-[11px] text-[#64748B] tabular">
                      {dev.model} · {formatBytes(dev.capacity.totalBytes, i18n.language)}
                    </div>
                  </div>
                  <button
                    type="button"
                    onClick={() => beginScan(dev.id)}
                    className="shrink-0 rounded-[6px] bg-[#1677FF] px-3 py-1.5 text-[12px] font-semibold text-white hover:bg-[#0958D9]"
                  >
                    {t('sdCard.start')}
                  </button>
                </div>
              ))}
            </div>
          </div>
        ) : null}
      </div>

      {/* Bottom Floating Info Banner */}
      <div className="mx-auto w-full max-w-4xl rounded-[8px] bg-[#EBF3FF] px-5 py-3 text-center text-[13px] text-[#475569]">
        <span>{t('sdCard.bannerPrefix')} </span>
        <button
          type="button"
          onClick={() => setShowExternal((v) => !v)}
          className="font-medium text-[#1677FF] underline decoration-[#1677FF]/60 underline-offset-2 hover:text-[#0958D9]"
        >
          {t('sdCard.showExternal')}
        </button>
        <span> {t('sdCard.bannerOr')} </span>
        <button
          type="button"
          onClick={() => toast.info(t('sdCard.helpToast'))}
          className="font-medium text-[#1677FF] underline decoration-[#1677FF]/60 underline-offset-2 hover:text-[#0958D9]"
        >
          {t('sdCard.getHelp')}
        </button>
      </div>
    </div>
  );
}

function FeatureBadgeItem({
  badge,
  title,
  body,
}: {
  badge: 'SDR' | 'SSR' | 'DVR';
  title: string;
  body: string;
}): React.JSX.Element {
  return (
    <div className="flex items-start gap-3">
      <div
        className="flex size-10 shrink-0 flex-col items-center justify-center rounded-[8px] border border-[#DCE6F5] bg-[#F4F7FC] text-[#475569]"
        aria-hidden
      >
        <span className="rounded-[3px] bg-[#64748B] px-1.5 py-0.5 text-[9px] font-bold tracking-wider text-white">
          {badge}
        </span>
      </div>
      <div className="min-w-0">
        <div className="text-[13.5px] font-bold text-[#1E293B]">{title}</div>
        <div className="mt-0.5 text-[12px] text-[#64748B]">{body}</div>
      </div>
    </div>
  );
}

function SdCardCenterIllustration(): React.JSX.Element {
  return (
    <svg
      viewBox="0 0 420 220"
      fill="none"
      className="h-[195px] w-full max-w-[420px] select-none"
      aria-hidden
    >
      {/* Soft shadow base */}
      <ellipse cx="210" cy="198" rx="155" ry="14" fill="#EBF2FF" />

      {/* Dashed orbital arc */}
      <ellipse
        cx="205"
        cy="108"
        rx="145"
        ry="72"
        stroke="#CBD5E1"
        strokeWidth="1.4"
        strokeDasharray="4 4"
      />

      {/* Top-left Drone (UAV) */}
      <g transform="translate(56, 36)">
        <line x1="6" y1="14" x2="38" y2="14" stroke="#64748B" strokeWidth="2" />
        <line
          x1="2"
          y1="8"
          x2="14"
          y2="8"
          stroke="#94A3B8"
          strokeWidth="2"
          strokeLinecap="round"
        />
        <line
          x1="30"
          y1="8"
          x2="42"
          y2="8"
          stroke="#94A3B8"
          strokeWidth="2"
          strokeLinecap="round"
        />
        <rect
          x="13"
          y="10"
          width="18"
          height="9"
          rx="4"
          fill="#E2E8F0"
          stroke="#64748B"
          strokeWidth="1.3"
        />
        <circle cx="22" cy="14.5" r="2.2" fill="#1677FF" />
      </g>

      {/* Bottom-left DSLR Camera */}
      <g transform="translate(54, 118)">
        <rect x="4" y="8" width="34" height="24" rx="4" fill="#94A3B8" />
        <path d="M13 8L16 3H26L29 8H13Z" fill="#64748B" />
        <circle cx="21" cy="20" r="8" fill="#475569" stroke="white" strokeWidth="1.5" />
        <circle cx="21" cy="20" r="3.5" fill="#60A5FA" />
      </g>

      {/* Top-right Action / Dash Cam */}
      <g transform="translate(274, 24)">
        <rect
          x="4"
          y="6"
          width="32"
          height="24"
          rx="4"
          fill="#CBD5E1"
          stroke="#64748B"
          strokeWidth="1.3"
        />
        <circle cx="15" cy="18" r="6.5" fill="#334155" stroke="white" strokeWidth="1.4" />
        <circle cx="15" cy="18" r="2.5" fill="#38BDF8" />
        <rect x="25" y="10" width="6" height="4" rx="1" fill="#64748B" />
      </g>

      {/* Right PC Tower */}
      <g transform="translate(285, 76)">
        <rect
          x="0"
          y="0"
          width="64"
          height="106"
          rx="6"
          fill="#E2E8F0"
          stroke="#94A3B8"
          strokeWidth="1.4"
        />
        <rect x="8" y="12" width="48" height="8" rx="2" fill="#94A3B8" />
        <rect x="8" y="26" width="48" height="5" rx="1.5" fill="#CBD5E1" />
        <circle cx="32" cy="48" r="4" fill="#64748B" />
      </g>

      {/* Central Monitor */}
      <g transform="translate(122, 54)">
        <rect x="0" y="0" width="152" height="102" rx="8" fill="#64748B" />
        <rect x="6" y="6" width="140" height="82" rx="4" fill="#F8FAFC" />
        {/* Screen skeleton bars */}
        <rect x="18" y="20" width="54" height="7" rx="3.5" fill="#E2E8F0" />
        <rect x="18" y="34" width="88" height="6" rx="3" fill="#EDF2F7" />
        <rect x="18" y="46" width="72" height="6" rx="3" fill="#EDF2F7" />
        {/* Stand */}
        <path d="M62 102H90L98 122H54L62 102Z" fill="#CBD5E1" />
        <rect x="44" y="122" width="64" height="5" rx="2.5" fill="#94A3B8" />

        {/* Orange Unplugged Warning Circle on top-right of monitor */}
        <g transform="translate(128, -14)">
          <circle cx="18" cy="18" r="18" fill="#FFF7ED" stroke="#FDBA74" strokeWidth="2" />
          <path d="M12 24L24 12" stroke="#F97316" strokeWidth="2.2" strokeLinecap="round" />
          <path
            d="M14 14L11 17L19 25L22 22"
            stroke="#F97316"
            strokeWidth="2"
            strokeLinecap="round"
            strokeLinejoin="round"
          />
        </g>
      </g>

      {/* Robot Mascot holding Blue SD Card in foreground */}
      <g transform="translate(222, 92)">
        {/* Blue SD Card held on left */}
        <g transform="translate(0, 26) rotate(-8)">
          <path
            d="M4 0H34L44 10V52C44 55.3 41.3 58 38 58H4C1.8 58 0 56.2 0 54V4C0 1.8 1.8 0 4 0Z"
            fill="#1677FF"
          />
          <rect x="6" y="6" width="4" height="9" rx="1" fill="#FBBF24" />
          <rect x="13" y="6" width="4" height="9" rx="1" fill="#FBBF24" />
          <rect x="20" y="6" width="4" height="9" rx="1" fill="#FBBF24" />
          <rect x="27" y="6" width="4" height="9" rx="1" fill="#FBBF24" />
          <text
            x="10"
            y="44"
            fill="white"
            fontSize="14"
            fontWeight="800"
            fontFamily="sans-serif"
          >
            SD
          </text>
        </g>

        {/* Robot Body & Head */}
        <g transform="translate(36, 4)">
          <rect
            x="14"
            y="48"
            width="38"
            height="36"
            rx="16"
            fill="white"
            stroke="#93C5FD"
            strokeWidth="2"
          />
          <rect x="22" y="56" width="22" height="14" rx="4" fill="#DBEAFE" />
          {/* Head */}
          <rect x="2" y="16" width="5" height="12" rx="2.5" fill="#1677FF" />
          <rect x="59" y="16" width="5" height="12" rx="2.5" fill="#1677FF" />
          <rect
            x="6"
            y="4"
            width="54"
            height="42"
            rx="20"
            fill="white"
            stroke="#93C5FD"
            strokeWidth="2"
          />
          <rect x="13" y="13" width="40" height="22" rx="10" fill="#0F172A" />
          <ellipse cx="25" cy="24" rx="3.5" ry="4.5" fill="#38BDF8" />
          <ellipse cx="41" cy="24" rx="3.5" ry="4.5" fill="#38BDF8" />
        </g>
      </g>
    </svg>
  );
}
