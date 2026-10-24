import * as React from 'react';
import { useDevices } from '@/queries/devices';
import { SplashScreen } from '@/features/splash/splash-screen';

const MIN_SPLASH_MS = 360;
const BOOT_SPLASH_MS = 180;

/**
 * Prevents first paint from becoming a blank frame or a half-populated home
 * page. Discovery errors intentionally release the gate: Home owns the
 * human-readable error state and its Retry action.
 */
export function StartupGate({ children }: { children: React.ReactNode }): React.JSX.Element {
  const devices = useDevices();
  const [minimumElapsed, setMinimumElapsed] = React.useState(false);
  const [booted, setBooted] = React.useState(false);

  React.useEffect(() => {
    const id = window.setTimeout(() => setMinimumElapsed(true), MIN_SPLASH_MS);
    return () => window.clearTimeout(id);
  }, []);
  React.useEffect(() => {
    const id = window.setTimeout(() => setBooted(true), BOOT_SPLASH_MS);
    return () => window.clearTimeout(id);
  }, []);

  if (!minimumElapsed || devices.isLoading) {
    return <SplashScreen phase={booted ? 'enumerating' : 'booting'} />;
  }
  return <>{children}</>;
}
