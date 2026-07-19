
# Roadtrip Optimizer

Route optimization for multi-day road trips under driving-distance constraints.

## Problem

Given a fixed route (e.g., Colorado to California) with candidate campsites at known positions with known ratings scaled, select overnight stops such that:
- Every daily leg respects a maximum driving distance
- Total cost (time, distance, or fuel) is minimized by 'OPTLOOP'
- A valid itinerary exists from start to end
- The routes with the best (lowest) score are returned by the program
- NOTE: For the sake of expanding possible routes, the end node is not a particular location in California, but instead just anywhere that is in the state of California

The user can select the range of miles they would like to drive in a day, and the program includes the minimum and maximum number of stops the route can have. To calculate the final score of each route, elongate each route by (MAX_STOPS - stops) * global_avg of all regions in the sample. That way, a shorter route must have truly exceptional site scores to beat out a longer route.

A passion project for further exploration of dynamic programming by implementing 1D and 2D memos. There are three output modes: 'ROADTRIP', 'TOP5', AND 'OPTLOOP':

- 'ROADTRIP': returns a detailed summary of the best route found, including site name, miles from the previous site, and the site and campground score
- 'TOP5': returns a detailed summary of the top 5 routes found, including site name, miles from the previous site, and the site and campground score (one small caveat is that most of the best routes return the same first site visited)
- 'OPTLOOP': A dynamic programming solution to the TSP for the shortest possible route to visit all locations and loop back home.

As an in-progress installation, this project does not use real site ratings or roadmap directions. The input files present are acquired from a google search of locations worth visiting and their coordinates, as well as random cities or towns that would be less appealing on a road trip. Cities/high-volume dwellings carry a higher/worse score. Distances are calculated by the haversine formula and weighted by a road factor of 1.25 to account for the straight-line travel found by the formula. Each location contains its respective distance from every single other site in the dataset. Perhaps on a future revisit, I would implement actual map APIs to calculate location ratings and actual campsites based on both their online review ratings and how secluded a campground is relative to nearby campgrounds. Also, I would find quick roadside attractions to stop at along the way when that data is available.

I created this program with a planned road trip from Colorado to California in mind and wanted to test my recent learnings and see how this route finder compares to online suggestions. While, of course, my input was inaccurate and somewhat arbitrary, I created location ratings based on my own research. Here is how my output differed from a Google search: 

## Solution
'ROADTRIP':
- The campsites/locations form a directed acyclic graph (DAG) ordered by position along the route. An edge `i → j` exists if and only if the distance from site `i` to site `j` is within one day's driving limit
- Solved via **dynamic programming** over the topological order (position):
'TOP5':
-  Same as 'ROADTRIP' but returns the top 5 memo outcomes
'OPTLOOP': Given a set of input locations to visit, employs Held-Karp as a dynamic programming solution to the TSP for the shortest possible route to visit all locations and loop back home. Still utilizes the mileage limits, so it does not just attack all sites and then drive straight home.

