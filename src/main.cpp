// =============================================================================
//  main.cpp — Point d'entrée principal de l'application StabileoViewer
// =============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "core/Camera.h"
#include "render/StructureRenderer.h"
#include "scene/DemoModels.h"
#include "scene/ModelLoader.h"
#include "scene/DxfImporter.h"
#include "scene/StructureModel.h"
#include "solver/LinearSolver.h"
#include "ui/UIManager.h"

#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>

struct AppContext {
    Camera camera;
    bool mouseLeftDown   = false;
    bool mouseRightDown  = false;
    bool mouseMiddleDown = false;
    double lastMouseX    = 0.0;
    double lastMouseY    = 0.0;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->mouseLeftDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        ctx->mouseRightDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        ctx->mouseMiddleDown = (action == GLFW_PRESS);
    }
    glfwGetCursorPos(window, &ctx->lastMouseX, &ctx->lastMouseY);
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;

    double dx = xpos - ctx->lastMouseX;
    double dy = ypos - ctx->lastMouseY;
    ctx->lastMouseX = xpos;
    ctx->lastMouseY = ypos;

    if (ImGui::GetIO().WantCaptureMouse) return;

    if (ctx->mouseLeftDown) {
        ctx->camera.rotate(static_cast<float>(dx), static_cast<float>(dy));
    } else if (ctx->mouseRightDown || ctx->mouseMiddleDown) {
        ctx->camera.pan(static_cast<float>(dx), static_cast<float>(dy));
    }
}

static void scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (ctx) {
        ctx->camera.zoom(static_cast<float>(yoffset));
    }
}

static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    if (action != GLFW_PRESS) return;

    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;

    switch (key) {
        case GLFW_KEY_1: ctx->camera.setFrontView(); break;
        case GLFW_KEY_2: ctx->camera.setTopView(); break;
        case GLFW_KEY_3: ctx->camera.setSideView(); break;
        case GLFW_KEY_4: ctx->camera.setIsometricView(); break;
        case GLFW_KEY_F: ctx->camera.fitToScene(ctx->boundsMin, ctx->boundsMax); break;
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        default: break;
    }
}

int main(int argc, char* argv[]) {
    // Mode test en ligne de commande (validation analytique et test DXF)
    if (argc > 1 && (std::string(argv[1]) == "--test" || std::string(argv[1]) == "-t")) {
        bool ok = solver::runValidationTest();

        std::cout << "[Test DXF] Test d'importation et calcul du portique DXF...\n";
        scene::generateSampleDxfFiles("assets/models");
        model::Structure dxfSt = scene::loadDxf("assets/models/industrial-portal-frame.dxf");
        bool dxfOk = !dxfSt.nodes.empty() && !dxfSt.elements.empty();
        if (dxfOk) {
            bool solved = solver::solveLinearStatic(dxfSt);
            std::cout << "[Test DXF] Résolution FEA DXF : " << (solved ? "SUCCÈS" : "ÉCHEC") << "\n";
            std::cout << "           Nœuds: " << dxfSt.nodes.size() << ", Éléments: " << dxfSt.elements.size() << "\n";
            dxfOk = solved;
        }

        std::cout << "[Test DXF] Test d'importation et calcul de la ferme Warren DXF...\n";
        model::Structure trussSt = scene::loadDxf("assets/models/warren-roof-truss.dxf");
        bool trussOk = !trussSt.nodes.empty() && !trussSt.elements.empty();
        if (trussOk) {
            bool solved = solver::solveLinearStatic(trussSt);
            std::cout << "[Test DXF] Résolution FEA Ferme Warren : " << (solved ? "SUCCÈS" : "ÉCHEC") << "\n";
            std::cout << "           Nœuds: " << trussSt.nodes.size() << ", Éléments: " << trussSt.elements.size() << "\n";
            trussOk = solved;
        }

        return (ok && dxfOk && trussOk) ? 0 : 1;
    }

    // ---- Initialisation GLFW ----
    if (!glfwInit()) {
        std::cerr << "[ERREUR] Échec de l'initialisation de GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4); // Anti-aliasing MSAA 4x

    const int initialWidth = 1600;
    const int initialHeight = 900;
    GLFWwindow* window = glfwCreateWindow(
        initialWidth, initialHeight,
        "Stabileo 3D Structural Viewer - C++20 / OpenGL 3.3",
        nullptr, nullptr
    );

    if (!window) {
        std::cerr << "[ERREUR] Impossible de créer la fenêtre GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Activer VSync (60 FPS)

    // ---- Initialisation GLEW ----
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "[ERREUR] Échec GLEW : " << glewGetErrorString(glewErr) << "\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Configuration OpenGL globale
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);

    // Contexte applicatif et caméra
    AppContext appCtx;
    glfwSetWindowUserPointer(window, &appCtx);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    // Initialisation ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    UIManager uiManager;
    uiManager.setupStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Pipeline de rendu
    StructureRenderer renderer;
    renderer.init();

    // Validation analytique du solveur 3D au démarrage
    solver::runValidationTest();

    // Génération des fichiers DXF de démonstration si absents
    scene::generateSampleDxfFiles("assets/models");

    // Modèle initial (Portique 2D avec HEA / IPE)
    model::Structure currentStructure = scene::createDemoPortalFrame();
    solver::solveLinearStatic(currentStructure);
    RenderState renderState;

    renderer.rebuild(currentStructure);
    currentStructure.computeBounds(appCtx.boundsMin, appCtx.boundsMax);
    appCtx.camera.fitToScene(appCtx.boundsMin, appCtx.boundsMax);

    auto loadStructureAndApply = [&](model::Structure&& newSt) {
        if (newSt.nodes.empty()) return;
        currentStructure = std::move(newSt);

        // Analyse par éléments finis (Direct Stiffness Method 3D)
        bool solved = solver::solveLinearStatic(currentStructure);
        if (!solved) {
            std::cerr << "[Main] Avertissement : calcul EF non convergent ou structure instable.\n";
        }

        renderer.rebuild(currentStructure);
        if (renderState.showDeformed) {
            renderer.rebuildDeformed(currentStructure, renderState.deformScale);
        }
        if (renderState.showDiagram) {
            renderer.rebuildDiagrams(currentStructure, renderState.diagramType, renderState.diagramScale);
        }
        if (renderState.showHeatmap) {
            renderer.rebuildHeatmap(currentStructure);
        }
        currentStructure.computeBounds(appCtx.boundsMin, appCtx.boundsMax);
        appCtx.camera.fitToScene(appCtx.boundsMin, appCtx.boundsMax);
    };

    auto loadDemo = [&](int demoIdx) {
        switch (demoIdx) {
            case 0:  loadStructureAndApply(scene::createDemoPortalFrame()); break;
            case 1:  loadStructureAndApply(scene::createDemoSpaceTruss()); break;
            case 2:  loadStructureAndApply(scene::createDemoContinuousBeam()); break;
            case 3:  loadStructureAndApply(scene::createDemo3DBuilding()); break;
            case 4:  loadStructureAndApply(scene::createDemoDiagridTower()); break;
            case 5:  loadStructureAndApply(scene::createDemoTower3D()); break;
            case 6:  loadStructureAndApply(scene::createDemoSuspensionBridge()); break;
            case 7:  loadStructureAndApply(scene::createDemoCableStayedBridge()); break;
            case 8:  loadStructureAndApply(scene::createDemoWarrenTruss()); break;
            case 9:  loadStructureAndApply(scene::createDemoOffshorePlatform()); break;
            case 10: loadStructureAndApply(scene::createDemoNaveIndustrial()); break;
            case 11: loadStructureAndApply(scene::createDemoPipeRack()); break;
            case 12: loadStructureAndApply(scene::createDemoGridSlab()); break;
            case 13: loadStructureAndApply(scene::createDemoGeodesicDome()); break;
            case 14: loadStructureAndApply(scene::createDemoThreeHingeArch()); break;
            default: break;
        }
    };

    // Mesure du FPS
    int fpsCounter = 0;
    int currentFps = 60;
    auto lastFpsTime = std::chrono::high_resolution_clock::now();

    // Boucle principale
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Calcul FPS
        auto now = std::chrono::high_resolution_clock::now();
        fpsCounter++;
        std::chrono::duration<float> elapsed = now - lastFpsTime;
        if (elapsed.count() >= 0.5f) {
            currentFps = static_cast<int>(fpsCounter / elapsed.count());
            fpsCounter = 0;
            lastFpsTime = now;
        }

        float currentTime = static_cast<float>(glfwGetTime());

        // Traitement du chargement de démo demandé par l'UI (modèles natifs)
        if (uiManager.pendingDemoLoad >= 0) {
            loadDemo(uiManager.pendingDemoLoad);
            uiManager.pendingDemoLoad = -1;
        }

        // Traitement du chargement d'une fixture JSON Stabileo
        if (!uiManager.pendingFixtureLoad.empty()) {
            std::string fix = uiManager.pendingFixtureLoad;
            uiManager.pendingFixtureLoad.clear();

            model::Structure loadedSt;
            if (fix.find(".json") != std::string::npos || fix.find('/') != std::string::npos || fix.find('\\') != std::string::npos) {
                loadedSt = scene::loadStructureFromJsonFile(fix);
            } else {
                loadedSt = scene::loadFixtureByName(fix);
            }
            if (!loadedSt.nodes.empty()) {
                loadStructureAndApply(std::move(loadedSt));
            }
        }

        // Traitement du chargement d'un fichier DXF (AutoCAD)
        if (!uiManager.pendingDxfLoad.empty()) {
            std::string dxfPath = uiManager.pendingDxfLoad;
            uiManager.pendingDxfLoad.clear();

            model::Structure loadedSt = scene::loadDxf(dxfPath, uiManager.dxfOptions);
            if (!loadedSt.nodes.empty()) {
                loadStructureAndApply(std::move(loadedSt));
            }
        }

        // Début de frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Dessin de l'interface
        uiManager.drawUI(renderState, currentStructure, appCtx.camera, appCtx.boundsMin, appCtx.boundsMax, currentFps);

        // Traitement des flags de rebuild
        if (uiManager.needsRebuild) {
            renderer.rebuild(currentStructure);
        }
        if (uiManager.needsDeformedRebuild && renderState.showDeformed) {
            renderer.rebuildDeformed(currentStructure, renderState.deformScale);
        }
        if (uiManager.needsDiagramRebuild && renderState.showDiagram) {
            renderer.rebuildDiagrams(currentStructure, renderState.diagramType, renderState.diagramScale);
        }
        if (uiManager.needsHeatmapRebuild && renderState.showHeatmap) {
            renderer.rebuildHeatmap(currentStructure);
        }

        // Animation oscillatoire de la déformée si demandée
        if (renderState.showDeformed && renderState.animateDeformed) {
            float osc = 0.5f + 0.5f * std::sin(currentTime * 3.5f);
            renderer.rebuildDeformed(currentStructure, renderState.deformScale * osc);
        }

        // Rendu ImGui
        ImGui::Render();

        // Mise à jour du viewport et du ratio caméra
        int displayW, displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        appCtx.camera.aspectRatio = (displayH > 0) ? (static_cast<float>(displayW) / static_cast<float>(displayH)) : 1.0f;

        // Effacement de l'écran avec un fond sombre d'ingénierie
        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Dessin de la scène 3D
        renderer.draw(appCtx.camera, renderState, currentTime);

        // Rendu des commandes de draw ImGui par-dessus la scène
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Nettoyage
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
