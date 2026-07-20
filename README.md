# Roadtrip Optimizer

Route optimization for multi-day road trips under daily driving-distance constraints. A passion project for further exploration of dynamic programming: the same program contains two structurally different DPs — a polynomial-state DAG DP for stop selection, and an exponential-state bitmask DP (Held-Karp) for loop ordering.

## Problem

Given candidate campsites at known coordinates with quality ratings, select overnight stops for a one-way trip (e.g., Colorado to California) such that:

- Every daily leg fits within a configurable mileage window (default 200-540 miles)
- The route ends at a terminal region (for expanded route options, the endpoint is anywhere flagged as terminal — e.g., any Southern California destination — rather than one fixed city)
- The total campsite quality score is minimized (lower = better)
- The route has between 4 and 7 stops

Separately, for a loop trip: given a curated set of must-visit locations, find the ordering that minimizes total driving miles, out from home and back, under the same daily windows.

## Scoring and normalization

Routes of different lengths cannot be compared by raw score sums (longer routes accumulate more) or plain averages (a one-great-stop route would dominate). Each route's score is normalized by padding: `finalScore = (rawSum + (MAX_STOPS - stops) * globalAvg) / MAX_STOPS`, where `globalAvg` is the average score across all regions in the dataset. A shorter route must have truly exceptional sites to beat a longer one. (Padding with the route's *own* average provably cancels out to a plain average — the global average works because it is independent of the route being scored.)

## Modes

- `ROADTRIP` — best single route: score, node sequence
- `TOP5` — five best route profiles with per-stop detail: site name, miles from previous stop, region and site scores. The top routes often share the same opening stops (a dominant prefix) — expected behavior, since a strong prefix anchors many completions.
- `OPTLOOP` — optimal loop through *every* region in the input via Held-Karp, minimizing total miles. Daily windows still apply to every leg, so it does not just hit all sites and sprint home. Capped at 22 regions (the DP table is 2^n x n states); intended for curated trip files, not the full 56-location dataset.

## The two DPs (why they differ)

Both modes extend partial routes one feasible leg at a time, keep the minimum, and track parent pointers. The difference is what the state must remember:

- **ROADTRIP/TOP5** — state `(stops, node)`. The route must always progress outward from the origin (a directed acyclic graph ordered by distance from home), so no route can ever revisit a region. The DP can *forget* which regions it visited: polynomial state, runs comfortably on 56 nodes.
- **OPTLOOP** — state `(visitedSet, node)`. A loop drives out and comes back, so the direction constraint is gone, and "have I been here already?" becomes a question the state must answer. The visited set is carried as a bitmask: exponential state, exact to ~20 nodes.

ROADTRIP recurrence: `dp[stops][node] = min over valid predecessors p of (dp[stops-1][p] + score(node))`, where an edge `p -> node` exists iff `node` is farther from the origin than `p` and the leg distance fits the window.

OPTLOOP recurrence: `dp[mask | (1<<v)][v] = min(dp[mask][u] + miles[u][v])` over set bits `u` and unset bits `v`, with window-infeasible legs excluded. Masks are filled in increasing integer order, so every submask state is final before it is read.

One deliberate asymmetry: the loop's *closing* leg home enforces only the daily maximum, not the minimum. The minimum bound encodes "meaningful progress," which the homecoming leg is not — a short final hop home is a feature, not a violation.

## Build and run
```bash
make
./roadtrip --mode ROADTRIP < western-usa.txt
./roadtrip --mode TOP5 --lo 150 < trip14.txt
./roadtrip --mode OPTLOOP --lo 50 < trip14.txt
```

`--lo` / `--hi` set the daily mileage window (default 200 / 540). Note that OPTLOOP must visit every region, so clustered destinations usually require a lower floor than the corridor modes.

## Input format
```
<originLat> <originLon>
<destinationCount>
<lat> <lon> <isTerminal> <siteCount>     (per region)
<name> <lat> <lon> <beautyScore>         (per site; single-token names, comma between city and state)
```
All pairwise driving distances are computed from coordinates via the haversine formula times a 1.25 road-circuity factor (roads are never straight lines). Regions are ordered by computed distance from the origin, which serves as the progress axis.

## Verification
checker.cpp reimplements the search as brute-force enumeration over all valid routes and is diffed against the solver across hundreds of randomly generated instances (gen.cpp) for **ROADTRIP** and **TOP5**, including infeasible ones — both programs must agree on every optimum and reject identically. Only checks that the optimal route is returned; it does not verify that calculations are correct.

## Gaps and Future Implementations
As an in-progress installation, this project does not use real site ratings or roadmap directions. The input files present are acquired from a google search of locations worth visiting and their coordinates, as well as random cities or towns that would be less appealing on a road trip. Cities/high-volume dwellings carry a higher/worse score. Distances are calculated by the haversine formula and weighted by a road factor of 1.25 to account for the straight-line travel found by the formula. Each location contains its respective distance from every single other site in the dataset. Perhaps on a future revisit, I would implement actual map APIs to calculate location ratings and actual campsites based on both their online review ratings and how secluded a campground is relative to nearby campgrounds. Also, I would find quick roadside attractions to stop at along the way when that data is available.

I created this program with a planned road trip from Colorado to California in mind and wanted to test my recent learnings and see how this route finder compares to online suggestions. While, of course, my input was inaccurate and somewhat arbitrary. For some places, I had to make up star ratings, mostly for cities and less desirable locations. Since all locations have relatively high Google ratings, I drew large cities down to ratings of 2.4 - 3.2, scenic towns 3.5- 4.2, lesser nature scenes and National Parks 4.2-4.3, top-ranked parks 4.8-5, with their rating adjusted based on travel guides' rankings of the top 15 National Parks to see. Therefore, the location ratings are weighted towards nature as well as mine and other people's opinions. Here is how my output differed from a Google search: 

