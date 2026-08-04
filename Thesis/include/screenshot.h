#pragma once
#include <string>

std::string makeScreenshotPath();
bool saveScreenshotPNG(const std::string& path, int width, int height);
