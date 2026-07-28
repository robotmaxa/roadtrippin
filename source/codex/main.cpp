#include "planner.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string resolve_dataset_path(const char* executable_path, int argc, char* argv[]) {
    if (argc >= 2) {
        return argv[1];
    }

    const std::filesystem::path executable = std::filesystem::absolute(executable_path);
    const std::vector<std::filesystem::path> candidates{
        "data/western-usa.txt",
        executable.parent_path() / "data" / "western-usa.txt",
        executable.parent_path().parent_path() / "data" / "western-usa.txt",
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate.string();
        }
    }

    return "data/western-usa.txt";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const std::string dataset_path = resolve_dataset_path(argv[0], argc, argv);
        const int visit_stops =
            argc >= 3 ? std::stoi(argv[2]) : roadtrip::kDefaultVisitStops;

        const auto dataset = roadtrip::load_dataset(dataset_path);
        const auto route = roadtrip::find_best_route_dfs(dataset, visit_stops);
        if (!route) {
            std::cerr << "No feasible route found with " << visit_stops
                      << " visit stops and a " << roadtrip::kMaxDailyMiles
                      << "-mile daily limit.\n";
            return 1;
        }
        roadtrip::print_route(dataset, *route);
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }

    return 0;
}
