#include <map>
#include <string>
#include <vector>

namespace Mute {
struct Config {
    int                              version     = 1;
    long long                        all         = 0;
    std::map<std::string, long long> players     = {};
    std::vector<std::string>         disabledCmd = {"me", "msg", "tell", "w"};
};
} // namespace Mute