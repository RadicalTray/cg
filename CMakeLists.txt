# initialization
cmake_minimum_required(VERSION 3.30) # idk
project(cg)

add_executable(main src/main.cpp src/window.cpp src/resources.cpp)
set_property(TARGET main PROPERTY CXX_STANDARD 23)

# stb stuff
target_include_directories(main PRIVATE stb)

# glad stuff
target_include_directories(main PRIVATE glad/include)
target_sources(main PRIVATE glad/src/gl.c)

# glfw stuff
add_subdirectory(glfw)
target_link_libraries(main PRIVATE glfw)

# glm stuff
add_subdirectory(glm)
target_link_libraries(main PRIVATE glm)



target_include_directories(main PRIVATE imgui)
target_include_directories(main PRIVATE imgui/backends)
target_sources(main PRIVATE imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp)
target_sources(main PRIVATE imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp)