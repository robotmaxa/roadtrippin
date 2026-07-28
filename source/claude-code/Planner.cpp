#include "Planner.hpp"

#include <algorithm>
#include <limits>

std::string directionName(Direction d) {
    return d == Direction::East ? "East" : "West";
}

double Trip::longestDay() const {
    double worst = 0.0;
    for (const Leg &leg : legs) worst = std::max(worst, leg.milesFromPrevious);
    return worst;
}

// Search scratch space. Kept in one struct so the recursive helper stays a
// cheap two-pointer call instead of hauling a dozen parameters around.
struct Planner::SearchState {
    const Place *start = nullptr;
    Direction direction = Direction::East;
    int targetStops = 0;

    std::vector<const Place *> candidates; // ordered along the heading
    std::vector<double> scores;            // candidates[i]->score()

    // suffixBest[i][k] = sum of the k highest scores among candidates[i..n).
    // An upper bound on what the rest of the trip could possibly be worth,
    // ignoring mileage -- which is exactly what makes it safe to prune with.
    std::vector<std::vector<double>> suffixBest;

    std::vector<int> path;    // indices into candidates
    std::vector<int> bestPath;
    double bestObjective = -std::numeric_limits<double>::infinity();
    double bestMiles = 0.0;
    double bestAppeal = 0.0;
};

Planner::Planner(const std::vector<Place> &places) : places_(&places) {}

std::vector<const Place *> Planner::candidatesAhead(const Place &start, Direction dir) const {
    constexpr double kEpsilon = 1e-9;
    std::vector<const Place *> ahead;

    for (const Place &p : *places_) {
        if (&p == &start) continue;
        const bool progresses = dir == Direction::East ? p.lon > start.lon + kEpsilon
                                                       : p.lon < start.lon - kEpsilon;
        if (progresses) ahead.push_back(&p);
    }

    // Order by progress along the heading. Because every leg must also make
    // progress, an ordered list means a trip is always a strictly increasing
    // sequence of indices -- so the search never revisits or backtracks.
    std::sort(ahead.begin(), ahead.end(), [dir](const Place *a, const Place *b) {
        if (a->lon != b->lon) return dir == Direction::East ? a->lon < b->lon : a->lon > b->lon;
        return a->name < b->name; // stable, deterministic output
    });
    return ahead;
}

void Planner::dfs(SearchState &st, int fromIndex, double milesSoFar, double appealSoFar) const {
    const int stopsSoFar = static_cast<int>(st.path.size());

    if (stopsSoFar == st.targetStops) {
        const double objective = appealSoFar - rules::MILE_PENALTY * milesSoFar;
        if (objective > st.bestObjective) {
            st.bestObjective = objective;
            st.bestPath = st.path;
            st.bestMiles = milesSoFar;
            st.bestAppeal = appealSoFar;
        }
        return;
    }

    const int remaining = st.targetStops - stopsSoFar;
    const int n = static_cast<int>(st.candidates.size());
    const Place &current = fromIndex < 0 ? *st.start : *st.candidates[fromIndex];

    for (int j = fromIndex + 1; j < n; ++j) {
        // Prune the whole tail: even the k best stops still reachable, with no
        // further mileage cost, can't beat what we already have.
        if (appealSoFar + st.suffixBest[j][remaining] - rules::MILE_PENALTY * milesSoFar <=
            st.bestObjective) {
            break; // suffixBest is non-increasing in j, so no later j helps
        }

        const double miles = drivingMiles(current, *st.candidates[j]);
        if (miles > rules::MAX_MILES_PER_DAY) continue;
        if (miles < rules::MIN_MILES_PER_DAY) continue;

        st.path.push_back(j);
        dfs(st, j, milesSoFar + miles, appealSoFar + st.candidates[j]->score());
        st.path.pop_back();
    }
}

Trip Planner::planExact(const Place &start, Direction dir, int stops) const {
    Trip trip;
    trip.start = &start;
    trip.direction = dir;

    if (stops < rules::MIN_STOPS || stops > rules::MAX_STOPS) return trip;

    SearchState st;
    st.start = &start;
    st.direction = dir;
    st.targetStops = stops;
    st.candidates = candidatesAhead(start, dir);
    if (static_cast<int>(st.candidates.size()) < stops) return trip;

    const int n = static_cast<int>(st.candidates.size());
    st.scores.reserve(n);
    for (const Place *p : st.candidates) st.scores.push_back(p->score());

    // Build the suffix bounds back-to-front, carrying the running top-K list.
    st.suffixBest.assign(n + 1, std::vector<double>(rules::MAX_STOPS + 1, 0.0));
    std::vector<double> top; // up to MAX_STOPS best scores in the suffix, descending
    for (int i = n - 1; i >= 0; --i) {
        top.insert(std::upper_bound(top.begin(), top.end(), st.scores[i], std::greater<double>()),
                   st.scores[i]);
        if (static_cast<int>(top.size()) > rules::MAX_STOPS) top.pop_back();

        double running = 0.0;
        for (int k = 1; k <= rules::MAX_STOPS; ++k) {
            if (k <= static_cast<int>(top.size())) running += top[k - 1];
            st.suffixBest[i][k] = running;
        }
    }

    st.path.reserve(stops);
    dfs(st, -1, 0.0, 0.0);

    if (st.bestPath.empty()) return trip;

    const Place *previous = &start;
    for (int index : st.bestPath) {
        const Place *p = st.candidates[index];
        trip.legs.push_back({p, drivingMiles(*previous, *p)});
        previous = p;
    }
    trip.totalMiles = st.bestMiles;
    trip.appeal = st.bestAppeal;
    trip.objective = st.bestObjective;
    return trip;
}

Trip Planner::planBest(const Place &start, Direction dir) const {
    Trip best;
    bool found = false;
    for (int stops = rules::MIN_STOPS; stops <= rules::MAX_STOPS; ++stops) {
        Trip candidate = planExact(start, dir, stops);
        if (!candidate.valid()) continue;
        if (!found || candidate.objective > best.objective) {
            best = candidate;
            found = true;
        }
    }
    if (!found) best.start = &start, best.direction = dir;
    return best;
}

Trip Planner::planBestEitherWay(const Place &start) const {
    Trip east = planBest(start, Direction::East);
    Trip west = planBest(start, Direction::West);

    if (!east.valid()) return west;
    if (!west.valid()) return east;
    return east.objective >= west.objective ? east : west;
}
