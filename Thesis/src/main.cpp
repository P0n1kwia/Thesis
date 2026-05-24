#include <iostream>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <happly.h>

static void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW error " << error << ": " << description << "\n";
}

int main()
{
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit())
        return 1;

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

    
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0));
    (void)model;


    happly::PLYData plyData;
    plyData.addElement("vertex", 0);
    (void)plyData;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    double prevTime  = glfwGetTime();
    float  fps       = 0.0f;
    int    clickCount = 0;
    ImVec4 bgColor   = { 0.12f, 0.12f, 0.12f, 1.0f };

    while (!glfwWindowShouldClose(window))
    {
        double now   = glfwGetTime();
        float  delta = static_cast<float>(now - prevTime);
        prevTime     = now;
        fps          = 1.0f / delta;

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Status");

        ImGui::Text("FPS: %.1f  (%.2f ms)", fps, delta * 1000.0f);
        ImGui::Separator();
        ImGui::Text("GLFW   OK");
        ImGui::Text("GLAD   OK  (OpenGL %s)", glGetString(GL_VERSION));
        ImGui::Text("ImGui  OK  (v%s)", IMGUI_VERSION);
        ImGui::Text("GLM    OK");
        ImGui::Text("happly OK");
        ImGui::Separator();

        if (ImGui::Button("Kliknij mnie"))
            ++clickCount;
        ImGui::SameLine();
        ImGui::Text("Kliknięcia: %d", clickCount);

        ImGui::ColorEdit3("Tło", reinterpret_cast<float*>(&bgColor));

        ImGui::End();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(bgColor.x, bgColor.y, bgColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
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
