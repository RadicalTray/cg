#include <string>
#include <print>

#include "window.h"
#include "resources.h"

Config parseArgs(int argc, char** argv);
void update(Resources *resources);
void draw(const Resources& resources, const int scr_width, const int scr_height);
void drawRain(const Resources& resources);

int main(int argc, char** argv) {
    Config config = parseArgs(argc, argv);
    std::println("Picture: {}", config.picture);

    GLFWwindow* window = windowInit();
    if (window == NULL) return -1;

    auto resource_init_result = resourcesInit(config);
    if (!resource_init_result) return -1;

    Resources resources = resource_init_result.value();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    glEnable(GL_BLEND);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int scr_width, scr_height;
        glfwGetFramebufferSize(window, &scr_width, &scr_height);

        update(&resources);
        draw(resources, scr_width, scr_height);

        glfwSwapBuffers(window);
    }

    resourcesDeinit(&resources);
    windowDeinit(&window);
}

void update(Resources* resources) {
    const float gravity = 0.01;
    for (size_t i = 0; i < RAIN_VERTICES_COUNT; i += 4) {
        RainVertex v0 = resources->rain_vertices[i];
        RainVertex v1 = resources->rain_vertices[i+1];
        RainVertex v2 = resources->rain_vertices[i+2];
        RainVertex v3 = resources->rain_vertices[i+3];

        v0.pos.y -= gravity;
        v1.pos.y -= gravity;
        v2.pos.y -= gravity;
        v3.pos.y -= gravity;

        resources->rain_vertices[i]   = v0;
        resources->rain_vertices[i+1] = v1;
        resources->rain_vertices[i+2] = v2;
        resources->rain_vertices[i+3] = v3;
    }
    glBindBuffer(GL_ARRAY_BUFFER, resources->buffers.rain_vert_buf);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(resources->rain_vertices), resources->rain_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw(const Resources& resources, const int scr_width, const int scr_height) {
    const Shaders shaders = resources.shaders;
    const Buffers buffers = resources.buffers;
    const GLuint image_texture = resources.texture;
    const RenderTarget render_target = resources.render_target;

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
        glDrawElements(GL_TRIANGLES, RAIN_INDICES_COUNT, GL_UNSIGNED_INT, 0);
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
        glBindTexture(GL_TEXTURE_2D, render_target.texture);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLuint)));
    }
}

Config parseArgs(int argc, char** argv) {
    return Config{
        .picture = argc >= 2 ? argv[1] : "assets/default.png",
    };
}
