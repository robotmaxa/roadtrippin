cat > README.md << 'EOF'
# Roadtrip Optimizer

Route optimization for multi-day road trips under driving-distance constraints.

## Problem

Given a fixed route (e.g., Ann Arbor to Colorado) with candidate campsites at known positions, select overnight stops such that:
- Every daily leg respects a maximum driving distance
- Total cost (time, distance, or fuel) is minimized
- A valid itinerary exists from start to end

## Solution

The campsites form a directed acyclic graph (DAG) ordered by position along the route. An edge `i → j` exists if and only if the distance from site `i` to site `j` is within one day's driving limit.

We solve via **dynamic programming** over the topological order (position):
