#pragma once

#include <cmath>
#include <string>

namespace roadtrip {

struct Point {
    double lat = 0.0;
    double lon = 0.0;
};

constexpr double EARTH_RADIUS_MILES = 3958.8;
constexpr double MAX_DAILY_MILES_CAP = 450.0;

// Great-circle distance in miles, used as the driving-distance estimate
// since no live routing API is available offline.
inline double haversineMiles(const Point &a, const Point &b) {
    constexpr double kPi = 3.14159265358979323846;
    double lat1 = a.lat * kPi / 180.0, lon1 = a.lon * kPi / 180.0;
    double lat2 = b.lat * kPi / 180.0, lon2 = b.lon * kPi / 180.0;
    double dlat = lat2 - lat1, dlon = lon2 - lon1;
    double h = std::sin(dlat / 2) * std::sin(dlat / 2) +
               std::cos(lat1) * std::cos(lat2) * std::sin(dlon / 2) * std::sin(dlon / 2);
    double c = 2 * std::atan2(std::sqrt(h), std::sqrt(1.0 - h));
    return EARTH_RADIUS_MILES * c;
}

// 8-point compass heading from a to b, for display only.
inline std::string compassHeading(const Point &a, const Point &b) {
    constexpr double kPi = 3.14159265358979323846;
    double lat1 = a.lat * kPi / 180.0, lat2 = b.lat * kPi / 180.0;
    double dlon = (b.lon - a.lon) * kPi / 180.0;
    double y = std::sin(dlon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(dlon);
    double bearing = std::atan2(y, x) * 180.0 / kPi;
    bearing = std::fmod(bearing + 360.0, 360.0);
    static const char *labels[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int idx = static_cast<int>(std::lround(bearing / 45.0)) % 8;
    return labels[idx];
}

} // namespace roadtrip
