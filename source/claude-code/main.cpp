#include "Dataset.hpp"
#include "Place.hpp"
#include "Planner.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void printUsage(const char *program) {
    std::cout << "Usage: " << program << " <dataset.txt> [start] [east|west|auto]\n"
              << "       " << program << " <dataset.txt> --list\n\n"
              << "Line 1 of the dataset is the default start and line 2 sets the default\n"
              << "heading, so the file alone is enough to run.\n\n"
              << "Examples:\n"
              << "  " << program << " data/western-usa.txt\n"
              << "  " << program << " data/western-usa.txt Moab west\n"
              << "  " << program << " data/catalog-usa.txt \"Denver, CO\" auto\n";
}

void printPlaces(const Dataset &ds) {
    std::vector<const Place *> sorted;
    for (const Place &p : ds.places) sorted.push_back(&p);
    std::sort(sorted.begin(), sorted.end(),
              [](const Place *a, const Place *b) { return a->name < b->name; });

    std::cout << "Dataset (" << sorted.size() << " places)\n\n";
    for (const Place *p : sorted) {
        std::cout << "  " << std::left << std::setw(40) << p->label() << std::right << std::fixed
                  << std::setprecision(2) << std::setw(6) << p->rating << std::setprecision(3)
                  << std::setw(11) << p->lon << "\n";
    }
}

std::string repeat(char c, int n) { return std::string(std::max(n, 0), c); }

void printTrip(const Trip &trip) {
    if (!trip.valid()) {
        std::cout << "No legal trip from " << trip.start->label() << " heading "
                  << directionName(trip.direction) << ".\n"
                  << "Nothing in range satisfies " << rules::MIN_STOPS << "-" << rules::MAX_STOPS
                  << " stops under " << std::fixed << std::setprecision(0)
                  << rules::MAX_MILES_PER_DAY << " miles/day.\n";
        return;
    }

    const double meanRating = trip.appeal / static_cast<double>(trip.legs.size());

    std::cout << "\n" << repeat('=', 78) << "\n";
    std::cout << "  TOTAL MILES DRIVEN: " << std::fixed << std::setprecision(0) << trip.totalMiles
              << "\n";
    std::cout << repeat('=', 78) << "\n";
    std::cout << "  Start      : " << trip.start->label() << "\n";
    std::cout << "  Heading    : " << directionName(trip.direction) << " only (never reversing)\n";
    std::cout << "  Trip length: " << trip.days() << " days / " << trip.legs.size() << " stops\n";
    std::cout << "  Longest day: " << std::setprecision(0) << trip.longestDay() << " mi (cap "
              << rules::MAX_MILES_PER_DAY << ")\n";
    std::cout << "  Trip rating: " << std::setprecision(2) << meanRating << " / 5 average over "
              << trip.legs.size() << " stops\n";
    std::cout << repeat('-', 78) << "\n";

    std::cout << "  " << std::left << std::setw(6) << "STOP" << std::setw(44) << "LOCATION"
              << std::right << std::setw(9) << "RATING" << std::setw(11) << "MILES" << "\n";
    std::cout << repeat('-', 78) << "\n";

    std::cout << "  " << std::left << std::setw(6) << "0" << std::setw(44) << trip.start->label()
              << std::right << std::setw(9) << "-" << std::setw(11) << "-" << "\n";

    int stopNumber = 1;
    for (const Leg &leg : trip.legs) {
        std::cout << "  " << std::left << std::setw(6) << stopNumber++ << std::setw(44)
                  << leg.place->label() << std::right << std::fixed << std::setprecision(2)
                  << std::setw(9) << leg.place->rating << std::setprecision(0) << std::setw(11)
                  << leg.milesFromPrevious << "\n";
    }
    std::cout << repeat('-', 78) << "\n";
    std::cout << "  " << std::left << std::setw(59) << "TOTAL" << std::right << std::setw(11)
              << std::setprecision(0) << trip.totalMiles << "\n";
    std::cout << repeat('=', 78) << "\n";
}

void printAlternatives(const Planner &planner, const Trip &best) {
    std::cout << "\n  Best trip at each legal length (heading " << directionName(best.direction)
              << "):\n";
    for (int stops = rules::MIN_STOPS; stops <= rules::MAX_STOPS; ++stops) {
        Trip t = planner.planExact(*best.start, best.direction, stops);
        std::cout << "    " << stops << " days: ";
        if (!t.valid()) {
            std::cout << "not possible\n";
            continue;
        }
        std::cout << std::fixed << std::setprecision(0) << std::setw(5) << t.totalMiles
                  << " mi, mean rating " << std::setprecision(2) << std::setw(5)
                  << t.appeal / static_cast<double>(t.legs.size());
        if (t.legs.size() == best.legs.size()) std::cout << "   <-- chosen";
        std::cout << "\n";
    }
    std::cout << "\n";
}

} // namespace

int main(int argc, char *argv[]) {
    if (argc < 2 || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        printUsage(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    Dataset ds;
    std::string error;
    if (!loadDataset(argv[1], ds, error)) {
        std::cerr << "Dataset error: " << error << "\n";
        return 1;
    }

    if (argc >= 3 && std::string(argv[2]) == "--list") {
        printPlaces(ds);
        return 0;
    }

    // Start defaults to the dataset's origin; heading defaults to whichever way
    // the destination line points.
    const Place *start = &ds.origin;
    std::string directionArg = ds.headingIsWest() ? "west" : "east";

    if (argc >= 3) {
        start = findPlace(ds.places, argv[2]);
        if (!start) {
            std::cerr << "Could not resolve a unique start location for \"" << argv[2] << "\".\n"
                      << "Run with --list to see what the dataset contains.\n";
            return 1;
        }
    }
    if (argc >= 4) directionArg = argv[3];

    std::transform(directionArg.begin(), directionArg.end(), directionArg.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const Planner planner(ds.places);
    Trip best;
    if (directionArg == "east") {
        best = planner.planBest(*start, Direction::East);
    } else if (directionArg == "west") {
        best = planner.planBest(*start, Direction::West);
    } else if (directionArg == "auto") {
        best = planner.planBestEitherWay(*start);
    } else {
        std::cerr << "Heading must be east, west, or auto.\n";
        return 1;
    }

    printTrip(best);
    if (best.valid()) printAlternatives(planner, best);
    return 0;
}
