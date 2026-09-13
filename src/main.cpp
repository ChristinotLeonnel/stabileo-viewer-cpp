// =============================================================================
//  main.cpp — Point d'entrée principal de l'application StabileoViewer
// =============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "core/Camera.h"
#include "core/Framebuffer.h"
#include "render/StructureRenderer.h"
#include "scene/DemoModels.h"
#include "scene/ModelLoader.h"
#include "scene/DxfImporter.h"
#include "scene/StructureModel.h"
#include "solver/LinearSolver.h"
#include "ui/UIManager.h"
#include "scripting/ScriptEngine.h"

#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <limits>

struct AppContext {
    Camera camera;
    bool mouseLeftDown   = false;
    bool mouseRightDown  = false;
    bool mouseMiddleDown = false;
    double lastMouseX    = 0.0;
    double lastMouseY    = 0.0;
    // Position au moment du clic gauche, pour distinguer un clic (sélection)
    // d'un glisser (rotation caméra).
    double mouseDownX    = 0.0;
    double mouseDownY    = 0.0;
    bool pickRequested   = false;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    RenderState* renderState = nullptr; // référence non-possédée, assignée dans main()
    UIManager*   uiManager   = nullptr;
};

/// Convertit une position relative au viewport (pixels) en un rayon 3D (origine + direction)
/// dans le repère du monde, à partir de la caméra courante.
static bool screenPointToRay(double relX, double relY, float vpW, float vpH, const Camera& camera,
                              glm::vec3& outOrigin, glm::vec3& outDir) {
    if (vpW <= 0.0f || vpH <= 0.0f) return false;

    float ndcX =  static_cast<float>(2.0 * relX / vpW - 1.0);
    float ndcY = -static_cast<float>(2.0 * relY / vpH - 1.0); // écran (Y bas) -> NDC (Y haut)

    glm::mat4 invVP = glm::inverse(camera.getProjectionMatrix() * camera.getViewMatrix());

    glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPt  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
    nearPt /= nearPt.w;
    farPt  /= farPt.w;

    outOrigin = glm::vec3(nearPt);
    outDir    = glm::normalize(glm::vec3(farPt - nearPt));
    return true;
}

/// Distance la plus courte entre un rayon et un point du monde.
static float rayPointDistance(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                               const glm::vec3& point, float& outT) {
    outT = glm::dot(point - rayOrigin, rayDir);
    glm::vec3 closest = rayOrigin + rayDir * outT;
    return glm::length(closest - point);
}

/// Distance la plus courte entre un rayon et un segment [a, b] (barre structurelle).
static float raySegmentDistance(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                 const glm::vec3& a, const glm::vec3& b, float& outT) {
    glm::vec3 u = rayDir;
    glm::vec3 v = b - a;
    glm::vec3 w0 = rayOrigin - a;
    float aC = glm::dot(u, u);
    float bC = glm::dot(u, v);
    float cC = glm::dot(v, v);
    float dC = glm::dot(u, w0);
    float eC = glm::dot(v, w0);
    float denom = aC * cC - bC * bC;

    float tc; // paramètre le long du segment [0,1]
    if (std::abs(denom) < 1e-6f) {
        tc = (cC > 1e-9f) ? std::clamp(eC / cC, 0.0f, 1.0f) : 0.0f;
    } else {
        tc = std::clamp((aC * eC - bC * dC) / denom, 0.0f, 1.0f);
    }
    glm::vec3 closestOnSeg = a + v * tc;
    outT = glm::dot(closestOnSeg - rayOrigin, u);
    glm::vec3 closestOnRay = rayOrigin + u * std::max(outT, 0.0f);
    return glm::length(closestOnRay - closestOnSeg);
}

/// Sélectionne le nœud ou la barre le plus proche du rayon souris (picking 3D).
static void performPicking(AppContext& ctx, GLFWwindow* /*window*/,
                            const model::Structure& structure, RenderState& renderState) {
    float vpW = ctx.uiManager ? ctx.uiManager->viewportSize.x : 1600.0f;
    float vpH = ctx.uiManager ? ctx.uiManager->viewportSize.y : 900.0f;
    float relX = static_cast<float>(ctx.mouseDownX - (ctx.uiManager ? ctx.uiManager->viewportPos.x : 0.0f));
    float relY = static_cast<float>(ctx.mouseDownY - (ctx.uiManager ? ctx.uiManager->viewportPos.y : 0.0f));

    if (relX < 0.0f || relX > vpW || relY < 0.0f || relY > vpH) return;

    glm::vec3 rayOrigin, rayDir;
    if (!screenPointToRay(relX, relY, vpW, vpH, ctx.camera, rayOrigin, rayDir)) {
        return;
    }

    const float nodePickRadius    = 0.20f; // tolérance de clic sur un nœud [m monde]
    const float elementPickRadius = 0.14f; // tolérance de clic sur une barre [m monde]

    int   bestNodeId = -1;
    float bestNodeT  = std::numeric_limits<float>::max();
    for (auto& n : structure.nodes) {
        float t;
        float d = rayPointDistance(rayOrigin, rayDir, n.position, t);
        if (d < nodePickRadius && t > 0.0f && t < bestNodeT) {
            bestNodeT = t;
            bestNodeId = n.id;
        }
    }

    int   bestElemId = -1;
    float bestElemT  = std::numeric_limits<float>::max();
    for (auto& e : structure.elements) {
        const auto* nI = structure.findNode(e.nodeI);
        const auto* nJ = structure.findNode(e.nodeJ);
        if (!nI || !nJ) continue;
        float t;
        float d = raySegmentDistance(rayOrigin, rayDir, nI->position, nJ->position, t);
        if (d < elementPickRadius && t > 0.0f && t < bestElemT) {
            bestElemT = t;
            bestElemId = e.id;
        }
    }

    // Priorité aux nœuds quand ils sont à une profondeur comparable (plus faciles à viser).
    if (bestNodeId >= 0 && (bestElemId < 0 || bestNodeT <= bestElemT + 0.05f)) {
        renderState.selectedNodeId    = bestNodeId;
        renderState.selectedElementId = -1;
    } else if (bestElemId >= 0) {
        renderState.selectedElementId = bestElemId;
        renderState.selectedNodeId    = -1;
    } else {
        renderState.selectedNodeId    = -1;
        renderState.selectedElementId = -1;
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;

    bool overViewport = ctx->uiManager ? ctx->uiManager->viewportHovered : !ImGui::GetIO().WantCaptureMouse;

    if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (ctx->mouseLeftDown) {
                double mx, my;
                glfwGetCursorPos(window, &mx, &my);
                double moved = std::hypot(mx - ctx->mouseDownX, my - ctx->mouseDownY);
                if (moved < 4.0) {
                    ctx->pickRequested = true; // clic net -> sélection
                }
            }
            ctx->mouseLeftDown = false;
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            ctx->mouseRightDown = false;
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            ctx->mouseMiddleDown = false;
        }
    } else if (action == GLFW_PRESS && overViewport) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            ctx->mouseLeftDown = true;
            glfwGetCursorPos(window, &ctx->mouseDownX, &ctx->mouseDownY);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            ctx->mouseRightDown = true;
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            ctx->mouseMiddleDown = true;
        }
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

    if (ctx->mouseLeftDown) {
        ctx->camera.rotate(static_cast<float>(dx), static_cast<float>(dy));
    } else if (ctx->mouseRightDown || ctx->mouseMiddleDown) {
        ctx->camera.pan(static_cast<float>(dx), static_cast<float>(dy));
    }
}

static void scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;

    bool overViewport = ctx->uiManager ? ctx->uiManager->viewportHovered : !ImGui::GetIO().WantCaptureMouse;
    if (!overViewport) return;

    float vpW = ctx->uiManager ? ctx->uiManager->viewportSize.x : 1600.0f;
    float vpH = ctx->uiManager ? ctx->uiManager->viewportSize.y : 900.0f;
    float relX = static_cast<float>(ctx->lastMouseX - (ctx->uiManager ? ctx->uiManager->viewportPos.x : 0.0f));
    float relY = static_cast<float>(ctx->lastMouseY - (ctx->uiManager ? ctx->uiManager->viewportPos.y : 0.0f));

    glm::vec3 rayOrigin, rayDir;
    if (screenPointToRay(relX, relY, vpW, vpH, ctx->camera, rayOrigin, rayDir)) {
        glm::vec3 viewDir = glm::normalize(ctx->camera.target - ctx->camera.getPosition());
        float denom = glm::dot(viewDir, rayDir);
        if (std::abs(denom) > 1e-5f) {
            float t = glm::dot(ctx->camera.target - rayOrigin, viewDir) / denom;
            glm::vec3 focus = rayOrigin + rayDir * t;
            ctx->camera.zoomToward(static_cast<float>(yoffset), focus);
            return;
        }
    }
    ctx->camera.zoom(static_cast<float>(yoffset));
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
        case GLFW_KEY_5: ctx->camera.orthographic = !ctx->camera.orthographic; break;
        case GLFW_KEY_F: ctx->camera.fitToScene(ctx->boundsMin, ctx->boundsMax); break;
        case GLFW_KEY_ESCAPE:
            // ERGONOMIE : Échap désélectionne l'objet courant au lieu de
            // fermer immédiatement l'application. Fermer tout le programme
            // sur une simple touche Échap (souvent pressée par réflexe pour
            // "annuler" une action) faisait perdre le travail en cours sans
            // confirmation.
            if (ctx->renderState &&
                (ctx->renderState->selectedNodeId >= 0 || ctx->renderState->selectedElementId >= 0)) {
                ctx->renderState->selectedNodeId    = -1;
                ctx->renderState->selectedElementId = -1;
            }
            break;
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
        std::cout << "[Test C# ScriptEngine] Initialisation du runtime .NET CoreCLR & Roslyn...\n";
        bool scriptRebuild = false;
        bool scriptOk = scripting::ScriptEngine::init(&dxfSt, &scriptRebuild);
        int pluginCount = scripting::ScriptEngine::getLoadedPluginCount();
        std::cout << "[Test C# ScriptEngine] Statut : " << (scriptOk ? "SUCCÈS" : "ÉCHEC")
                  << " | " << pluginCount << " plugin(s) C# actif(s).\n";
        scripting::ScriptEngine::shutdown();

        return (ok && dxfOk && trussOk && scriptOk) ? 0 : 1;
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
    appCtx.renderState = &renderState;
    appCtx.uiManager   = &uiManager;

    // Framebuffer pour le rendu de la Vue 3D dockable
    Framebuffer fbo;
    fbo.init(1600, 900);

    renderer.rebuild(currentStructure);
    currentStructure.computeBounds(appCtx.boundsMin, appCtx.boundsMax);
    appCtx.camera.fitToScene(appCtx.boundsMin, appCtx.boundsMax);

    // Initialisation du sous-système de Scripting C# (CoreCLR / Hazel Engine)
    bool csharpNeedsRebuild = false;
    scripting::ScriptEngine::init(&currentStructure, &csharpNeedsRebuild);

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

        // BUGFIX: une sélection de nœud/barre de l'ancien modèle pouvait
        // rester active et pointer, par coïncidence d'ID, vers un élément
        // sans rapport dans le nouveau modèle chargé.
        renderState.selectedNodeId    = -1;
        renderState.selectedElementId = -1;
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
    float lastFrameTime = static_cast<float>(glfwGetTime());

    // Boucle principale
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Calcul FPS et delta temps
        auto now = std::chrono::high_resolution_clock::now();
        fpsCounter++;
        std::chrono::duration<float> elapsed = now - lastFpsTime;
        if (elapsed.count() >= 0.5f) {
            currentFps = static_cast<int>(fpsCounter / elapsed.count());
            fpsCounter = 0;
            lastFpsTime = now;
        }

        float currentTime = static_cast<float>(glfwGetTime());
        float dt = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;
        appCtx.camera.update(dt);

        // Sélection à la souris (nœud / barre) demandée par un clic net dans la vue 3D
        if (appCtx.pickRequested) {
            appCtx.pickRequested = false;
            performPicking(appCtx, window, currentStructure, renderState);
        }

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

        // Dessin de l'interface et de la fenêtre Vue 3D dockée
        uiManager.drawUI(renderState, currentStructure, appCtx.camera,
                         appCtx.boundsMin, appCtx.boundsMax, currentFps,
                         fbo.texture);

        // Traitement des flags de rebuild
        if (uiManager.needsRebuild || csharpNeedsRebuild) {
            renderer.rebuild(currentStructure);
            currentStructure.computeBounds(appCtx.boundsMin, appCtx.boundsMax);
            if (csharpNeedsRebuild) {
                appCtx.camera.fitToScene(appCtx.boundsMin, appCtx.boundsMax);
                csharpNeedsRebuild = false;
            }
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

        // Rendu ImGui pour construire les commandes graphiques
        ImGui::Render();

        // 1. Rendu hors-écran de la scène 3D dans le Framebuffer OpenGL (FBO)
        int fboW = static_cast<int>(uiManager.viewportSize.x);
        int fboH = static_cast<int>(uiManager.viewportSize.y);
        if (fboW > 0 && fboH > 0) {
            fbo.resize(fboW, fboH);
            fbo.bind();
            glViewport(0, 0, fboW, fboH);
            appCtx.camera.aspectRatio = static_cast<float>(fboW) / static_cast<float>(fboH);

            glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderer.draw(appCtx.camera, renderState, currentTime);
            fbo.unbind();
        }

        // 2. Rendu de la fenêtre principale GLFW (contenant le DockSpace et toutes les fenêtres ImGui)
        int displayW, displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);

        glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Nettoyage
    scripting::ScriptEngine::shutdown();
    fbo.cleanup();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
