#pragma once
// =============================================================================
//  UIManager.h — Interface utilisateur Dear ImGui pour StabileoViewer (Docking)
// =============================================================================

#include "render/StructureRenderer.h"
#include "scene/StructureModel.h"
#include "scene/DxfImporter.h"
#include "core/Camera.h"
#include <imgui.h>
#include <string>
#include <vector>

class UIManager {
public:
    /// Configure le thème sombre moderne d'ingénierie.
    void setupStyle();

    /// Dessine le DockSpace et l'ensemble des panneaux ImGui.
    /// Retourne true si un rebuild graphique est nécessaire.
    bool drawUI(RenderState& state, model::Structure& structure, Camera& camera,
                const glm::vec3& boundsMin, const glm::vec3& boundsMax, int fps);

    /// Force la réinitialisation de la disposition des fenêtres dockées.
    void resetLayout() { resetDockingLayout_ = true; }

    // Indique quel modèle de démo charger (-1 = aucun changement)
    int  pendingDemoLoad = -1;

    // Nom de fixture JSON à charger (vide si aucun)
    std::string pendingFixtureLoad;

    // Fichier DXF à charger (vide si aucun)
    std::string pendingDxfLoad;
    scene::DxfImportOptions dxfOptions;

    // Flags de rebuild maillage
    bool needsRebuild         = false;
    bool needsDeformedRebuild = false;
    bool needsDiagramRebuild  = false;
    bool needsHeatmapRebuild  = false;

    // Visibilité individuelle de chaque onglet / fenêtre dockable
    bool showDisplayLayers    = true;
    bool showDeformedResults  = true;
    bool showDiagramsResults  = true;
    bool showHeatmapResults   = true;
    bool showInspector        = true;
    bool showCppCatalog       = true;
    bool showJsonCatalog      = true;
    bool showDxfImporter      = true;
    bool showTableNodes       = true;
    bool showTableElements    = true;
    bool showTableReactions   = true;
    bool showDemoImGui        = false;

private:
    void drawMainMenuBar(model::Structure& structure, Camera& camera,
                         const glm::vec3& boundsMin, const glm::vec3& boundsMax,
                         RenderState& state, int fps);
    void drawQuickToolbar(Camera& camera, const glm::vec3& boundsMin, const glm::vec3& boundsMax, RenderState& state);

    // Chaque onglet est une fenêtre indépendante dockable
    void drawDisplayLayersWindow(RenderState& state);
    void drawDeformedResultsWindow(RenderState& state);
    void drawDiagramsResultsWindow(RenderState& state);
    void drawHeatmapResultsWindow(RenderState& state);
    void drawInspectorWindow(const RenderState& state, const model::Structure& structure);
    void drawCppCatalogWindow();
    void drawJsonCatalogWindow();
    void drawDxfImporterWindow();
    void drawTableNodesWindow(const model::Structure& structure);
    void drawTableElementsWindow(const model::Structure& structure);
    void drawTableReactionsWindow(const model::Structure& structure);

    void buildDefaultDockLayout(ImGuiID dockspaceId);

    bool resetDockingLayout_ = false;
    float prevDeformScale_ = -1.0f;
    scene::DiagramType prevDiagramType_ = scene::DiagramType::My;
    float prevDiagramScale_ = -1.0f;

    // État du catalogue de modèles
    int selectedFixtureIdx_ = 0;
    char customJsonPath_[256] = "";
    char customDxfPath_[256] = "assets/models/industrial-portal-frame.dxf";
    int dxfUnitIdx_ = 0;   // 0: Mètres, 1: Millimètres, 2: Centimètres, 3: Pouces
    int dxfUpAxisIdx_ = 0; // 0: Y-Up (2D Elevation), 1: Z-Up vers Y-Up (3D CAD)
};
