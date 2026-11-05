/* cr_visited_runs.h - M-127: the og's chain cycle-guard data structure.
 *
 * The og's FAT/exFAT chain walker inserts each contiguous cluster RUN
 * into a red-black range tree and treats a repeated cluster as the
 * cycle break ("a cycle-breaker so corrupt FAT loops (a cluster
 * pointing backward) cannot hang the scan" - corpus
 * SearchDLL/fat-exfat-engine-algorithms-v2.md:43-56). The pre-M-127
 * core used a visited VECTOR with a linear scan per cluster - O(n^2)
 * per chain, 65,536^2/2 = 2.1e9 comparisons on a maximal chain (the
 * 100+ s stall class on chain-heavy FAT32). This run-coalescing
 * containment set is the og's shape: each contiguous run is ONE
 * interval, extension of the tail run is O(log runs), total
 * O(clusters x log runs) worst case and O(runs x log runs) typical -
 * the milliseconds class.
 *
 * Semantics are IDENTICAL to the vector guard: visit() returns true
 * exactly when the cluster was already covered (the cycle verdict);
 * the caps and per-step cancel checks are the CALLERS' (unchanged -
 * Q3/D-14 register those).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_VISITED_RUNS_H_
#define CR_VISITED_RUNS_H_

#include <map>

namespace cr {
namespace fat {

class VisitedRuns {
public:
    /* Record cluster c as visited. Returns true when c was ALREADY
     * covered by a visited run - the cycle verdict (the vector guard's
     * linear-scan hit, at O(log runs) instead of O(clusters)). */
    bool visit(u32 c) {
        auto it = runs_.upper_bound(c);
        if (it != runs_.begin()) {
            --it;
            if (c <= it->second)
                return true; /* inside the run [first, second] */
            if (c == it->second + 1) {
                /* extend the tail run rightward (the common contiguous
                 * walk) and coalesce an adjacent follower */
                it->second = c;
                auto nx = std::next(it);
                if (nx != runs_.end() && nx->first == u32(c) + 1) {
                    it->second = nx->second;
                    runs_.erase(nx);
                }
                return false;
            }
        }
        /* a new run: coalesce with an adjacent follower (the fragmented
         * chain that hops forward by one) */
        auto [ins, ok] = runs_.emplace(c, c);
        (void)ok;
        auto nx = std::next(ins);
        if (nx != runs_.end() && nx->first == u32(c) + 1) {
            ins->second = nx->second;
            runs_.erase(nx);
        }
        return false;
    }

    size_t run_count() const { return runs_.size(); }

private:
    std::map<u32, u32> runs_; /* run start -> inclusive end */
};

} // namespace fat
} // namespace cr

#endif // CR_VISITED_RUNS_H_
