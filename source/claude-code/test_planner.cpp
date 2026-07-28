#include "Dataset.hpp"
#include "Place.hpp"
#include "Planner.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const std::string &what) {
    ++checks;
    if (condition) return;
    ++failures;
    std::cout << "  FAIL: " << what << "\n";
}

// Every rule the planner claims to enforce, re-verified from the outside.
void validateTrip(const Trip &trip, const std::string &label) {
    check(trip.valid(), label + ": produced a trip");
    if (!trip.valid()) return;

    const int stops = static_cast<int>(trip.legs.size());
    check(stops >= rules::MIN_STOPS && stops <= rules::MAX_STOPS,
          label + ": stop count within [4,7], got " + std::to_string(stops));
    check(trip.days() >= rules::MIN_DAYS && trip.days() <= rules::MAX_DAYS,
          label + ": day count within [4,7]");

    const Place *previous = trip.start;
    double running = 0.0;
    for (int i = 0; i < stops; ++i) {
        const Place *p = trip.legs[i].place;
        const std::string leg = label + ": leg " + std::to_string(i + 1);

        // Never reverse heading.
        const bool progresses = trip.direction == Direction::East ? p->lon > previous->lon
                                                                  : p->lon < previous->lon;
        check(progresses, leg + " keeps heading " + directionName(trip.direction));

        // Daily mileage window, both ends.
        check(trip.legs[i].milesFromPrevious <= rules::MAX_MILES_PER_DAY + 1e-6,
              leg + " under the 450 mi cap");
        check(trip.legs[i].milesFromPrevious >= rules::MIN_MILES_PER_DAY - 1e-6,
              leg + " above the 50 mi floor");

        // Reported per-leg mileage matches a fresh computation.
        const double recomputed = drivingMiles(*previous, *p);
        check(std::abs(recomputed - trip.legs[i].milesFromPrevious) < 1e-6,
              leg + " mileage is self-consistent");

        running += trip.legs[i].milesFromPrevious;
        previous = p;
    }

    check(std::abs(running - trip.totalMiles) < 1e-6, label + ": legs sum to the reported total");

    // No stop appears twice.
    for (int i = 0; i < stops; ++i)
        for (int j = i + 1; j < stops; ++j)
            check(trip.legs[i].place != trip.legs[j].place, label + ": no repeated stops");
}

const Place &mustFind(const Dataset &ds, const std::string &name) {
    const Place *p = findPlace(ds.places, name);
    if (!p) {
        std::cout << "  FATAL: dataset is missing \"" << name << "\"\n";
        std::exit(2);
    }
    return *p;
}

} // namespace

int main() {
    Dataset ds;
    std::string error;
    if (!loadDataset("data/western-usa.txt", ds, error)) {
        std::cout << "  FATAL: " << error << "\n(run from the project root)\n";
        return 2;
    }

    std::cout << "Dataset loads:\n";
    check(ds.places.size() == 59, "western-usa.txt has 59 places");
    check(ds.headingIsWest(), "destination line points west of the origin");

    std::cout << "Malformed datasets are rejected, not silently accepted:\n";
    Dataset junk;
    std::string why;
    check(!loadDataset("data/does-not-exist.txt", junk, why), "missing file rejected");
    check(!loadDataset("Makefile", junk, why), "non-dataset file rejected");

    const Planner planner(ds.places);

    std::cout << "Constraint checks across many start points and both headings:\n";
    const char *starts[] = {"Moab", "Durango", "LasVegas", "Flagstaff", "SaltLakeCity", "Page"};
    for (const char *name : starts) {
        const Place &start = mustFind(ds, name);
        for (Direction dir : {Direction::East, Direction::West}) {
            Trip t = planner.planBest(start, dir);
            if (!t.valid()) continue; // legitimately impossible near an edge
            validateTrip(t, std::string(name) + " " + directionName(dir));
        }
    }

    std::cout << "The dataset origin plans without naming a start:\n";
    Trip fromOrigin = planner.planBest(ds.origin, Direction::West);
    validateTrip(fromOrigin, "origin west");

    std::cout << "Exact-length requests honor the requested length:\n";
    for (int stops = rules::MIN_STOPS; stops <= rules::MAX_STOPS; ++stops) {
        Trip t = planner.planExact(ds.origin, Direction::West, stops);
        check(t.valid() && static_cast<int>(t.legs.size()) == stops,
              "origin west with exactly " + std::to_string(stops) + " stops");
        validateTrip(t, "origin west/" + std::to_string(stops));
    }

    std::cout << "Lengths outside the 4-7 range are rejected:\n";
    check(!planner.planExact(ds.origin, Direction::West, 3).valid(), "3 stops rejected");
    check(!planner.planExact(ds.origin, Direction::West, 8).valid(), "8 stops rejected");

    std::cout << "Nature outranks cities:\n";
    check(mustFind(ds, "ZionNP").score() > mustFind(ds, "LasVegas").score(),
          "Zion outscores Las Vegas");
    check(mustFind(ds, "GrandCanyonNP").score() > mustFind(ds, "Phoenix").score(),
          "Grand Canyon outscores Phoenix");

    double datasetMean = 0.0;
    for (const Place &p : ds.places) datasetMean += p.rating;
    datasetMean /= static_cast<double>(ds.places.size());
    for (const Leg &leg : fromOrigin.legs)
        check(leg.place->rating > datasetMean,
              "chosen stop " + leg.place->label() + " beats the dataset mean");

    // Regression guard. MILE_PENALTY once sat at 0.0015, calibrated for an
    // unbounded 0-16 rating scale. On a 0-5 scale that let a 260-mile detour
    // (0.39 of penalty) reject a strictly better-rated park, inverting the
    // stated priority. Tie the constant to the rating scale so a future rescale
    // fails here instead of silently changing the answers.
    std::cout << "Mileage stays a tiebreaker, never a decider:\n";
    const double widestSwing =
        rules::MAX_STOPS * (rules::MAX_MILES_PER_DAY - rules::MIN_MILES_PER_DAY);
    check(rules::MILE_PENALTY * widestSwing < 0.01,
          "the widest possible mileage swing cannot outweigh a 0.01 rating gap");

    std::cout << "Dead end at the edge of the map:\n";
    Trip deadEnd = planner.planBest(mustFind(ds, "YosemiteNP"), Direction::West);
    check(!deadEnd.valid(), "no westbound trip exists from the westernmost stop");

    std::cout << "\n" << (checks - failures) << "/" << checks << " checks passed\n";
    if (failures) {
        std::cout << failures << " FAILED\n";
        return 1;
    }
    std::cout << "All checks passed.\n";
    return 0;
}
