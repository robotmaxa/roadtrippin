#ifndef PLANNER_HPP
#define PLANNER_HPP

#include "Place.hpp"

#include <vector>

// ---------------------------------------------------------------------------
// Trial-run constants. The trip length is allowed to float anywhere in
// [MIN_STOPS, MAX_STOPS]; one stop is one driving day, so the day range and
// the stop range are the same range.
// ---------------------------------------------------------------------------
namespace rules {
constexpr int MIN_STOPS = 4;
constexpr int MAX_STOPS = 7;
constexpr int MIN_DAYS = MIN_STOPS;
constexpr int MAX_DAYS = MAX_STOPS;

// Hard cap: no single day may exceed this.
constexpr double MAX_MILES_PER_DAY = 450.0;

// Two stops closer than this are really one day's sightseeing, not two days
// of driving, so they can't be consecutive nights.
constexpr double MIN_MILES_PER_DAY = 50.0;

// Mileage is a tiebreaker and nothing more: between two trips whose stops are
// equally good, prefer the one with less windshield time. It must never buy a
// worse stop in exchange for a shorter drive.
//
// Ratings are 0-5 carried to two decimals, so the smallest gap that means
// anything is 0.01. Total trip mileage spans at most
//     MAX_STOPS * (MAX_MILES_PER_DAY - MIN_MILES_PER_DAY) = 2800 miles,
// so the penalty is safe exactly when PENALTY * 2800 < 0.01. The value below
// leaves ~3.5x of margin.
//
// The previous 0.0015 was calibrated against an unbounded 0-16 scale, where
// per-stop gaps ran whole points. On a 0-5 scale it silently inverted the
// priority: a 260-mile detour cost 0.39, enough to reject a strictly
// better-rated park. Deriving the constant from the rating scale keeps that
// from recurring the next time the scale changes.
constexpr double MILE_PENALTY = 1e-6;
} // namespace rules

// You commit to a heading at the start and never reverse it. Latitude is
// unconstrained -- north and south are always free.
enum class Direction { East, West };

std::string directionName(Direction d);

struct Leg {
    const Place *place = nullptr;
    double milesFromPrevious = 0.0;
};

struct Trip {
    const Place *start = nullptr;
    Direction direction = Direction::East;
    std::vector<Leg> legs;
    double totalMiles = 0.0;
    double appeal = 0.0;    // sum of category-weighted stop scores
    double objective = 0.0; // appeal minus the mileage tiebreaker

    bool valid() const { return legs.size() >= rules::MIN_STOPS; }
    int days() const { return static_cast<int>(legs.size()); }
    double longestDay() const;
};

class Planner {
public:
    explicit Planner(const std::vector<Place> &places);

    // Best trip with exactly `stops` stops. Returns an invalid Trip if the
    // constraints can't be satisfied.
    Trip planExact(const Place &start, Direction dir, int stops) const;

    // Best trip over the whole legal length range, in the given direction.
    Trip planBest(const Place &start, Direction dir) const;

    // Best trip over the whole legal length range, trying both headings.
    Trip planBestEitherWay(const Place &start) const;

private:
    struct SearchState;

    const std::vector<Place> *places_;

    // Places strictly ahead of `start` along `dir`, ordered by how far along
    // the heading they are.
    std::vector<const Place *> candidatesAhead(const Place &start, Direction dir) const;

    void dfs(SearchState &st, int fromIndex, double milesSoFar, double appealSoFar) const;
};

#endif // PLANNER_HPP
