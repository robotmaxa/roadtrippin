#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "Location.hpp"

namespace roadtrip {

struct RouteStop {
    std::string name;
    std::string category;
    Point pos;
    double legMiles = 0.0;
    double cumulativeMiles = 0.0;
    bool isStart = false;
};

struct RouteResult {
    bool found = false;
    std::vector<RouteStop> stops; // stops[0] is the start, stops.back() is the destination
    double totalScore = 0.0;
    double totalMiles = 0.0;
};

// Finds the highest-scoring chain of at most `maxStops` stops from `start`
// to `destination`.
//
// Rules enforced:
//   1. One-directional travel: every stop must be strictly closer (great-
//      circle distance) to the destination than the stop before it. This
//      allows any detour shape (not just a straight line) while forbidding
//      backtracking, in any compass direction.
//   2. No single leg may exceed the max-daily-miles cap (450 by default).
//   3. The whole trip fits in `maxStops` days/stops -- this is the user's
//      chosen trip length. Without a cap here, rule 1 alone lets the
//      optimizer zig-zag through every scenic stop in the country as long
//      as each individual hop nudges closer to the destination, producing
//      a technically-legal but absurdly long "road trip".
//   4. National parks / natural wonders always outrank cities (see
//      Location::score()), so the optimizer prefers routing through scenic
//      stops whenever the above constraints allow it.
//
// Implementation: candidates form a DAG where an edge u -> v exists iff v is
// closer to the destination than u and the leg fits the daily cap; combined
// with the day budget this is a classic bounded-hop-count longest path.
// dp[k][i] = best cumulative score reaching node i as the k-th stop.
// O(maxStops * n^2), trivial for a few hundred locations.
class RouteOptimizer {
public:
    RouteOptimizer(double maxDailyMiles, int maxStops)
        : maxDailyMiles_(maxDailyMiles), maxStops_(maxStops) {}

    // `candidates` is the pool of possible waypoints; callers should exclude
    // `start` and `destination` from it beforehand.
    RouteResult solve(const Location &start, const Location &destination,
                       const std::vector<Location> &candidates) const {
        RouteResult result;
        const double destRemainingAtStart = haversineMiles(start.pos, destination.pos);

        std::vector<const Location *> nodes;
        std::vector<double> remaining; // distance from node to destination
        for (const auto &loc : candidates) {
            double rem = haversineMiles(loc.pos, destination.pos);
            if (rem < destRemainingAtStart - kProgressEps) {
                nodes.push_back(&loc);
                remaining.push_back(rem);
            }
        }
        // Destination is always the final, mandatory node.
        nodes.push_back(&destination);
        remaining.push_back(0.0);

        const size_t n = nodes.size();
        const int K = std::max(1, maxStops_);
        const double NEG_INF = -std::numeric_limits<double>::infinity();

        // dp[k][i]: best score/miles/parent reaching node i as the k-th stop.
        std::vector<std::vector<double>> dpScore(K + 1, std::vector<double>(n, NEG_INF));
        std::vector<std::vector<double>> dpKey(K + 1, std::vector<double>(n, NEG_INF));
        std::vector<std::vector<double>> dpMiles(K + 1, std::vector<double>(n, 0.0));
        std::vector<std::vector<int>> parent(K + 1, std::vector<int>(n, -1));

        for (size_t i = 0; i < n; ++i) {
            double distFromStart = haversineMiles(start.pos, nodes[i]->pos);
            if (distFromStart <= maxDailyMiles_ + kDistanceEps) {
                dpScore[1][i] = nodes[i]->score();
                dpMiles[1][i] = distFromStart;
                dpKey[1][i] = dpScore[1][i] - kMileageTieBreak * dpMiles[1][i];
            }
        }

        for (int k = 2; k <= K; ++k) {
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = 0; j < n; ++j) {
                    if (dpScore[k - 1][j] == NEG_INF) continue;
                    if (!(remaining[i] < remaining[j] - kProgressEps)) continue;
                    double leg = haversineMiles(nodes[j]->pos, nodes[i]->pos);
                    if (leg > maxDailyMiles_ + kDistanceEps) continue;

                    double candidateScore = dpScore[k - 1][j] + nodes[i]->score();
                    double candidateMiles = dpMiles[k - 1][j] + leg;
                    double candidateKey = candidateScore - kMileageTieBreak * candidateMiles;
                    if (candidateKey > dpKey[k][i]) {
                        dpScore[k][i] = candidateScore;
                        dpMiles[k][i] = candidateMiles;
                        dpKey[k][i] = candidateKey;
                        parent[k][i] = static_cast<int>(j);
                    }
                }
            }
        }

        const size_t destIdx = n - 1;
        int bestK = -1;
        double bestKey = NEG_INF;
        for (int k = 1; k <= K; ++k) {
            if (dpScore[k][destIdx] == NEG_INF) continue;
            if (dpKey[k][destIdx] > bestKey) {
                bestKey = dpKey[k][destIdx];
                bestK = k;
            }
        }
        if (bestK == -1) {
            return result; // destination unreachable within maxStops days
        }

        std::vector<size_t> path;
        int curI = static_cast<int>(destIdx);
        int curK = bestK;
        while (curI != -1) {
            path.push_back(static_cast<size_t>(curI));
            int prevI = parent[curK][curI];
            --curK;
            curI = prevI;
        }
        std::reverse(path.begin(), path.end());

        result.found = true;
        result.totalScore = dpScore[bestK][destIdx];
        result.totalMiles = dpMiles[bestK][destIdx];

        RouteStop startStop;
        startStop.name = start.displayName();
        startStop.category = start.categoryLabel();
        startStop.pos = start.pos;
        startStop.isStart = true;
        result.stops.push_back(startStop);

        Point prev = start.pos;
        double cumulative = 0.0;
        for (size_t idx : path) {
            const Location &loc = *nodes[idx];
            double leg = haversineMiles(prev, loc.pos);
            cumulative += leg;
            RouteStop stop;
            stop.name = loc.displayName();
            stop.category = loc.categoryLabel();
            stop.pos = loc.pos;
            stop.legMiles = leg;
            stop.cumulativeMiles = cumulative;
            result.stops.push_back(stop);
            prev = loc.pos;
        }
        return result;
    }

private:
    double maxDailyMiles_;
    int maxStops_;
    // Guards against float noise when comparing remaining/leg distances to
    // the exact daily cap or to zero progress.
    static constexpr double kProgressEps = 0.5;   // miles
    static constexpr double kDistanceEps = 1e-6;  // miles
    // Purely a tie-breaker between equal-scoring paths (prefer fewer total
    // miles); far too small to ever override a real scenic-score difference.
    static constexpr double kMileageTieBreak = 1e-5;
};

} // namespace roadtrip
