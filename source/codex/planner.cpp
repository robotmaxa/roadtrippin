#include "planner.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace roadtrip {
namespace {

constexpr double kEarthRadiusMiles = 3958.8;
constexpr double kPi = 3.14159265358979323846;

double deg_to_rad(double degrees) {
    return degrees * kPi / 180.0;
}

bool same_coordinates(const Location& a, const Location& b) {
    constexpr double kCoordinateTolerance = 0.0001;
    return std::abs(a.latitude - b.latitude) <= kCoordinateTolerance &&
           std::abs(a.longitude - b.longitude) <= kCoordinateTolerance;
}

std::string known_name_for_coordinates(double latitude, double longitude) {
    const Location point{"", latitude, longitude, 0.0, false};
    const Location denver{"Denver,CO", 39.7392, -104.9903, 0.0, false};
    const Location san_francisco{"SanFrancisco,CA", 37.7749, -122.4194, 0.0, true};

    if (same_coordinates(point, denver)) {
        return denver.name;
    }
    if (same_coordinates(point, san_francisco)) {
        return san_francisco.name;
    }
    return "";
}

}  // namespace

bool moves_toward_destination(const Location& from,
                              const Location& to,
                              int direction) {
    constexpr double kTolerance = 0.0001;
    const double delta = to.longitude - from.longitude;
    return direction > 0 ? delta >= -kTolerance : delta <= kTolerance;
}

double distance_miles(const Location& a, const Location& b) {
    const double lat1 = deg_to_rad(a.latitude);
    const double lat2 = deg_to_rad(b.latitude);
    const double dlat = deg_to_rad(b.latitude - a.latitude);
    const double dlon = deg_to_rad(b.longitude - a.longitude);

    const double sin_dlat = std::sin(dlat / 2.0);
    const double sin_dlon = std::sin(dlon / 2.0);
    const double h = sin_dlat * sin_dlat +
                     std::cos(lat1) * std::cos(lat2) * sin_dlon * sin_dlon;
    return 2.0 * kEarthRadiusMiles * std::asin(std::sqrt(h));
}

Dataset load_dataset(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open dataset: " + path);
    }

    Dataset dataset;
    dataset.start.name = "Start";
    dataset.destination.name = "Destination";
    dataset.destination.destination = true;

    if (!(input >> dataset.start.latitude >> dataset.start.longitude)) {
        throw std::runtime_error("Dataset is missing start coordinates.");
    }
    if (!(input >> dataset.destination.latitude >> dataset.destination.longitude)) {
        throw std::runtime_error("Dataset is missing destination coordinates.");
    }

    const std::string known_start =
        known_name_for_coordinates(dataset.start.latitude, dataset.start.longitude);
    const std::string known_destination =
        known_name_for_coordinates(dataset.destination.latitude, dataset.destination.longitude);
    if (!known_start.empty()) {
        dataset.start.name = known_start;
    }
    if (!known_destination.empty()) {
        dataset.destination.name = known_destination;
    }

    int region_count = 0;
    if (!(input >> region_count) || region_count < 0) {
        throw std::runtime_error("Dataset has an invalid region count.");
    }

    for (int region = 0; region < region_count; ++region) {
        double region_latitude = 0.0;
        double region_longitude = 0.0;
        int site_count = 0;
        if (!(input >> region_latitude >> region_longitude >> site_count) ||
            site_count < 0) {
            throw std::runtime_error("Dataset has an invalid region header.");
        }

        for (int site = 0; site < site_count; ++site) {
            Location location;
            if (!(input >> location.name >> location.latitude >>
                  location.longitude >> location.rating)) {
                throw std::runtime_error("Dataset has an invalid site row.");
            }
            if (same_coordinates(location, dataset.start)) {
                dataset.start.name = location.name;
            }
            if (same_coordinates(location, dataset.destination)) {
                dataset.destination.name = location.name;
                location.destination = true;
            }
            dataset.candidates.push_back(location);
        }
    }

    return dataset;
}

std::optional<RouteResult> find_best_route_dfs(const Dataset& dataset,
                                               int visit_stops) {
    if (visit_stops < kMinVisitStops || visit_stops > kMaxVisitStops) {
        throw std::invalid_argument("Visit stops must be between 4 and 7.");
    }

    const int direction =
        dataset.destination.longitude >= dataset.start.longitude ? 1 : -1;

    std::vector<Location> places;
    std::vector<int> candidate_index;
    places.push_back(dataset.start);
    for (int i = 0; i < static_cast<int>(dataset.candidates.size()); ++i) {
        const auto& candidate = dataset.candidates[i];
        if (same_coordinates(candidate, dataset.start) ||
            same_coordinates(candidate, dataset.destination)) {
            continue;
        }
        if (moves_toward_destination(dataset.start, candidate, direction) &&
            moves_toward_destination(candidate, dataset.destination, direction)) {
            places.push_back(candidate);
            candidate_index.push_back(i);
        }
    }
    places.push_back(dataset.destination);

    const int start = 0;
    const int destination = static_cast<int>(places.size()) - 1;
    const int n = static_cast<int>(places.size());

    std::vector<std::vector<double>> dist(n, std::vector<double>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            dist[i][j] = distance_miles(places[i], places[j]);
        }
    }

    // Optimistic suffix bound: best possible remaining ratings, ignoring mileage.
    std::vector<double> sorted_ratings;
    for (int i = 1; i < destination; ++i) {
        sorted_ratings.push_back(places[i].rating);
    }
    std::sort(sorted_ratings.begin(), sorted_ratings.end(), std::greater<double>());

    std::vector<double> best_rating_prefix(visit_stops + 1, 0.0);
    for (int i = 1; i <= visit_stops && i <= static_cast<int>(sorted_ratings.size()); ++i) {
        best_rating_prefix[i] = best_rating_prefix[i - 1] + sorted_ratings[i - 1];
    }
    for (int i = static_cast<int>(sorted_ratings.size()) + 1; i <= visit_stops; ++i) {
        best_rating_prefix[i] = best_rating_prefix[i - 1];
    }

    RouteResult best;
    double best_value = -std::numeric_limits<double>::infinity();
    std::vector<int> current;
    std::vector<bool> used(n, false);
    used[start] = true;
    used[destination] = true;

    auto dfs = [&](auto&& self,
                   int at,
                   int remaining,
                   double rating_sum,
                   double miles) -> void {
        if (rating_sum + best_rating_prefix[remaining] - miles * kMileagePenalty <=
            best_value) {
            return;
        }

        if (remaining == 0) {
            if (!moves_toward_destination(places[at], places[destination], direction) ||
                dist[at][destination] > kMaxDailyMiles) {
                return;
            }

            const double total_miles = miles + dist[at][destination];
            const double value = rating_sum - total_miles * kMileagePenalty;
            if (value > best_value) {
                best_value = value;
                best.stops = current;
                best.rating_sum = rating_sum;
                best.total_miles = total_miles;
                best.route_value = value;
            }
            return;
        }

        for (int next = 1; next < destination; ++next) {
            if (used[next]) {
                continue;
            }
            if (!moves_toward_destination(places[at], places[next], direction)) {
                continue;
            }
            if (dist[at][next] > kMaxDailyMiles) {
                continue;
            }

            used[next] = true;
            current.push_back(next);
            self(self,
                 next,
                 remaining - 1,
                 rating_sum + places[next].rating,
                 miles + dist[at][next]);
            current.pop_back();
            used[next] = false;
        }
    };

    dfs(dfs, start, visit_stops, 0.0, 0.0);

    if (best.stops.empty()) {
        return std::nullopt;
    }

    for (int& stop : best.stops) {
        stop = candidate_index[stop - 1];
    }
    return best;
}

void print_route(const Dataset& dataset, const RouteResult& route) {
    std::cout << "Best Natural Roadtrip Route - Total Miles Driven: "
              << std::fixed << std::setprecision(0) << route.total_miles << "\n\n";

    std::cout << "Start: " << dataset.start.name << "\n";
    Location previous = dataset.start;
    for (int i = 0; i < static_cast<int>(route.stops.size()); ++i) {
        const Location& stop = dataset.candidates[route.stops[i]];
        std::cout << "Stop " << (i + 1) << ": " << stop.name
                  << " | rating " << std::fixed << std::setprecision(1)
                  << stop.rating << "/5 | " << std::setprecision(0)
                  << distance_miles(previous, stop) << " miles from last\n";
        previous = stop;
    }

    std::cout << "Stop " << (route.stops.size() + 1)
              << ": " << dataset.destination.name << " | destination | "
              << std::fixed << std::setprecision(0)
              << distance_miles(previous, dataset.destination)
              << " miles from last\n\n";

    std::cout << "Raw place score: " << std::fixed << std::setprecision(1)
              << route.rating_sum << "/" << route.stops.size() * 5 << "\n";
    std::cout << "Route value: " << std::fixed << std::setprecision(3)
              << route.route_value << "\n";
    std::cout << "Total miles exact: " << std::fixed << std::setprecision(1)
              << route.total_miles << "\n";
}

}  // namespace roadtrip
