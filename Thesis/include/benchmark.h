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
