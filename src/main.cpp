#include <string>
#include <print>

#include "window.h"
#include "resources.h"

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
    Shaders shaders = resources.shaders;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        shaders.updateScaler(window);

        draw(resources);

        glfwSwapBuffers(window);
    }

    resourcesDeinit(&resources);
    windowDeinit(&window);
}

void draw(const Resources& resources) {
    const Shaders shaders = resources.shaders;

    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    shaders.useTexture();
    glBindVertexArray(resources.buffer.vert_arr);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

Config parseArgs(int argc, char** argv) {
    return Config{
        .picture = argc >= 2 ? argv[1] : "assets/default.png",
    };
}
