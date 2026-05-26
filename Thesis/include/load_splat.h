#include <vector>
#include <happly.h>

struct Splat
{
    float position[3];
    float sh[3];
    float opacity;
    float scale[3];
    float quaternion[4];
};

std::vector<Splat> loadSplats(const std::string& path);