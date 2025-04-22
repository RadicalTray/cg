#include <print>

#include "window.h"
#include "resources.h"

// GOAL: render "rain"
// TODO:
//  - [ ] Render texture
//  - [ ] Mouse movement
//  - [ ] "Rain"

struct Config {
    std::string picture;
};

Config parseArgs(int argc, char** argv);
void draw(const Resources& resources);

int main(int argc, char** argv) {
    Config config = parseArgs(argc, argv);
    std::println("Picture: {}", config.picture);

    GLFWwindow* window = windowInit();
    if (window == nullptr) return -1;

    auto resource_init_result = resourcesInit();
    if (!resource_init_result) return -1;

    Resources resources = resource_init_result.value();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        draw(resources);

        glfwSwapBuffers(window);
    }

    resourcesDeinit(&resources);
    windowDeinit(&window);
}

void draw(const Resources& resources) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(resources.shaders.main);
    glBindVertexArray(resources.buffer.vert_arr);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

Config parseArgs(int argc, char** argv) {
    return Config{
        .picture = argc >= 2 ? argv[1] : "assets/default.jpg",
    };
}
