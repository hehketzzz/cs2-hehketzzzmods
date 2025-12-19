cmake_minimum_required(VERSION 3.15)
project(CS2_Cheat)

set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR}
    imgui
    imgui/backends
)

# Source files
set(SOURCES
    main.cpp
    Memory.cpp
    ESP.cpp
    Aimbot.cpp
    TriggerBot.cpp
    BunnyHop.cpp
    GUI.cpp
    Config.cpp
)

# Add executable
add_executable(CS2_Cheat ${SOURCES})

# Link libraries
target_link_libraries(CS2_Cheat
    ${OPENGL_LIBRARIES}
    kernel32.lib
    user32.lib
    gdi32.lib
    opengl32.lib
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
)

# Copy ImGui files to build directory
file(COPY imgui/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/imgui)