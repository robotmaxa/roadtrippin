# Road Trip Optimizer

Given a start and a destination, finds the best-scoring chain of stops
between them: national parks and natural wonders always outrank cities,
no single day's drive exceeds 450 miles, and every stop must move you
strictly closer to the destination than the one before (no backtracking,
though the route doesn't have to be a straight line). You pick the trip
length in days; the optimizer fits the best scenic stops it can into that
budget.

## Build

```
g++ -O2 -std=c++17 -Iinclude src/main.cpp -o road_trip
```

or with CMake:

```
cmake -S . -B build
cmake --build build
```

Either way, the program expects `data/locations.csv` to be reachable at
`data/locations.csv` relative to the working directory it's run from (the
CMake build copies `data/` next to the binary automatically).

## Run

```
./road_trip                                          # interactive prompts
./road_trip "Denver, CO" "Grand Canyon National Park" # direct
./road_trip "Denver, CO" "Zion National Park" 5       # 5-day trip
./road_trip "Denver, CO" "Zion National Park" 5 300   # + 300 mi/day cap
./road_trip --list                                    # see every known location
```

Location names can be typed loosely ("Zion", "Yellowstone", "Denver, CO")
-- matching is case-insensitive and falls back to substring search. If a
query matches more than one location, the program lists the candidates so
you can be more specific.

## How it works

- **Data**: `data/locations.csv` is a curated set of national parks,
  natural wonders, and interstate-corridor cities across the continental
  US, each with a lat/lon and a 0-5 base rating. Cities exist mainly to
  bridge long gaps between scenic stops. Distances are estimated with the
  haversine (great-circle) formula, since no live routing API is used --
  real driving distance will run a bit longer, especially on winding
  mountain roads.
- **Scoring**: every national park / natural wonder gets a flat bonus on
  top of its rating (`Location::score()` in `include/roadtrip/Location.hpp`)
  large enough that any natural stop always outscores any city, regardless
  of raw rating.
- **Direction constraint**: a stop is only reachable if it's strictly
  closer to the destination (great-circle distance) than the previous
  stop. This is what "always move toward the destination, never go the
  opposite way" means here -- it permits meandering detours in any
  direction as long as net progress is made, and it works for trips in any
  compass heading (not just east/west).
- **Trip length**: the direction rule alone doesn't bound total mileage --
  without a stop-count cap, the optimizer would happily zig-zag through
  every scenic stop in the country as long as each hop is individually
  legal. The number of days you choose caps the path length (`RouteOptimizer`
  runs a day-bounded longest-path DP over the resulting DAG), which is what
  keeps the result an actual road trip instead of a 20,000-mile detour
  marathon. If you don't specify a day count, the program suggests one
  based on the direct distance plus a little slack for detours.
- **Optimizer**: `include/roadtrip/RouteOptimizer.hpp` builds a DAG where
  an edge `u -> v` exists iff `v` is closer to the destination than `u` and
  the leg is within the daily mile cap, then finds the highest-scoring path
  from start to destination using at most the chosen number of days.

## Extending

Add more rows to `data/locations.csv` (format: `name,state,lat,lon,category,rating`,
category one of `NATIONAL_PARK`, `NATURAL_WONDER`, `CITY`) to widen coverage
or fix a stretch where no route can be found because consecutive waypoints
are more than 450 miles apart.
