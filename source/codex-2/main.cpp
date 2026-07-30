#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusMiles = 3958.8;
constexpr double kMaxDailyMiles = 450.0;

enum class Kind { Natural, Park, City, Destination };

struct Place {
    std::string name;
    std::string state;
    double latitude;
    double longitude;
    Kind kind;
    double rating;
    double ratingMax = 10.0;
};

struct FileTrip {
    Place start;
    Place destination;
    std::vector<Place> places;
};

struct Candidate {
    Place place;
    double progress = 0.0;
    double offRoute = 0.0;
};

struct State {
    double score = -std::numeric_limits<double>::infinity();
    double miles = std::numeric_limits<double>::infinity();
    int previous = -1;
};

double radians(double degrees) { return degrees * kPi / 180.0; }

double haversine(const Place& a, const Place& b) {
    const double dLat = radians(b.latitude - a.latitude);
    const double dLon = radians(b.longitude - a.longitude);
    const double lat1 = radians(a.latitude);
    const double lat2 = radians(b.latitude);
    const double h = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(lat1) * std::cos(lat2) *
                         std::sin(dLon / 2) * std::sin(dLon / 2);
    return 2.0 * kEarthRadiusMiles * std::asin(std::sqrt(h));
}

// A practical estimate for planning without an online maps service.
double roadMiles(const Place& a, const Place& b) {
    const double air = haversine(a, b);
    return air * (air < 80.0 ? 1.18 : 1.13);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<Place> places() {
    using K = Kind;
    return {
        // Natural destinations and parks.
        {"Acadia National Park", "ME", 44.3386, -68.2733, K::Park, 9.7},
        {"Niagara Falls", "NY", 43.0962, -79.0377, K::Natural, 9.7},
        {"Letchworth State Park", "NY", 42.5701, -78.0512, K::Park, 8.7},
        {"Delaware Water Gap", "PA", 41.1292, -74.9057, K::Natural, 8.4},
        {"Shenandoah National Park", "VA", 38.2928, -78.6796, K::Park, 9.3},
        {"New River Gorge National Park", "WV", 38.0689, -81.0831, K::Park, 9.1},
        {"Great Smoky Mountains National Park", "TN", 35.6118, -83.4895, K::Park, 9.6},
        {"Mammoth Cave National Park", "KY", 37.1862, -86.1000, K::Park, 9.3},
        {"Cumberland Falls", "KY", 36.8380, -84.3436, K::Natural, 8.7},
        {"Cuyahoga Valley National Park", "OH", 41.2808, -81.5678, K::Park, 8.3},
        {"Indiana Dunes National Park", "IN", 41.6533, -87.0524, K::Park, 8.2},
        {"Pictured Rocks National Lakeshore", "MI", 46.5631, -86.3161, K::Natural, 9.2},
        {"Sleeping Bear Dunes", "MI", 44.8567, -86.0575, K::Natural, 9.1},
        {"Voyageurs National Park", "MN", 48.4841, -92.8380, K::Park, 9.0},
        {"Badlands National Park", "SD", 43.8554, -102.3397, K::Park, 9.5},
        {"Wind Cave National Park", "SD", 43.5724, -103.4393, K::Park, 8.8},
        {"Theodore Roosevelt National Park", "ND", 46.9790, -103.5387, K::Park, 9.1},
        {"Ozark National Scenic Riverways", "MO", 37.1397, -91.2571, K::Natural, 8.6},
        {"Hot Springs National Park", "AR", 34.5217, -93.0424, K::Park, 8.4},
        {"Buffalo National River", "AR", 35.9850, -92.7577, K::Natural, 9.0},
        {"Tallgrass Prairie Preserve", "KS", 38.4320, -96.5593, K::Natural, 8.2},
        {"Wichita Mountains Wildlife Refuge", "OK", 34.7664, -98.7023, K::Natural, 8.8},
        {"Palo Duro Canyon", "TX", 34.9372, -101.6586, K::Natural, 9.1},
        {"Big Bend National Park", "TX", 29.1275, -103.2425, K::Park, 9.8},
        {"Guadalupe Mountains National Park", "TX", 31.9230, -104.8855, K::Park, 9.3},
        {"Carlsbad Caverns National Park", "NM", 32.1479, -104.5567, K::Park, 9.6},
        {"White Sands National Park", "NM", 32.7791, -106.1717, K::Park, 9.5},
        {"Bandelier National Monument", "NM", 35.7789, -106.2709, K::Natural, 8.8},
        {"Great Sand Dunes National Park", "CO", 37.7916, -105.5943, K::Park, 9.4},
        {"Rocky Mountain National Park", "CO", 40.3428, -105.6836, K::Park, 9.7},
        {"Black Canyon of the Gunnison", "CO", 38.5754, -107.7416, K::Park, 9.2},
        {"Garden of the Gods", "CO", 38.8730, -104.8869, K::Natural, 9.0},
        {"Yellowstone National Park", "WY", 44.4280, -110.5885, K::Park, 10.0},
        {"Grand Teton National Park", "WY", 43.7904, -110.6818, K::Park, 9.9},
        {"Devils Tower National Monument", "WY", 44.5902, -104.7146, K::Natural, 9.2},
        {"Glacier National Park", "MT", 48.7596, -113.7870, K::Park, 9.9},
        {"Craters of the Moon", "ID", 43.4166, -113.5167, K::Natural, 9.0},
        {"Arches National Park", "UT", 38.7331, -109.5925, K::Park, 9.8},
        {"Canyonlands National Park", "UT", 38.2136, -109.9025, K::Park, 9.7},
        {"Capitol Reef National Park", "UT", 38.3670, -111.2615, K::Park, 9.3},
        {"Bryce Canyon National Park", "UT", 37.6283, -112.1677, K::Park, 9.8},
        {"Zion National Park", "UT", 37.2982, -113.0263, K::Park, 9.9},
        {"Grand Canyon National Park", "AZ", 36.1069, -112.1129, K::Park, 10.0},
        {"Monument Valley", "AZ", 36.9980, -110.0986, K::Natural, 9.6},
        {"Petrified Forest National Park", "AZ", 34.9099, -109.8068, K::Park, 9.1},
        {"Saguaro National Park", "AZ", 32.2967, -111.1666, K::Park, 9.2},
        {"Joshua Tree National Park", "CA", 33.8734, -115.9010, K::Park, 9.5},
        {"Death Valley National Park", "CA", 36.5054, -117.0794, K::Park, 9.7},
        {"Yosemite National Park", "CA", 37.8651, -119.5383, K::Park, 10.0},
        {"Sequoia National Park", "CA", 36.4864, -118.5658, K::Park, 9.8},
        {"Redwood National and State Parks", "CA", 41.2132, -124.0046, K::Park, 9.8},
        {"Crater Lake National Park", "OR", 42.9446, -122.1090, K::Park, 9.7},
        {"Columbia River Gorge", "OR", 45.7253, -121.7297, K::Natural, 9.5},
        {"Olympic National Park", "WA", 47.8021, -123.6044, K::Park, 9.8},
        {"Mount Rainier National Park", "WA", 46.8800, -121.7269, K::Park, 9.8},
        {"North Cascades National Park", "WA", 48.7718, -121.2985, K::Park, 9.6},

        // Cities and travel hubs make the 450-mile daily constraint practical.
        {"Boston", "MA", 42.3601, -71.0589, K::City, 3.5},
        {"New York City", "NY", 40.7128, -74.0060, K::City, 3.4},
        {"Philadelphia", "PA", 39.9526, -75.1652, K::City, 3.2},
        {"Washington", "DC", 38.9072, -77.0369, K::City, 3.4},
        {"Pittsburgh", "PA", 40.4406, -79.9959, K::City, 3.0},
        {"Charlotte", "NC", 35.2271, -80.8431, K::City, 2.8},
        {"Atlanta", "GA", 33.7490, -84.3880, K::City, 3.2},
        {"Jacksonville", "FL", 30.3322, -81.6557, K::City, 2.8},
        {"Miami", "FL", 25.7617, -80.1918, K::City, 3.5},
        {"Cleveland", "OH", 41.4993, -81.6944, K::City, 2.7},
        {"Detroit", "MI", 42.3314, -83.0458, K::City, 2.8},
        {"Chicago", "IL", 41.8781, -87.6298, K::City, 3.8},
        {"Nashville", "TN", 36.1627, -86.7816, K::City, 3.5},
        {"Memphis", "TN", 35.1495, -90.0490, K::City, 3.1},
        {"Birmingham", "AL", 33.5186, -86.8104, K::City, 2.6},
        {"New Orleans", "LA", 29.9511, -90.0715, K::City, 3.7},
        {"Minneapolis", "MN", 44.9778, -93.2650, K::City, 3.1},
        {"St. Louis", "MO", 38.6270, -90.1994, K::City, 3.1},
        {"Kansas City", "MO", 39.0997, -94.5786, K::City, 3.0},
        {"Omaha", "NE", 41.2565, -95.9345, K::City, 2.7},
        {"Fargo", "ND", 46.8772, -96.7898, K::City, 2.4},
        {"Sioux Falls", "SD", 43.5446, -96.7311, K::City, 2.4},
        {"Rapid City", "SD", 44.0805, -103.2310, K::City, 2.8},
        {"Oklahoma City", "OK", 35.4676, -97.5164, K::City, 2.7},
        {"Dallas", "TX", 32.7767, -96.7970, K::City, 3.1},
        {"Austin", "TX", 30.2672, -97.7431, K::City, 3.6},
        {"Houston", "TX", 29.7604, -95.3698, K::City, 3.2},
        {"San Antonio", "TX", 29.4241, -98.4936, K::City, 3.5},
        {"Amarillo", "TX", 35.2220, -101.8313, K::City, 2.6},
        {"El Paso", "TX", 31.7619, -106.4850, K::City, 2.8},
        {"Albuquerque", "NM", 35.0844, -106.6504, K::City, 3.3},
        {"Denver", "CO", 39.7392, -104.9903, K::City, 3.6},
        {"Cheyenne", "WY", 41.1400, -104.8202, K::City, 2.4},
        {"Billings", "MT", 45.7833, -108.5007, K::City, 2.5},
        {"Boise", "ID", 43.6150, -116.2023, K::City, 3.0},
        {"Salt Lake City", "UT", 40.7608, -111.8910, K::City, 3.3},
        {"Las Vegas", "NV", 36.1699, -115.1398, K::City, 3.7},
        {"Phoenix", "AZ", 33.4484, -112.0740, K::City, 3.2},
        {"Tucson", "AZ", 32.2226, -110.9747, K::City, 3.0},
        {"San Diego", "CA", 32.7157, -117.1611, K::City, 3.7},
        {"Los Angeles", "CA", 34.0522, -118.2437, K::City, 3.7},
        {"Fresno", "CA", 36.7378, -119.7871, K::City, 2.4},
        {"San Francisco", "CA", 37.7749, -122.4194, K::City, 4.0},
        {"Sacramento", "CA", 38.5816, -121.4944, K::City, 2.8},
        {"Reno", "NV", 39.5296, -119.8138, K::City, 2.8},
        {"Portland", "OR", 45.5152, -122.6784, K::City, 3.6},
        {"Seattle", "WA", 47.6062, -122.3321, K::City, 3.8},
        {"Spokane", "WA", 47.6588, -117.4260, K::City, 2.7}
    };
}

std::optional<Place> parseCoordinates(const std::string& input, const std::string& name) {
    std::istringstream stream(input);
    double lat = 0.0, lon = 0.0;
    char comma = '\0';
    if ((stream >> lat >> comma >> lon) && comma == ',' && lat >= -90 && lat <= 90 &&
        lon >= -180 && lon <= 180) {
        return Place{name, "", lat, lon, Kind::Destination, 0.0};
    }
    return std::nullopt;
}

std::optional<Place> findPlace(const std::string& input, const std::vector<Place>& data) {
    const std::string query = lower(trim(input));
    for (const auto& place : data) {
        if (lower(place.name) == query || lower(place.name + ", " + place.state) == query)
            return place;
    }
    std::vector<const Place*> partial;
    for (const auto& place : data) {
        if (lower(place.name).find(query) != std::string::npos) partial.push_back(&place);
    }
    if (partial.size() == 1) return *partial.front();
    return parseCoordinates(query, input);
}

std::string label(const Place& place) {
    return place.state.empty() ? place.name : place.name + ", " + place.state;
}

double projectedProgress(const Place& start, const Place& destination, const Place& point) {
    // Equirectangular local projection, sufficient for ordering continental routes.
    const double meanLat = radians((start.latitude + destination.latitude) / 2.0);
    const double dx = (destination.longitude - start.longitude) * std::cos(meanLat);
    const double dy = destination.latitude - start.latitude;
    const double px = (point.longitude - start.longitude) * std::cos(meanLat);
    const double py = point.latitude - start.latitude;
    const double denom = dx * dx + dy * dy;
    return denom == 0.0 ? 0.0 : (px * dx + py * dy) / denom;
}

double offRouteMiles(const Place& start, const Place& destination, const Place& point) {
    const double progress = std::clamp(projectedProgress(start, destination, point), 0.0, 1.0);
    Place projected{"", "", start.latitude + progress * (destination.latitude - start.latitude),
                    start.longitude + progress * (destination.longitude - start.longitude),
                    Kind::Destination, 0.0};
    return haversine(point, projected);
}

double visitValue(const Candidate& candidate) {
    if (candidate.place.kind == Kind::Destination) return 0.0;
    const double normalizedRating = candidate.place.rating / candidate.place.ratingMax;
    // A highly rated city is useful, but it must not outrank a similarly rated
    // natural destination. Cities primarily serve as overnight connectors.
    if (candidate.place.kind == Kind::City)
        return normalizedRating * 8.0 - candidate.offRoute * 0.055;
    const double natureBonus = candidate.place.kind == Kind::Natural ? 28.0 :
                               candidate.place.kind == Kind::Park ? 24.0 : 0.0;
    return normalizedRating * 80.0 + natureBonus - candidate.offRoute * 0.055;
}

Kind classifyFilePlace(const std::string& name) {
    const std::string value = lower(name);
    const auto endsWith = [&value](const std::string& suffix) {
        return value.size() >= suffix.size() &&
               value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    if (endsWith("np") ||
        value.find("nationalpark") != std::string::npos ||
        endsWith("sp"))
        return Kind::Park;
    if (value.find("valley") != std::string::npos ||
        value.find("canyon") != std::string::npos ||
        value.find("dunes") != std::string::npos ||
        value.find("monument") != std::string::npos ||
        value.find("bend") != std::string::npos)
        return Kind::Natural;
    return Kind::City;
}

std::optional<FileTrip> loadTripFile(const std::string& path, std::string& error) {
    std::ifstream input(path);
    if (!input) {
        error = "Could not open data file '" + path + "'.";
        return std::nullopt;
    }

    double startLat = 0.0, startLon = 0.0, destinationLat = 0.0, destinationLon = 0.0;
    std::size_t count = 0;
    if (!(input >> startLat >> startLon >> destinationLat >> destinationLon >> count)) {
        error = "The file header must contain start coordinates, destination coordinates, and record count.";
        return std::nullopt;
    }

    FileTrip trip{
        {"Start", "", startLat, startLon, Kind::Destination, 0.0, 5.0},
        {"Destination", "", destinationLat, destinationLon, Kind::Destination, 0.0, 5.0},
        {}
    };
    trip.places.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        double markerLat = 0.0, markerLon = 0.0;
        int marker = 0;
        std::string name;
        double latitude = 0.0, longitude = 0.0, rating = 0.0;
        if (!(input >> markerLat >> markerLon >> marker >>
              name >> latitude >> longitude >> rating)) {
            error = "Record " + std::to_string(i + 1) +
                    " is incomplete; expected two lines per record.";
            return std::nullopt;
        }
        if (marker != 1 || std::abs(markerLat - latitude) > 0.001 ||
            std::abs(markerLon - longitude) > 0.001) {
            error = "Record " + std::to_string(i + 1) +
                    " has mismatched coordinate lines or an invalid marker.";
            return std::nullopt;
        }
        if (rating < 0.0 || rating > 5.0) {
            error = "Record " + std::to_string(i + 1) + " has a rating outside 0-5.";
            return std::nullopt;
        }

        const auto comma = name.rfind(',');
        std::string state;
        if (comma != std::string::npos) {
            state = name.substr(comma + 1);
            name = name.substr(0, comma);
        }
        Place place{name, state, latitude, longitude, classifyFilePlace(name), rating, 5.0};
        if (haversine(trip.start, place) < 1.0) trip.start = place;
        if (haversine(trip.destination, place) < 1.0) trip.destination = place;
        trip.places.push_back(std::move(place));
    }

    std::string extra;
    if (input >> extra) {
        error = "The file contains data after the declared " + std::to_string(count) + " records.";
        return std::nullopt;
    }
    return trip;
}

std::optional<std::vector<Candidate>> planTrip(
    const Place& start, const Place& destination, const std::vector<Place>& data,
    double corridorMiles) {
    std::vector<Candidate> nodes;
    nodes.push_back({start, 0.0, 0.0});
    for (const auto& place : data) {
        if (lower(place.name) == lower(start.name) ||
            lower(place.name) == lower(destination.name)) continue;
        const double progress = projectedProgress(start, destination, place);
        const double offset = offRouteMiles(start, destination, place);
        if (progress > 0.006 && progress < 0.994 && offset <= corridorMiles)
            nodes.push_back({place, progress, offset});
    }
    nodes.push_back({destination, 1.0, 0.0});
    std::sort(nodes.begin() + 1, nodes.end() - 1,
              [](const Candidate& a, const Candidate& b) { return a.progress < b.progress; });

    std::vector<State> best(nodes.size());
    best[0] = {0.0, 0.0, -1};
    for (std::size_t j = 1; j < nodes.size(); ++j) {
        for (std::size_t i = 0; i < j; ++i) {
            if (!std::isfinite(best[i].score)) continue;
            const double leg = roadMiles(nodes[i].place, nodes[j].place);
            if (leg > kMaxDailyMiles + 1e-9) continue;
            const double directProgressMiles =
                haversine(start, destination) * (nodes[j].progress - nodes[i].progress);
            const double detour = std::max(0.0, leg - directProgressMiles);
            double score = best[i].score + visitValue(nodes[j]) - detour * 0.10 - 7.0;
            // Prefer natural stops; cities are mainly used to satisfy the daily cap.
            if (nodes[j].place.kind == Kind::City) score -= 8.0;
            const double miles = best[i].miles + leg;
            if (score > best[j].score + 1e-9 ||
                (std::abs(score - best[j].score) < 1e-9 && miles < best[j].miles)) {
                best[j] = {score, miles, static_cast<int>(i)};
            }
        }
    }

    if (!std::isfinite(best.back().score)) return std::nullopt;
    std::vector<Candidate> route;
    for (int index = static_cast<int>(nodes.size()) - 1; index >= 0;
         index = best[static_cast<std::size_t>(index)].previous) {
        route.push_back(nodes[static_cast<std::size_t>(index)]);
    }
    std::reverse(route.begin(), route.end());
    return route;
}

std::string kindName(Kind kind) {
    switch (kind) {
        case Kind::Natural: return "Natural wonder";
        case Kind::Park: return "Park";
        case Kind::City: return "City / overnight hub";
        case Kind::Destination: return "Destination";
    }
    return "";
}

void printRoute(const std::vector<Candidate>& route) {
    double total = 0.0;
    for (std::size_t i = 1; i < route.size(); ++i)
        total += roadMiles(route[i - 1].place, route[i].place);

    std::cout << "\nBEST FORWARD-ONLY ROAD TRIP — TOTAL "
              << std::fixed << std::setprecision(0) << total << " MILES\n";
    std::cout << "============================================================\n";
    std::cout << "Start: " << label(route.front().place) << "\n\n";
    std::cout << std::left << std::setw(6) << "Stop" << std::setw(42) << "Location"
              << std::right << std::setw(12) << "Leg miles" << "\n";
    std::cout << std::string(60, '-') << "\n";
    for (std::size_t i = 1; i < route.size(); ++i) {
        const double leg = roadMiles(route[i - 1].place, route[i].place);
        std::cout << std::left << std::setw(6) << i
                  << std::setw(42) << label(route[i].place).substr(0, 41)
                  << std::right << std::setw(12) << std::fixed << std::setprecision(0)
                  << leg << "\n";
        if (i + 1 < route.size())
            std::cout << "      " << kindName(route[i].place.kind)
                      << " | attraction rating " << std::setprecision(1)
                      << route[i].place.rating << "/" << route[i].place.ratingMax << "\n";
    }
    std::cout << std::string(60, '=') << "\n";
    std::cout << "TOTAL MILES DRIVEN: " << std::fixed << std::setprecision(0) << total << "\n";
    std::cout << "DRIVING DAYS / LEGS: " << route.size() - 1 << "\n";
    std::cout << "(All legs are <= " << std::setprecision(0) << kMaxDailyMiles
              << " estimated road miles.)\n";
}

} // namespace

int main(int argc, char* argv[]) {
    auto data = places();
    std::string startInput, destinationInput;
    std::optional<Place> fileStart;
    std::optional<Place> fileDestination;

    if (argc == 2) {
        std::string error;
        const auto fileTrip = loadTripFile(argv[1], error);
        if (!fileTrip) {
            std::cerr << error << "\n";
            return 1;
        }
        auto loaded = *fileTrip;
        // Give header coordinates a friendly name when they match a built-in place.
        for (const auto& known : data) {
            if (loaded.start.name == "Start" && haversine(loaded.start, known) < 1.0)
                loaded.start = known;
            if (loaded.destination.name == "Destination" &&
                haversine(loaded.destination, known) < 1.0)
                loaded.destination = known;
        }
        data = std::move(loaded.places);
        fileStart = loaded.start;
        fileDestination = loaded.destination;
    } else if (argc >= 3) {
        startInput = argv[1];
        destinationInput = argv[2];
    } else {
        std::cout << "Road Trip Planner\n"
                  << "Enter a known U.S. city/attraction or coordinates as latitude,longitude.\n";
        std::cout << "Start: ";
        std::getline(std::cin, startInput);
        std::cout << "Destination: ";
        std::getline(std::cin, destinationInput);
    }

    const auto start = fileStart ? fileStart : findPlace(startInput, data);
    const auto destination = fileDestination ? fileDestination : findPlace(destinationInput, data);
    if (!start || !destination) {
        std::cerr << "Could not identify ";
        if (!start) std::cerr << "start '" << startInput << "'";
        if (!start && !destination) std::cerr << " or ";
        if (!destination) std::cerr << "destination '" << destinationInput << "'";
        std::cerr << ".\nUse a listed place name or latitude,longitude (example: 39.7392,-104.9903).\n";
        return 1;
    }
    if (haversine(*start, *destination) < 1.0) {
        std::cerr << "Start and destination must be different locations.\n";
        return 1;
    }

    // Begin with an enjoyable corridor and widen it only when needed for connectivity.
    std::optional<std::vector<Candidate>> route;
    double usedCorridor = 0.0;
    for (const double corridor : {180.0, 300.0, 500.0, 800.0, 1300.0}) {
        route = planTrip(*start, *destination, data, corridor);
        if (route) {
            usedCorridor = corridor;
            break;
        }
    }
    if (!route) {
        std::cerr << "No route can satisfy the 450-mile daily limit with the embedded stops.\n"
                  << "Try coordinates closer to a major U.S. travel corridor or add places to the dataset.\n";
        return 2;
    }

    printRoute(*route);
    std::cout << "Search corridor used: " << usedCorridor << " miles from the general route.\n"
              << "Mileage is estimated; verify roads, closures, and exact distances before travel.\n";
    return 0;
}
