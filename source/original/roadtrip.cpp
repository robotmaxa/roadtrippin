#include "roadtrip.hpp"
#include <cmath>
#include <utility>
#include <algorithm>
#include <iomanip>

using namespace std;

const double INF = 1e18;
const double UNSET = -1.0;

// how many nights the trip lasts, i.e. how many regions it stops in.
// overwritten from Options in main. the trip length is an input, not
// something to optimize: without it fixed, routes of different lengths are
// not comparable without inventing a value for a marginal night.
size_t NIGHTS = 7;

// sanity ceiling on --nights. the DP allocates (NIGHTS + 1) * regions states
// and runs in O(NIGHTS * regions^2)
const int MAX_NIGHTS = 365;

// held-karp allocates 2^n * n entries, so the mode is capped well below
// the point where that table stops fitting in memory
const size_t MAX_LOOP_REGIONS = 20;

// overwritten from Options in main; keep the defaults in sync with the
// ones declared there.
// WINDOW_LO / WINDOW_HI bound a day of driving between two stops.
// WINDOW_FINAL separately bounds the last hop into the destination, and the
// leg home in loop mode. those are different questions: a day of driving is
// a day you plan around, while the final approach is just arriving. the
// minimum has always been waived there for exactly this reason; the ceiling
// is waived to the same degree.
//
// lo is based on productive driving days and hi is based on a typical long drive day.
// the final ceiling is set by the data: a trip can only end at a region
// within reach of the destination, and the worst input on hand has its
// nearest region 637 mi out (just a parameter for current data).
double WINDOW_LO = 50.0;
double WINDOW_HI = 450.0;
double WINDOW_FINAL = 700.0;

// descending by road remaining, so lower node index means farther from the
// destination and node order matches progress order
struct MarkCompare {
    bool operator()(const Region &a, const Region &b) const {
        return a.mileMark > b.mileMark;
    }
};

// "better" ordering: higher score wins, ties broken by larger parent. an
// unreached arrival loses to everything. every candidate for a given state
// carries a distinct parent, so this is a strict total order over them and
// the best one is unique.
static bool betterArrival(const Arrival &a, const Arrival &b){
    if (!b.reached) return a.reached;
    if (!a.reached) return false;
    if (a.score != b.score) return a.score > b.score;
    return a.parent > b.parent;
}

const double PI = 3.14159265358979323846;
const double EARTH_RADIUS = 3958.8;

// roads are never straight lines
const double ROAD_FACTOR = 1.25;   

// scoring: beauty comes straight from the input as a star rating
// (out of 5, higher = better). higher is better everywhere.
// Eventually use API, and map surces similar to OnXOffroad maps to calculate actual campsites/locations
double computeSiteScore(const Campsite& s){
    return s.beautyScore;
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
        cout << "Enter 'mode' and one of either 'ROADTRIP' or 'OPTLOOP' to run your program.\n"
             << "  --nights <n>    overnight stops to plan for (default 7, ROADTRIP only)\n"
             << "  --lo <miles>    shortest acceptable drive between stops (default 50)\n"
             << "  --hi <miles>    longest acceptable drive between stops (default 450)\n"
             << "  --final <miles> longest acceptable final drive into the destination,\n"
             << "                  and the leg home in OPTLOOP (default 700)\n";
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

// input format:
//   line 1: origin latitude and longitude (trip start, node 0)
//   line 2: destination latitude and longitude (where the trip must end up)
//   line 3: number of regions
//   per region: header line "lat longi siteCount",
//               then siteCount lines of "name lat longi beautyScore"
//   where names are single tokens, comma between city and state
//   (e.g. "Moab,UT 38.5733 -109.5498 1.2")
// all pairwise driving miles are computed from coordinates via the
// haversine formula times a road circuity factor; regions are ordered by
// remaining distance to the destination, which is the progress axis: a leg
// is only progress if it leaves less road ahead than it found.
// node 0 is always the starting location and has no sites
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

    // the origin sits on the same axis as everything else: it is simply the
    // point with the most road left, so the whole trip runs downhill from it
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

    // progress order: regions with less road left get higher node indices, so
    // node index still increases along the direction of travel. the tie-break
    // rules downstream are written in terms of node index and rely on this.
    sort(t.node_tracker.begin() + 1, t.node_tracker.end(), MarkCompare());

    computeMiles(t);
}

// great-circle distance in miles between two lat/lon points.
// the cos(p1)*cos(p2) term shrinks the longitude contribution at
// higher latitudes, where a degree of longitude spans fewer miles.
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
                                           t.node_tracker[q].lat, t.node_tracker[q].longi) * ROAD_FACTOR;
        }
    }
}

// for mapping implementation. returns r.sites.size() when the region has no
// campsites at all, so callers must check before indexing.
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

// edge exists only if q leaves strictly less road to the destination than p
// did, and the actual driving distance between them fits the daily window.
// measuring progress toward the destination rather than away from the origin
// is what stops a route from detouring sideways: growing the radius from the
// start is not the same as getting closer to where you are going.
// the strict decrease keeps the graph acyclic, so the DP needs no visited set.
bool validEdge(const TripData& t, size_t p, size_t q){
    if (t.node_tracker[q].mileMark >= t.node_tracker[p].mileMark) {
        return false;
    }

    double d = t.miles[p * static_cast<size_t>(t.regions) + q];
    return d >= WINDOW_LO && d <= WINDOW_HI;
}

// a region can be the last stop if the destination is within reach of it.
// neither WINDOW_LO nor WINDOW_HI applies: the daily window describes a day
// of driving you plan around, and the final approach is just arriving.
// WINDOW_FINAL bounds it independently.
bool canFinish(const TripData& t, size_t q){
    return t.node_tracker[q].mileMark <= WINDOW_FINAL;
}

// forward DP over (stops, region). each state keeps only the single best way
// to arrive there, since every extension of a state depends on its score
// alone: a suboptimal arrival can never beat the optimal one downstream.
void runDP(TripData& t){
    size_t n = static_cast<size_t>(t.regions);

    t.dp.assign(NIGHTS + 1, vector<Arrival>(n));

    for (size_t q = 1; q < n; ++q) {
        if (validEdge(t, 0, q)) {
            Arrival& base = t.dp[1][q];
            base.score = t.node_tracker[q].avgCampScore;
            base.parent = 0;
            base.reached = true;
        }
    }

    for (size_t stops = 2; stops <= NIGHTS; ++stops) {
        for (size_t q = 1; q < n; ++q) {
            Arrival best;

            for (size_t p = 1; p < n; ++p) {
                const Arrival& from = t.dp[stops - 1][p];

                if (p == q || !from.reached || !validEdge(t, p, q)) {
                    continue;
                }

                Arrival cand;
                cand.score = from.score + t.node_tracker[q].avgCampScore;
                cand.parent = p;
                cand.reached = true;

                if (betterArrival(cand, best)) {
                    best = cand;
                }
            }

            t.dp[stops][q] = best;
        }
    }
}

double finalScore(const TripData& t, size_t node, size_t stops){
    // every candidate now has exactly the same number of stops, so this is a
    // plain mean with nothing to normalize against. it ranks identically to
    // the raw sum; dividing just keeps the printed figure on the same scale
    // as a single campsite rating.
    return t.dp[stops][node].score / static_cast<double>(stops);
}

vector<size_t> buildRoute(const TripData& t, size_t node, size_t stops){
    vector<size_t> route;
    route.reserve(stops + 1);
    size_t cur = node;
    size_t s = stops;

    while (cur != 0) {
        route.push_back(cur);
        cur = t.dp[s][cur].parent;
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

// one line of the itinerary. a region with no campsites has nothing to name:
// a loop must pass through every region, and those are waypoints rather than
// places to sleep. both modes print through here so that check lives in one
// place instead of being repeated and eventually forgotten.
static void printStop(size_t num, const Region& reg, size_t node, double leg){
    size_t best = bestSiteInRegion(reg);
    bool named = (best != reg.sites.size());

    cout << "  stop " << num << ": " << (named ? reg.sites[best].name : "(no campsites)")
         << " (node " << node << ")"
         << " with " << reg.mileMark << " mi left"
         << " (+" << leg << " mi drive)";

    if (named) {
        cout << ", site score " << computeSiteScore(reg.sites[best]);
    }

    cout << "\n";
}

void bestRoute(TripData& t){
    size_t n = static_cast<size_t>(t.regions);
    double bestSoFar = UNSET;
    size_t bestNode = n;

    // exactly NIGHTS stops, so the only choice left is where to spend them
    for (size_t q = 1; q < n; ++q) {
        if (!canFinish(t, q) || !t.dp[NIGHTS][q].reached) {
            continue;
        }

        double score = finalScore(t, q, NIGHTS);
        if (score > bestSoFar || (score == bestSoFar && q > bestNode)) {
            bestSoFar = score;
            bestNode = q;
        }
    }

    if (bestNode == n) {
        cerr << "Cannot construct route\n";
        exit(1);
    }

    vector<size_t> route = buildRoute(t, bestNode, NIGHTS);
    size_t n2 = static_cast<size_t>(t.regions);

    double driven = 0.0;
    for (size_t i = 1; i < route.size(); ++i) {
        driven += t.miles[route[i - 1] * n2 + route[i]];
    }

    // the closing drive is exactly the last stop's remaining-road figure,
    // since mileMark already measures distance to the destination
    double closing = t.node_tracker[bestNode].mileMark;

    cout << fixed << setprecision(2) << bestSoFar << "\n";
    for (size_t i = 0; i < route.size(); ++i) {
        cout << route[i];
        if (i + 1 < route.size()) cout << " ";
    }
    cout << "\n";

    for (size_t i = 1; i < route.size(); ++i) {
        printStop(i, t.node_tracker[route[i]], route[i],
                  t.miles[route[i - 1] * n2 + route[i]]);
    }

    cout << "  destination: +" << closing << " mi\n";
    cout << "  total: " << driven + closing << " mi\n";
}

void heldKarp(TripData& t){
    size_t n = static_cast<size_t>(t.regions);

    if (n > MAX_LOOP_REGIONS) {
        cerr << "Error: too many regions for OPTLOOP\n";
        exit(1);
    }

    size_t numMasks = size_t(1) << n;

    t.loopDp.assign(numMasks * n, INF);
    t.loopParent.assign(numMasks * n, -1);

    t.loopDp[size_t(1) * n + 0] = 0.0;

    for (size_t mask = 1; mask < numMasks; ++mask) {
        for (size_t i = 0; i < n; ++i) {
            if (!(mask & (size_t(1) << i))) continue;

            double cur = t.loopDp[mask * n + i];
            if (cur >= INF) continue;

            for (size_t u = 0; u < n; ++u) {
                if (mask & (size_t(1) << u)) continue;

                double cost = t.miles[i * n + u];
                if (cost < WINDOW_LO || cost > WINDOW_HI) continue;

                size_t nextMask = mask | (size_t(1) << u);

                if (cur + cost < t.loopDp[nextMask * n + u]) {
                    t.loopDp[nextMask * n + u] = cur + cost;
                    t.loopParent[nextMask * n + u] = static_cast<int>(i);
                }
            }
        }
    }
}

void minPath(TripData& t){
    size_t n = static_cast<size_t>(t.regions);
    size_t numMasks = size_t(1) << n;
    size_t FULL = numMasks - 1;

    double minTotalCost = INF;
    int lastCity = -1;

    for (size_t i = 1; i < n; ++i) {
        if (t.loopDp[FULL * n + i] >= INF) continue;

        double returnCost = t.miles[i * n + 0];
        if (returnCost > WINDOW_FINAL) continue;

        double totalCost = t.loopDp[FULL * n + i] + returnCost;

        if (totalCost < minTotalCost) {
            minTotalCost = totalCost;
            lastCity = static_cast<int>(i);
        }
    }

    if (lastCity == -1) {
        cerr << "Cannot construct loop\n";
        exit(1);
    }

    vector<size_t> optimalPath;
    size_t mask = FULL;
    int currentCity = lastCity;

    optimalPath.push_back(0);

    while (currentCity != -1) {
        optimalPath.push_back(static_cast<size_t>(currentCity));

        int prevCity = t.loopParent[mask * n + static_cast<size_t>(currentCity)];

        mask &= ~(size_t(1) << currentCity);
        currentCity = prevCity;
    }

    reverse(optimalPath.begin(), optimalPath.end());

    cout << fixed << setprecision(2) << minTotalCost << "\n";

    for (size_t f = 0; f < optimalPath.size(); ++f) {
        const Region& reg = t.node_tracker[optimalPath[f]];

        if (f == 0) {
            cout << "  start: origin (node 0)\n";
            continue;
        }

        double leg = t.miles[optimalPath[f - 1] * n + optimalPath[f]];

        if (f + 1 == optimalPath.size()) {
            cout << "  return: origin (node 0), +" << leg << " mi\n";
            continue;
        }

        printStop(f, reg, optimalPath[f], leg);
    }
}




int main(int argc, char* argv[]){
    TripData t;
    Options o1;

    getOptions(argc, argv, o1);

    WINDOW_LO = o1.windowLo;
    WINDOW_HI = o1.windowHi;
    WINDOW_FINAL = o1.windowFinal;
    NIGHTS = static_cast<size_t>(o1.nights);

    read_regions(t);

    if (o1.mode == "OPTLOOP"){
        heldKarp(t);
        minPath(t);
    }

    if (o1.mode == "ROADTRIP"){
        // every stop is a distinct region, so a trip cannot outlast them.
        // catching this here gives a reason instead of an unexplained
        // "Cannot construct route" after the DP has churned through it.
        if (NIGHTS > static_cast<size_t>(t.regions) - 1) {
            cerr << "Error: --nights is " << NIGHTS << " but only "
                 << t.regions - 1 << " regions are available\n";
            exit(1);
        }

        runDP(t);
        bestRoute(t);
    }

    return 0;
}
