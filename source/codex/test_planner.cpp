#include "planner.hpp"

#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {

int tests_run = 0;
int tests_failed = 0;

void expect(bool condition, const std::string& message) {
    ++tests_run;
    if (!condition) {
        ++tests_failed;
        std::cerr << "FAIL: " << message << "\n";
    }
}

void expect_near(double actual, double expected, double tolerance, const std::string& message) {
    expect(std::fabs(actual - expected) <= tolerance, message);
}

void test_dataset_loads() {
    const auto dataset = roadtrip::load_dataset("data/western-usa.txt");
    expect(dataset.start.name == "Denver,CO", "start name should come from dataset");
    expect(dataset.destination.name == "SanFrancisco,CA",
           "known destination coordinate should get a readable name");
    expect(dataset.candidates.size() == 59, "western dataset should contain 59 candidates");
}

void test_invalid_stop_counts_throw() {
    const auto dataset = roadtrip::load_dataset("data/western-usa.txt");
    bool low_threw = false;
    bool high_threw = false;

    try {
        (void)roadtrip::find_best_route_dfs(dataset, 3);
    } catch (const std::invalid_argument&) {
        low_threw = true;
    }

    try {
        (void)roadtrip::find_best_route_dfs(dataset, 8);
    } catch (const std::invalid_argument&) {
        high_threw = true;
    }

    expect(low_threw, "visit stop count below 4 should throw");
    expect(high_threw, "visit stop count above 7 should throw");
}

void test_western_route_constraints() {
    const auto dataset = roadtrip::load_dataset("data/western-usa.txt");
    const auto route = roadtrip::find_best_route_dfs(dataset, 7);
    expect(route.has_value(), "western route should exist");
    if (!route) {
        return;
    }

    expect(route->stops.size() == 7, "7 should mean 7 visit stops before destination");
    expect_near(route->rating_sum, 32.8, 0.01, "western route score should match expected");
    expect_near(route->total_miles, 1013.9, 0.1, "western route miles should match expected");
    expect_near(route->route_value, 17.592, 0.01, "western route value should match expected");

    const int direction =
        dataset.destination.longitude >= dataset.start.longitude ? 1 : -1;
    std::set<int> seen;
    roadtrip::Location previous = dataset.start;
    for (const int stop_index : route->stops) {
        expect(seen.insert(stop_index).second, "route should not repeat stops");
        const auto& stop = dataset.candidates[stop_index];
        expect(roadtrip::moves_toward_destination(previous, stop, direction),
               "route should not reverse east/west direction");
        expect(roadtrip::distance_miles(previous, stop) <= roadtrip::kMaxDailyMiles + 0.01,
               "route leg should stay within 450 miles");
        previous = stop;
    }
    expect(roadtrip::moves_toward_destination(previous, dataset.destination, direction),
           "final leg should not reverse east/west direction");
    expect(roadtrip::distance_miles(previous, dataset.destination) <=
               roadtrip::kMaxDailyMiles + 0.01,
           "final leg should stay within 450 miles");
}

}  // namespace

int main() {
    test_dataset_loads();
    test_invalid_stop_counts_throw();
    test_western_route_constraints();

    if (tests_failed != 0) {
        std::cerr << tests_failed << " of " << tests_run << " tests failed.\n";
        return 1;
    }

    std::cout << "All " << tests_run << " tests passed.\n";
    return 0;
}
