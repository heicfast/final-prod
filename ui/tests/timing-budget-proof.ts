// Root-cause proof for ui-ci run 44391744949's M31 red.
//
// The repair-page flow awaits TWO serial mock latencies:
//   device enumeration latency(700,200)  -> device list renders (device selected)
//   repair dry-run     latency(600,200)  -> fires only after a device exists
// Total settle time: mean ~1300 ms, worst ~1600 ms.
// RTL's findByText default timeout is 1000 ms — on a slow/coverage-instrumented
// runner the findByText(/no repairable problems/) times out while the dry-run
// skeleton is still in the DOM (exactly what run 44391744949 captured).
//
// This harness reproduces the mock's latency distribution over 2000 trials and
// reports the tail the test budget must cover.
function latencyMs(meanMs: number, jitterMs: number): number {
  const ms = meanMs + (Math.random() - 0.5) * 2 * jitterMs;
  return Math.max(20, ms);
}

let over = 0;
const N = 2000;
let worst = 0;
for (let i = 0; i < N; i++) {
  const total = latencyMs(700, 200) + latencyMs(600, 200);
  if (total > worst) worst = total;
  if (total > 1000) over++;
}
console.log(`trials=${N} worst-total=${worst.toFixed(0)}ms over-1000ms=${over} (${((over / N) * 100).toFixed(1)}%)`);

// Plus RTL polls on an interval (default 50ms), so the effective budget is
// timeout rounded up to the next poll; and the dry-run query only STARTS after
// the device list query resolves and React commits + effect fires. A 2500 ms
// findByText budget covers mean+3σ of the whole chain with margin.
const mean = 1300;
const sigma = Math.sqrt((200 / Math.sqrt(3)) ** 2 * 2); // uniform jitter stdev per leg
console.log(`mean=${mean}ms sigma=${sigma.toFixed(0)}ms mean+3sigma=${(mean + 3 * sigma).toFixed(0)}ms`);
