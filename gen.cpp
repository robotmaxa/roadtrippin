
// random test input generator: prints a valid nested-format input to stdout,
#include <getopt.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

struct GenOptions {
    unsigned int seed = 0;
    int regions = 15;
};

static void getOptions(int argc, char **argv, GenOptions &options) {
  opterr = false;

  option longOptions[] = {
    {"seed", required_argument, nullptr, 's'},
    {"regions", required_argument, nullptr, 'r'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, '\0'}
  };

  int choice;
  int index = 0;

  while ((choice = getopt_long(argc, argv, "s:r:h", longOptions, &index)) != -1) {
    switch (choice) {
      case 'h':
        cout << "Enter 'seed' and 'regions' to generate a random input.\n";
        exit(0);

      case 's':
        options.seed = static_cast<unsigned int>(atoi(optarg));
        break;

      case 'r':
        options.regions = atoi(optarg);
        break;

      default:
        cerr << "Error: invalid command line option\n";
        exit(1);
    }
  }

  if (options.regions < 2) {
    cerr << "Error: need at least 2 regions\n";
    exit(1);
  }
}

int main(int argc, char* argv[]){
    GenOptions o1;
    getOptions(argc, argv, o1);
    srand(o1.seed);

    cout << fixed << setprecision(1);
    cout << o1.regions << "\n";

    // node 0: start location, no sites
    cout << "0 0 0\n";

    for (int i = 1; i < o1.regions; ++i) {
        // drive time spread over the 3-36 hour corridor, one decimal place
        double driveTime = 3.0 + (rand() % 331) / 10.0;

        // regions deep enough along the corridor can be destination
        int terminalFlag = 0;
        if (driveTime > 24.0 && rand() % 5 < 3) {
            terminalFlag = 1;
        }

        int siteCount = 1 + rand() % 6;

        cout << driveTime << " " << terminalFlag << " " << siteCount << "\n";

        for (int s = 0; s < siteCount; ++s) {
            int x = rand() % 100001 - 50000;
            int y = rand() % 100001 - 50000;
            double beauty = 0.2 + (rand() % 39) / 10.0;
            double secluded = 0.2 + (rand() % 39) / 10.0;
            cout << x << " " << y << " " << beauty << " " << secluded << "\n";
        }
    }
}
