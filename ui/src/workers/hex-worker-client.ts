/**
 * Hex worker client — spins up the module worker with a graceful in-line
 * fallback for environments without Worker (jsdom tests). Both paths produce
 * identical bounded output; the worker path is the default in browser and
 * WebView2.
 */

export interface HexRow {
  offset: number;
  hex: string;
  ascii: string;
}

export async function hexWorker(bytes: Uint8Array, baseOffset: number): Promise<HexRow[]> {
  if (typeof Worker !== 'undefined') {
    return new Promise<HexRow[]>((resolve, reject) => {
      const worker = new Worker(new URL('./hex.worker.ts', import.meta.url), { type: 'module' });
      worker.onmessage = (e: MessageEvent<HexRow[]>) => {
        resolve(e.data);
        worker.terminate();
      };
      worker.onerror = (e) => {
        reject(new Error(e.message || 'hex worker failed'));
        worker.terminate();
      };
      // Copy into a transferable buffer (the Uint8Array may be a view over a larger buffer).
      const copy = bytes.slice().buffer;
      worker.postMessage({ bytes: copy, baseOffset }, [copy]);
    });
  }
  return hexInline(bytes, baseOffset);
}

function hexInline(bytes: Uint8Array, baseOffset: number): HexRow[] {
  const rows: HexRow[] = [];
  const HEX: string[] = [];
  for (let i = 0; i < 256; i++) HEX.push(i.toString(16).padStart(2, '0'));
  for (let i = 0; i < bytes.length; i += 16) {
    const slice = bytes.subarray(i, i + 16);
    let hex = '';
    let ascii = '';
    for (let j = 0; j < slice.length; j++) {
      const b = slice[j]!;
      hex += HEX[b] + (j === 7 ? '  ' : ' ');
      ascii += b >= 32 && b < 127 ? String.fromCharCode(b) : '·';
    }
    while (hex.length < 16 * 3 + 1) hex += ' ';
    rows.push({ offset: baseOffset + i, hex, ascii });
  }
  return rows;
}
