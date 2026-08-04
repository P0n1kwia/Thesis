#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.h>
#include <camera.h>
#include <load_splat.h>
#include <splat_renderer.h>


#ifdef _WIN32
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
    state.renderer = &renderer;
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

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Status");
        ImGui::Text("FPS: %.1f  (%.2f ms)", fps, delta * 1000.f);
        ImGui::Separator();
        ImGui::Text("Left-drag: orbit  |  Right-drag: pan  |  Scroll: zoom");
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("File: %s", state.currentSceneName.empty() ? "(none)" : state.currentSceneName.c_str());
            ImGui::Text("Splats: %zu", state.splatCount);
            ImGui::TextWrapped("Drop a .ply file onto the window to load it.");
            if (!state.lastLoadError.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.2f, 1.0f), "%s", state.lastLoadError.c_str());
        }
        ImGui::End();

        ImGui::Render();

        int w, h; 
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        splatShader.use();
        splatShader.setVec2("uScreenSize", glm::vec2(w, h));
        glClearColor(0.12f, 0.12f, 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        splatShader.use();
        renderer.preprocess(computeShader, camera, glm::vec2(w, h));
        if (camera.needsSort()) {
            renderer.sort(camera);
            camera.onSortComplete();
        }
        splatShader.use();
        splatShader.setVec2("uScreenSize", glm::vec2(w, h));
        renderer.draw(splatShader, camera,glm::vec2(w,h));

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
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
