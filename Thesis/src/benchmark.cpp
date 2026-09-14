#include <benchmark.h>

#include <camera_path.h>
#include <load_splat.h>
#include <camera.h>
#include <shader.h>
#include <gpu_timer.h>
#include <screenshot.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>

namespace
{
    bool nextArgValue(int argc, char** argv, int& i, std::string& out)
    {
        if (i + 1 >= argc) return false;
        out = argv[++i];
        return true;
    }

    // Nearest-rank percentile: p=0.5 -> median, p=0.99 -> "1% low" tail value.
    float percentile(std::vector<float> values, float p)
    {
        if (values.empty()) return 0.f;
        std::sort(values.begin(), values.end());
        size_t rank = static_cast<size_t>(std::ceil(p * static_cast<float>(values.size())));
        rank = std::clamp<size_t>(rank, 1, values.size());
        return values[rank - 1];
    }

    float mean(const std::vector<float>& values)
    {
        if (values.empty()) return 0.f;
        return std::accumulate(values.begin(), values.end(), 0.f) / static_cast<float>(values.size());
    }
}

bool parseBenchmarkArgs(int argc, char** argv, BenchmarkArgs& args)
{
    bool found = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::string val;
        if (arg == "--bench")
        {
            if (!nextArgValue(argc, argv, i, val))
            {
                std::cerr << "--bench requires a scene path\n";
                continue;
            }
            args.scenePath = val;
            found = true;
        }
        else if (arg == "--camera-path" && nextArgValue(argc, argv, i, val)) args.cameraPathPath = val;
        else if (arg == "--frames" && nextArgValue(argc, argv, i, val)) args.frames = std::max(1, std::atoi(val.c_str()));
        else if (arg == "--out" && nextArgValue(argc, argv, i, val)) args.outCsvPath = val;
        else if (arg == "--width" && nextArgValue(argc, argv, i, val)) args.width = std::max(1, std::atoi(val.c_str()));
        else if (arg == "--height" && nextArgValue(argc, argv, i, val)) args.height = std::max(1, std::atoi(val.c_str()));
        else if (arg == "--opacity" && nextArgValue(argc, argv, i, val)) args.minOpacity = std::clamp(std::strtof(val.c_str(), nullptr), 0.0f, 1.0f);
        else if (arg == "--scale" && nextArgValue(argc, argv, i, val)) args.scaleMultiplier = std::max(0.0f, std::strtof(val.c_str(), nullptr));
        else if (arg == "--max-radius" && nextArgValue(argc, argv, i, val)) args.maxRadiusPx = std::max(1.0f, std::strtof(val.c_str(), nullptr));
        else if (arg == "--sh-degree" && nextArgValue(argc, argv, i, val)) args.shDegree = std::clamp(std::atoi(val.c_str()), 0, 3);
        else if (arg == "--max-splats" && nextArgValue(argc, argv, i, val)) args.maxSplats = static_cast<size_t>(std::max(0, std::atoi(val.c_str())));
        else if (arg == "--validate-sort") args.validateSort = true;
        else if (arg == "--screenshot-frame" && nextArgValue(argc, argv, i, val)) args.screenshotFrame = std::atoi(val.c_str());
        else if (arg == "--screenshot-out" && nextArgValue(argc, argv, i, val)) args.screenshotOut = val;
        else if (arg == "--sort" && nextArgValue(argc, argv, i, val))
            args.sortMethod = (val == "cpu" || val == "CPU") ? SortMethod::CPU : SortMethod::GPU;
    }
    args.enabled = found;
    return found;
}

int runBenchmark(GLFWwindow* window, SplatRenderer& renderer, Camera& camera,
    const BenchmarkShaders& shaders, const BenchmarkArgs& args)
{
    if (args.cameraPathPath.empty())
    {
        std::cerr << "--bench requires --camera-path <file.json>\n";
        return 1;
    }

    CameraPath path;
    try
    {
        path = CameraPath::loadFromFile(args.cameraPathPath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load camera path: " << e.what() << "\n";
        return 1;
    }

    float msLoad = 0.f;
    try
    {
        auto tLoadStart = std::chrono::steady_clock::now();
        auto splats = loadSplats(args.scenePath);
        if (args.maxSplats > 0 && splats.size() > args.maxSplats)
        {
            std::vector<Splat> sampled;
            sampled.reserve(args.maxSplats);
            const double step = static_cast<double>(splats.size()) / static_cast<double>(args.maxSplats);
            for (size_t i = 0; i < args.maxSplats; ++i)
                sampled.push_back(splats[static_cast<size_t>(static_cast<double>(i) * step)]);
            splats = std::move(sampled);
        }
        renderer.upload(splats);
        glFinish();
        auto tLoadEnd = std::chrono::steady_clock::now();
        msLoad = std::chrono::duration<float, std::milli>(tLoadEnd - tLoadStart).count();
        std::cout << "Benchmark: loaded " << splats.size() << " splats from " << args.scenePath
            << " in " << msLoad << " ms\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load scene '" << args.scenePath << "': " << e.what() << "\n";
        return 1;
    }

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glm::vec2 screenSize(static_cast<float>(fbW), static_cast<float>(fbH));
    glViewport(0, 0, fbW, fbH);

    std::ofstream csv(args.outCsvPath);
    if (!csv.is_open())
    {
        std::cerr << "Failed to open output CSV '" << args.outCsvPath << "'\n";
        return 1;
    }
    csv << "frame,ms_total,ms_sort,ms_gpu,visible_splats,drawn_fragments\n";

    std::vector<float> msTotalAll, msSortAll, msGpuAll;
    msTotalAll.reserve(args.frames);
    msSortAll.reserve(args.frames);
    msGpuAll.reserve(args.frames);

    RenderParams renderParams;
    renderParams.minOpacity = args.minOpacity;
    renderParams.scaleMultiplier = args.scaleMultiplier;
    renderParams.maxRadiusPx = args.maxRadiusPx;
    renderParams.shDegree = args.shDegree;
    GpuTimer frameTimer;
    GpuTimer sortGpuTimer;

    std::cout << "Benchmark: " << args.frames << " frames, sort="
        << (args.sortMethod == SortMethod::GPU ? "GPU" : "CPU")
        << ", " << fbW << "x" << fbH
        << ", opacity=" << renderParams.minOpacity
        << ", scale=" << renderParams.scaleMultiplier
        << ", max-radius=" << renderParams.maxRadiusPx
        << ", sh-degree=" << renderParams.shDegree
        << ", splats=" << renderer.getSplatCount()
        << ", out=" << args.outCsvPath << "\n";

    if (args.validateSort)
    {
        camera.applyConfig(path.sample(0.0f));
        renderer.preprocess(shaders.computeShader, camera, screenSize, renderParams);
        bool gatherOk = renderer.debugValidateGatherStage(shaders.gatherShader);
        bool scanOk = renderer.debugValidateHistogramScanStage(shaders.gatherShader,
            shaders.histogramShader, shaders.scanWorkgroupsShader, shaders.scanBinsShader);
        bool scatterOk = renderer.debugValidateScatterStage(shaders.gatherShader,
            shaders.histogramShader, shaders.scanWorkgroupsShader, shaders.scanBinsShader,
            shaders.scatterShader);
        bool compareOk = renderer.debugCompareSortMethods(camera, shaders.gatherShader,
            shaders.histogramShader, shaders.scanWorkgroupsShader, shaders.scanBinsShader,
            shaders.scatterShader);
        renderer.sort(camera, SortMethod::GPU, shaders.gatherShader, shaders.histogramShader,
            shaders.scanWorkgroupsShader, shaders.scanBinsShader, shaders.scatterShader);
        bool monotonicOk = renderer.debugCheckSortMonotonic();
        bool validationOk = gatherOk && scanOk && scatterOk && compareOk && monotonicOk;
        std::cout << "Sort validation: gather=" << gatherOk << ", scan=" << scanOk
            << ", scatter=" << scatterOk << ", cpu-gpu=" << compareOk
            << ", monotonic=" << monotonicOk << "\n";
        if (!validationOk)
            return 2;
    }

    for (int frame = 0; frame < args.frames; ++frame)
    {
        camera.applyConfig(path.sample(static_cast<float>(frame)));

        glFinish();
        auto tFrameStart = std::chrono::steady_clock::now();

        frameTimer.begin();

        renderer.preprocess(shaders.computeShader, camera, screenSize, renderParams);

        float msSort = 0.f;
        if (args.sortMethod == SortMethod::GPU)
        {
            sortGpuTimer.begin();
            renderer.sort(camera, args.sortMethod, shaders.gatherShader, shaders.histogramShader,
                shaders.scanWorkgroupsShader, shaders.scanBinsShader, shaders.scatterShader);
            msSort = sortGpuTimer.endAndWaitMs();
        }
        else
        {
            auto tSortStart = std::chrono::steady_clock::now();
            renderer.sort(camera, args.sortMethod, shaders.gatherShader, shaders.histogramShader,
                shaders.scanWorkgroupsShader, shaders.scanBinsShader, shaders.scatterShader);
            glFinish();
            auto tSortEnd = std::chrono::steady_clock::now();
            msSort = std::chrono::duration<float, std::milli>(tSortEnd - tSortStart).count();
        }
        camera.onSortComplete();

        renderer.resetFragmentCounter();

        shaders.splatShader.use();
        shaders.splatShader.setVec2("uScreenSize", screenSize);
        shaders.splatShader.setInt("uDebugMode", 0);
        shaders.splatShader.setInt("uCountFragments", 1);
        CameraConfig camCfg = camera.getConfig();
        shaders.splatShader.setFloat("uNear", camCfg.nearPlane);
        shaders.splatShader.setFloat("uFar", camCfg.radius * 2.5f);
        shaders.splatShader.setFloat("uViewportOffsetX", 0.0f);
        shaders.splatShader.setFloat("uViewportScale", 1.0f);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glClearColor(0.12f, 0.12f, 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.draw(shaders.splatShader, camera, screenSize);

        float msGpu = frameTimer.endAndWaitMs();
        auto tFrameEnd = std::chrono::steady_clock::now();

        float msTotal = std::chrono::duration<float, std::milli>(tFrameEnd - tFrameStart).count();
        uint32_t visibleSplats = renderer.getDrawCount();
        uint32_t drawnFragments = renderer.readFragmentCounter();

        csv << frame << ',' << msTotal << ',' << msSort << ',' << msGpu << ','
            << visibleSplats << ',' << drawnFragments << '\n';

        msTotalAll.push_back(msTotal);
        msSortAll.push_back(msSort);
        msGpuAll.push_back(msGpu);

        if (frame == args.screenshotFrame)
        {
            std::string path2 = args.screenshotOut.empty() ? makeScreenshotPath() : args.screenshotOut;
            bool ok = saveScreenshotPNG(path2, fbW, fbH);
            std::cout << (ok ? "Saved screenshot: " : "Screenshot failed: ") << path2 << "\n";
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    auto summarize = [](const char* label, const std::vector<float>& v)
    {
        std::cout << label << ": mean=" << mean(v) << " ms  median=" << percentile(v, 0.5f)
            << " ms  p99=" << percentile(v, 0.99f) << " ms\n";
    };

    std::cout << "\n--- Benchmark summary (" << args.frames << " frames) ---\n";
    summarize("ms_total", msTotalAll);
    summarize("ms_sort ", msSortAll);
    summarize("ms_gpu  ", msGpuAll);

    csv << "#summary,metric,mean,median,p99\n";
    csv << "#summary,ms_total," << mean(msTotalAll) << ',' << percentile(msTotalAll, 0.5f) << ',' << percentile(msTotalAll, 0.99f) << '\n';
    csv << "#summary,ms_sort," << mean(msSortAll) << ',' << percentile(msSortAll, 0.5f) << ',' << percentile(msSortAll, 0.99f) << '\n';
    csv << "#summary,ms_gpu," << mean(msGpuAll) << ',' << percentile(msGpuAll, 0.5f) << ',' << percentile(msGpuAll, 0.99f) << '\n';
    csv << "#summary,ms_load," << msLoad << ',' << msLoad << ',' << msLoad << '\n';
    csv.close();

    std::cout << "Wrote " << args.outCsvPath << "\n";
    return 0;
}
