// checker: independent brute-force verifier for the roadtrip optimizer.
// shares parsing, distance computation, and scoring with the solver by
// necessity (both must interpret input identically); the SEARCH is fully
// independent: exhaustive DFS over all valid routes for ROADTRIP, and
// exhaustive permutation enumeration for OPTLOOP. diff its output against
// the solver across many random instances to validate the DP and the
// Held-Karp table.
#include "roadtrip.hpp"
#include <cmath>
#include <algorithm>
#include <iomanip>

using namespace std;

const double INF = 1e18;
const double UNSET = -1.0;

size_t NIGHTS = 7;
const int MAX_NIGHTS = 365;

double WINDOW_LO = 50.0;
double WINDOW_HI = 450.0;
double WINDOW_FINAL = 700.0;

struct MarkCompare {
    bool operator()(const Region &a, const Region &b) const {
        return a.mileMark > b.mileMark;
    }
};

const double PI = 3.14159265358979323846;
const double EARTH_RADIUS = 3958.8;
const double ROAD_FACTOR = 1.25;

double computeSiteScore(const Campsite& s){
    return s.beautyScore;
}

// private copy so the checker's output is byte-comparable with the solver's.
// returns r.sites.size() when the region has none; callers must check.
size_t bestSiteInRegion(const Region& r){
    size_t best = r.sites.size();
    double bestScore = UNSET;

    for (size_t s = 0; s < r.sites.size(); ++s) {
        double sc = computeSiteScore(r.sites[s]);
        if (sc > bestScore) {
            bestScore = sc;
            best = s;
        }
    }

    return best;
}

static void getOptions(int argc, char **argv, Options &options) {
  opterr = false;

  option longOptions[] = {
    {"mode", required_argument, nullptr, 'm'},
    {"lo", required_argument, nullptr, 'l'},
    {"hi", required_argument, nullptr, 'u'},
    {"final", required_argument, nullptr, 'f'},
    {"nights", required_argument, nullptr, 'n'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, '\0'}
  };

  int choice;
  int index = 0;

  while ((choice = getopt_long(argc, argv, "m:l:u:n:f:h", longOptions, &index)) != -1) {
    switch (choice) {
      case 'h':
        cout << "Brute-force checker. Modes: ROADTRIP, OPTLOOP.\n";
        exit(0);

      case 'n':
        options.nights = atoi(optarg);
        break;

      case 'm':
        options.mode = optarg;
        break;

      case 'l':
        options.windowLo = atof(optarg);
        break;

      case 'u':
        options.windowHi = atof(optarg);
        break;

      case 'f':
        options.windowFinal = atof(optarg);
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

  if (options.mode != "ROADTRIP" && options.mode != "OPTLOOP") {
    cerr << "Error: invalid mode\n";
    exit(1);
  }

  if (options.windowLo <= 0 || options.windowHi < options.windowLo
      || options.windowFinal <= 0) {
    cerr << "Error: invalid window bounds\n";
    exit(1);
  }

  if (options.nights < 1 || options.nights > MAX_NIGHTS) {
    cerr << "Error: --nights must be between 1 and " << MAX_NIGHTS << "\n";
    exit(1);
  }
}

void read_regions(TripData& t){
    double originLat;
    double originLon;
    int regionCount;

    if (!(cin >> originLat >> originLon >> t.destLat >> t.destLon >> regionCount)
        || regionCount < 1) {
        cerr << "Error: need an origin, a destination and at least 1 region\n";
        exit(1);
    }

    t.regions = regionCount + 1;
    size_t n = static_cast<size_t>(t.regions);
    t.node_tracker.resize(n);

    t.node_tracker[0].lat = originLat;
    t.node_tracker[0].longi = originLon;
    t.node_tracker[0].mileMark =
        haversine(originLat, originLon, t.destLat, t.destLon) * ROAD_FACTOR;

    for (size_t i = 1; i < n; ++i) {
        Region r;
        int siteCount;

        if (!(cin >> r.lat >> r.longi >> siteCount) || siteCount < 0) {
            cerr << "Error: malformed region header\n";
            exit(1);
        }

        r.sites.resize(static_cast<size_t>(siteCount));
        double total = 0.0;

        for (size_t s = 0; s < static_cast<size_t>(siteCount); ++s) {
            Campsite site;

            if (!(cin >> site.name >> site.lat >> site.longi >> site.beautyScore)) {
                cerr << "Error: malformed campsite\n";
                exit(1);
            }

            total += computeSiteScore(site);
            r.sites[s] = site;
        }

        if (siteCount > 0) {
            r.avgCampScore = total / static_cast<double>(siteCount);
        }

        r.mileMark = haversine(r.lat, r.longi, t.destLat, t.destLon) * ROAD_FACTOR;
        t.node_tracker[i] = r;
    }

    sort(t.node_tracker.begin() + 1, t.node_tracker.end(), MarkCompare());
    computeMiles(t);
}

double haversine(double lat1, double lon1, double lat2, double lon2){
    double p1 = lat1 * PI / 180.0;
    double p2 = lat2 * PI / 180.0;
    double lat_diff = (lat2 - lat1) * PI / 180.0;
    double long_diff = (lon2 - lon1) * PI / 180.0;

    double a = sin(lat_diff / 2.0) * sin(lat_diff / 2.0)
             + cos(p1) * cos(p2) * sin(long_diff / 2.0) * sin(long_diff / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return EARTH_RADIUS * c;
}

void computeMiles(TripData& t){
    size_t n = static_cast<size_t>(t.regions);
    t.miles.assign(n * n, 0.0);

    for (size_t p = 0; p < n; ++p) {
        for (size_t q = 0; q < n; ++q) {
            if (p == q) {
                continue;
            }

            t.miles[p * n + q] = haversine(t.node_tracker[p].lat, t.node_tracker[p].longi,
                                           t.node_tracker[q].lat, t.node_tracker[q].longi)
                                 * ROAD_FACTOR;
        }
    }
}

bool validEdge(const TripData& t, size_t p, size_t q){
    if (t.node_tracker[q].mileMark >= t.node_tracker[p].mileMark) {
        return false;
    }

    double d = t.miles[p * static_cast<size_t>(t.regions) + q];
    return d >= WINDOW_LO && d <= WINDOW_HI;
}

// mirrors the solver: the final hop is bounded by WINDOW_FINAL alone
bool canFinish(const TripData& t, size_t q){
    return t.node_tracker[q].mileMark <= WINDOW_FINAL;
}

// ---------------- ROADTRIP brute force ----------------
// exhaustive DFS over every window-and-progress-valid route of exactly
// NIGHTS stops that can reach the destination. no memoization anywhere:
// this is the whole point.
struct BruteBest {
    double score = UNSET;
    size_t node = 0;
    size_t stops = 0;
    bool found = false;
    vector<size_t> route;
};

double normalized(double rawSum, size_t stops){
    return rawSum / static_cast<double>(stops);
}

void dfs(const TripData& t, size_t node, size_t stops, double rawSum,
         vector<size_t>& path, BruteBest& best){
    if (stops == NIGHTS && canFinish(t, node)) {
        double score = normalized(rawSum, stops);

        bool take = false;
        if (!best.found || score > best.score) {
            take = true;
        } else if (score == best.score && node > best.node) {
            take = true;
        } else if (score == best.score && node == best.node && stops > best.stops) {
            take = true;
        } else if (score == best.score && node == best.node && stops == best.stops) {
            // interior tie: the DP breaks per-state ties toward the larger
            // parent, which cascades into preferring the route that is
            // lexicographically larger read from the END backwards
            for (size_t i = path.size(); i > 0; --i) {
                if (path[i - 1] != best.route[i - 1]) {
                    take = path[i - 1] > best.route[i - 1];
                    break;
                }
            }
        }

        if (take) {
            best.found = true;
            best.score = score;
            best.node = node;
            best.stops = stops;
            best.route = path;
        }
    }

    if (stops == NIGHTS) {
        return;
    }

    size_t n = static_cast<size_t>(t.regions);
    for (size_t q = 1; q < n; ++q) {
        if (q == node || !validEdge(t, node, q)) {
            continue;
        }

        path.push_back(q);
        dfs(t, q, stops + 1, rawSum + t.node_tracker[q].avgCampScore, path, best);
        path.pop_back();
    }
}

void bruteRoadtrip(TripData& t){
    BruteBest best;
    vector<size_t> path;
    path.push_back(0);
    dfs(t, 0, 0, 0.0, path, best);

    if (!best.found) {
        cerr << "Cannot construct route\n";
        exit(1);
    }

    size_t n2 = static_cast<size_t>(t.regions);

    double driven = 0.0;
    for (size_t i = 1; i < best.route.size(); ++i) {
        driven += t.miles[best.route[i - 1] * n2 + best.route[i]];
    }

    double closing = t.node_tracker[best.node].mileMark;

    cout << fixed << setprecision(2) << best.score << "\n";
    for (size_t i = 0; i < best.route.size(); ++i) {
        cout << best.route[i];
        if (i + 1 < best.route.size()) cout << " ";
    }
    cout << "\n";

    for (size_t i = 1; i < best.route.size(); ++i) {
        const Region& reg = t.node_tracker[best.route[i]];
        size_t bestSite = bestSiteInRegion(reg);

        cout << "  stop " << i << ": ";

        if (bestSite == reg.sites.size()) {
            cout << "(no campsites)";
        } else {
            cout << reg.sites[bestSite].name;
        }

        cout << " (node " << best.route[i] << ")"
             << " with " << reg.mileMark << " mi left"
             << " (+" << t.miles[best.route[i - 1] * n2 + best.route[i]] << " mi drive)";

        if (bestSite != reg.sites.size()) {
            cout << ", site score " << computeSiteScore(reg.sites[bestSite]);
        }

        cout << "\n";
    }

    cout << "  destination: +" << closing << " mi\n";
    cout << "  total: " << driven + closing << " mi\n";
}

// ---------------- OPTLOOP brute force ----------------
// every permutation of nodes 1..n-1 is a candidate visiting order; apply
// the identical leg rules as the solver (full window on every leg, HI-only
// on the closing leg home) and keep the minimum total. practical to n-1 = 9.
void bruteLoop(TripData& t){
    size_t n = static_cast<size_t>(t.regions);

    if (n - 1 > 9) {
        cerr << "Error: too many regions for brute-force OPTLOOP\n";
        exit(1);
    }

    vector<size_t> perm;
    for (size_t i = 1; i < n; ++i) {
        perm.push_back(i);
    }

    double bestTotal = INF;
    bool found = false;

    do {
        double total = 0.0;
        bool ok = true;
        size_t prev = 0;

        for (size_t i = 0; i < perm.size(); ++i) {
            double leg = t.miles[prev * n + perm[i]];
            if (leg < WINDOW_LO || leg > WINDOW_HI) {
                ok = false;
                break;
            }
            total += leg;
            prev = perm[i];
        }

        if (ok) {
            double back = t.miles[prev * n + 0];
            if (back > WINDOW_FINAL) {
                ok = false;
            } else {
                total += back;
            }
        }

        if (ok && total < bestTotal) {
            bestTotal = total;
            found = true;
        }
    } while (next_permutation(perm.begin(), perm.end()));

    if (!found) {
        cerr << "Cannot construct loop\n";
        exit(1);
    }

    cout << fixed << setprecision(2) << bestTotal << "\n";
}

int main(int argc, char* argv[]){
    TripData t;
    Options o1;

    getOptions(argc, argv, o1);
    read_regions(t);

    WINDOW_LO = o1.windowLo;
    WINDOW_HI = o1.windowHi;
    WINDOW_FINAL = o1.windowFinal;
    NIGHTS = static_cast<size_t>(o1.nights);

    if (o1.mode == "ROADTRIP"){
        if (NIGHTS > static_cast<size_t>(t.regions) - 1) {
            cerr << "Error: --nights is " << NIGHTS << " but only "
                 << t.regions - 1 << " regions are available\n";
            exit(1);
        }

        bruteRoadtrip(t);
    }

    if (o1.mode == "OPTLOOP"){
        bruteLoop(t);
    }
}
