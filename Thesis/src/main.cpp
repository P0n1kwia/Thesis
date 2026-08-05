#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <chrono>
#include <cstdio>
#include <vector>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.h>
#include <camera.h>
#include <load_splat.h>
#include <splat_renderer.h>
#include <gpu_timer.h>
#include <screenshot.h>
#include <camera_presets.h>
#include <nfd.hpp>


#ifdef _WIN32
#define NOMINMAX
#include <windows.h>

extern "C" {
    // Forces use of the dedicated NVIDIA GPU
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

    // Forces use of the dedicated AMD GPU (good practice to include both)
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
const unsigned int WINDOW_HEIGHT = 720;
const unsigned int WINDOW_WIDTH = 1280;
constexpr int FPS_HISTORY_SIZE = 120;
struct AppState {
    Camera* camera = nullptr;
    SplatRenderer* renderer = nullptr;
    bool leftDown  = false;
    bool rightDown = false;
    double lastX = 0.0, lastY = 0.0;
    int fbHeight = WINDOW_HEIGHT;

    std::string currentSceneName;
    size_t splatCount = 0;
    std::string lastLoadError;

    bool screenshotRequested = false;
    std::string lastScreenshotPath;

    std::string presetsPath = "camera_presets.json";
    std::vector<CameraPreset> presets;
    int selectedPreset = -1;
    char presetNameBuf[128] = "";

    // 0=RGB 1=Depth 2=Alpha 3=Overdraw 4=Ellipse outline 5=Splat ID
    bool splitScreenEnabled = false;
    int debugModeLeft = 0;
    int debugModeRight = 0;

    RenderParams renderParams;

    float smoothedFps = 0.f;
    std::vector<float> fpsHistory = std::vector<float>(FPS_HISTORY_SIZE, 0.0f);
    int fpsHistoryIdx = 0;
    float sortTimeMs = 0.f;
    float gpuTimeMs = 0.f;

    std::vector<float> extentHistogramData;
};

static bool hasPlyExtension(const std::string& path)
{
    if (path.size() < 4) return false;
    std::string ext = path.substr(path.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".ply";
}

static bool loadSceneFromPath(AppState& state, const std::string& path)
{
    if (!hasPlyExtension(path))
    {
        state.lastLoadError = "Not a .ply file: " + path;
        return false;
    }
    try
    {
        auto splats = loadSplats(path);
        state.renderer->upload(splats);
        state.currentSceneName = std::filesystem::path(path).filename().string();
        state.splatCount = splats.size();
        state.lastLoadError.clear();
        return true;
    }
    catch (const std::exception& e)
    {
        state.lastLoadError = "Failed to load '" + path + "': " + e.what();
        return false;
    }
}

static void errorCB(int err, const char* desc)
{
    std::cerr << "GLFW error " << err << ": " << desc << "\n";
}

static void mouseButtonCB(GLFWwindow* w, int btn, int action, int)
{
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
    if (btn == GLFW_MOUSE_BUTTON_LEFT)  s->leftDown  = (action == GLFW_PRESS);
    if (btn == GLFW_MOUSE_BUTTON_RIGHT) s->rightDown = (action == GLFW_PRESS);
    if (action == GLFW_PRESS) glfwGetCursorPos(w, &s->lastX, &s->lastY);
}

static void cursorCB(GLFWwindow* w, double x, double y)
{
    auto* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
    float dx = float(x - s->lastX), dy = float(y - s->lastY);
    s->lastX = x;
    s->lastY = y;
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (s->leftDown)  s->camera->orbit(glm::radians(dx * 0.3f), glm::radians(-dy * 0.3f), 1.f);
    if (s->rightDown) {
        float scale = 2.0f * s->camera->getRadius() * tanf(s->camera->getFovY() * 0.5f) / s->fbHeight;
        s->camera->pan(-dx * scale, dy * scale);
    }
}

static void scrollCB(GLFWwindow* w, double, double yoff)
{
    if (ImGui::GetIO().WantCaptureMouse) return;
    static_cast<AppState*>(glfwGetWindowUserPointer(w))->camera->zoom(static_cast<float>(yoff));
}

static void fbSizeCB(GLFWwindow* w, int width, int height)
{
    glViewport(0, 0, width, height);
    auto* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
    if (s->camera) s->camera->onViewportResize(width, height);
    s->fbHeight = height;
}

static void dropCB(GLFWwindow* w, int count, const char** paths)
{
    if (count < 1) return;
    auto* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
    bool ok = loadSceneFromPath(*s, paths[0]);
    if (count > 1)
    {
        std::string note = "Dropped " + std::to_string(count) + " files; only the first was loaded.";
        s->lastLoadError = ok ? note : (s->lastLoadError + " | " + note);
    }
}

int main()
{
    NFD::Guard nfdGuard;

    glfwSetErrorCallback(errorCB);
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Thesis", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return 1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << "  |  " << glGetString(GL_RENDERER) << "\n";

    // Camera: start at (0,0,3) facing origin
    CameraConfig cfg;
    cfg.yaw    = glm::radians(90.f);
    cfg.radius = 3.f;
    Camera camera(WINDOW_WIDTH, WINDOW_HEIGHT, cfg);

    // Input — set callbacks before ImGui so ImGui chains to ours
    AppState state{ &camera };
    {
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        state.fbHeight = fbH;
    }
    glfwSetWindowUserPointer(window, &state);
    glfwSetMouseButtonCallback(window, mouseButtonCB);
    glfwSetCursorPosCallback(window, cursorCB);
    glfwSetScrollCallback(window, scrollCB);
    glfwSetFramebufferSizeCallback(window, fbSizeCB);
    glfwSetDropCallback(window, dropCB);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


    double prevTime = glfwGetTime();
    float  fps      = 0.f;

    try
    {
    Shader splatShader("shaders/splat.vert", "shaders/splat.frag");
    Shader computeShader(Shader::ComputeShader{}, "shaders/preprocessing.comp");
    SplatRenderer renderer;
    GpuTimer gpuTimer;
    state.renderer = &renderer;
    state.presets = loadPresets(state.presetsPath);
    if (loadSceneFromPath(state, "resources/bonsai.ply"))
        std::cout << "Loaded " << state.splatCount << " splats\n";
    else
        std::cerr << "Startup load failed: " << state.lastLoadError << "\n";
    splatShader.use();
    splatShader.setVec2("uScreenSize", glm::vec2(WINDOW_WIDTH, WINDOW_HEIGHT));

    while (!glfwWindowShouldClose(window))
    {
        double now   = glfwGetTime();
        float  delta = static_cast<float>(now - prevTime);
        prevTime = now;
        fps      = 1.f / delta;
        state.smoothedFps = glm::mix(state.smoothedFps, fps, 0.1f);
        state.fpsHistory[state.fpsHistoryIdx] = state.smoothedFps;
        state.fpsHistoryIdx = (state.fpsHistoryIdx + 1) % FPS_HISTORY_SIZE;

        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Status");
        ImGui::Text("Left-drag: orbit  |  Right-drag: pan  |  Scroll: zoom");
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("File: %s", state.currentSceneName.empty() ? "(none)" : state.currentSceneName.c_str());
            ImGui::Text("Splats: %zu", state.splatCount);
            ImGui::Text("VRAM (scene data): %.1f MB", renderer.getEstimatedVramBytes() / (1024.0 * 1024.0));
            ImGui::TextWrapped("Drop a .ply file onto the window to load it.");
            if (ImGui::Button("Load..."))
            {
                NFD::UniquePathU8 outPath;
                nfdu8filteritem_t filterItem[1] = { { "PLY files", "ply" } };
                nfdresult_t nfdResult = NFD::OpenDialog(outPath, filterItem, 1);
                if (nfdResult == NFD_OKAY)
                    loadSceneFromPath(state, outPath.get());
                else if (nfdResult == NFD_ERROR)
                    state.lastLoadError = std::string("File dialog error: ") + NFD::GetError();
            }
            if (!state.lastLoadError.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.2f, 1.0f), "%s", state.lastLoadError.c_str());
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Capture", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Screenshot (F2)"))
                state.screenshotRequested = true;
            if (!state.lastScreenshotPath.empty())
                ImGui::TextWrapped("Saved: %s", state.lastScreenshotPath.c_str());
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F2, false))
            state.screenshotRequested = true;
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputText("Name", state.presetNameBuf, IM_ARRAYSIZE(state.presetNameBuf));
            ImGui::SameLine();
            if (ImGui::Button("Save"))
            {
                std::string name(state.presetNameBuf);
                if (!name.empty())
                {
                    auto it = std::find_if(state.presets.begin(), state.presets.end(),
                        [&](const CameraPreset& p) { return p.name == name; });
                    if (it != state.presets.end())
                        it->config = camera.getConfig();
                    else
                        state.presets.push_back({ name, camera.getConfig() });
                    savePresets(state.presetsPath, state.presets);
                }
            }

            ImGui::BeginChild("PresetList", ImVec2(0, 100), true);
            for (int i = 0; i < static_cast<int>(state.presets.size()); ++i)
            {
                bool isSelected = (state.selectedPreset == i);
                if (ImGui::Selectable(state.presets[i].name.c_str(), isSelected))
                    state.selectedPreset = i;
            }
            ImGui::EndChild();

            bool hasSelection = state.selectedPreset >= 0 &&
                state.selectedPreset < static_cast<int>(state.presets.size());
            if (!hasSelection) ImGui::BeginDisabled();
            if (ImGui::Button("Load"))
                camera.applyConfig(state.presets[state.selectedPreset].config);
            ImGui::SameLine();
            if (ImGui::Button("Delete"))
            {
                state.presets.erase(state.presets.begin() + state.selectedPreset);
                savePresets(state.presetsPath, state.presets);
                state.selectedPreset = -1;
            }
            if (!hasSelection) ImGui::EndDisabled();
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Opacity threshold", &state.renderParams.minOpacity, 0.0f, 0.1f, "%.4f");
            ImGui::SliderFloat("Scale multiplier", &state.renderParams.scaleMultiplier, 0.1f, 3.0f);
            ImGui::SliderFloat("Dilation", &state.renderParams.dilation, 0.0f, 2.0f);
            ImGui::SliderFloat("Max splat radius (px)", &state.renderParams.maxRadiusPx, 1.0f, 1024.0f);
            ImGui::SliderInt("SH degree", &state.renderParams.shDegree, 0, 2);
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("0 = DC only, 1 = +linear, 2 = +quadratic (full).\nDegree 3 not yet implemented.");
            }
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static const char* debugModeNames[] = {
                "RGB", "Depth", "Alpha", "Overdraw", "Ellipse outline", "Splat ID"
            };
            ImGui::Checkbox("Split screen", &state.splitScreenEnabled);
            ImGui::Combo(state.splitScreenEnabled ? "Left" : "Debug mode",
                &state.debugModeLeft, debugModeNames, IM_ARRAYSIZE(debugModeNames));
            if (state.splitScreenEnabled)
                ImGui::Combo("Right", &state.debugModeRight, debugModeNames, IM_ARRAYSIZE(debugModeNames));
        }
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char fpsOverlay[32];
            std::snprintf(fpsOverlay, sizeof(fpsOverlay), "%.1f fps", state.smoothedFps);
            ImGui::Text("FPS: %.1f  (%.2f ms)", state.smoothedFps, delta * 1000.f);
            ImGui::PlotLines("##FpsHistory", state.fpsHistory.data(), static_cast<int>(state.fpsHistory.size()),
                state.fpsHistoryIdx, fpsOverlay, 0.0f, FLT_MAX, ImVec2(0, 60));
            ImGui::Text("Sort time: %.3f ms", state.sortTimeMs);
            ImGui::Text("GPU time: %.3f ms", state.gpuTimeMs);
            ImGui::Separator();
            ImGui::Text("Splats loaded: %u", renderer.getSplatCount());
            ImGui::Text("Splats after culling: %u", renderer.getDrawCount());
            ImGui::Text("Splats drawn: %u", renderer.getDrawCount());

            if (ImGui::TreeNode("Histograms"))
            {
                const auto& splats = renderer.getSplats();
                const auto& visibleIndices = renderer.getVisibleIndices();

                std::vector<float> visibleOpacities;
                std::vector<float> visibleScales;
                visibleOpacities.reserve(visibleIndices.size());
                visibleScales.reserve(visibleIndices.size());
                for (uint32_t idx : visibleIndices)
                {
                    const Splat& s = splats[idx];
                    visibleOpacities.push_back(s.opacity);
                    visibleScales.push_back(std::max({ s.scale[0], s.scale[1], s.scale[2] }));
                }

                ImVec2 plotSize(0, 120);
                if (!visibleOpacities.empty() && ImPlot::BeginPlot("Opacity (visible splats)", plotSize))
                {
                    ImPlot::SetupAxes("opacity", "count");
                    ImPlot::PlotHistogram("##opacity", visibleOpacities.data(), static_cast<int>(visibleOpacities.size()));
                    ImPlot::EndPlot();
                }
                if (!visibleScales.empty() && ImPlot::BeginPlot("Scale (visible splats)", plotSize))
                {
                    ImPlot::SetupAxes("max(scale.xyz)", "count");
                    ImPlot::PlotHistogram("##scale", visibleScales.data(), static_cast<int>(visibleScales.size()));
                    ImPlot::EndPlot();
                }

                if (ImGui::Button("Refresh histogram"))
                {
                    auto extents = renderer.fetchVisibleScreenExtents();
                    state.extentHistogramData.clear();
                    state.extentHistogramData.reserve(extents.size());
                    for (const glm::vec2& e : extents)
                        state.extentHistogramData.push_back(std::max(e.x, e.y));
                }
                ImGui::SameLine();
                ImGui::TextDisabled("Screen extent (px, on-demand)");
                if (!state.extentHistogramData.empty() && ImPlot::BeginPlot("Screen extent (last refresh)", plotSize))
                {
                    ImPlot::SetupAxes("extent [px]", "count");
                    ImPlot::PlotHistogram("##extent", state.extentHistogramData.data(), static_cast<int>(state.extentHistogramData.size()));
                    ImPlot::EndPlot();
                }

                ImGui::TreePop();
            }
        }
        ImGui::End();

        if (state.splitScreenEnabled)
        {
            float splitX = static_cast<float>(w / 2);
            ImGui::GetForegroundDrawList()->AddLine(
                ImVec2(splitX, 0.0f), ImVec2(splitX, static_cast<float>(h)),
                IM_COL32(255, 255, 255, 180), 1.5f);
        }

        ImGui::Render();

        glViewport(0, 0, w, h);
        splatShader.use();
        splatShader.setVec2("uScreenSize", glm::vec2(w, h));
        glClearColor(0.12f, 0.12f, 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float gpuMs;
        if (gpuTimer.tryGetResultMs(gpuMs))
            state.gpuTimeMs = gpuMs;

        splatShader.use();
        gpuTimer.begin();
        renderer.preprocess(computeShader, camera, glm::vec2(w, h), state.renderParams);
        if (camera.needsSort()) {
            auto sortStart = std::chrono::steady_clock::now();
            renderer.sort(camera);
            auto sortEnd = std::chrono::steady_clock::now();
            state.sortTimeMs = std::chrono::duration<float, std::milli>(sortEnd - sortStart).count();
            camera.onSortComplete();
        }

        CameraConfig camCfg = camera.getConfig();
        auto drawView = [&](int viewportX, int viewportW, int debugMode)
        {
            glViewport(viewportX, 0, viewportW, h);
            splatShader.use();
            splatShader.setVec2("uScreenSize", glm::vec2(w, h));
            splatShader.setInt("uDebugMode", debugMode);
            splatShader.setFloat("uNear", camCfg.nearPlane);
            splatShader.setFloat("uFar", camCfg.radius * 2.5f);
            splatShader.setFloat("uViewportOffsetX", static_cast<float>(viewportX));
            splatShader.setFloat("uViewportScale", static_cast<float>(viewportW) / static_cast<float>(w));
            if (debugMode == 3)
                glBlendFunc(GL_ONE, GL_ONE);
            else
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            renderer.draw(splatShader, camera, glm::vec2(w, h));
        };

        if (state.splitScreenEnabled)
        {
            int leftW = w / 2;
            int rightW = w - leftW;
            drawView(0, leftW, state.debugModeLeft);
            drawView(leftW, rightW, state.debugModeRight);
        }
        else
        {
            drawView(0, w, state.debugModeLeft);
        }
        gpuTimer.end();
        glViewport(0, 0, w, h);

        if (state.screenshotRequested)
        {
            std::string path = makeScreenshotPath();
            state.lastScreenshotPath = saveScreenshotPNG(path, w, h) ? path : "Screenshot failed";
            state.screenshotRequested = false;
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
