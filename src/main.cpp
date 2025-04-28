#include <chrono>
#include <optional>
#include <print>
#include <random>
#include <string>
#include <cstring>

#include "window.h"
#include "resources.h"

Config parseArgs(int argc, char** argv);
void update(Resources* resources, const float dt_s, std::uniform_real_distribution<>& dis, std::mt19937& gen, const Config config);
void draw(const Resources& resources, const int scr_width, const int scr_height, const uint32_t rain_count);

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

    auto resource_init_result = resourcesInit(config, dis, gen);
    if (!resource_init_result) return -1;

    Resources resources = resource_init_result.value();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
    auto end_time = start_time;
    while (!glfwWindowShouldClose(window)) {
        float dt_s = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count()/float(1000);
        start_time = std::chrono::steady_clock::now();

        glfwPollEvents();

        int scr_width, scr_height;
        glfwGetFramebufferSize(window, &scr_width, &scr_height);

        update(&resources, dt_s, dis, gen, config);
        draw(resources, scr_width, scr_height, rain_count);

        glfwSwapBuffers(window);

        end_time = std::chrono::steady_clock::now();
    }

    resourcesDeinit(&resources);
    windowDeinit(&window);
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
    uint32_t rain_count = 256;
    float speed = 1.0;
    std::optional<std::string> picture = std::nullopt;
    float color[5] = {};
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
