#ifndef ROADTRIP_PLANNER_HPP
#define ROADTRIP_PLANNER_HPP

#include <optional>
#include <string>
#include <vector>

namespace roadtrip {

constexpr int kMinVisitStops = 4;
constexpr int kMaxVisitStops = 7;
constexpr int kDefaultVisitStops = 7;
constexpr double kMaxDailyMiles = 450.0;
constexpr double kMileagePenalty = 0.015;

struct Location {
    std::string name;
    double latitude = 0.0;
    double longitude = 0.0;
    double rating = 0.0;
    bool destination = false;
};

struct Dataset {
    Location start;
    Location destination;
    std::vector<Location> candidates;
};

struct RouteResult {
    std::vector<int> stops;
    double rating_sum = 0.0;
    double total_miles = 0.0;
    double route_value = 0.0;
};

double distance_miles(const Location& a, const Location& b);
Dataset load_dataset(const std::string& path);
bool moves_toward_destination(const Location& from,
                              const Location& to,
                              int direction);

std::optional<RouteResult> find_best_route_dfs(const Dataset& dataset,
                                               int visit_stops = kDefaultVisitStops);

void print_route(const Dataset& dataset, const RouteResult& route);

}  // namespace roadtrip

#endif
