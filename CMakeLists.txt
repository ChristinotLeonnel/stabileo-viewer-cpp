# =============================================================================
# CMakeLists.txt — StabileoViewer
#
# Moteur graphique 3D de visualisation structurelle en C++20.
# Compatible Visual Studio 2026 (générateur "Visual Studio 18 2026").
#
# Dépendances (récupérées automatiquement via FetchContent) :
#   - GLFW 3.4            : Fenêtrage et contexte OpenGL
#   - GLM 1.0.1           : Mathématiques 3D (vecteurs, matrices, quaternions)
#   - Dear ImGui (master) : Interface utilisateur immédiate
#   - GLEW 2.2.0          : Chargement des fonctions OpenGL
#   - nlohmann/json 3.11.3: Sérialisation JSON
#
# Utilisation :
#   cmake -B build -G "Visual Studio 18 2026" -A x64
#   cmake --build build --config Release
#   build\bin\Release\StabileoViewer.exe
# =============================================================================

cmake_minimum_required(VERSION 3.21)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
project(StabileoViewer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Output directories ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release")

# ===========================================================================
#  Bibliothèques Locales (dossier libs/ au même niveau que src/)
#  Avantages : 100% hors-ligne, zéro clone git, configuration CMake instantanée.
# ===========================================================================

# 1. GLFW 3.4
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)

# 2. GLM 1.0.1
add_subdirectory(libs/glm)

# 3. GLEW 2.2.0
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glew)

# 4. Dear ImGui (Docking)
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui")
add_library(imgui_lib STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw libglew_static)

# 5. nlohmann/json 3.11.3
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/json)

# 6. Eigen 3.4.0 (Header-only)
set(EIGEN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/eigen")

# ===========================================================================
#  StabileoViewer — Exécutable principal
# ===========================================================================
file(GLOB_RECURSE APP_SOURCES
    "src/*.cpp"
    "src/*.h"
)

add_executable(StabileoViewer ${APP_SOURCES})

target_include_directories(StabileoViewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${EIGEN_INCLUDE_DIR}
)

target_link_libraries(StabileoViewer PRIVATE
    libglew_static
    glfw
    glm::glm
    imgui_lib
    nlohmann_json::nlohmann_json
    opengl32
)

# Visual Studio : répertoire de travail pour le débogage F5
set_target_properties(StabileoViewer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Copie automatique des assets (shaders, modèles 3D JSON) à côté de l'exécutable
add_custom_command(TARGET StabileoViewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:StabileoViewer>/assets"
)

# ---- MSVC-specific flags ----
if(MSVC)
    target_compile_options(StabileoViewer PRIVATE /W4 /MP /utf-8)
    target_compile_definitions(StabileoViewer PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

message(STATUS "")
message(STATUS "=== StabileoViewer configured successfully ===")
message(STATUS "  Generator    : ${CMAKE_GENERATOR}")
message(STATUS "  C++ Standard : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Sources      : ${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "  Output       : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "")
