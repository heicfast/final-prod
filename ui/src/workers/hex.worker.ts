/// <reference lib="webworker" />
/**
 * Hex preview worker — formats bounded byte slices into offset/hex/ascii rows
 * off the main thread (grid #37/#38). Input is capped by the connector
 * contract, never by the UI.
 */

export interface HexRequest {
  bytes: ArrayBuffer;
  baseOffset: number;
}

export interface HexRow {
  offset: number;
  hex: string;
  ascii: string;
}

self.onmessage = (e: MessageEvent<HexRequest>) => {
  const { bytes, baseOffset } = e.data;
  const view = new Uint8Array(bytes);
  const rows: HexRow[] = [];
  const HEX = [];
  for (let i = 0; i < 256; i++) HEX.push(i.toString(16).padStart(2, '0'));

  for (let i = 0; i < view.length; i += 16) {
    const slice = view.subarray(i, i + 16);
    let hex = '';
    let ascii = '';
    for (let j = 0; j < slice.length; j++) {
      const b = slice[j];
      hex += HEX[b] + (j === 7 ? '  ' : ' ');
      ascii += b >= 32 && b < 127 ? String.fromCharCode(b) : '·';
    }
    // pad hex to fixed width so the ascii column aligns
    while (hex.length < 16 * 3 + 1) hex += ' ';
    rows.push({ offset: baseOffset + i, hex, ascii });
  }
  (self as unknown as Worker).postMessage(rows);
};
