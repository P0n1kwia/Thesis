#include <camera_path.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <algorithm>

using json = nlohmann::json;

namespace
{
    glm::vec3 vec3FromJson(const json& j, const glm::vec3& fallback)
    {
        if (!j.is_array() || j.size() != 3) return fallback;
        return glm::vec3(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
    }
}

CameraPath CameraPath::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Camera path: cannot open '" + path + "'");

    json root;
    try
    {
        file >> root;
    }
    catch (const json::exception& e)
    {
        throw std::runtime_error("Camera path: malformed JSON in '" + path + "': " + e.what());
    }

    if (!root.contains("keyframes") || !root.at("keyframes").is_array() || root.at("keyframes").empty())
        throw std::runtime_error("Camera path '" + path + "': 'keyframes' must be a non-empty array");

    CameraConfig defaults;
    CameraPath result;
    float nextImplicitFrame = 0.f;

    for (const auto& entry : root.at("keyframes"))
    {
        CameraPathKeyframe kf;
        kf.config.fovY = entry.value("fovY", defaults.fovY);
        kf.config.nearPlane = entry.value("nearPlane", defaults.nearPlane);
        kf.config.farPlane = entry.value("farPlane", defaults.farPlane);
        kf.config.target = entry.contains("target") ? vec3FromJson(entry.at("target"), defaults.target) : defaults.target;
        kf.config.radius = entry.value("radius", defaults.radius);
        kf.config.yaw = entry.value("yaw", defaults.yaw);
        kf.config.pitch = entry.value("pitch", defaults.pitch);
        kf.frame = entry.value("frame", nextImplicitFrame);
        nextImplicitFrame = kf.frame + 1.f;
        result.keyframes.push_back(kf);
    }

    std::stable_sort(result.keyframes.begin(), result.keyframes.end(),
        [](const CameraPathKeyframe& a, const CameraPathKeyframe& b) { return a.frame < b.frame; });

    return result;
}

CameraConfig CameraPath::sample(float frame) const
{
    if (keyframes.empty())
        throw std::runtime_error("CameraPath::sample() called on an empty path");

    if (keyframes.size() == 1 || frame <= keyframes.front().frame)
        return keyframes.front().config;
    if (frame >= keyframes.back().frame)
        return keyframes.back().config;

    size_t hi = 0;
    while (hi < keyframes.size() && keyframes[hi].frame < frame) ++hi;
    size_t lo = hi - 1;

    const CameraConfig& a = keyframes[lo].config;
    const CameraConfig& b = keyframes[hi].config;
    float span = keyframes[hi].frame - keyframes[lo].frame;
    float t = span > 0.f ? (frame - keyframes[lo].frame) / span : 0.f;

    CameraConfig out;
    out.fovY = glm::mix(a.fovY, b.fovY, t);
    out.nearPlane = glm::mix(a.nearPlane, b.nearPlane, t);
    out.farPlane = glm::mix(a.farPlane, b.farPlane, t);
    out.target = glm::mix(a.target, b.target, t);
    out.radius = glm::mix(a.radius, b.radius, t);
    out.yaw = glm::mix(a.yaw, b.yaw, t);
    out.pitch = glm::mix(a.pitch, b.pitch, t);
    return out;
}
