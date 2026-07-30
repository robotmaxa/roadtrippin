#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Location.hpp"

namespace roadtrip {

// Loads the curated CSV of national parks, natural wonders, and cities, and
// resolves free-text user input ("Denver, CO", "Yellowstone") to entries.
class LocationDatabase {
public:
    explicit LocationDatabase(const std::string &csvPath) { load(csvPath); }

    const std::vector<Location> &all() const { return locations_; }

    // Case-insensitive lookup. An exact match on "Name" or "Name, State" wins
    // outright; otherwise every entry whose display name contains the query
    // is returned so the caller can report ambiguity.
    std::vector<const Location *> find(const std::string &query) const {
        std::string q = normalize(query);
        std::vector<const Location *> exact, partial;
        for (const auto &loc : locations_) {
            std::string full = normalize(loc.displayName());
            std::string nameOnly = normalize(loc.name);
            if (full == q || nameOnly == q) {
                exact.push_back(&loc);
            } else if (full.find(q) != std::string::npos) {
                partial.push_back(&loc);
            }
        }
        return !exact.empty() ? exact : partial;
    }

private:
    std::vector<Location> locations_;

    static std::string normalize(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    static Category parseCategory(const std::string &s) {
        if (s == "NATIONAL_PARK") return Category::NationalPark;
        if (s == "NATURAL_WONDER") return Category::NaturalWonder;
        return Category::City;
    }

    void load(const std::string &csvPath) {
        std::ifstream in(csvPath);
        if (!in) {
            throw std::runtime_error("could not open location database: " + csvPath);
        }

        std::string line;
        std::getline(in, line); // header row, discarded
        int lineNo = 1;
        while (std::getline(in, line)) {
            ++lineNo;
            if (line.empty() || line[0] == '#') continue;

            std::stringstream ss(line);
            std::string name, state, latS, lonS, catS, ratingS;
            std::getline(ss, name, ',');
            std::getline(ss, state, ',');
            std::getline(ss, latS, ',');
            std::getline(ss, lonS, ',');
            std::getline(ss, catS, ',');
            std::getline(ss, ratingS, ',');
            if (name.empty() || latS.empty() || lonS.empty()) {
                throw std::runtime_error("malformed row " + std::to_string(lineNo) + " in " + csvPath);
            }

            Location loc;
            loc.name = name;
            loc.state = state;
            loc.pos.lat = std::stod(latS);
            loc.pos.lon = std::stod(lonS);
            loc.category = parseCategory(catS);
            loc.rating = ratingS.empty() ? 0.0 : std::stod(ratingS);
            locations_.push_back(std::move(loc));
        }
        if (locations_.empty()) {
            throw std::runtime_error("location database is empty: " + csvPath);
        }
    }
};

} // namespace roadtrip
