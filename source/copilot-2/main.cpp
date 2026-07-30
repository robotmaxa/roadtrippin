#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>

struct Location {
    std::string name;
    double lat;
    double lon;
    double rating; // natural places have higher rating
};

struct Step {
    int index;
    const Location* loc;
    double milesFromLast;
    double totalMiles;
};

const double EARTH_RADIUS_MILES = 3958.8;
const double MAX_LEG_MILES = 450.0;

// Haversine distance in miles
double haversine(double lat1, double lon1, double lat2, double lon2) {
    auto deg2rad = [](double d) { return d * M_PI / 180.0; };
    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);
    lat1 = deg2rad(lat1);
    lat2 = deg2rad(lat2);

    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::sin(dLon/2) * std::sin(dLon/2) * std::cos(lat1) * std::cos(lat2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return EARTH_RADIUS_MILES * c;
}

// Distance from a location to destination
double distToDest(const Location& loc, const Location& dest) {
    return haversine(loc.lat, loc.lon, dest.lat, dest.lon);
}

// Check if moving from current to next is "toward" destination
bool isTowardDestination(const Location& current, const Location& next, const Location& dest) {
    double dCurrent = distToDest(current, dest);
    double dNext = distToDest(next, dest);
    return dNext < dCurrent;
}

// Hard-coded dataset from your prompt
std::vector<Location> loadDataset() {
    std::vector<Location> locs;

    // Format: name lat lon rating
    // Cities and towns
    locs.push_back({"Denver,CO", 39.7392, -104.9903, 2.8});
    locs.push_back({"ColoradoSprings,CO", 38.8339, -104.8214, 3.0});
    locs.push_back({"Breckenridge,CO", 39.4817, -106.0384, 3.8});
    locs.push_back({"Vail,CO", 39.6403, -106.3742, 3.8});
    locs.push_back({"SteamboatSprings,CO", 40.4850, -106.8317, 3.7});
    locs.push_back({"Aspen,CO", 39.1911, -106.8175, 3.9});
    locs.push_back({"Telluride,CO", 37.9375, -107.8123, 4.1});
    locs.push_back({"Ouray,CO", 38.0228, -107.6714, 4.1});
    locs.push_back({"Silverton,CO", 37.8119, -107.6626, 3.9});
    locs.push_back({"Durango,CO", 37.2753, -107.8801, 3.7});
    locs.push_back({"GrandJunction,CO", 39.0639, -108.5506, 2.6});
    locs.push_back({"Moab,UT", 38.5733, -109.5498, 4.0});
    locs.push_back({"GreenRiver,UT", 38.9958, -110.1598, 2.9});
    locs.push_back({"Torrey,UT", 38.2991, -111.4196, 3.6});
    locs.push_back({"Escalante,UT", 37.7703, -111.6021, 3.6});
    locs.push_back({"Kanab,UT", 37.0475, -112.5263, 3.6});
    locs.push_back({"Springdale,UT", 37.1889, -112.9986, 4.0});
    locs.push_back({"StGeorge,UT", 37.0965, -113.5684, 2.7});
    locs.push_back({"SaltLakeCity,UT", 40.7608, -111.8910, 2.5});
    locs.push_back({"ParkCity,UT", 40.6461, -111.4980, 3.5});
    locs.push_back({"JacksonHole,WY", 43.4799, -110.7624, 4.1});
    locs.push_back({"Cody,WY", 44.5263, -109.0565, 3.5});
    locs.push_back({"Bozeman,MT", 45.6796, -111.0386, 3.1});
    locs.push_back({"Page,AZ", 36.9147, -111.4558, 3.8});
    locs.push_back({"Flagstaff,AZ", 35.1983, -111.6513, 2.7});
    locs.push_back({"Williams,AZ", 35.2495, -112.1910, 3.4});
    locs.push_back({"Sedona,AZ", 34.8697, -111.7601, 4.2});
    locs.push_back({"Jerome,AZ", 34.7489, -112.1138, 3.5});
    locs.push_back({"Prescott,AZ", 34.5400, -112.4685, 3.3});
    locs.push_back({"Phoenix,AZ", 33.4484, -112.0740, 2.4});
    locs.push_back({"Tucson,AZ", 32.2226, -110.9747, 2.6});
    locs.push_back({"Tombstone,AZ", 31.7118, -110.0678, 3.3});
    locs.push_back({"Bisbee,AZ", 31.4482, -109.9148, 3.4});
    locs.push_back({"LasVegas,NV", 36.1716, -115.1398, 2.4});
    locs.push_back({"BoulderCity,NV", 35.9786, -114.8324, 3.2});
    locs.push_back({"Ely,NV", 39.2494, -114.8744, 3.0});
    locs.push_back({"Tonopah,NV", 38.0692, -117.2305, 2.9});
    locs.push_back({"TwentyninePalms,CA", 34.1356, -116.0542, 3.0});
    locs.push_back({"PalmSprings,CA", 33.8303, -116.5453, 2.9});

    // Natural places (parks, novelties) – higher ratings
    locs.push_back({"GoblinValleySP", 38.5647, -110.7140, 4.3});
    locs.push_back({"RockyMountainNP", 40.3428, -105.6836, 4.7});
    locs.push_back({"GreatSandDunesNP", 37.7916, -105.5943, 4.4});
    locs.push_back({"BlackCanyonNP", 38.5754, -107.7416, 4.4});
    locs.push_back({"MesaVerdeNP", 37.2402, -108.4613, 4.5});
    locs.push_back({"ArchesNP", 38.7331, -109.5925, 4.7});
    locs.push_back({"CanyonlandsNP", 38.3269, -109.8783, 4.6});
    locs.push_back({"MonumentValley", 36.9914, -110.1939, 4.6});
    locs.push_back({"CapitolReefNP", 38.3670, -111.1742, 4.5});
    locs.push_back({"BryceCanyonNP", 37.5930, -112.1871, 4.8});
    locs.push_back({"ZionNP", 37.2982, -113.0263, 4.9});
    locs.push_back({"HorseshoeBend", 36.8619, -111.3743, 4.4});
    locs.push_back({"GrandCanyonNP", 36.0544, -112.1401, 4.9});
    locs.push_back({"SaguaroNP", 32.2967, -111.1666, 4.3});
    locs.push_back({"PetrifiedForestNP", 34.9099, -109.8068, 4.2});
    locs.push_back({"YellowstoneNP", 44.4280, -110.5885, 4.8});
    locs.push_back({"JoshuaTreeNP", 34.1341, -116.3131, 4.5});
    locs.push_back({"YosemiteNP", 37.8651, -119.5383, 4.9});
    locs.push_back({"SequoiaNP", 36.4864, -118.5658, 4.7});
    locs.push_back({"DeathValleyNP", 36.5054, -117.0794, 4.5});

    return locs;
}

// Find location by name (exact match)
int findLocationIndex(const std::vector<Location>& locs, const std::string& name) {
    for (size_t i = 0; i < locs.size(); ++i) {
        if (locs[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

// Global best route tracking
struct RouteResult {
    double bestScore = -std::numeric_limits<double>::infinity();
    std::vector<const Location*> bestPath;
    double bestMiles = 0.0;
};

double computeScore(double totalMiles, double totalRating) {
    double lambda = 0.2; // penalty weight
    return totalRating - lambda * (totalMiles / 100.0);
}

// DFS search
void dfs(const std::vector<Location>& locs,
         int currentIdx,
         int destIdx,
         std::vector<bool>& visited,
         std::vector<const Location*>& path,
         double totalMiles,
         double totalRating,
         RouteResult& result)
{
    const Location& current = locs[currentIdx];
    const Location& dest = locs[destIdx];

    // If reached destination, evaluate route
    if (currentIdx == destIdx) {
        double score = computeScore(totalMiles, totalRating);
        if (score > result.bestScore) {
            result.bestScore = score;
            result.bestPath = path;
            result.bestMiles = totalMiles;
        }
        return;
    }

    // Simple pruning: if already much longer than best and lower rating, stop
    if (result.bestScore > -std::numeric_limits<double>::infinity()) {
        double currentScoreEstimate = computeScore(totalMiles, totalRating);
        if (currentScoreEstimate + 1.0 < result.bestScore) {
            return;
        }
    }

    for (size_t nextIdx = 0; nextIdx < locs.size(); ++nextIdx) {
        if (visited[nextIdx]) continue;
        const Location& next = locs[nextIdx];

        double legMiles = haversine(current.lat, current.lon, next.lat, next.lon);
        if (legMiles > MAX_LEG_MILES) continue;

        if (!isTowardDestination(current, next, dest)) continue;

        visited[nextIdx] = true;
        path.push_back(&next);

        dfs(locs, static_cast<int>(nextIdx), destIdx, visited, path,
            totalMiles + legMiles, totalRating + next.rating, result);

        path.pop_back();
        visited[nextIdx] = false;
    }
}

void printRoute(const RouteResult& result) {
    if (result.bestPath.empty()) {
        std::cout << "No valid route found under constraints.\n";
        return;
    }

    std::cout << "\nBest route found:\n";
    std::cout << "----------------------------------------\n";

    double totalMiles = 0.0;
    double lastLat = result.bestPath.front()->lat;
    double lastLon = result.bestPath.front()->lon;

    for (size_t i = 0; i < result.bestPath.size(); ++i) {
        const Location* loc = result.bestPath[i];
        double legMiles = 0.0;
        if (i == 0) {
            legMiles = 0.0;
        } else {
            legMiles = haversine(lastLat, lastLon, loc->lat, loc->lon);
        }
        totalMiles += legMiles;

        std::cout << "Stop " << (i + 1) << ": " << loc->name << "\n";
        std::cout << "  Miles from last: " << std::fixed << std::setprecision(1) << legMiles << "\n";
        std::cout << "  Total miles so far: " << std::fixed << std::setprecision(1) << totalMiles << "\n\n";

        lastLat = loc->lat;
        lastLon = loc->lon;
    }

    std::cout << "----------------------------------------\n";
    std::cout << "Final total miles: " << std::fixed << std::setprecision(1) << totalMiles << "\n";
    std::cout << "Route score: " << result.bestScore << "\n";
}

int main() {
    std::vector<Location> locs = loadDataset();

    std::cout << "Road Trip Planner (natural places prioritized)\n\n";
    std::cout << "Available locations:\n";
    for (const auto& loc : locs) {
        std::cout << "  " << loc.name << " (rating " << loc.rating << ")\n";
    }

    std::cout << "\nEnter start location (exact name, e.g. Denver,CO): ";
    std::string startName;
    std::getline(std::cin, startName);

    std::cout << "Enter destination location (exact name, e.g. YosemiteNP): ";
    std::string destName;
    std::getline(std::cin, destName);

    int startIdx = findLocationIndex(locs, startName);
    int destIdx = findLocationIndex(locs, destName);

    if (startIdx == -1 || destIdx == -1) {
        std::cerr << "Start or destination not found in dataset.\n";
        return 1;
    }

    std::vector<bool> visited(locs.size(), false);
    std::vector<const Location*> path;
    RouteResult result;

    visited[startIdx] = true;
    path.push_back(&locs[startIdx]);

    dfs(locs, startIdx, destIdx, visited, path,
        0.0, locs[startIdx].rating, result);

    printRoute(result);

    return 0;
}
