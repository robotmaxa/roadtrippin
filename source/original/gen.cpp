
// random test input generator: prints a valid nested-format input to stdout,

#include <getopt.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
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

    cout << fixed << setprecision(4);
    cout << "39.7392 -104.9903\n";               // origin: Denver
    cout << "37.7749 -122.4194\n";               // destination: San Francisco
    cout << (o1.regions - 1) << "\n";

    for (int i = 1; i < o1.regions; ++i) {
        // coordinates in a western-US box; distance structure emerges
        // from the geometry rather than being assigned directly
        double lat = 32.0 + (rand() % 1201) / 100.0;    // 32 - 44
        double lon = -102.0 - (rand() % 1801) / 100.0;  // -102 - -120

        int siteCount = 1 + rand() % 3;
        cout << lat << " " << lon << " " << siteCount << "\n";

        for (int s = 0; s < siteCount; ++s) {
            double slat = lat + (rand() % 41 - 20) / 100.0;
            double slon = lon + (rand() % 41 - 20) / 100.0;
            double beauty = 0.2 + (rand() % 39) / 10.0;
            cout << "Region" << i << "Site" << s << " " << slat << " " << slon
                 << " " << beauty << "\n";
        }
    }
}
