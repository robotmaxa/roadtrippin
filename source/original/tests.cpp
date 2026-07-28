// tests.cpp -- runs the three binaries through the command line and checks
// what they print. build and run with `make test`.
//
// three groups: the small fixture files, every rejection path, and a
// differential sweep of the DP against the brute-force checker. the sweep is
// the real correctness argument; the rest guards against regressions.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// 200 seeds x 7 lengths. seed 110 at 6 nights is the one instance in this
// range where the two programs tie, so the range is wide enough to exercise
// that path rather than only the cases that agree exactly.
const int SWEEP_SEEDS = 200;

int passed = 0;
int failed = 0;

// runs a command, collects everything it printed, and reports whether it
// exited cleanly. stdout and stderr are merged on purpose: the error messages
// are part of the interface being tested.
static bool run(const string& cmd, vector<string>& lines){
    int status = system((cmd + " > tests.out 2>&1").c_str());

    lines.clear();
    ifstream in("tests.out");
    string line;

    while (getline(in, line)) {
        lines.push_back(line);
    }

    return status == 0;
}

static void pass(const string& name){
    ++passed;
    cout << "  pass  " << name << "\n";
}

static void fail(const string& name, const string& want, const string& got){
    ++failed;
    cout << "  FAIL  " << name << "\n"
         << "          want: " << want << "\n"
         << "          got:  " << got << "\n";
}

// a run that should succeed, checked on its score line and its node sequence
static void expectRoute(const string& name, const string& args,
                        const string& score, const string& route){
    vector<string> out;
    bool ok = run("./roadtrip " + args, out);

    if (ok && out.size() > 1 && out[0] == score && out[1] == route) {
        pass(name);
        return;
    }

    string got = out.empty() ? "(nothing)" : out[0];
    if (out.size() > 1) got += " / " + out[1];
    fail(name, score + " / " + route, got);
}

// a run that should be rejected, checked on its message and a failing exit
static void expectError(const string& name, const string& args, const string& msg){
    vector<string> out;
    bool ok = run("./roadtrip " + args, out);

    if (!ok && !out.empty() && out[0] == msg) {
        pass(name);
        return;
    }

    fail(name, msg, out.empty() ? "(nothing)" : out[0]);
}

// the DP and the brute-force checker must agree on the whole output. gen is
// deterministic on a seed, so both can be fed the same instance without
// writing an input file.
//
// one exception: when two routes score exactly the same, the two programs can
// pick different ones. the DP settles ties while relaxing interior states,
// the checker settles them on completed routes, and neither order is more
// correct. that is a tie, not a disagreement, so the score is what must
// match. a differing score is always a real failure.
static void sweep(){
    cout << "\ndifferential sweep vs brute force\n";

    int agree = 0;
    int ties = 0;
    int disagree = 0;
    int routes = 0;

    for (int seed = 1; seed <= SWEEP_SEEDS; ++seed) {
        for (int nights = 1; nights <= 7; ++nights) {
            string inst = "./gen --seed " + to_string(seed) + " --regions 14";
            string flags = " --mode ROADTRIP --nights " + to_string(nights);

            vector<string> dp;
            vector<string> brute;
            bool found = run(inst + " | ./roadtrip" + flags, dp);
            run(inst + " | ./checker" + flags, brute);

            if (dp == brute) {
                ++agree;
                if (found) ++routes;
            } else if (!dp.empty() && !brute.empty() && dp[0] == brute[0]) {
                ++ties;
                ++routes;
            } else {
                ++disagree;
                if (disagree <= 3) {
                    cout << "  FAIL  seed " << seed << ", nights " << nights
                         << ": " << dp[0] << " vs " << brute[0] << "\n";
                }
            }
        }
    }

    if (disagree == 0) {
        string note = "ROADTRIP matches brute force on all "
                      + to_string(agree + ties) + " runs ("
                      + to_string(routes) + " with a route";
        if (ties > 0) {
            note += ", " + to_string(ties) + " equal-scoring tie";
            if (ties > 1) note += "s";
        }
        pass(note + ")");
    } else {
        fail("ROADTRIP sweep", "0 disagreements", to_string(disagree));
    }

    int loopAgree = 0;
    int loopDisagree = 0;

    for (int seed = 1; seed <= SWEEP_SEEDS; ++seed) {
        string inst = "./gen --seed " + to_string(seed) + " --regions 9";

        vector<string> dp;
        vector<string> brute;
        run(inst + " | ./roadtrip --mode OPTLOOP", dp);
        run(inst + " | ./checker --mode OPTLOOP", brute);

        // the checker prints only the total, so compare that one line
        if (!dp.empty() && !brute.empty() && dp[0] == brute[0]) {
            ++loopAgree;
        } else {
            ++loopDisagree;
        }
    }

    if (loopDisagree == 0) {
        pass("OPTLOOP matches permutation search on all "
             + to_string(loopAgree) + " runs");
    } else {
        fail("OPTLOOP sweep", "0 disagreements", to_string(loopDisagree));
    }
}

int main(){
    cout << "fixtures\n";

    // four evenly spaced regions, every leg legal: a 4-night trip uses them all
    expectRoute("linear: uses every region in progress order",
                "--mode ROADTRIP --nights 4 < linear.txt", "4.30", "0 1 2 3 4");

    // a 2.0 region sits between the origin and a 4.8 one. the DP must skip it
    // rather than take it for coming first
    expectRoute("choice: skips the low-rated region",
                "--mode ROADTRIP --nights 2 < choice.txt", "4.40", "0 2 3");

    // one region holds a 3.0 and a 5.0 site. it scores their mean, 4.0, even
    // though a trip would use the 5.0. pins the placeholder scoring in place
    expectRoute("multisite: region scores the mean of its sites, not the best",
                "--mode ROADTRIP --nights 2 < multisite.txt", "4.00", "0 1 2");

    // a region with no campsites scores 0.0, the worst value, so it is skipped
    expectRoute("siteless: avoided when there is an alternative",
                "--mode ROADTRIP --nights 2 < siteless.txt", "4.20", "0 1 3");

    // ...but three nights must pass through it, without crashing on a region
    // that has no site to name
    expectRoute("siteless: traversed without crashing when forced",
                "--mode ROADTRIP --nights 3 < siteless.txt", "2.80", "0 1 2 3");

    // two regions share coordinates, so one can be used but not both
    expectRoute("tie: equal-distance regions, one is chosen",
                "--mode ROADTRIP --nights 2 < tie.txt", "4.25", "0 2 3");

    cout << "\nno-route and rejection paths\n";

    // chaining both tied regions would need a leg with no progress, which the
    // strict-decrease rule forbids. this is the invariant that keeps the graph
    // acyclic, so it is worth asserting directly
    expectError("tie: cannot chain two regions at equal distance",
                "--mode ROADTRIP --nights 3 < tie.txt", "Cannot construct route");

    // every region is nearer to the origin than the daily minimum, so no edge
    // exists at all
    expectError("unreachable: no legal first leg",
                "--mode ROADTRIP --nights 1 < unreachable.txt", "Cannot construct route");

    // a negative site count, caught while parsing rather than at resize
    expectError("badheader: negative site count rejected",
                "--mode ROADTRIP --nights 1 < badheader.txt",
                "Error: malformed region header");

    expectError("nights below range", "--mode ROADTRIP --nights 0 < linear.txt",
                "Error: --nights must be between 1 and 365");
    expectError("nights above range", "--mode ROADTRIP --nights 366 < linear.txt",
                "Error: --nights must be between 1 and 365");
    expectError("nights exceeds region count", "--mode ROADTRIP --nights 5 < linear.txt",
                "Error: --nights is 5 but only 4 regions are available");
    expectError("mode is required", "< linear.txt", "Error: --mode is required");
    expectError("mode must be known", "--mode NOPE < linear.txt", "Error: invalid mode");
    expectError("unknown option", "--mode ROADTRIP --bogus < linear.txt",
                "Error: invalid command line option");
    expectError("window bounds must order", "--mode ROADTRIP --lo 500 --hi 100 < linear.txt",
                "Error: invalid window bounds");
    expectError("empty input", "--mode ROADTRIP < /dev/null",
                "Error: need an origin, a destination and at least 1 region");

    cout << "\nreal datasets\n";

    expectRoute("western-usa: 7 nights on defaults", "--mode ROADTRIP < western-usa.txt",
                "4.80", "0 4 26 44 45 48 58 59");
    expectRoute("trip14: 7 nights on defaults", "--mode ROADTRIP < trip14.txt",
                "4.79", "0 3 7 9 11 12 13 15");

    // OPTLOOP ignores the destination and the progress axis, so it only needs
    // to produce its known total
    vector<string> loop;
    if (run("./roadtrip --mode OPTLOOP < trip14.txt", loop)
        && !loop.empty() && loop[0] == "2732.12") {
        pass("trip14: OPTLOOP tour total");
    } else {
        fail("trip14: OPTLOOP tour total", "2732.12",
             loop.empty() ? "(nothing)" : loop[0]);
    }

    sweep();

    remove("tests.out");

    cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
