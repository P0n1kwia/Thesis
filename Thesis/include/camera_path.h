#pragma once
#include <string>
#include <vector>
#include <camera.h>

// One pose on a deterministic camera trajectory used by the headless benchmark
// (see benchmark.h). "frame" is the timeline position at which `config` is
// reached exactly; frames in between are linearly interpolated.
struct CameraPathKeyframe
{
    float frame = 0.f;
    CameraConfig config;
};

// A camera trajectory loaded from JSON, sampled once per benchmark frame.
// Deterministic and repeatable across renderer configurations/runs, which is
// the whole point: FPS numbers gathered while "moving the mouse" aren't
// comparable between two variants of the renderer.
class CameraPath
{
public:
    static CameraPath loadFromFile(const std::string& path);

    // Returns the interpolated config at the given timeline position. Frames
    // before the first / after the last keyframe hold at the nearest end.
    CameraConfig sample(float frame) const;

    bool empty() const { return keyframes.empty(); }
    size_t keyframeCount() const { return keyframes.size(); }

private:
    std::vector<CameraPathKeyframe> keyframes;
};
