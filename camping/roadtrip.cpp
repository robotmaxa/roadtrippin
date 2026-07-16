#include "roadtrip.hpp"
#include <utility>
#include <algorithm>
#include <istream>
#include <fstream>
#include <cmath>
#include <iomanip>

using namespace std;

const size_t MIN_STOPS = 4;
const size_t MAX_STOPS = 7;
const double INF = 1e18;

double WINDOW_LO = 5.0;
double WINDOW_HI = 8.0;

// scoring equation: beauty and secludedness come straight from the input
// as arbitrary values (0 = perfect, higher = worse), and combine here.
// seclusion is weighted heavier. lower total is better everywhere.
const double W_BEAUTY = 1.0;
const double W_SECLUDED = 2.0;

double computeSiteScore(const Campsite& s){
    return W_BEAUTY * s.beautyScore + W_SECLUDED * s.secludedScore;
}

static void getOptions(int argc, char **argv, Options &options) {
  opterr = false;

  option longOptions[] = {
    {"mode", required_argument, nullptr, 'm'},
    {"lo", required_argument, nullptr, 'l'},
    {"hi", required_argument, nullptr, 'u'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, '\0'}
  };

  int choice;
  int index = 0;

  while ((choice = getopt_long(argc, argv, "m:l:u:h", longOptions, &index)) != -1) {
    switch (choice) {
      case 'h':
        cout << "Enter 'mode' and one of either 'ROADTRIP' or 'TOP3' to run your program.\n"
             << "Optional: --lo <hours> --hi <hours> to change the driving window (default 5-8).\n";
        exit(0);

      case 'm':
        options.mode = optarg;
        break;

      case 'l':
        options.windowLo = atof(optarg);
        break;

      case 'u':
        options.windowHi = atof(optarg);
        break;

      default:
        cerr << "Error: invalid command line option\n";
        exit(1);
    }
  }

  if (options.mode.empty()) {
    cerr << "Error: --mode is required\n";
    exit(1);
  }

  if (options.mode != "ROADTRIP" && options.mode != "TOP3") {
    cerr << "Error: invalid mode\n";
    exit(1);
  }

  if (options.windowLo <= 0 || options.windowHi < options.windowLo) {
    cerr << "Error: invalid window bounds\n";
    exit(1);
  }
}

// input format:
//   line 1: region count
//   per region: header line "driveTime isTerminal siteCount",
//               then siteCount lines of "x y beautyScore secludedScore"
// node 0 is always the starting location, driveTime 0, no sites
void read_regions(TripData& t){
    cin >> t.regions;
    size_t n = static_cast<size_t>(t.regions);
    t.node_tracker.resize(n);

    for (size_t i = 0; i < n; ++i) {
        Region r;
        int terminalFlag;
        int siteCount;
        cin >> r.driveTime >> terminalFlag >> siteCount;

        if (terminalFlag == 1){
            r.isTerminal = true;
        } else {
            r.isTerminal = false;
        }

        r.sites.resize(static_cast<size_t>(siteCount));
        double total = 0.0;

        for (size_t s = 0; s < static_cast<size_t>(siteCount); ++s) {
            Campsite site;
            cin >> site.x >> site.y >> site.beautyScore >> site.secludedScore;
            total += computeSiteScore(site);
            r.sites[s] = site;
        }

        if (siteCount > 0) {
            r.avgCampScore = total / static_cast<double>(siteCount);
        }

        t.node_tracker[i] = r;
    }
}

void computeGlobalAvg(TripData& t){
    size_t n = static_cast<size_t>(t.regions);
    double total = 0.0;

    for (size_t i = 1; i < n; ++i) {
        total += t.node_tracker[i].avgCampScore;
    }

    t.globalAvg = total / static_cast<double>(n - 1);
}

size_t bestSiteInRegion(const Region& r){
    size_t best = r.sites.size();
    double bestScore = INF;

    for (size_t s = 0; s < r.sites.size(); ++s) {
        double sc = computeSiteScore(r.sites[s]);
        if (sc < bestScore) {
            bestScore = sc;
            best = s;
        }
    }

    return best;
}

bool validEdge(const TripData& t, size_t p, size_t q){
    double gap = t.node_tracker[q].driveTime - t.node_tracker[p].driveTime;
    return gap >= WINDOW_LO && gap <= WINDOW_HI;
}

void runDP(TripData& t){
    size_t n = static_cast<size_t>(t.regions);
    t.dpSum.assign((MAX_STOPS + 1) * n, INF);
    t.dpParent.assign((MAX_STOPS + 1) * n, n);

    for (size_t q = 1; q < n; ++q) {
        if (validEdge(t, 0, q)) {
            t.dpSum[1 * n + q] = t.node_tracker[q].avgCampScore;
            t.dpParent[1 * n + q] = 0;
        }
    }

    for (size_t stops = 2; stops <= MAX_STOPS; ++stops) {
        for (size_t q = 1; q < n; ++q) {
            double best = INF;
            size_t bestParent = n;

            for (size_t p = 1; p < n; ++p) {
                if (p == q || t.dpSum[(stops - 1) * n + p] >= INF) {
                    continue;
                }

                if (!validEdge(t, p, q)) {
                    continue;
                }

                double cand = t.dpSum[(stops - 1) * n + p] + t.node_tracker[q].avgCampScore;
                if (cand < best) {
                    best = cand;
                    bestParent = p;
                }
            }

            if (bestParent != n) {
                t.dpSum[stops * n + q] = best;
                t.dpParent[stops * n + q] = bestParent;
            }
        }
    }
}

double finalScore(const TripData& t, size_t node, size_t stops){
    size_t n = static_cast<size_t>(t.regions);
    double raw = t.dpSum[stops * n + node];
    double padded = raw + static_cast<double>(MAX_STOPS - stops) * t.globalAvg;
    return padded / static_cast<double>(MAX_STOPS);
}

vector<size_t> buildRoute(const TripData& t, size_t node, size_t stops){
    size_t n = static_cast<size_t>(t.regions);
    vector<size_t> route;
    route.reserve(stops + 1);
    size_t cur = node;
    size_t s = stops;

    while (cur != 0) {
        route.push_back(cur);
        cur = t.dpParent[s * n + cur];
        --s;
    }
    route.push_back(0);

    size_t lo = 0, hi = route.size() - 1;
    while (lo < hi) {
        swap(route[lo], route[hi]);
        ++lo;
        --hi;
    }

    return route;
}

void bestRoute(TripData& t){
    size_t n = static_cast<size_t>(t.regions);
    double bestSoFar = INF;
    size_t bestNode = n;
    size_t bestStops = 0;

    for (size_t stops = MIN_STOPS; stops <= MAX_STOPS; ++stops) {
        for (size_t q = 1; q < n; ++q) {
            if (!t.node_tracker[q].isTerminal || t.dpSum[stops * n + q] >= INF) {
                continue;
            }

            double score = finalScore(t, q, stops);
            if (score < bestSoFar) {
                bestSoFar = score;
                bestNode = q;
                bestStops = stops;
            }
        }
    }

    if (bestNode == n) {
        cerr << "Cannot construct route\n";
        exit(1);
    }

    vector<size_t> route = buildRoute(t, bestNode, bestStops);

    cout << fixed << setprecision(2) << bestSoFar << "\n";
    for (size_t i = 0; i < route.size(); ++i) {
        cout << route[i];
        if (i + 1 < route.size()) cout << " ";
    }
    cout << "\n";
}

// enumerates every reachable terminal state (terminal region, stop count),
// scores each completed route profile, and prints the best three
void topRoutes(TripData& t){
    size_t n = static_cast<size_t>(t.regions);
    vector<pair<double, pair<size_t, size_t>>> finals;

    for (size_t stops = MIN_STOPS; stops <= MAX_STOPS; ++stops) {
        for (size_t q = 1; q < n; ++q) {
            if (!t.node_tracker[q].isTerminal || t.dpSum[stops * n + q] >= INF) {
                continue;
            }

            finals.push_back({finalScore(t, q, stops), {q, stops}});
        }
    }

    if (finals.empty()) {
        cerr << "Cannot construct route\n";
        exit(1);
    }

    sort(finals.begin(), finals.end());

    size_t count = min(finals.size(), static_cast<size_t>(3));
    for (size_t r = 0; r < count; ++r) {
        size_t node = finals[r].second.first;
        size_t stops = finals[r].second.second;
        vector<size_t> route = buildRoute(t, node, stops);

        cout << "Route " << r + 1 << ": "
             << fixed << setprecision(2) << finals[r].first << "\n";

        for (size_t i = 0; i < route.size(); ++i) {
            const Region& reg = t.node_tracker[route[i]];

            if (i == 0) {
                cout << "  start: node " << route[i] << "\n";
                continue;
            }

            size_t bestSite = bestSiteInRegion(reg);
            cout << "  stop " << i << ": node " << route[i]
                 << " at hour " << reg.driveTime
                 << ", region score " << reg.avgCampScore
                 << ", best site (" << reg.sites[bestSite].x
                 << ", " << reg.sites[bestSite].y << ") beauty "
                 << reg.sites[bestSite].beautyScore << ", score "
                 << computeSiteScore(reg.sites[bestSite]) << "\n";
        }
    }
}

int main(int argc, char* argv[]){
    TripData t;
    Options o1;

    getOptions(argc, argv, o1);
    read_regions(t);

    WINDOW_LO = o1.windowLo;
    WINDOW_HI = o1.windowHi;

    if (o1.mode == "ROADTRIP"){
        computeGlobalAvg(t);
        runDP(t);
        bestRoute(t);
    }

    if (o1.mode == "TOP3"){
        computeGlobalAvg(t);
        runDP(t);
        topRoutes(t);
    }
}
