#include "Place.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {

constexpr double kEarthRadiusMiles = 3958.8;

// Highways don't fly straight. Empirically road distance in the lower 48 runs
// ~15-20% longer than great-circle, so the mileage cap stays honest.
constexpr double kRoadWindingFactor = 1.18;

double toRadians(double degrees) { return degrees * M_PI / 180.0; }

// The dataset format is whitespace-delimited, so place names arrive as single
// tokens ("BryceCanyonNP", "GrandCanyonNationalPark"). Comparing with spaces
// removed lets a human type "Grand Canyon National Park" and still match.
std::string normalize(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        if (std::isspace(ch)) continue;
        out.push_back(static_cast<char>(std::tolower(ch)));
    }
    return out;
}

} // namespace

std::string Place::label() const { return state.empty() ? name : name + ", " + state; }

const Place *findPlace(const std::vector<Place> &places, const std::string &query) {
    const std::string needle = normalize(query);
    if (needle.empty()) return nullptr;

    const Place *exact = nullptr;
    const Place *partial = nullptr;
    int partialCount = 0;

    for (const Place &p : places) {
        const std::string name = normalize(p.name);
        const std::string qualified = normalize(p.label());

        if (needle == name || needle == qualified) {
            if (exact) return nullptr; // ambiguous
            exact = &p;
        } else if (qualified.find(needle) != std::string::npos) {
            partial = &p;
            ++partialCount;
        }
    }

    if (exact) return exact;
    return partialCount == 1 ? partial : nullptr;
}

double drivingMiles(const Place &a, const Place &b) {
    const double dLat = toRadians(b.lat - a.lat);
    const double dLon = toRadians(b.lon - a.lon);
    const double lat1 = toRadians(a.lat);
    const double lat2 = toRadians(b.lat);

    const double h = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(lat1) * std::cos(lat2) *
                         std::sin(dLon / 2) * std::sin(dLon / 2);
    const double greatCircle = 2 * kEarthRadiusMiles * std::asin(std::sqrt(h));
    return greatCircle * kRoadWindingFactor;
}
