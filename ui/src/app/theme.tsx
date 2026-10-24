import { createContext, useCallback, useContext, useEffect, useMemo, useState, type ReactNode } from 'react';

export type ThemeMode = 'system' | 'light' | 'dark' | 'high-contrast';
export type Density = 'comfortable' | 'compact';

interface ThemeContextValue {
  mode: ThemeMode;
  /** Effective visual theme ('dark' when system resolves dark). */
  resolved: 'light' | 'dark';
  setMode: (mode: ThemeMode) => void;
  density: Density;
  setDensity: (density: Density) => void;
}

const STORAGE_KEY = 'rw.ui.theme';
const DENSITY_KEY = 'rw.ui.density';

const ThemeContext = createContext<ThemeContextValue | null>(null);

function readStoredMode(): ThemeMode {
  try {
    const v = localStorage.getItem(STORAGE_KEY);
    if (v === 'light' || v === 'dark' || v === 'high-contrast' || v === 'system') return v;
  } catch {
    /* storage unavailable — fall through */
  }
  return 'system';
}

function readStoredDensity(): Density {
  try {
    const v = localStorage.getItem(DENSITY_KEY);
    if (v === 'comfortable' || v === 'compact') return v;
  } catch {
    /* storage unavailable */
  }
  return 'comfortable';
}

function systemPrefersDark(): boolean {
  return typeof window !== 'undefined' && window.matchMedia('(prefers-color-scheme: dark)').matches;
}

/** Applies token classes to <html>. High-contrast tracks its own light/dark pair. */
function applyTheme(mode: ThemeMode, systemDark: boolean): 'light' | 'dark' {
  const root = document.documentElement;
  root.classList.remove('dark', 'hc');
  let resolved: 'light' | 'dark' = 'light';
  if (mode === 'high-contrast') {
    resolved = systemDark ? 'dark' : 'light';
    root.classList.add('hc');
    if (systemDark) root.classList.add('dark');
  } else if (mode === 'dark') {
    resolved = 'dark';
    root.classList.add('dark');
  } else if (mode === 'light') {
    resolved = 'light';
  } else {
    resolved = systemDark ? 'dark' : 'light';
    if (systemDark) root.classList.add('dark');
  }
  // HC must also react to forced-colors (Windows HC) by tracking it on data attr.
  if (typeof window !== 'undefined' && window.matchMedia('(forced-colors: active)').matches) {
    root.dataset.forcedColors = 'active';
  } else {
    delete root.dataset.forcedColors;
  }
  return resolved;
}

export function ThemeProvider({ children }: { children: ReactNode }): React.JSX.Element {
  const [mode, setModeState] = useState<ThemeMode>(() => readStoredMode());
  const [density, setDensityState] = useState<Density>(() => readStoredDensity());
  const [systemDark, setSystemDark] = useState<boolean>(() => systemPrefersDark());

  useEffect(() => {
    const mq = window.matchMedia('(prefers-color-scheme: dark)');
    const onChange = (e: MediaQueryListEvent): void => setSystemDark(e.matches);
    mq.addEventListener('change', onChange);
    return () => mq.removeEventListener('change', onChange);
  }, []);

  useEffect(() => {
    const fc = window.matchMedia('(forced-colors: active)');
    const onFc = () => {
      applyTheme(mode, systemDark);
    };
    fc.addEventListener('change', onFc);
    return () => fc.removeEventListener('change', onFc);
  }, [mode, systemDark]);

  const resolved = useMemo(() => applyTheme(mode, systemDark), [mode, systemDark]);

  useEffect(() => {
    document.documentElement.dataset.density = density;
  }, [density]);

  const setMode = useCallback((next: ThemeMode) => {
    setModeState(next);
    try {
      localStorage.setItem(STORAGE_KEY, next);
    } catch {
      /* storage unavailable */
    }
  }, []);

  const setDensity = useCallback((next: Density) => {
    setDensityState(next);
    try {
      localStorage.setItem(DENSITY_KEY, next);
    } catch {
      /* storage unavailable */
    }
  }, []);

  const value = useMemo<ThemeContextValue>(
    () => ({ mode, resolved, setMode, density, setDensity }),
    [mode, resolved, setMode, density, setDensity],
  );

  return <ThemeContext.Provider value={value}>{children}</ThemeContext.Provider>;
}

export function useTheme(): ThemeContextValue {
  const ctx = useContext(ThemeContext);
  if (!ctx) throw new Error('useTheme must be used within ThemeProvider');
  return ctx;
}
