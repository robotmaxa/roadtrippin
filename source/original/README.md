# Roadtrip Optimizer

Plans a multi-night camping road trip from an origin to a fixed destination.
You supply candidate regions with coordinates and campsites; it returns which
regions to overnight in.

Everything is derived from geometry — no precomputed distances, no hand-set
endpoint flags. Two independent solvers, each verified against exhaustive
search.

**Tech:** C++17, dynamic programming, Held-Karp, graph algorithms

## Building

```
make            # builds roadtrip, checker, gen
make test       # builds the test runner and runs the whole suite
make clean
```

Or by hand:

```
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -O2 -o roadtrip roadtrip.cpp
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -O2 -o checker  checker.cpp
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -O2 -o gen      gen.cpp
```

## Running

```
./roadtrip --mode ROADTRIP --nights 7 --lo 50 --hi 450 --final 700 < western-usa.txt
```

| flag | default | meaning |
|---|---|---|
| `--mode` | required | `ROADTRIP` or `OPTLOOP` |
| `--nights <n>` | 7 | overnight stops to plan for (ROADTRIP only) |
| `--lo <miles>` | 50 | shortest acceptable drive between stops |
| `--hi <miles>` | 450 | longest acceptable drive between stops |
| `--final <miles>` | 700 | longest final drive into the destination, and the leg home in `OPTLOOP` |

Exit code is `0` on success, `1` on any failure (no route, bad input, bad flags).

### Modes

| Mode | Problem | Algorithm |
|---|---|---|
| `ROADTRIP` | Best N-night route to the destination | DP over `(nights, region)` on a DAG |
| `OPTLOOP` | Min-mileage tour of every region, returning home | Held-Karp exact TSP |

### Sample output

```
$ ./roadtrip --mode ROADTRIP < western-usa.txt
4.80
0 4 26 44 45 48 58 59
  stop 1: RockyMountainNP (node 4) with 1142.24 mi left (+69.42 mi drive), site score 4.70
  stop 2: ArchesNP (node 26) with 873.15 mi left (+295.11 mi drive), site score 4.70
  stop 3: GrandCanyonNP (node 44) with 724.80 mi left (+289.94 mi drive), site score 4.90
  stop 4: BryceCanyonNP (node 45) with 699.21 mi left (+132.92 mi drive), site score 4.80
  stop 5: ZionNP (node 48) with 644.34 mi left (+62.92 mi drive), site score 4.90
  stop 6: SequoiaNP (node 58) with 287.71 mi left (+388.94 mi drive), site score 4.70
  stop 7: YosemiteNP (node 59) with 196.71 mi left (+136.59 mi drive), site score 4.90
  destination: +196.71 mi
  total: 1572.57 mi
```

The `mi left` column decreases strictly down the list. That is the progress
invariant, visible in the output.

## Input format

```
line 1:      origin lat lon
line 2:      destination lat lon
line 3:      region count
per region:  lat lon siteCount
             siteCount lines of: name lat lon beautyScore
```

Names are single tokens (`Moab,UT`). Beauty is a star rating out of 5, higher
is better. Node 0 is the origin and has no sites.

## How it works

- **Distance** — haversine great-circle miles times `ROAD_FACTOR = 1.25`, since
  roads are not straight lines.
- **`mileMark`** — road distance from a region *to the destination*. This is the
  progress axis.
- **`miles[n*n]`** — full pairwise matrix; what travel actually costs.
- **Score** — `avgCampScore`, the mean beauty rating of a region's campsites.
  The reported figure is the mean across the trip's stops. See Limitations.

### Constraints

1. **Progress** — every leg must strictly *decrease* `mileMark`. Getting farther
   from where you started is not progress; getting closer to where you are going
   is.
2. **Daily window** — every leg between stops falls in `[--lo, --hi]`.
3. **Closing leg** — the final region must be within `--final` of the
   destination. Neither `--lo` nor `--hi` applies: a day of driving is a day you
   plan around, the final approach is just arriving. `OPTLOOP` bounds its leg
   home the same way.
4. **Fixed length** — exactly `--nights` stops.

Constraint 1 is load-bearing: strict monotonicity makes the graph **acyclic**, so
no visited-set is needed and the DP stays polynomial. That is the entire reason
`ROADTRIP` is not as expensive as `OPTLOOP`.

## Complexity

Let **n** = nodes (regions + origin), **N** = nights, **S** = total campsites.

| Component | Time | Space |
|---|---|---|
| `read_regions` | O(n + S) + O(n log n) sort | O(n + S) |
| `computeMiles` | O(n²) | O(n²) |
| `validEdge` | O(1) | — |
| `runDP` | **O(N · n²)** | **O(N · n)** |
| `buildRoute` | O(N) | O(N) |
| `bestRoute` | O(n) | — |
| `heldKarp` | **O(2ⁿ · n²)** | **O(2ⁿ · n)** |
| `minPath` | O(n) | O(n) |

**ROADTRIP end to end:** O(N · n²) time, O(n²) space. The distance matrix
dominates memory, the DP dominates time — `N × n` states, each relaxed against
`n` predecessors.

**OPTLOOP end to end:** O(2ⁿ · n²) time, O(2ⁿ · n) space, capped at 20 regions
where the two tables reach roughly 500 MiB combined. That cap is a memory wall,
not a time one, and the program refuses past it rather than failing at runtime.

The gap between O(N · n²) and O(2ⁿ · n²) is the point: `ROADTRIP` has a natural
ordering to exploit, a tour does not.

## Design notes

- **One arrival per DP state, not k-best.** Every extension of a state depends
  only on its score, so a suboptimal arrival can never win downstream.
  Ties break toward the larger parent index — a strict total order, since each
  candidate for a state carries a distinct parent.
- **Fixed trip length rather than a range.** An earlier version searched 4–7
  stops and padded shorter routes with the global average. That made the score
  monotonically increasing in stop count, so it was structurally incapable of
  preferring a shorter trip — a *perfect* 4-night route could not beat a
  mediocre 7-night one. Fixing the length removes the invented exchange rate
  between trip length and quality; the score is now a plain mean.
- **Endpoints are derived, not flagged.** Any region within `--final` of the
  destination can finish, replacing an input field that could contradict the
  geometry.
- **`--final` is separate from `--hi`.** One flag doing both jobs coupled a day's
  drive to the final approach, so a dataset with no region near the destination
  would starve the finish test even though its daily legs were fine. Splitting
  them made both bounds mean one thing each.
- **Regions sort descending by `mileMark`** so node index increases along the
  direction of travel. The tie-break rules are written in terms of node index
  and depend on this.

### Choosing the window

Measured on `western-usa.txt` at 7 nights, holding the other two flags at their
defaults:

| `--lo` | score | total mi |
|---|---|---|
| 50 | **4.80** | 1572.57 |
| 75 | 4.74 | 1481.62 |
| 100 | 4.74 | 1481.62 |
| 125 | 4.73 | 1738.80 |
| 150 | 4.67 | 1732.09 |

| `--hi` | score | total mi |
|---|---|---|
| 450 | **4.80** | 1572.57 |
| 500 | 4.81 | 2167.66 |
| 550 | 4.81 | 2244.87 |
| 700 | 4.81 | 2244.87 |

- **`--lo`** is set by route quality. Lowering it only ever adds edges, so the
  attainable score is weakly decreasing in `--lo`. It also trades against trip
  length, since every extra night needs another region at least `--lo` from the
  last.
- **`--hi`** is the knee of the mileage curve. 450 scores 4.80 over 1573 miles;
  500 buys the last 0.01 of score for 595 extra miles.
- **`--final`** is set by the data: a trip can only end at a region within
  `--final` of the destination. Both bundled datasets now reach within 197 miles
  (Yosemite), so 700 is loose on them — it is sized for inputs whose nearest
  region is far out, and it is what keeps the daily ceiling free to be tight.

Lowering `--lo` does not break the math — acyclicity comes from `mileMark`, not
the window — but a low floor lets a "night" be a short hop rather than a full
day of driving. 50 maximizes route quality; raise it if you want every stop
earned by real mileage.

## Verification

```
make test
```

`tests.cpp` is the runner. It drives `roadtrip` and `checker` through the command
line, the same way a user would, so it exercises the real interface rather than
internals. Three groups:

**Fixtures** — small hand-built inputs, each isolating one rule. Every one is
a file you can read in ten seconds and run by hand.

| File | Run with | Asserts |
|---|---|---|
| `linear.txt` | `--nights 4` | four evenly spaced regions are all used, in progress order |
| `choice.txt` | `--nights 2` | a 2.0-rated region between origin and a 4.8 is skipped, not taken for being first |
| `multisite.txt` | `--nights 2` | a region holding a 3.0 and a 5.0 scores their **mean**, 4.0 — pins the placeholder scoring |
| `siteless.txt` | `--nights 2` / `3` | a region with no campsites is avoided when possible, traversed without crashing when forced |
| `tie.txt` | `--nights 2` / `3` | two regions at equal distance: one is chosen, and they cannot chain — the acyclicity invariant |
| `unreachable.txt` | any | every region is nearer than `--lo`, so no edge exists at all |
| `badheader.txt` | any | a negative site count is caught while parsing, before `vector::resize` |

**Rejection paths** — all eleven error exits, each checked for its exact message
and a nonzero code.

**Differential sweep** — the real correctness argument. `checker.cpp` solves the
same instances by exhaustive search with no memoization: DFS over every valid
route for `ROADTRIP`, full permutation enumeration for `OPTLOOP`. It shares
parsing, geometry and scoring by necessity — both programs must read input
identically — but the search shares no code. `gen` is deterministic on a seed,
so both binaries are fed the same instance without a temporary file.

To reproduce one case by hand:

```
./gen --seed 7 --regions 14 > in.txt
diff <(./roadtrip --mode ROADTRIP --nights 5 < in.txt) \
     <(./checker  --mode ROADTRIP --nights 5 < in.txt)
```

Keep `--regions` small for `OPTLOOP` checks; permutation search is factorial and
refuses above 9 regions.

Current results, comparing full output rather than scores alone:

| Check | Instances | Result |
|---|---|---|
| ROADTRIP vs exhaustive DFS, 200 seeds × nights 1–7 | 1400 | **1400 / 1400** on score, 1399 on full output |
| OPTLOOP vs permutation search, 200 seeds × 9 regions | 200 | **200 / 200** |

Two details behind those numbers, because both are easy to overstate:

- **529 of the 1400** ROADTRIP cases are instances both programs declare
  infeasible. An agreed rejection is as much a correctness result as a matching
  route, but it is not a route, so it is worth counting separately: 871 produced
  one.
- **One case out of 1400** matches on score while choosing a different, equally
  optimal route (seed 110 at 6 nights). When two regions' scores differ by about
  1 ULP, the DP settles the tie while relaxing interior states and the checker
  settles it on completed routes. Neither ordering is more correct, so the score
  is what must match. A differing *score* is always a hard failure.

## Limitations

1. **`avgCampScore` is a placeholder.** It averages every site in a region though
   a trip uses only one, and beauty is the only signal in the input — seclusion,
   availability, fees and access are all absent. Replace the derivation wholesale
   when real campsite data arrives rather than tuning weights on top of it.
2. **The checker shares the input path.** The *search* is independently verified;
   parsing, haversine and scoring are not.
3. **Total mileage is reported but never constrained.** A hard cap would require
   `(stops, node, miles)` state, and miles is continuous — so either discretize
   and lose the exactness guarantee, or keep a Pareto frontier per state.
4. **`Cannot construct route` carries no diagnostic.** It does not distinguish a
   window too narrow to chain regions from nothing reaching the destination.
5. **`gen.cpp` scatters regions in a box** rather than along the corridor, so
   sparse instances are frequently infeasible at the defaults: at 7 nights,
   0/25 seeds solve with 8 regions, 2/25 with 12, 9/25 with 15, and 25/25 with
   20. Raising `--hi` to 700 lifts the 12-region case to 21/25. The generator is
   useful for verification, where agreed rejections are still evidence, but it
   is not a realistic corridor.
6. **A region colocated with the origin is unreachable.** `validEdge` requires a
   strict decrease in `mileMark`, so a region at the origin's exact coordinates
   can never be entered. `western-usa.txt` contains such a region (`Denver,CO`),
   making it 59 regions of which 58 are usable.
