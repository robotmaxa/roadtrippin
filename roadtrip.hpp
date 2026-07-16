#include <getopt.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

using namespace std;

struct Options {
  string mode;
  double windowLo = 5.0;
  double windowHi = 8.0;
};

struct Campsite {
    double x = 0;
    double y = 0;
    double beautyScore = 0.0;
    double secludedScore = 0.0;
};

struct Region {
    double driveTime = 0.0;
    bool isTerminal = false;
    vector<Campsite> sites;
    double avgCampScore = 0.0;
};

struct TripData {
    int regions = 0;
    vector<Region> node_tracker;
    double globalAvg = 0.0;

    vector<double> dpSum;
    vector<size_t> dpParent;
};

void read_regions(TripData& t);
double computeSiteScore(const Campsite& s);
void computeGlobalAvg(TripData& t);
size_t bestSiteInRegion(const Region& r);
bool validEdge(const TripData& t, size_t p, size_t q);
void runDP(TripData& t);
double finalScore(const TripData& t, size_t node, size_t stops);
vector<size_t> buildRoute(const TripData& t, size_t node, size_t stops);
void bestRoute(TripData& t);
void topRoutes(TripData& t);
