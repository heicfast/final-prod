/**
 * Real IPC adapter — the cr_ui engine bridge (ui/src-tauri/src/commands/).
 *
 * The ONE backend the shipped product has: every method maps to a Tauri
 * command from contracts/command-surface.json (validate.mjs phase 2
 * cross-checks both directions — every command invoked here is in the
 * manifest, and every manifest command that serves a connector method is
 * invoked here).
 *
 * Error model (contracts/error-codes.json): the bridge rejects with
 * `{ code: number, bridgeCode: string | null, message }`. `bridgeCode` maps
 * directly to the UI vocabulary; `code` maps through the engine table below.
 * The UI renders reasonKey strings only — never string-matches messages.
 * ERR_CANCELLED never crosses as an error (the bridge translates it to
 * scan/recovery status 'cancelled'). ERR_ELEVATION_REQUIRED (0x500C0001)
 * crosses as elevation-required — the honest engine vocabulary; in the
 * shipped product it is unreachable (D1: the launch-time manifest already
 * guarantees the administrator token).
 */

import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import type {
  AppInfo,
  ApplyConsentEvent,
  ApplyResult,
  ConsentRequestEvent,
  ConnectorError,
  ConnectorErrorCode,
  Device,
  DestinationValidation,
  DiskHealth,
  LostCandidateRow,
  MountPoint,
  NodeDetail,
  PreviewPayload,
  RecoveryConnector,
  RecoveryFinishedEvent,
  RecoveryItem,
  RecoveryJob,
  RecoveryOptions,
  RecoveryProgressEvent,
  RecoveryReport,
  RecoverySelection,
  RepairDryRun,
  RepairUndoEntry,
  ResultsFilter,
  ResultsPage,
  ResultTreeMode,
  ScanAutosavedEvent,
  ScanFinishedEvent,
  ScanParams,
  ScanProgressEvent,
  SaveProgressEvent,
  LostCandidateEvent,
  ScanState,
  SessionLoadProgressEvent,
  SessionMeta,
  SortDirection,
  SortField,
  TreePage,
} from '../contract';

// ---------------------------------------------------------------------------
// Error mapping (the single translation point)
// ---------------------------------------------------------------------------

/** reasonKey for every ConnectorErrorCode — the UI renders these via t(). */
export const REASON_KEYS: Readonly<Record<ConnectorErrorCode, string>> = {
  'device-gone': 'errors.deviceGone',
  'device-permission': 'errors.devicePermission',
  'elevation-required': 'errors.elevationRequired',
  'scan-not-found': 'errors.scanNotFound',
  'scan-not-paused': 'errors.scanNotPaused',
  'scan-not-running': 'errors.scanNotRunning',
  'scan-scope-invalid': 'errors.scanScopeInvalid',
  'consent-not-found': 'errors.consentNotFound',
  'session-corrupt': 'errors.sessionCorrupt',
  'session-not-found': 'errors.sessionNotFound',
  'destination-overlap': 'recovery.overlapReason',
  'destination-invalid': 'errors.destinationInvalid',
  'destination-unwritable': 'errors.destinationUnwritable',
  'out-of-space': 'recovery.insufficientSpaceBody',
  'preview-unsupported': 'preview.reason.unsupportedKind',
  'preview-too-large': 'preview.reason.tooLarge',
  'preview-damaged': 'preview.reason.damaged',
  'preview-encrypted': 'preview.reason.encrypted',
  'preview-timeout': 'preview.reason.damaged',
  'preview-missing-data': 'preview.reason.damaged',
  'recovery-item-failed': 'recovery.itemFailedReason',
  'repair-consent-required': 'repair.consentRequired',
  'repair-failed': 'errors.repairFailed',
  'health-unavailable': 'errors.healthUnavailable',
  'not-implemented': 'errors.notImplemented',
  unknown: 'errors.unknown',
};

/** Engine numeric space (cr_error.h) -> UI vocabulary — error-codes.json. */
const ENGINE_CODE_MAP: Readonly<Record<number, ConnectorErrorCode>> = {
  0x50020001: 'device-permission', // ERR_OPEN_FAILED (bridge refines to device-gone)
  0x50020002: 'recovery-item-failed', // ERR_READ_FAILED
  0x50020003: 'recovery-item-failed', // ERR_WRITE_FAILED
  0x50020004: 'unknown', // ERR_RANGE_OUT_OF_BOUNDS (defensive)
  0x50020006: 'recovery-item-failed', // ERR_RETRY_EXHAUSTED
  0x50020007: 'device-gone', // ERR_DEVICE_DISCONNECTED (the v4 meaning; BF-07)
  0x50030001: 'session-corrupt', // ERR_BAD_MAGIC
  0x50030002: 'session-corrupt', // ERR_BAD_CHECKSUM (resume identity mismatch)
  0x50030003: 'scan-scope-invalid', // ERR_BAD_PARAMETER (context-refined)
  0x50030004: 'unknown', // ERR_TRUNCATED (defensive)
  0x50030005: 'repair-failed', // ERR_FIXUP_FAILED
  0x50030006: 'unknown', // ERR_CYCLE_LIMIT (informational)
  0x50030007: 'unknown', // ERR_DEPTH_LIMIT (informational)
  0x50030008: 'not-implemented', // ERR_UNSUPPORTED (context-refined)
  0x50030009: 'unknown', // ERR_NO_MEMORY
  0x50040001: 'session-corrupt', // ERR_SESSION_FORMAT
  0x50040002: 'session-corrupt', // ERR_SESSION_IO
  0x50040003: 'session-corrupt', // ERR_SESSION_VERSION
  0x50050001: 'destination-overlap', // ERR_DEST_OVERLAP (F12)
  0x50050002: 'destination-unwritable', // ERR_DEST_UNWRITABLE
  0x50050003: 'recovery-item-failed', // ERR_HASH_MISMATCH
  0x50050004: 'out-of-space', // ERR_DEST_FULL
  0x50050005: 'recovery-item-failed', // ERR_DEST_NAME_EXHAUSTED
  0x50050006: 'destination-invalid', // ERR_DEST_PATH_TOO_LONG (M-074 umbrella, BF-15)
  0x50050007: 'destination-invalid', // ERR_DEST_PATH_INVALID
  0x50050008: 'destination-invalid', // ERR_DEST_PATH_NETWORK
  0x50060001: 'device-permission', // ERR_FVE_LOCKED (volume status 'locked')
  0x50060002: 'device-permission', // ERR_FVE_ELEVATION_REQUIRED (FVE store ACL)
  0x50070001: 'health-unavailable', // ERR_HEALTH_UNAVAILABLE
  0x50080001: 'repair-failed', // ERR_REPAIR_REFUSED
  0x50080002: 'repair-failed', // ERR_REPAIR_UNDO_FAILED
  0x50080003: 'repair-failed', // ERR_VOLUME_LOCK_FAILED
  0x50080004: 'repair-consent-required', // ERR_APPLY_CONSENT_DENIED
  0x50080005: 'repair-failed', // ERR_APPLY_STALE
  0x50080006: 'repair-failed', // ERR_APPLY_REFUSED
  0x50080007: 'repair-failed', // ERR_APPLY_REFRESH_FAILED
  0x500a0001: 'unknown', // ERR_IMAGE_CORRUPT (no image surface)
  0x50090001: 'unknown', // ERR_INTERNAL
  0x500c0001: 'elevation-required', // ERR_ELEVATION_REQUIRED (the user-mode posture)
};

function toConnectorError(raw: unknown): ConnectorError {
  const payload = raw as { code?: unknown; bridgeCode?: unknown; message?: unknown };
  let code: ConnectorErrorCode = 'unknown';
  let detailCode: number | undefined;
  if (typeof payload?.bridgeCode === 'string' && payload.bridgeCode in REASON_KEYS) {
    code = payload.bridgeCode as ConnectorErrorCode;
  } else if (typeof payload?.code === 'number') {
    detailCode = payload.code;
    code = ENGINE_CODE_MAP[payload.code] ?? 'unknown';
  }
  const message =
    typeof payload?.message === 'string' && payload.message.length > 0
      ? payload.message
      : 'The recovery backend reported a failure without a message.';
  return Object.assign(new Error(message), {
    code,
    reasonKey: REASON_KEYS[code],
    detail: detailCode !== undefined ? { code: detailCode } : undefined,
  });
}

/**
 * Wrap an invoke promise: resolve on success, rethrow mapped on rejection.
 * (Call sites keep the literal command-name-first invoke shape that
 * validate.mjs phase 2 cross-checks against the manifest.)
 */
function bridge<T>(p: Promise<unknown>): Promise<T> {
  return p.then(
    (v) => v as T,
    (raw: unknown) => {
      throw toConnectorError(raw);
    },
  );
}

function connError(code: ConnectorErrorCode, message: string): ConnectorError {
  return Object.assign(new Error(message), { code, reasonKey: REASON_KEYS[code] });
}

/** A hex payload's bytes arrive as a JSON array — the contract wants Uint8Array. */
function normalizePreview(p: PreviewPayload | null): PreviewPayload | null {
  if (p === null) return null;
  if (p.kind === 'hex' && !ArrayBuffer.isView(p.bytes)) {
    return { ...p, bytes: new Uint8Array(p.bytes as readonly number[]) };
  }
  return p;
}

// ---------------------------------------------------------------------------
// The connector
// ---------------------------------------------------------------------------

export function createTauriConnector(): RecoveryConnector {
  return {
    devices: {
      listDevices: async (): Promise<Device[]> => bridge(invoke('devices_list')),

      getDeviceInfo: async (deviceId: string): Promise<Device> => {
        const all = await bridge<Device[]>(invoke('devices_list'));
        const direct = all.find((d) => d.id === deviceId);
        if (direct) return direct;
        // A volume id resolves to the owning device (scan-target semantics).
        const owner = all.find((d) => d.volumes.some((v) => v.id === deviceId));
        if (owner) return owner;
        throw connError('device-gone', `device ${deviceId} not found`);
      },

      refreshDevices: async (): Promise<Device[]> => bridge(invoke('devices_refresh')),

      unlockVolume: async (volumeId: string, passwords: readonly string[]): Promise<void> =>
        bridge(invoke('volume_unlock_dictionary', { volumeId, passwords })),
    },

    scan: {
      startScan: async (params: ScanParams): Promise<{ scanId: string }> =>
        bridge(
          invoke('scan_start', {
            targetId: params.targetId,
            pathScope: params.pathScope ?? null,
            bitlockerFvek: params.bitlockerFvek ?? null,
          }),
        ),

      pauseScan: async (scanId: string): Promise<void> =>
        bridge(invoke('scan_pause', { scanId })),

      resumeScan: async (scanId: string): Promise<void> =>
        bridge(invoke('scan_resume', { scanId })),

      cancelScan: async (scanId: string): Promise<void> =>
        bridge(invoke('scan_cancel', { scanId })),

      getScanState: async (scanId: string): Promise<ScanState> =>
        bridge(invoke('scan_state', { scanId })),

      onScanProgress: (
        scanId: string,
        cb: (event: ScanProgressEvent) => void,
      ): (() => void) => {
        const pending = listen<ScanProgressEvent>(`scan-progress:${scanId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      // W-01 (the dead-letter fix): the bridge's scan-finished / save-progress
      // / lost-candidate / scan-autosaved emissions — listened at last.
      onScanFinished: (
        scanId: string,
        cb: (event: ScanFinishedEvent) => void,
      ): (() => void) => {
        const pending = listen<ScanFinishedEvent>(`scan-finished:${scanId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      onSaveProgress: (
        scanId: string,
        cb: (event: SaveProgressEvent) => void,
      ): (() => void) => {
        const pending = listen<SaveProgressEvent>(`save-progress:${scanId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      onLostCandidate: (
        scanId: string,
        cb: (event: LostCandidateEvent) => void,
      ): (() => void) => {
        const pending = listen<LostCandidateEvent>(`lost-candidate:${scanId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      onScanAutosaved: (
        scanId: string,
        cb: (event: ScanAutosavedEvent) => void,
      ): (() => void) => {
        const pending = listen<ScanAutosavedEvent>(`scan-autosaved:${scanId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      // G-1 (a): the pre-scan repair consent surface. The engine's run
      // thread emits this at most once per scan; the prompt ALSO renders
      // from getScanState().pendingConsent (re-hydration for subscribe-late
      // and navigate-away-and-back).
      onConsentRequest: (
        scanId: string,
        cb: (event: ConsentRequestEvent) => void,
      ): (() => void) => {
        const pending = listen<ConsentRequestEvent>(`consent-request:${scanId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      respondToConsent: async (requestId: string, allow: boolean): Promise<void> =>
        bridge(invoke('consent_respond', { requestId, allow })),
    },

    results: {
      getResultTree: async (
        scanId: string,
        parentId: string | null,
        mode: ResultTreeMode,
      ): Promise<TreePage> => bridge(invoke('results_tree', { scanId, parentId, mode })),

      queryResults: async (
        scanId: string,
        filter: ResultsFilter,
        sort: { field: SortField; direction: SortDirection },
        page: { offset: number; limit: number },
      ): Promise<ResultsPage> =>
        bridge(invoke('results_query', { scanId, filter, sort, page })),

      getNodeDetail: async (scanId: string, nodeId: string): Promise<NodeDetail> =>
        bridge(invoke('node_detail', { scanId, nodeId })),

      countResults: async (scanId: string, filter: ResultsFilter): Promise<number> =>
        bridge(invoke('results_count', { scanId, filter })),
    },

    preview: {
      requestPreview: async (scanId: string, nodeId: string): Promise<PreviewPayload> => {
        const payload = await bridge<PreviewPayload | null>(
          invoke('preview_open', { scanId, nodeId }),
        );
        const normalized = normalizePreview(payload);
        if (normalized === null) {
          throw connError('preview-missing-data', 'the node has no previewable content');
        }
        return normalized;
      },

      getThumbnail: async (
        scanId: string,
        nodeId: string,
      ): Promise<{ dataUrl: string } | null> =>
        bridge(invoke('preview_thumbnail', { scanId, nodeId })),
    },

    sessions: {
      saveSession: async (scanId: string, path: string | null): Promise<SessionMeta> =>
        bridge(invoke('session_save', { scanId, path })),

      cancelSave: async (scanId: string): Promise<void> =>
        bridge(invoke('scan_save_cancel', { scanId })),

      listSessions: async (): Promise<SessionMeta[]> => bridge(invoke('sessions_list')),

      loadSession: async (path: string): Promise<{ scanId: string }> =>
        bridge(invoke('session_load', { path })),

      onSessionLoadProgress: (
        path: string,
        cb: (event: SessionLoadProgressEvent) => void,
      ): (() => void) => {
        const pending = listen<SessionLoadProgressEvent>(
          `session-load-progress:${path}`,
          (e) => {
            cb(e.payload);
          },
        );
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      getSessionMeta: async (path: string): Promise<SessionMeta> =>
        bridge(invoke('session_meta', { path })),

      resumeLastSession: async (): Promise<{ scanId: string; path: string }> =>
        bridge(invoke('session_resume_last')),

      resumeSession: async (path: string): Promise<{ scanId: string; targetId: string }> =>
        bridge(invoke('session_resume', { path })),
    },

    recovery: {
      startRecovery: async (
        scanId: string,
        selection: RecoverySelection,
        destination: string,
        options: RecoveryOptions,
      ): Promise<{ jobId: string }> =>
        bridge(invoke('recovery_start', { scanId, selection, destination, options })),

      cancelRecovery: async (jobId: string): Promise<void> =>
        bridge(invoke('recovery_cancel', { jobId })),

      getRecoveryState: async (jobId: string): Promise<RecoveryJob> =>
        bridge(invoke('recovery_state', { jobId })),

      getRecoveryItems: async (
        jobId: string,
        page: { offset: number; limit: number },
      ): Promise<readonly RecoveryItem[]> => bridge(invoke('recovery_items', { jobId, page })),

      onRecoveryProgress: (
        jobId: string,
        cb: (event: RecoveryProgressEvent) => void,
      ): (() => void) => {
        const pending = listen<RecoveryProgressEvent>(`recovery-progress:${jobId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      onRecoveryFinished: (
        jobId: string,
        cb: (event: RecoveryFinishedEvent) => void,
      ): (() => void) => {
        const pending = listen<RecoveryFinishedEvent>(`recovery-finished:${jobId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      getRecoveryReport: async (jobId: string): Promise<RecoveryReport> =>
        bridge(invoke('recovery_report', { jobId })),
    },

    health: {
      getHealth: async (deviceId: string): Promise<DiskHealth> =>
        bridge(invoke('health_read', { deviceId })),
    },

    repair: {
      // G-1 gap cluster: the bridge throws the typed not-implemented with the
      // truthful scope — these surfaces render the message, never a dead button.
      dryRunRepair: async (deviceId: string): Promise<RepairDryRun> =>
        bridge(invoke('repair_dry_run', { deviceId })),

      applyRepair: async (
        deviceId: string,
        options: { consentToken: string },
      ): Promise<{ applied: number }> => bridge(invoke('repair_apply', { deviceId, options })),

      getUndoLog: async (deviceId: string): Promise<readonly RepairUndoEntry[]> =>
        bridge(invoke('repair_undo_log', { deviceId })),

      undoRepairs: async (deviceId: string): Promise<{ rolledBack: number }> =>
        bridge(invoke('repair_undo', { deviceId })),
    },

    lostPartition: {
      applyLostPartition: async (
        deviceId: string,
        candidate: LostCandidateRow,
        rebuildTable?: boolean,
      ): Promise<ApplyResult> =>
        bridge(
          invoke('lost_partition_apply', {
            deviceId,
            candidate,
            rebuildTable: rebuildTable ?? false,
          }),
        ),

      undoLastApply: async (deviceId: string): Promise<ApplyResult> =>
        bridge(invoke('lost_partition_undo', { deviceId })),

      saveApplyUndo: async (deviceId: string, path: string): Promise<void> =>
        bridge(invoke('lost_partition_save_undo', { deviceId, path })),

      onApplyConsent: (
        deviceId: string,
        cb: (event: ApplyConsentEvent) => void,
      ): (() => void) => {
        const pending = listen<ApplyConsentEvent>(`apply-consent-request:${deviceId}`, (e) => {
          cb(e.payload);
        });
        return () => {
          void pending.then((unlisten) => unlisten());
        };
      },

      respondToApplyConsent: async (requestId: string, allow: boolean): Promise<void> =>
        bridge(invoke('consent_respond', { requestId, allow })),

      pendingApplyConsent: async (deviceId: string): Promise<ApplyConsentEvent | null> =>
        bridge(invoke('apply_consent_pending', { deviceId })),
    },

    system: {
      getAppInfo: async (): Promise<AppInfo> => bridge(invoke('system_app_info')),

      loadUserSigns: async (content: string): Promise<{ count: number }> =>
        bridge(invoke('system_load_user_signs', { content })),

      validateDestination: async (
        path: string,
        sourceDeviceId: string,
        neededBytes: number,
      ): Promise<DestinationValidation> =>
        bridge(invoke('system_validate_destination', { path, sourceDeviceId, neededBytes })),

      pickDirectory: async (hint?: {
        volumeId?: string | null;
        excludeDeviceId?: string | null;
      }): Promise<string | null> =>
        bridge(invoke('system_pick_directory', { hint: hint ?? null })),

      listMountPoints: async (): Promise<readonly MountPoint[]> =>
        bridge(invoke('system_list_mounts')),

      openPath: async (path: string): Promise<void> =>
        bridge(invoke('system_open_path', { path })),
    },
  };
}
