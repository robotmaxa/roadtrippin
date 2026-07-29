# Roadtrip Optimizer — Specification

Normative specification for the solver, the reference checker, and the input
generator. "MUST" marks required behavior; anything else is rationale.

Status: implemented and verified. See [Acceptance](#8-acceptance-criteria).

---

## 1. Purpose and scope

Given an origin, a fixed destination, and a set of candidate camping regions,
select which regions to overnight in.

In scope:

- **ROADTRIP** — choose exactly N overnight stops maximizing mean campsite
  quality, subject to daily mileage limits and monotone progress toward the
  destination.
- **OPTLOOP** — the minimum-mileage closed tour visiting every region once and
  returning to the origin. Independent of the destination and the progress axis.

Out of scope: real road networks, campsite availability, time-of-year, cost,
multi-vehicle, and any optimization of total trip mileage.

## 2. Deliverables

| File | Role |
|---|---|
| `roadtrip.hpp` | Shared structs and declarations |
| `roadtrip.cpp` | Solver, both modes |
| `checker.cpp` | Independent brute-force reference |
| `gen.cpp` | Random instance generator |

All C++17, no external dependencies, stdin/stdout only. Every translation unit
MUST compile clean under `-std=c++17 -Wall -Wextra -Werror -pedantic -O2`.

## 3. Input

Whitespace-delimited on stdin:

```
line 1:      originLat originLon
line 2:      destLat destLon
line 3:      regionCount
per region:  lat longi siteCount
             siteCount lines of: name lat longi beautyScore
```

`name` MUST be a single token (`Moab,UT`). `beautyScore` is a star rating out of
5 where higher is better. Node 0 is the origin; it is not a region line and has
no campsites. Total node count is `regionCount + 1`.

There is no endpoint flag. Whether a region can end a trip is derived
(§5.4).

### 3.1 Validation

Every read MUST be checked. Malformed input MUST fail loudly rather than degrade
into a misleading "no route" result. All messages to stderr, `exit(1)`:

| Condition | Message |
|---|---|
| header unreadable, or `regionCount < 1` | `Error: need an origin, a destination and at least 1 region` |
| region line unreadable, or `siteCount < 0` | `Error: malformed region header` |
| campsite line unreadable | `Error: malformed campsite` |

A negative count MUST NOT reach `vector::resize`; an uncaught `std::length_error`
that still exits 0 is the specific failure this prevents.

## 4. Data model

```cpp
struct Options  { string mode; double windowLo = 50.0, windowHi = 450.0, windowFinal = 700.0; int nights = 7; };
struct Campsite { string name; double lat = 0.0, longi = 0.0, beautyScore = 0.0; };
struct Region   { double lat = 0.0, longi = 0.0, mileMark = 0.0;
                  vector<Campsite> sites; double avgCampScore = 0.0; };
struct Arrival  { double score = -1.0; size_t parent = 0; bool reached = false; };
struct TripData { int regions = 0; double destLat = 0.0, destLon = 0.0;
                  vector<Region> node_tracker; vector<double> miles;
                  vector<vector<Arrival>> dp;
                  vector<double> loopDp; vector<int> loopParent; };
```

### 4.1 Constants

| Name | Value | Meaning |
|---|---|---|
| `EARTH_RADIUS` | 3958.8 | miles |
| `PI` | 3.14159265358979323846 | |
| `ROAD_FACTOR` | 1.25 | roads are not straight lines |
| `WINDOW_LO` / `WINDOW_HI` | 50.0 / 450.0 | daily drive bounds, overwritten from `Options` |
| `WINDOW_FINAL` | 700.0 | final approach + leg home, overwritten from `Options` |
| `NIGHTS` | 7 | default, overwritten from `Options` |
| `MAX_NIGHTS` | 365 | sanity ceiling on `--nights` |
| `MAX_LOOP_REGIONS` | 20 | Held-Karp memory cap |
| `INF` | 1e18 | |
| `UNSET` | -1.0 | |

File-scope defaults MUST stay in sync with the `Options` defaults.

## 5. Geometry and derived quantities

### 5.1 Distance

`haversine(lat1, lon1, lat2, lon2)` returns great-circle miles. Every distance
used anywhere MUST be multiplied by `ROAD_FACTOR`.

### 5.2 Two distinct distance concepts

These MUST NOT be conflated:

- **`Region::mileMark`** — road distance from a region **to the destination**.
  The progress axis, nothing else.
- **`TripData::miles`** — flattened `n × n` matrix of pairwise road distances
  between nodes. What travel actually costs.

The origin MUST also receive a `mileMark` (origin → destination). Without it the
origin reads as zero road remaining and, under a strictly-decreasing rule,
nothing is reachable from it.

### 5.3 Ordering

After reading all regions, sort `node_tracker.begin() + 1 .. end()` **descending**
by `mileMark`, leaving node 0 fixed. Build `miles` **after** the sort — the matrix
is indexed by post-sort node numbers.

Descending order means node index increases along the direction of travel. The
tie-break rules in §6.3 and §7 are expressed in terms of node index and depend
on this.

### 5.4 Scoring

`computeSiteScore(site)` returns `site.beautyScore`. Higher is better
everywhere; there is no inversion anywhere in the program.

`Region::avgCampScore` is the mean site score over the region, or `0.0` when the
region has no sites — therefore the worst possible value, not the best.

> **This derivation is a placeholder** and MUST be documented as such. It
> averages every site though a trip uses one per region, and beauty is the only
> signal in the input. When real campsite data arrives, replace the derivation
> wholesale rather than tuning weights on top of it.

### 5.5 Edge rule

`validEdge(t, p, q)` is true iff **both**:

1. `mileMark[q] < mileMark[p]` — strictly less road remaining;
2. `miles[p*n + q] ∈ [WINDOW_LO, WINDOW_HI]`.

Compare with plain `<` / `>=` / `<=`. No epsilon: these distances come from
haversine over continuous coordinates, so exact boundary hits are measure-zero.

Condition 1 is load-bearing. Strict monotonicity makes the graph **acyclic**, so
the DP needs no visited-set and stays polynomial. It also encodes the modeling
claim that progress means approaching the destination, not receding from the
origin — the latter permits arbitrary sideways detours.

### 5.6 Finishing

`canFinish(t, q)` is `mileMark[q] <= WINDOW_FINAL`.

Neither `WINDOW_LO` nor `WINDOW_HI` MUST apply. The daily window describes a day
of driving you plan around; the final approach is just arriving. OPTLOOP's leg
home MUST be bounded by `WINDOW_FINAL` on the same reasoning.

## 6. Mode ROADTRIP

### 6.1 Trip length

The trip is exactly `NIGHTS` stops. Trip length is an **input, not an objective**:
without it fixed, routes of different lengths cannot be compared without
inventing the value of a marginal night.

Guards:

| Condition | Message |
|---|---|
| `nights < 1` or `> MAX_NIGHTS` | `Error: --nights must be between 1 and 365` |
| `nights > regionCount` | `Error: --nights is <n> but only <r> regions are available` |

The second check MUST run after input is read and before the DP. Every stop is a
distinct region, so a longer trip is impossible; reporting that beats an
unexplained "Cannot construct route" after the DP has churned through it.

### 6.2 The DP

`runDP(TripData&)` fills `dp[stops][node]` for `stops` in `1..NIGHTS`, keeping
**one** `Arrival` per state. A k-best list, rank dimension, or priority_queue
MUST NOT be used: every extension of a state depends on its score alone, so a
suboptimal arrival can never win downstream.

- **Seed** — for each `q` with `validEdge(0, q)`, set `dp[1][q]` to
  `{avgCampScore[q], parent = 0, reached = true}`.
- **Relax** — for `stops` in `2..NIGHTS`, for each `q`, scan every `p != q` with
  `dp[stops-1][p].reached && validEdge(p, q)`, keeping the best candidate scoring
  `dp[stops-1][p].score + avgCampScore[q]`.

### 6.3 Selection order

`betterArrival(a, b)`: an unreached arrival loses to everything; otherwise higher
score wins; ties go to the **larger** parent.

Each candidate for a given state carries a distinct parent, so this is a strict
total order over them and the winner is unique.

### 6.4 Objective

```
finalScore(node, stops) = dp[stops][node].score / stops
```

A plain mean. Every candidate has the same stop count, so there is nothing to
normalize against; dividing only keeps the printed figure on the scale of a
single campsite rating.

> An earlier revision searched a 4–7 stop range and padded shorter routes with
> the global average. That reduced to
> `globalAvg + (stops/MAX) × (mean − globalAvg)`, monotonically increasing in
> stop count for any above-average route — structurally unable to prefer a
> shorter trip. A *perfect* 4-night route could not beat a mediocre 7-night one.
> Do not reintroduce length-dependent normalization.

### 6.5 Reconstruction and terminal choice

`buildRoute(node, stops)` walks `parent` backward while `cur != 0`, decrementing
`stops` per hop, pushes node 0, and reverses in place.

`bestRoute` scans every `q` that `canFinish` and is reached at `NIGHTS`,
maximizing `finalScore`, breaking ties toward the larger node. Sentinels:
`bestSoFar = UNSET`, `bestNode = n`. If nothing qualifies, print
`Cannot construct route` to stderr and `exit(1)`.

### 6.6 Output

```
4.71
0 3 4 18 26 28 44 48
  stop 1: GreatSandDunesNP (node 3) with 1146.91 mi left (+173.06 mi drive), site score 4.40
  ...
  destination: +644.34 mi
  total: 2442.36 mi
```

Line 1 is the score, line 2 the node sequence. All figures at
`fixed << setprecision(2)`. `total` includes the closing drive, which equals the
final region's `mileMark`.

`bestSiteInRegion(r)` returns the highest-scoring site index, or `r.sites.size()`
when the region has none. That sentinel MUST be documented on the function and
**checked at every call site**; print `(no campsites)` and omit the
`, site score ...` clause rather than indexing out of bounds.

## 7. Mode OPTLOOP

Minimum-mileage closed tour visiting every node once, returning to the origin.
Independent of the destination and the progress axis.

Reject `n > MAX_LOOP_REGIONS` with `Error: too many regions for OPTLOOP`. The
table is `2^n × n`; at the cap the two arrays reach roughly 500 MiB.

`heldKarp` fills `loopDp[mask*n + i]` = minimum miles having visited exactly
`mask`, standing at `i`; `loopParent` holds the predecessor as `int`, initialized
`-1`. Base case `loopDp[1*n + 0] = 0.0`. Every leg MUST satisfy the full window.

> Bit 0 is set in the base mask and only ever added to, so node 0 is never a
> transition target and `loopParent[·*n + 0]` remains `-1`. Reconstruction uses
> this as its termination condition.

`minPath` closes the tour: over all `i >= 1` reachable at the full mask, minimize
`loopDp[FULL*n + i] + miles[i*n + 0]`, rejecting a return leg only when it
exceeds `WINDOW_HI` — no minimum on the leg home. If none qualifies, print
`Cannot construct loop` to stderr and `exit(1)`.

Reconstruct by pushing node 0, then walking parents while clearing each visited
bit until the parent is `-1`, then reversing. Output is total miles, then
`start:`, one `stop N:` line per region, and `return:`.

## 8. CLI

`getopt_long` with `opterr = false`; short forms `m:l:u:n:h`.

| Flag | Default | Meaning |
|---|---|---|
| `--mode` | required | `ROADTRIP` or `OPTLOOP` |
| `--nights <n>` | 7 | overnight stops (ROADTRIP only) |
| `--lo <miles>` | 50 | shortest drive between stops |
| `--hi <miles>` | 450 | longest drive between stops |
| `--final <miles>` | 700 | final approach, and leg home in OPTLOOP |
| `--help` | | usage, `exit(0)` |

| Condition | Message |
|---|---|
| unknown option | `Error: invalid command line option` |
| no `--mode` | `Error: --mode is required` |
| mode not recognized | `Error: invalid mode` |
| `windowLo <= 0`, `windowHi < windowLo`, or `windowFinal <= 0` | `Error: invalid window bounds` |

`main` MUST copy options into the file-scope globals **before** reading input,
then read, then dispatch, then `return 0`. Exit code is `0` on success, `1` on
any failure.

### 8.1 Default rationale

The defaults are set by the data:

- `--hi` has a floor near 637 for the instances on hand — a trip can only end at
  a region within `--hi` of the destination, and the worst instance's nearest
  region is 637 mi out. 700 leaves margin.
- `--lo` trades against trip length; every extra night needs another region at
  least `--lo` from the last, and above roughly 150 only short trips have the
  spacing.

Lowering `--lo` far below 100 does not break correctness — acyclicity comes from
`mileMark`, not the window — but it lets the optimizer spend nights on 60-mile
hops, clustering stops rather than earning them by driving.

## 9. Reference checker

Same input format and CLI; supports ROADTRIP and OPTLOOP. It necessarily
duplicates parsing, geometry, and scoring — both programs MUST read input
identically — but the **search MUST share no code**.

- **ROADTRIP** — exhaustive DFS over every valid route of exactly `NIGHTS` stops
  that can finish, no memoization. Output MUST be byte-identical to the solver's,
  which requires its own `bestSiteInRegion`.
- **OPTLOOP** — `next_permutation` over nodes `1..n-1`, identical leg rules,
  refusing above 9 non-origin nodes with
  `Error: too many regions for brute-force OPTLOOP`. Prints the total only.

The DFS MUST replicate the solver's tie policy to agree on routes: higher score,
then larger terminal node, then more stops, and on a full tie the path that is
lexicographically larger **read from the end backward** — what the larger-parent
rule cascades into.

Input validation MUST be mirrored, or the two diverge on malformed input and the
diff becomes unreliable.

## 10. Generator

`--seed`, `--regions`, `--help`. Emits Denver origin (`39.7392 -104.9903`),
San Francisco destination (`37.7749 -122.4194`), then `regions - 1` blocks with
coordinates in a western-US box (lat 32–44, lon −102 to −120), 1–3 sites each,
beauty roughly 0.2–4.1. Coordinates at `setprecision(4)`.

## 11. Complexity

Let **n** = nodes, **N** = nights, **S** = campsites.

| Component | Time | Space |
|---|---|---|
| `read_regions` | O(n + S) + O(n log n) | O(n + S) |
| `computeMiles` | O(n²) | O(n²) |
| `runDP` | **O(N · n²)** | **O(N · n)** |
| `bestRoute` + `buildRoute` | O(n + N) | O(N) |
| `heldKarp` | **O(2ⁿ · n²)** | **O(2ⁿ · n)** |
| `minPath` | O(n) | O(n) |

ROADTRIP end to end: O(N · n²) time, O(n²) space. OPTLOOP: O(2ⁿ · n²) time,
O(2ⁿ · n) space, capped at 20 regions.

Checker, verification only: ROADTRIP DFS O(n^N); OPTLOOP O(n!).

## 12. Acceptance criteria

1. All four files compile clean under `-Wall -Wextra -Werror -pedantic -O2`.
2. Solver output MUST match the checker's **score** on every generated instance
   at every trip length from 1 to `NIGHTS`. Full output MUST match except where
   two routes score identically (see below).
3. OPTLOOP MUST match permutation search on every instance within the
   brute-force cap.
4. Both shipped fixtures MUST produce a route on defaults.
5. Every §3.1 and §8 error path MUST produce its exact message and `exit(1)`.
6. A region with zero campsites MUST NOT crash any mode.
7. Two test runs started concurrently MUST NOT interfere. Captured output goes
   to a per-process scratch file; a fixed name lets one run read another's
   results, which presents as a fixture failing on data it never saw.

Current measured results, from `make test`:

| Check | Instances | Result |
|---|---|---|
| ROADTRIP vs exhaustive DFS, 200 seeds × nights 1–7 | 1400 | 1400 / 1400 on score, 1399 on full output |
| OPTLOOP vs permutation search, 200 seeds × 9 regions | 200 | 200 / 200 |

871 of the 1400 ROADTRIP cases produce a route; the remaining 529 are agreed
rejections, which is a correctness result but not a route.

Route-level disagreement at an *identical* score is expected and MUST NOT be
treated as a defect — one case in the current 1400 (seed 110, 6 nights). When
two regions' scores differ by about 1 ULP, the DP resolves the tie while
relaxing interior states and the checker resolves it on completed routes.
Neither ordering is more correct. A differing score is always a real failure.

## 13. Known gaps

1. `avgCampScore` is a placeholder (§5.4).
2. The checker shares the input path; the *search* is independently verified,
   parsing and geometry are not.
3. Total mileage is reported but never constrained. A hard cap needs
   `(stops, node, miles)` state, and miles is continuous — so either discretize
   and lose exactness, or keep a Pareto frontier per state.
4. `Cannot construct route` carries no diagnostic distinguishing a too-narrow
   window from nothing reaching the destination.
5. `gen.cpp` scatters regions in a box rather than along the corridor, so it can
   produce instances no sane parameters solve.
