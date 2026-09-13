#pragma once
#include <string>

struct ImageCompareResult
{
    double psnrDb;
    double ssim;
    int width;
    int height;
};

ImageCompareResult compareImages(const std::string& pathA, const std::string& pathB);
