#include <iostream>
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

struct AppState {
    Camera* camera = nullptr;
    bool leftDown  = false;
    bool rightDown = false;
    double lastX = 0.0, lastY = 0.0;
};

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
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
    float dx = static_cast<float>(x - s->lastX);
    float dy = static_cast<float>(y - s->lastY);
    s->lastX = x;
    s->lastY = y;
    if (s->leftDown)  s->camera->orbit(glm::radians(dx * 0.3f), glm::radians(-dy * 0.3f), 1.f);
    if (s->rightDown) s->camera->pan(-dx * 0.005f, dy * 0.005f);
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
    Camera camera(1280, 720, cfg);

    // Input — set callbacks before ImGui so ImGui chains to ours
    AppState state{ &camera };
    glfwSetWindowUserPointer(window, &state);
    glfwSetMouseButtonCallback(window, mouseButtonCB);
    glfwSetCursorPosCallback(window, cursorCB);
    glfwSetScrollCallback(window, scrollCB);
    glfwSetFramebufferSizeCallback(window, fbSizeCB);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    double prevTime = glfwGetTime();
    float  fps      = 0.f;

    auto splats = loadSplats("resources/point_cloud.ply");
    std::cout << "Loaded " << splats.size() << " splats\n";

    Shader splatShader("shaders/splat_vert.glsl", "shaders/splat_frag.glsl");

    SplatRenderer renderer;
    renderer.upload(splats);

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
        ImGui::End();

        ImGui::Render();

        int w, h; 
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        splatShader.use();
        renderer.draw(splatShader, camera);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
