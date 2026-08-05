#pragma once
#include <string>
#include <vector>
#include <camera.h>

struct CameraPreset {
    std::string name;
    CameraConfig config;
};

std::vector<CameraPreset> loadPresets(const std::string& path);
void savePresets(const std::string& path, const std::vector<CameraPreset>& presets);
