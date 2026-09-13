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

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ===========================================================================
#  GLFW — Fenêtrage, contexte OpenGL, entrées souris/clavier
# ===========================================================================
message(STATUS "[StabileoViewer] Fetching GLFW 3.4 ...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ===========================================================================
#  GLM — Mathématiques 3D (vecteurs, matrices, quaternions)
# ===========================================================================
message(STATUS "[StabileoViewer] Fetching GLM 1.0.1 ...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# ===========================================================================
#  GLEW — Chargement des fonctions OpenGL (via glew-cmake wrapper)
# ===========================================================================
message(STATUS "[StabileoViewer] Fetching GLEW 2.2.0 ...")
FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG        glew-cmake-2.2.0
    GIT_SHALLOW    TRUE
)
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glew)

# ===========================================================================
#  Dear ImGui — Interface utilisateur immédiate
# ===========================================================================
message(STATUS "[StabileoViewer] Fetching Dear ImGui ...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# Build ImGui as a static library with GLFW + OpenGL3 backends
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# Link GLEW (PUBLIC) so that __has_include(<GL/glew.h>) succeeds in the
# ImGui OpenGL3 backend, triggering GLEW auto-detection.
target_link_libraries(imgui_lib PUBLIC glfw libglew_static)

# ===========================================================================
#  nlohmann/json — Sérialisation JSON header-only
# ===========================================================================
message(STATUS "[StabileoViewer] Fetching nlohmann/json 3.11.3 ...")
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json)

# ===========================================================================
#  Eigen 3.4.0 — Algèbre linéaire pour le solveur d'analyse par éléments finis
# ===========================================================================
message(STATUS "[StabileoViewer] Fetching Eigen 3.4.0 ...")
FetchContent_Declare(
    eigen
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip
)
set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(eigen)

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
    ${eigen_SOURCE_DIR}
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
