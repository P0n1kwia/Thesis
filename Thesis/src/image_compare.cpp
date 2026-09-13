#include <image_compare.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
    std::vector<float> toLuma(const unsigned char* pixels, int width, int height, int channels)
    {
        std::vector<float> luma(static_cast<size_t>(width) * height);
        for (int i = 0; i < width * height; ++i)
        {
            const unsigned char* px = pixels + static_cast<size_t>(i) * channels;
            float r = static_cast<float>(px[0]);
            float g = static_cast<float>(channels > 1 ? px[1] : px[0]);
            float b = static_cast<float>(channels > 2 ? px[2] : px[0]);
            luma[i] = 0.299f * r + 0.587f * g + 0.114f * b;
        }
        return luma;
    }

    double computeSSIM(const std::vector<float>& a, const std::vector<float>& b, int width, int height, int block = 8)
    {
        constexpr double C1 = (0.01 * 255.0) * (0.01 * 255.0);
        constexpr double C2 = (0.03 * 255.0) * (0.03 * 255.0);


        block = std::max(1, std::min({ block, width, height }));

        double ssimSum = 0.0;
        int blockCount = 0;

        for (int by = 0; by + block <= height; by += block)
        {
            for (int bx = 0; bx + block <= width; bx += block)
            {
                double meanA = 0.0, meanB = 0.0;
                const int n = block * block;
                for (int y = 0; y < block; ++y)
                    for (int x = 0; x < block; ++x)
                    {
                        size_t idx = static_cast<size_t>(by + y) * width + (bx + x);
                        meanA += a[idx];
                        meanB += b[idx];
                    }
                meanA /= n;
                meanB /= n;

                double varA = 0.0, varB = 0.0, covAB = 0.0;
                for (int y = 0; y < block; ++y)
                    for (int x = 0; x < block; ++x)
                    {
                        size_t idx = static_cast<size_t>(by + y) * width + (bx + x);
                        double da = a[idx] - meanA;
                        double db = b[idx] - meanB;
                        varA += da * da;
                        varB += db * db;
                        covAB += da * db;
                    }
                const int normN = std::max(1, n - 1); // n==1 (1x1 block) has no sample variance
                varA /= normN;
                varB /= normN;
                covAB /= normN;

                double numerator = (2.0 * meanA * meanB + C1) * (2.0 * covAB + C2);
                double denominator = (meanA * meanA + meanB * meanB + C1) * (varA + varB + C2);
                ssimSum += numerator / denominator;
                ++blockCount;
            }
        }
        return blockCount > 0 ? ssimSum / blockCount : 1.0;
    }
}

ImageCompareResult compareImages(const std::string& pathA, const std::string& pathB)
{
    int wA, hA, cA, wB, hB, cB;
    unsigned char* pixA = stbi_load(pathA.c_str(), &wA, &hA, &cA, 0);
    if (!pixA)
        throw std::runtime_error("Failed to load image '" + pathA + "'");

    unsigned char* pixB = stbi_load(pathB.c_str(), &wB, &hB, &cB, 0);
    if (!pixB)
    {
        stbi_image_free(pixA);
        throw std::runtime_error("Failed to load image '" + pathB + "'");
    }

    if (wA != wB || hA != hB)
    {
        stbi_image_free(pixA);
        stbi_image_free(pixB);
        throw std::runtime_error("Image dimensions differ: " + std::to_string(wA) + "x" + std::to_string(hA) +
            " vs " + std::to_string(wB) + "x" + std::to_string(hB));
    }

    const int compareChannels = std::min({ cA, cB, 3 });
    const long long pixelCount = static_cast<long long>(wA) * hA;

    double sqErrorSum = 0.0;
    for (long long i = 0; i < pixelCount; ++i)
    {
        for (int c = 0; c < compareChannels; ++c)
        {
            double diff = static_cast<double>(pixA[i * cA + c]) - static_cast<double>(pixB[i * cB + c]);
            sqErrorSum += diff * diff;
        }
    }
    double mse = sqErrorSum / (static_cast<double>(pixelCount) * compareChannels);
    double psnr = mse <= 1e-12 ? 100.0 : 10.0 * std::log10((255.0 * 255.0) / mse);

    std::vector<float> lumaA = toLuma(pixA, wA, hA, cA);
    std::vector<float> lumaB = toLuma(pixB, wB, hB, cB);
    double ssim = computeSSIM(lumaA, lumaB, wA, hA);

    stbi_image_free(pixA);
    stbi_image_free(pixB);

    return { psnr, ssim, wA, hA };
}
