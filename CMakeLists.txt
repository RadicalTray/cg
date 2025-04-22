# initialization
cmake_minimum_required(VERSION 3.31) # idk
project(cg)

add_executable(main src/main.cpp src/window.cpp)
set_property(TARGET main PROPERTY CXX_STANDARD 23)

# stb stuff
target_include_directories(main PRIVATE stb)

# glad stuff
target_include_directories(main PRIVATE glad/include)
target_sources(main PRIVATE glad/src/gl.c)

# glfw stuff
add_subdirectory(glfw)
target_link_libraries(main PRIVATE glfw)
