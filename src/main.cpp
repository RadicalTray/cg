#include <string>
#include <print>

#include "window.h"
#include "resources.h"

// GOAL: render "rain"
// TODO:
//  - [ ] Maintain ratio
//  - [ ] Mouse movement
//  - [ ] "Rain"

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

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        draw(resources);

        glUniform1f(glGetUniformLocation(resources.shaders.main, "u_time"), glfwGetTime());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resources.texture);
        glUniform1i(glGetUniformLocation(resources.shaders.main, "u_texture"), 0);

        glfwSwapBuffers(window);
    }

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
        .picture = argc >= 2 ? argv[1] : "assets/default2.png",
    };
}
