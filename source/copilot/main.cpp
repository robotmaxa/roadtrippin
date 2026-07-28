#include <iostream>
#include <vector>
#include <cmath>
#include <string>

struct Location {
    std::string name;
    double lat;
    double lon;
    double rating;
};

struct Step {
    const Location* loc;
    double milesFromLast;
};

struct Route {
    std::vector<Step> steps;
    double totalMiles = 0.0;
    double score = 0.0;
};

double haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 3958.8;
    const double toRad = M_PI / 180.0;

    double dLat = (lat2 - lat1) * toRad;
    double dLon = (lon2 - lon1) * toRad;

    lat1 *= toRad;
    lat2 *= toRad;

    double a = std::sin(dLat/2)*std::sin(dLat/2) +
               std::sin(dLon/2)*std::sin(dLon/2)*std::cos(lat1)*std::cos(lat2);

    return R * 2 * std::asin(std::sqrt(a));
}

// WESTBOUND ONLY: next.lon must be <= current.lon
bool westOnly(double currentLon, double nextLon) {
    return nextLon <= currentLon;
}

double computeScore(const Route& r) {
    double ratingSum = 0.0;
    for (auto& s : r.steps) ratingSum += s.loc->rating;
    return ratingSum * 10.0 - r.totalMiles / 300.0;
}

void dfs(const Location& current,
         const std::vector<Location>& all,
         Route& best,
         Route& curr,
         std::vector<bool>& visited,
         int depth,
         int minDepth,
         int maxDepth)
{
    if (depth >= minDepth && depth <= maxDepth) {
        double s = computeScore(curr);
        if (s > best.score) {
            best = curr;
            best.score = s;
        }
    }
    if (depth == maxDepth) return;

    for (size_t i = 0; i < all.size(); ++i) {
        if (visited[i]) continue;

        const Location& next = all[i];

        double miles = haversine(current.lat, current.lon, next.lat, next.lon);
        if (miles > 450.0) continue;

        if (!westOnly(current.lon, next.lon)) continue;

        visited[i] = true;
        curr.steps.push_back({&next, miles});
        curr.totalMiles += miles;

        dfs(next, all, best, curr, visited, depth + 1, minDepth, maxDepth);

        curr.totalMiles -= miles;
        curr.steps.pop_back();
        visited[i] = false;
    }
}

int main() {
    Location start = {"Denver,CO (Start)", 39.7392, -104.9903, 0.0};

    std::vector<Location> locs = {
        {"Denver,CO",39.7392,-104.9903,2.8},
        {"ColoradoSprings,CO",38.8339,-104.8214,3.0},
        {"Breckenridge,CO",39.4817,-106.0384,3.8},
        {"Vail,CO",39.6403,-106.3742,3.8},
        {"SteamboatSprings,CO",40.485,-106.8317,3.7},
        {"Aspen,CO",39.1911,-106.8175,3.9},
        {"Telluride,CO",37.9375,-107.8123,4.1},
        {"Ouray,CO",38.0228,-107.6714,4.1},
        {"Silverton,CO",37.8119,-107.6626,3.9},
        {"Durango,CO",37.2753,-107.8801,3.7},
        {"GrandJunction,CO",39.0639,-108.5506,2.6},
        {"Moab,UT",38.5733,-109.5498,4.0},
        {"GreenRiver,UT",38.9958,-110.1598,2.9},
        {"Torrey,UT",38.2991,-111.4196,3.6},
        {"Escalante,UT",37.7703,-111.6021,3.6},
        {"Kanab,UT",37.0475,-112.5263,3.6},
        {"Springdale,UT",37.1889,-112.9986,4.0},
        {"StGeorge,UT",37.0965,-113.5684,2.7},
        {"SaltLakeCity,UT",40.7608,-111.891,2.5},
        {"ParkCity,UT",40.6461,-111.498,3.5},
        {"JacksonHole,WY",43.4799,-110.7624,4.1},
        {"Cody,WY",44.5263,-109.0565,3.5},
        {"Bozeman,MT",45.6796,-111.0386,3.1},
        {"Page,AZ",36.9147,-111.4558,3.8},
        {"Flagstaff,AZ",35.1983,-111.6513,2.7},
        {"Williams,AZ",35.2495,-112.191,3.4},
        {"Sedona,AZ",34.8697,-111.7601,4.2},
        {"Jerome,AZ",34.7489,-112.1138,3.5},
        {"Prescott,AZ",34.54,-112.4685,3.3},
        {"Phoenix,AZ",33.4484,-112.074,2.4},
        {"Tucson,AZ",32.2226,-110.9747,2.6},
        {"Tombstone,AZ",31.7118,-110.0678,3.3},
        {"Bisbee,AZ",31.4482,-109.9148,3.4},
        {"LasVegas,NV",36.1716,-115.1398,2.4},
        {"BoulderCity,NV",35.9786,-114.8324,3.2},
        {"Ely,NV",39.2494,-114.8744,3.0},
        {"Tonopah,NV",38.0692,-117.2305,2.9},
        {"TwentyninePalms,CA",34.1356,-116.0542,3.0},
        {"PalmSprings,CA",33.8303,-116.5453,2.9},
        {"GoblinValleySP",38.5647,-110.714,4.3},
        {"RockyMountainNP",40.3428,-105.6836,4.7},
        {"GreatSandDunesNP",37.7916,-105.5943,4.4},
        {"BlackCanyonNP",38.5754,-107.7416,4.4},
        {"MesaVerdeNP",37.2402,-108.4613,4.5},
        {"ArchesNP",38.7331,-109.5925,4.7},
        {"CanyonlandsNP",38.3269,-109.8783,4.6},
        {"MonumentValley",36.9914,-110.1939,4.6},
        {"CapitolReefNP",38.367,-111.1742,4.5},
        {"BryceCanyonNP",37.593,-112.1871,4.8},
        {"ZionNP",37.2982,-113.0263,4.9},
        {"HorseshoeBend",36.8619,-111.3743,4.4},
        {"GrandCanyonNP",36.0544,-112.1401,4.9},
        {"SaguaroNP",32.2967,-111.1666,4.3},
        {"PetrifiedForestNP",34.9099,-109.8068,4.2},
        {"YellowstoneNP",44.428,-110.5885,4.8},
        {"JoshuaTreeNP",34.1341,-116.3131,4.5},
        {"YosemiteNP",37.8651,-119.5383,4.9},
        {"SequoiaNP",36.4864,-118.5658,4.7},
        {"DeathValleyNP",36.5054,-117.0794,4.5}
    };

    Route best;
    best.score = -1e9;

    int minDays = 4;
    int maxDays = 7;

    std::vector<bool> visited(locs.size(), false);
    Route curr;

    dfs(start, locs, best, curr, visited, 0, minDays, maxDays);

    std::cout << "Best Westbound Route From Denver\n";
    std::cout << "Total Miles: " << best.totalMiles << "\n";
    std::cout << "Score: " << best.score << "\n\n";

    double cumulative = 0.0;
    for (size_t i = 0; i < best.steps.size(); ++i) {
        cumulative += best.steps[i].milesFromLast;
        std::cout << "Stop " << (i+1) << ": "
                  << best.steps[i].loc->name
                  << " | Miles: " << best.steps[i].milesFromLast
                  << " | Cumulative: " << cumulative << "\n";
    }

    return 0;
}
