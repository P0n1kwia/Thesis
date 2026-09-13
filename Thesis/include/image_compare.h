#pragma once
#include <string>

struct ImageCompareResult
{
    double psnrDb;
    double ssim;
    int width;
    int height;
};

// Loads two images (any format stb_image supports) of matching dimensions and
// computes PSNR [dB] over RGB and a block-based SSIM approximation over luma.
// Used for rozdz. 5.3 (poprawność wizualna): compare a screenshot (F2, or
// --bench --screenshot-frame) against a reference render at the same camera
// pose. Throws std::runtime_error on load failure or a dimension mismatch.
ImageCompareResult compareImages(const std::string& pathA, const std::string& pathB);
