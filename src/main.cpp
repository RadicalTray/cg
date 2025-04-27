#include <string>
#include <print>

#include "window.h"
#include "resources.h"

Config parseArgs(int argc, char** argv);
void draw(const Resources& resources, const int scr_width, const int scr_height);

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

        int scr_width, scr_height;
        glfwGetFramebufferSize(window, &scr_width, &scr_height);

        draw(resources, scr_width, scr_height);

        glfwSwapBuffers(window);
    }

    resourcesDeinit(&resources);
    windowDeinit(&window);
}

void draw(const Resources& resources, const int scr_width, const int scr_height) {
    Shaders shaders = resources.shaders;
    Buffer buffer = resources.buffer;
    GLuint texture = resources.texture;
    RenderTarget render_target = resources.render_target;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, scr_width, scr_height);

    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaders.texture);
    glBindVertexArray(buffer.vert_arr);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(GLuint)));
}

Config parseArgs(int argc, char** argv) {
    return Config{
        .picture = argc >= 2 ? argv[1] : "assets/default.png",
    };
}
