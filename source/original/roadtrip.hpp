#include <getopt.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

using namespace std;

struct Options {
  string mode;
  double windowLo = 50.0;
  double windowHi = 450.0;
  double windowFinal = 700.0;
  int nights = 7;
};

struct Campsite {
    string name;
    double lat = 0.0;
    double longi = 0.0;
    double beautyScore = 0.0;
};

struct Region {
    double lat = 0.0;
    double longi = 0.0;
    double mileMark = 0.0;   // computed miles remaining to the destination

    vector<Campsite> sites;

    // PLACEHOLDER SCORING. this is the only quantity the DP maximizes, but
    // the way it is derived from `sites` is a stand-in, not a considered
    // model: it is the unweighted mean of every site's beauty rating, even
    // though a trip only ever uses one site per region, and beauty is the
    // only signal in the input. seclusion, availability, fees, access and
    // road surface are all absent. when real campsite data arrives (see the
    // note on computeSiteScore), replace the whole derivation here rather
    // than adjusting weights on top of it.
    double avgCampScore = 0.0;
};

// the best way to arrive at one region having made a given number of stops
struct Arrival {
    double score = -1.0;
    size_t parent = 0;
    bool reached = false;
};

struct TripData {
    int regions = 0;
    double destLat = 0.0;
    double destLon = 0.0;
    vector<Region> node_tracker;
    vector<double> miles;    // flattened n x n pairwise miles, computed from coordinates

    vector<vector<Arrival>> dp;   // dp[stops][node]
    vector<double> loopDp;
    vector<int> loopParent;
};

double haversine(double lat1, double lon1, double lat2, double lon2);
void read_regions(TripData& t);
void computeMiles(TripData& t);
double computeSiteScore(const Campsite& s);
size_t bestSiteInRegion(const Region& r);
bool validEdge(const TripData& t, size_t p, size_t q);
bool canFinish(const TripData& t, size_t q);
void runDP(TripData& t);
double finalScore(const TripData& t, size_t node, size_t stops);
vector<size_t> buildRoute(const TripData& t, size_t node, size_t stops);
void bestRoute(TripData& t);
void heldKarp(TripData& t);
void minPath(TripData& t);
