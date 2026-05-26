#include <vector>
#include <happly.h>

struct RawSplat
{
    float position[3];
    float sh[3];
    float opacity;
    float scale[3];
    float quaternion[4];
};
std::vector<RawSplat> loadRawSplats(const std::string& path);