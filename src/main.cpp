#include <vector>
#include <fstream>
#include <chrono>
#include <optional>
#include <print>
#include <random>
#include <string>
#include <cstring>

#include "window.h"
#include "resources.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Window {
    double wheel_xoffset;
    double wheel_yoffset;
};

void captureScreen(int width, int height, const std::string& filename = "screenshot.png") {
    const int channels = 4;
    std::vector<unsigned char> pixels(width * height * channels);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<unsigned char> flipped(pixels.size());
    for (int y = 0; y < height; ++y) {
        std::copy(
            pixels.begin() + y * width * channels,
            pixels.begin() + (y + 1) * width * channels,
            flipped.begin() + (height - 1 - y) * width * channels
        );
    }

    stbi_write_png(filename.c_str(), width, height, channels, flipped.data(), width * channels);
    std::println("Captured screen to {}", filename);
}

void processInput(
    Resources* resources,
    int scr_width,
    int scr_height,
    bool *p_held,
    bool hold,
    double* p_old_posx,
    double* p_old_posy,
    double mouse_posx,
    double mouse_posy,
    glm::vec2* p_old_cam_pos
);
Config parseArgs(int argc, char** argv);
void update(Resources* resources, const float dt_s, std::uniform_real_distribution<>& dis, std::mt19937& gen, const Config config);
void draw(const Resources& resources, const int scr_width, const int scr_height, const uint32_t rain_count);

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Window *win_user = (Window*)glfwGetWindowUserPointer(window);
    win_user->wheel_xoffset = xoffset;
    win_user->wheel_yoffset = yoffset;
}

int main(int argc, char** argv) {
    Config config = parseArgs(argc, argv);
    std::println("Picture: {}", config.picture);
    std::println("Rain count: {}", config.rain_count);
    std::println("Speed: {}", config.speed);
    std::println("Color: {} {} {} {}->{}", config.color[0], config.color[1], config.color[2], config.color[3], config.color[4]);

    const uint32_t rain_count = config.rain_count;

    GLFWwindow* window = windowInit();
    if (window == NULL) return -1;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);

    auto resource_init_result = new std::optional<Resources>(resourcesInit(config, dis, gen));
    if (!resource_init_result) return -1;

    Resources& resources = (*resource_init_result).value();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    Window win_user = {};
    glfwSetWindowUserPointer(window, &win_user);

    glm::vec2 old_cam_pos = {0.0, 0.0};

    double xpos = 0;
    double ypos = 0;
    glfwGetCursorPos(window, &xpos, &ypos);
    double old_xpos = xpos;
    double old_ypos = ypos;

    bool held = false;

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
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    auto end_time = start_time;
    while (!glfwWindowShouldClose(window)) {
        float dt_s = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()/float(1000);
        start_time = std::chrono::steady_clock::now();

        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create UI elements (button to capture screen)
        ImGui::Begin("Capture Screen");
        if (ImGui::Button("Capture Screen")) {
            requestCapture = true; // set capture flag
        }
        ImGui::End();

        glfwGetCursorPos(window, &xpos, &ypos);
        bool hold = GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

        int scr_width, scr_height;
        glfwGetFramebufferSize(window, &scr_width, &scr_height);

        processInput(&resources, scr_width, scr_height, &held, hold, &old_xpos, &old_ypos, xpos, ypos, &old_cam_pos);
        update(&resources, dt_s, dis, gen, config);
        draw(resources, scr_width, scr_height, rain_count);

        // Perform screen capture BEFORE rendering UI
        if (requestCapture) {
            glBindFramebuffer(GL_FRAMEBUFFER, resources.droplet_render_target.framebuffer);
            captureScreen(resources.droplet_render_target.width, resources.droplet_render_target.height);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        end_time = std::chrono::steady_clock::now();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    resourcesDeinit(&resources);
    windowDeinit(&window);
}

void processInput(
    Resources* resources,
    int scr_width,
    int scr_height,
    bool *p_held,
    bool hold,
    double* p_old_posx,
    double* p_old_posy,
    double mouse_posx,
    double mouse_posy,
    glm::vec2* p_old_cam_pos
) {
    glm::vec2 delta = {mouse_posx - *p_old_posx, mouse_posy - *p_old_posy};
    delta = delta * glm::vec2(2.0, 2.0)/glm::vec2(scr_width, scr_height);
    glm::vec2 cam_pos = *p_old_cam_pos + delta;

    bool held = *p_held;
    if (!held && hold) {
        *p_old_posx = mouse_posx;
        *p_old_posy = mouse_posy;
    } else {
        if (held) {
            if (hold) {
                glUseProgram(resources->shaders.screen);
                glUniform2f(1, cam_pos.x, -cam_pos.y);
            } else {
                *p_old_cam_pos = cam_pos;
            }
        }
    }

    *p_held = hold;
}

void update(Resources* resources, const float dt_s, std::uniform_real_distribution<>& dis, std::mt19937& gen, const Config config) {
    const float speed = config.speed;
    const uint32_t rain_count = config.rain_count;
    for (size_t i = 0; i < 4*rain_count; i += 4) {
        RainQuad quad = {
            .v = {
                resources->rain_vertices[i],
                resources->rain_vertices[i+1],
                resources->rain_vertices[i+2],
                resources->rain_vertices[i+3],
            },
        };

        if (quad.v[0].pos.y <= -1.0) {
            quad.setPosY(0, 1.0 + (dis(gen)+1.0) + quad.height());
            quad.randomPosX(0, dis, gen);
        } else {
            quad.translatePosY(-speed*dt_s);
        }

        resources->rain_vertices[i]   = quad.v[0];
        resources->rain_vertices[i+1] = quad.v[1];
        resources->rain_vertices[i+2] = quad.v[2];
        resources->rain_vertices[i+3] = quad.v[3];
    }
    glBindBuffer(GL_ARRAY_BUFFER, resources->buffers.rain_vert_buf);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 4*rain_count*sizeof(RainVertex), resources->rain_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw(const Resources& resources, const int scr_width, const int scr_height, const uint32_t rain_count) {
    const Shaders shaders = resources.shaders;
    const Buffers buffers = resources.buffers;
    const GLuint image_texture = resources.texture;
    const RenderTarget render_target = resources.render_target;
    const RenderTarget droplet_render_target = resources.droplet_render_target;

    // to render target
    {
        glBindFramebuffer(GL_FRAMEBUFFER, render_target.framebuffer);
        glViewport(0, 0, render_target.width, render_target.height);

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        // render image
        glUseProgram(shaders.texture);
        glBindVertexArray(buffers.vert_arr);
        glBindTexture(GL_TEXTURE_2D, image_texture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(GLuint)));

        // render rain
        glUseProgram(resources.shaders.rain);
        glBindVertexArray(resources.buffers.rain_vert_arr);
        glDrawElements(GL_TRIANGLES, 6*rain_count, GL_UNSIGNED_INT, 0);
    }

    // to another render target
    {
        glBindFramebuffer(GL_FRAMEBUFFER, droplet_render_target.framebuffer);
        glViewport(0, 0, droplet_render_target.width, droplet_render_target.height);

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaders.texture);
        glBindVertexArray(buffers.vert_arr);
        glBindTexture(GL_TEXTURE_2D, render_target.texture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(GLuint)));

        // render droplets
        glUseProgram(resources.shaders.droplet);
        glUniform1f(glGetUniformLocation(resources.shaders.droplet, "u_time"), glfwGetTime());
        glUniform1i(glGetUniformLocation(resources.shaders.droplet, "u_texture"), 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(GLuint)));
    }

    // to window
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, scr_width, scr_height);

        glClearColor(0.1, 0.1, 0.1, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaders.screen);
        glUniform2f(0, float(scr_height)/(scr_width), 1.0);
        glBindVertexArray(buffers.vert_arr);
        glBindTexture(GL_TEXTURE_2D, droplet_render_target.texture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLuint)));
    }
}

Config parseArgs(int argc, char** argv) {
    uint32_t rain_count = 256;
    float speed = 1.0;
    std::optional<std::string> picture = std::nullopt;
    float color[5] = {0.0, 0.0, 1.0, 0.0, 1.0};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            i += 1;
            rain_count = atoi(argv[i]);
            if (rain_count > RAIN_PARTICLES_COUNT) {
                std::println("Rain count cannot exceed {}!", RAIN_PARTICLES_COUNT);
                rain_count = RAIN_PARTICLES_COUNT;
            }
        } else if (strcmp(argv[i], "-s") == 0) {
            i += 1;
            speed = atof(argv[i]);
            if (speed <= 0.0) {
                std::println("Speed cannot be <= 0.0!");
                speed = 1.0;
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            i += 1;
            char *s = argv[i];
            int num = 0;
            for (int j = 0; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == '/') {
                    argv[i][j] = '\0';
                    color[num] = atof(s);
                    num += 1;
                    s = argv[i] + j + 1;
                }
            }
            color[num] = atof(s);
        } else {
            picture = std::string(argv[i]);
        }
    }
    if (!picture) picture = "assets/default.png";

    Config conf = Config{
        .picture = picture.value(),
        .rain_count = rain_count,
        .speed = speed,
    };
    for (int i = 0; i < 5; i++) conf.color[i] = color[i];

    return conf;
}
