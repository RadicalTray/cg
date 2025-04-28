#include "window.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <print>

const uint32_t SCR_WIDTH = 800;
const uint32_t SCR_HEIGHT = 600;

GLFWwindow* windowInit() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::println("ERR: Failed to create GLFW window");
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::println("ERR: Failed to initialized GLAD");
        glfwTerminate();
        return nullptr;
    }
    std::println("INFO: Initialized OpenGL window OpenGL version {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    std::println("INFO: GL Version: {}", (char*)glGetString(GL_VERSION));
    std::println("INFO: GLSL Version: {}", (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::println("INFO: GL Renderer: {}", (char*)glGetString(GL_RENDERER));
    std::println("INFO: GL Vendor: {}", (char*)glGetString(GL_VENDOR));

    return window;
}

void windowDeinit(GLFWwindow** window) {
    *window = nullptr;
    glfwTerminate();
}
