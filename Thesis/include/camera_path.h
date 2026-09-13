#pragma once
#include <string>
#include <vector>
#include <camera.h>

struct CameraPathKeyframe
{
    float frame = 0.f;
    CameraConfig config;
};

class CameraPath
{
public:
    static CameraPath loadFromFile(const std::string& path);

    CameraConfig sample(float frame) const;

    bool empty() const { return keyframes.empty(); }
    size_t keyframeCount() const { return keyframes.size(); }

private:
    std::vector<CameraPathKeyframe> keyframes;
};
