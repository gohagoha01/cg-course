cmake_minimum_required(VERSION 3.16)
project(cg_course C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

include_directories(
    ${GLFW_INCLUDE_DIRS}
    external/glad/include
    external/stb
)

add_executable(cg 
    main.cpp 
    external/glad/src/gl.c
)

target_link_libraries(cg PRIVATE OpenGL::GL ${GLFW_LIBRARIES} pthread dl)
