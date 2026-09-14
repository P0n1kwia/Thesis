#pragma once
#include <string>
#include <splat_renderer.h>

class Camera;
class Shader;
struct GLFWwindow;

struct BenchmarkArgs
{
    bool enabled = false;
    std::string scenePath;
    std::string cameraPathPath;
    int frames = 600;
    std::string outCsvPath = "benchmark_results.csv";
    SortMethod sortMethod = SortMethod::GPU;
    int width = 1280;
    int height = 720;
    float minOpacity = 0.0039f;
    float scaleMultiplier = 1.0f;
    float maxRadiusPx = 1024.0f;
    int shDegree = 3;
    size_t maxSplats = 0;
    bool validateSort = false;

    int screenshotFrame = -1;
    std::string screenshotOut;
};


struct BenchmarkShaders
{
    Shader& splatShader;
    Shader& computeShader;
    Shader& gatherShader;
    Shader& histogramShader;
    Shader& scanWorkgroupsShader;
    Shader& scanBinsShader;
    Shader& scatterShader;
};

bool parseBenchmarkArgs(int argc, char** argv, BenchmarkArgs& args);

int runBenchmark(GLFWwindow* window, SplatRenderer& renderer, Camera& camera,
    const BenchmarkShaders& shaders, const BenchmarkArgs& args);
