# Road Trip Planner

A C++17 command-line planner that finds an attraction-rich, forward-only road trip
between user-selected locations in the contiguous United States.

The optimizer:

- never selects a stop that moves backward along the start-to-destination axis;
- limits every driving day/leg to 450 estimated road miles;
- strongly favors national parks and natural wonders over cities;
- allows worthwhile side routes within an automatically widened search corridor;
- reports each stop, miles from the previous stop, and total miles driven.

## Build

```sh
cmake -S . -B build
cmake --build build
```

If CMake is unavailable, compile directly:

```sh
clang++ -std=c++17 -Wall -Wextra -Wpedantic src/main.cpp -o road_trip_planner
```

## Run

Interactive:

```sh
./build/road_trip_planner
```

With command-line arguments:

```sh
./build/road_trip_planner "New York City" "Los Angeles"
./build/road_trip_planner "Denver" "Seattle"
```

Or load a trip dataset whose first two lines are start and destination
coordinates, whose third line is the record count, and whose records use the
two-line format demonstrated in `road_trip_data.txt`:

```sh
./build/road_trip_planner road_trip_data.txt
```

Locations can be embedded place names or coordinates:

```sh
./build/road_trip_planner "39.7392,-104.9903" "47.6062,-122.3321"
```

Name matching is case-insensitive. State suffixes are accepted, such as
`Denver, CO`.

## How routing works

Places are projected onto the axis from the start to the destination. Only places
with increasing projected progress are connected, so a route can detour sideways
but cannot head opposite the destination. A directed acyclic graph connects stops
whose estimated driving distance is at most 450 miles. Dynamic programming then
maximizes attraction value while penalizing added mileage and low-value overnight
stops.

Ratings are normalized to their source scale, so `4.5/5` and `9/10` have equal
base quality. Natural places carry a large category bonus and their normalized
rating has much more influence than a city's rating. Cities have deliberately low
value and are selected mainly when needed to keep a trip feasible.

## Important limitations

The project is offline and uses an embedded U.S. dataset. Road mileage is estimated
from great-circle distance with a road-circuity multiplier; it does not query live
roads. Always verify actual routes, weather, closures, permits, and daily distances
with a current mapping service before traveling.

To expand coverage, add `Place` entries in `places()` in `src/main.cpp`. A production
version could replace `roadMiles()` with a routing API and load attractions from an
external database without changing the optimizer's overall structure.
