#include <string>
#include <print>
#include <vector>
#include <fstream>
#include "window.h"
#include "resources.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// GOAL: render "rain"
// TODO:
//  - [ ] Maintain ratio
//  - [ ] Mouse movement
//  - [ ] "Rain"

void captureScreen(int width, int height, const std::string& filename = "screenshot.png") {
    std::vector<unsigned char> pixels(width * height * 3); // 3 bytes per pixel (RGB)
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<unsigned char> flipped(pixels.size());
    for (int y = 0; y < height; ++y) {
        std::copy(
            pixels.begin() + y * width * 3,
            pixels.begin() + (y + 1) * width * 3,
            flipped.begin() + (height - 1 - y) * width * 3
        );
    }

    stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), width * 3);
    std::println("Captured screen to {}", filename);
}

Config parseArgs(int argc, char** argv);
void draw(const Resources& resources);

int main(int argc, char** argv) {
    Config config = parseArgs(argc, argv);
    std::println("Picture: {}", config.picture);

    GLFWwindow* window = windowInit();
    if (window == NULL) return -1;

    auto resource_init_result = resourcesInit(config);
    if (!resource_init_result) return -1;

    Resources resources = resource_init_result.value();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Variables for capture success message
    bool showCaptureSuccess = false;
    float captureSuccessTimer = 0.0f;
    const float successMessageDuration = 2.0f; // seconds

    bool requestCapture = false; // capture flag

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create UI
        ImGui::Begin("Capture Screen");
        if (ImGui::Button("Capture Screen")) {
            requestCapture = true; // set capture flag
        }
        ImGui::End();

        // Render OpenGL content (your game, no UI yet)
        draw(resources);

        // Perform screen capture BEFORE rendering UI
        if (requestCapture) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            captureScreen(width, height);

            showCaptureSuccess = true;
            captureSuccessTimer = 0.0f;
            requestCapture = false;
        }

        // Render UI now
        if (showCaptureSuccess) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::Begin("Success", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav);

            ImGui::Text("Capture Success!");
            ImGui::End();

            captureSuccessTimer += ImGui::GetIO().DeltaTime;
            if (captureSuccessTimer >= successMessageDuration) {
                showCaptureSuccess = false;
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    resourcesDeinit(&resources);
    windowDeinit(&window);
}

void draw(const Resources& resources) {
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(resources.shaders.main);
    glBindVertexArray(resources.buffer.vert_arr);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

Config parseArgs(int argc, char** argv) {
    return Config{
        .picture = argc >= 2 ? argv[1] : "assets/default.png",
    };
}
