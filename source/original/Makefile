# Roadtrip Optimizer

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -pedantic -O2

BINS := roadtrip checker gen

.PHONY: all test clean

all: $(BINS)

roadtrip: roadtrip.cpp roadtrip.hpp
	$(CXX) $(CXXFLAGS) -o $@ roadtrip.cpp

checker: checker.cpp roadtrip.hpp
	$(CXX) $(CXXFLAGS) -o $@ checker.cpp

gen: gen.cpp
	$(CXX) $(CXXFLAGS) -o $@ gen.cpp

tests: tests.cpp
	$(CXX) $(CXXFLAGS) -o $@ tests.cpp

# fixture checks, every rejection path, and a differential sweep of the DP
# against the brute-force checker. needs all three binaries built, because it
# drives them through the command line the same way a user would.
test: $(BINS) tests
	./tests

clean:
	rm -f $(BINS) tests
