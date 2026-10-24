/// <reference lib="webworker" />
/**
 * Hex preview worker — formats bounded byte slices into offset/hex/ascii rows
 * off the main thread (grid #37/#38). Input is capped by the connector
 * contract, never by the UI. Row formatting itself lives in the shared
 * lib/hex-format module (single implementation for worker + fallback).
 */
import { formatHexRows, type HexRow } from '@/lib/hex-format';

export type { HexRow };

interface HexRequest {
  bytes: ArrayBuffer;
  baseOffset: number;
}

self.onmessage = (e: MessageEvent<HexRequest>) => {
  const { bytes, baseOffset } = e.data;
  const rows = formatHexRows(new Uint8Array(bytes), baseOffset);
  (self as unknown as Worker).postMessage(rows);
};
