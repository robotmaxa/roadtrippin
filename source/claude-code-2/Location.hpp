#pragma once

#include <string>

#include "Geo.hpp"

namespace roadtrip {

enum class Category { NationalPark, NaturalWonder, City };

// Natural spots always outrank cities: this bonus exceeds the max possible
// raw rating (0-5), so any natural location's score beats any city's
// regardless of how their raw ratings compare.
constexpr double NATURAL_BONUS = 5.0;

struct Location {
    std::string name;
    std::string state;
    Point pos;
    Category category = Category::City;
    double rating = 0.0; // raw 0-5 quality score

    double score() const {
        return rating + (category == Category::City ? 0.0 : NATURAL_BONUS);
    }

    bool isNatural() const { return category != Category::City; }

    std::string displayName() const {
        return state.empty() ? name : name + ", " + state;
    }

    std::string categoryLabel() const {
        switch (category) {
            case Category::NationalPark: return "National Park";
            case Category::NaturalWonder: return "Natural Wonder";
            default: return "City";
        }
    }
};

} // namespace roadtrip
