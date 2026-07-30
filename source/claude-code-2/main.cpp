// Road Trip Optimizer
// ---------------------------------------------------------------------------
// Given a start and destination (typed by name), finds the highest-scoring
// chain of stops between them: national parks and natural wonders always
// outrank cities, every leg is capped at 450 miles/day, and every stop must
// move strictly closer to the destination than the one before it (no
// backtracking, though the route doesn't have to be a straight line).
//
// Usage:
//   ./road_trip                                   interactive prompts
//   ./road_trip "Denver, CO" "Yellowstone" [days] [milesPerDay]
//   ./road_trip --list                             show every known location
//
// Trip length: the user picks how many days/stops the trip should be. This
// matters because rule 1 alone (always end up closer to the destination)
// still permits a route that zig-zags through every scenic stop in the
// country as long as each hop is legal -- capping the number of stops is
// what keeps the result a reasonable "road trip" instead of a cross-country
// scenic-collecting marathon.
// ---------------------------------------------------------------------------

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "roadtrip/Geo.hpp"
#include "roadtrip/Location.hpp"
#include "roadtrip/LocationDatabase.hpp"
#include "roadtrip/RouteOptimizer.hpp"

using namespace roadtrip;

namespace {

constexpr const char *kDataFile = "data/locations.csv";

void printAllLocations(const LocationDatabase &db) {
    std::cout << "Known locations (" << db.all().size() << "):\n";
    for (const auto &loc : db.all()) {
        std::cout << "  " << std::left << std::setw(42) << loc.displayName()
                  << std::setw(16) << loc.categoryLabel()
                  << "rating " << loc.rating << "\n";
    }
}

const Location *resolveLocation(const LocationDatabase &db, const std::string &query,
                                 const std::string &roleLabel) {
    auto matches = db.find(query);
    if (matches.size() == 1) return matches.front();
    if (matches.empty()) {
        std::cerr << "No location found matching \"" << query << "\" for " << roleLabel << ".\n";
        std::cerr << "Run with --list to see every supported location.\n";
        return nullptr;
    }
    std::cerr << "\"" << query << "\" matches multiple locations for " << roleLabel
              << " -- be more specific:\n";
    for (auto *m : matches) std::cerr << "  - " << m->displayName() << "\n";
    return nullptr;
}

void printRoute(const RouteResult &r) {
    std::cout << "\n================ BEST ROAD TRIP ROUTE ================\n";
    std::cout << "Total stops:        " << (r.stops.size() - 1) << " (plus start)\n";
    std::cout << "Total miles driven: " << std::fixed << std::setprecision(1) << r.totalMiles << " mi\n";
    std::cout << "Total scenic score: " << std::fixed << std::setprecision(1) << r.totalScore << "\n";
    std::cout << "-------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(7) << "Stop"
              << std::setw(32) << "Location"
              << std::setw(16) << "Category"
              << std::right << std::setw(10) << "Leg (mi)"
              << std::setw(12) << "Total (mi)" << "\n";
    std::cout << "-------------------------------------------------------------------------\n";

    Point prev = r.stops.front().pos;
    for (size_t i = 0; i < r.stops.size(); ++i) {
        const auto &s = r.stops[i];
        std::string stopLabel = s.isStart ? "Start" : std::to_string(i);
        std::cout << std::left << std::setw(7) << stopLabel
                  << std::setw(32) << s.name
                  << std::setw(16) << (s.isStart ? std::string("-") : s.category)
                  << std::right << std::setw(10) << std::fixed << std::setprecision(1)
                  << (s.isStart ? 0.0 : s.legMiles)
                  << std::setw(12) << std::fixed << std::setprecision(1) << s.cumulativeMiles
                  << "\n";
        prev = s.pos;
    }
    std::cout << "===========================================================================\n";
}

} // namespace

int main(int argc, char *argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);

    std::unique_ptr<LocationDatabase> db;
    try {
        db = std::make_unique<LocationDatabase>(kDataFile);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (!args.empty() && (args[0] == "--list" || args[0] == "-l")) {
        printAllLocations(*db);
        return 0;
    }

    std::string startQuery, destQuery;
    bool cliMode = args.size() >= 2;

    if (cliMode) {
        startQuery = args[0];
        destQuery = args[1];
    } else {
        std::cout << "=== Road Trip Optimizer ===\n";
        std::cout << "(Run with --list to see every supported location.)\n\n";
        std::cout << "Enter start location (e.g. \"Chicago, IL\"): ";
        std::getline(std::cin, startQuery);
        std::cout << "Enter destination location (e.g. \"Yellowstone\"): ";
        std::getline(std::cin, destQuery);
    }

    const Location *start = resolveLocation(*db, startQuery, "start");
    const Location *destination = resolveLocation(*db, destQuery, "destination");
    if (!start || !destination) return 1;
    if (start->displayName() == destination->displayName()) {
        std::cerr << "Start and destination must be different locations.\n";
        return 1;
    }

    double maxDailyMiles = MAX_DAILY_MILES_CAP;
    if (cliMode && args.size() >= 4) {
        maxDailyMiles = std::stod(args[3]);
    } else if (!cliMode) {
        std::cout << "Max miles per day [default " << MAX_DAILY_MILES_CAP << ", cap "
                  << MAX_DAILY_MILES_CAP << "]: ";
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty()) maxDailyMiles = std::stod(line);
    }
    if (maxDailyMiles <= 0 || maxDailyMiles > MAX_DAILY_MILES_CAP + 1e-6) {
        std::cerr << "Max daily miles must be > 0 and <= " << MAX_DAILY_MILES_CAP
                  << ". Clamping to " << MAX_DAILY_MILES_CAP << ".\n";
        maxDailyMiles = MAX_DAILY_MILES_CAP;
    }

    // The direct distance sets a floor on how many days the trip needs; the
    // suggested default adds a little slack so the optimizer has room to
    // route through nearby scenic stops instead of being forced in a
    // straight line.
    const double directDistance = haversineMiles(start->pos, destination->pos);
    const int minDays = std::max(1, static_cast<int>(std::ceil(directDistance / maxDailyMiles - 1e-9)));
    const int suggestedDays = std::min(30, minDays + std::max(2, minDays / 3));

    int tripDays = suggestedDays;
    if (cliMode && args.size() >= 3) {
        tripDays = std::stoi(args[2]);
    } else if (!cliMode) {
        std::cout << "Direct distance: " << std::fixed << std::setprecision(0) << directDistance
                  << " mi (minimum " << minDays << " day(s) at this daily cap)\n";
        std::cout << "Trip length in days [default " << suggestedDays << "]: ";
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty()) tripDays = std::stoi(line);
    }
    if (tripDays < 1 || tripDays > 90) {
        std::cerr << "Trip length must be between 1 and 90 days. Clamping.\n";
        tripDays = std::clamp(tripDays, 1, 90);
    }

    std::vector<Location> candidates;
    candidates.reserve(db->all().size());
    for (const auto &loc : db->all()) {
        if (&loc == start || &loc == destination) continue;
        candidates.push_back(loc);
    }

    RouteOptimizer optimizer(maxDailyMiles, tripDays);
    RouteResult result = optimizer.solve(*start, *destination, candidates);

    std::cout << "\nFrom: " << start->displayName() << "\nTo:   " << destination->displayName() << "\n";
    std::cout << "Trip length: " << tripDays << " day(s) max, " << maxDailyMiles << " mi/day cap\n";

    if (!result.found) {
        std::cout << "\nNo valid route found: the destination could not be reached within "
                  << tripDays << " day(s) at " << maxDailyMiles << " mi/day (needs at least "
                  << minDays << " day(s) for the direct distance alone). Try more days or a "
                  << "higher daily cap.\n";
        return 1;
    }

    printRoute(result);
    return 0;
}
