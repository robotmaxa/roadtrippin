#ifndef DATASET_HPP
#define DATASET_HPP

#include "Place.hpp"

#include <string>
#include <vector>

// One dataset file. Format:
//
//   line 1:  origin lat lon                 -- where the trip starts
//   line 2:  destination lat lon            -- sets the default heading
//   line 3:  region count N
//   then N regions, each:
//     "lat lon siteCount"
//     siteCount lines of "name lat lon rating"
//
// Names are single tokens with an optional ",ST" suffix ("Moab,UT",
// "ArchesNP"). Ratings are 0-5, higher is better, and already encode the
// nature-over-cities preference.
//
// The destination is a heading hint, not a required endpoint: nothing forces
// the trip to finish near it. It exists so a file can say "this is a westbound
// dataset" without the caller having to know the geography.
struct Dataset {
    Place origin;
    double destLat = 0.0;
    double destLon = 0.0;
    std::vector<Place> places;

    bool headingIsWest() const { return destLon < origin.lon; }
};

// Loads `path` into `out`. On failure returns false and puts a human-readable
// reason in `error` -- malformed input is reported, never silently accepted.
bool loadDataset(const std::string &path, Dataset &out, std::string &error);

#endif // DATASET_HPP
