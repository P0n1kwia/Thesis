#include <camera_presets.h>

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static json configToJson(const CameraConfig& cfg)
{
    return json{
        {"fovY", cfg.fovY},
        {"nearPlane", cfg.nearPlane},
        {"farPlane", cfg.farPlane},
        {"target", {cfg.target.x, cfg.target.y, cfg.target.z}},
        {"radius", cfg.radius},
        {"yaw", cfg.yaw},
        {"pitch", cfg.pitch},
    };
}

static CameraConfig configFromJson(const json& j)
{
    CameraConfig cfg;
    cfg.fovY = j.at("fovY").get<float>();
    cfg.nearPlane = j.at("nearPlane").get<float>();
    cfg.farPlane = j.at("farPlane").get<float>();
    auto t = j.at("target");
    cfg.target = glm::vec3(t.at(0).get<float>(), t.at(1).get<float>(), t.at(2).get<float>());
    cfg.radius = j.at("radius").get<float>();
    cfg.yaw = j.at("yaw").get<float>();
    cfg.pitch = j.at("pitch").get<float>();
    return cfg;
}

std::vector<CameraPreset> loadPresets(const std::string& path)
{
    std::vector<CameraPreset> presets;
    std::ifstream file(path);
    if (!file.is_open())
        return presets;

    try
    {
        json root;
        file >> root;
        for (const auto& entry : root.at("presets"))
        {
            CameraPreset preset;
            preset.name = entry.at("name").get<std::string>();
            preset.config = configFromJson(entry.at("config"));
            presets.push_back(std::move(preset));
        }
    }
    catch (const json::exception&)
    {
        // Malformed/incompatible file — treat as no saved presets rather than crashing.
        return {};
    }
    return presets;
}

void savePresets(const std::string& path, const std::vector<CameraPreset>& presets)
{
    json root;
    json& arr = root["presets"] = json::array();
    for (const auto& preset : presets)
    {
        arr.push_back(json{
            {"name", preset.name},
            {"config", configToJson(preset.config)},
        });
    }

    std::ofstream file(path);
    file << root.dump(4);
}
