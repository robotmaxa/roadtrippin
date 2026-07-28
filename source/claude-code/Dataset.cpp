#include "Dataset.hpp"

#include <fstream>
#include <sstream>

namespace {

// "Moab,UT" -> ("Moab", "UT"); "ArchesNP" -> ("ArchesNP", "").
void splitName(const std::string &token, std::string &name, std::string &state) {
    const std::string::size_type comma = token.find(',');
    if (comma == std::string::npos) {
        name = token;
        state.clear();
        return;
    }
    name = token.substr(0, comma);
    state = token.substr(comma + 1);
}

std::string at(int region) { return " (region " + std::to_string(region + 1) + ")"; }

} // namespace

bool loadDataset(const std::string &path, Dataset &out, std::string &error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot open \"" + path + "\"";
        return false;
    }

    double oLat = 0, oLon = 0, dLat = 0, dLon = 0;
    int count = 0;

    if (!(in >> oLat >> oLon)) {
        error = "expected an origin \"lat lon\" on line 1";
        return false;
    }
    if (!(in >> dLat >> dLon)) {
        error = "expected a destination \"lat lon\" on line 2";
        return false;
    }
    if (!(in >> count) || count < 1) {
        error = "expected a positive region count on line 3";
        return false;
    }

    out = Dataset{};
    out.origin.name = "Origin";
    out.origin.lat = oLat;
    out.origin.lon = oLon;
    out.destLat = dLat;
    out.destLon = dLon;
    out.places.reserve(static_cast<size_t>(count));

    for (int r = 0; r < count; ++r) {
        double rLat = 0, rLon = 0;
        int siteCount = 0;
        if (!(in >> rLat >> rLon >> siteCount) || siteCount < 0) {
            error = "malformed region header" + at(r);
            return false;
        }

        for (int s = 0; s < siteCount; ++s) {
            std::string token;
            Place p;
            if (!(in >> token >> p.lat >> p.lon >> p.rating)) {
                error = "malformed site line" + at(r);
                return false;
            }
            splitName(token, p.name, p.state);
            out.places.push_back(p);
        }
    }

    if (out.places.empty()) {
        error = "dataset declares " + std::to_string(count) + " regions but contains no sites";
        return false;
    }
    return true;
}
