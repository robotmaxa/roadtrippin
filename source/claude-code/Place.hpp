#ifndef PLACE_HPP
#define PLACE_HPP

#include <string>
#include <vector>

// A candidate stop, read from a dataset file.
//
// `rating` is the 0-5 quality score straight out of the file. The "parks and
// natural novelties outrank cities" rule lives in that number rather than in
// code: the bundled datasets already rate nature above towns. That keeps the
// planner honest about where its priorities come from -- swap the file, swap
// the priorities, no recompile.
struct Place {
    std::string name;
    std::string state;
    double lat = 0.0;
    double lon = 0.0; // degrees east, negative across the US
    double rating = 0.0;

    double score() const { return rating; }

    // "Moab, UT" when a state is known, otherwise just "Moab".
    std::string label() const;
};

// Case-insensitive lookup by name or "name, state". Returns nullptr unless the
// query matches exactly one place, so an ambiguous query is an error rather
// than an arbitrary pick.
const Place *findPlace(const std::vector<Place> &places, const std::string &query);

// Great-circle distance in miles, inflated by a road-winding factor so the
// mileage budget means something on real highways.
double drivingMiles(const Place &a, const Place &b);

#endif // PLACE_HPP
