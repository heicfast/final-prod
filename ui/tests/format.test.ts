import { describe, expect, it } from 'vitest';
import {
  formatBytes,
  formatCountCompact,
  formatDuration,
  formatPercent,
  formatThroughput,
} from '@/lib/format';
import { mulberry32, randInt } from '@/connector/mock/rng';

describe('formatting (grid #49 — Intl-based)', () => {
  it('formats bytes with binary steps (Windows convention, corpus parity)', () => {
    expect(formatBytes(0, 'en')).toBe('0 B');
    expect(formatBytes(1536, 'en')).toBe('1.5 KB');
    expect(formatBytes(5 * 1024 ** 3, 'en')).toBe('5 GB');
    expect(formatBytes(149.45 * 1024 ** 3, 'en')).toBe('149.5 GB');
    expect(formatBytes(-1, 'en')).toBe('—');
  });

  it('formats durations as mm:ss / hh:mm:ss', () => {
    expect(formatDuration(65, 'en')).toBe('01:05');
    expect(formatDuration(3671, 'en')).toBe('01:01:11');
    expect(formatDuration(0, 'en')).toBe('00:00');
  });

  it('formats percentages clamped to [0,1]', () => {
    expect(formatPercent(0.42, 'en')).toBe('42%');
    expect(formatPercent(1.4, 'en')).toBe('100%');
    expect(formatPercent(-0.2, 'en')).toBe('0%');
  });

  it('formats throughput with units', () => {
    expect(formatThroughput(2 * 1024 ** 2, 'en')).toBe('2 MB/s');
    expect(formatThroughput(0, 'en')).toBe('—');
  });

  it('compacts large counts', () => {
    expect(formatCountCompact(1_200_000, 'en')).toMatch(/1\.2M/);
  });
});

describe('mulberry32 RNG', () => {
  it('produces identical sequences for identical seeds', () => {
    const a = mulberry32(42);
    const b = mulberry32(42);
    for (let i = 0; i < 100; i++) {
      expect(a()).toBe(b());
    }
  });

  it('randInt stays within bounds', () => {
    const rng = mulberry32(7);
    for (let i = 0; i < 200; i++) {
      const v = randInt(rng, 3, 9);
      expect(v).toBeGreaterThanOrEqual(3);
      expect(v).toBeLessThanOrEqual(9);
    }
  });
});
