#pragma once
// =============================================================================
//  UIManager.h — Interface utilisateur Dear ImGui pour StabileoViewer (Docking)
// =============================================================================

#include "render/StructureRenderer.h"
#include "scene/StructureModel.h"
#include "scene/DxfImporter.h"
#include "core/Camera.h"
#include "ui/ViewCube.h"
#include "ui/HazelUI.h"
#include "ui/SceneHierarchyPanel.h"
#include "ui/ContentBrowserPanel.h"
#include "ui/StructuralModelPanel.h"
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
                const glm::vec3& boundsMin, const glm::vec3& boundsMax, int fps,
                GLuint viewportTexture = 0);

    /// Force la réinitialisation de la disposition des fenêtres dockées.
    void resetLayout() { resetDockingLayout_ = true; }

    // Indique quel modèle de démo charger (-1 = aucun changement)
    int  pendingDemoLoad = -1;

    // Nom de fixture JSON à charger (vide si aucun)
    std::string pendingFixtureLoad;

    // Fichier DXF à charger (vide si aucun)
    std::string pendingDxfLoad;
    scene::DxfImportOptions dxfOptions;

    // Géométrie et état de la fenêtre Vue 3D dockée (pour l'orientation et le picking caméra)
    ImVec2 viewportPos{0.0f, 0.0f};
    ImVec2 viewportSize{1280.0f, 720.0f};
    bool   viewportHovered = false;
    bool   viewportFocused = false;

    // Cube de navigation 3D interactif (Robot Structural Analysis / AutoCAD)
    ui::ViewCube viewCube;
    bool showViewCube         = true;

    // Flags de rebuild maillage
    bool needsRebuild         = false;
    bool needsDeformedRebuild = false;
    bool needsDiagramRebuild  = false;
    bool needsHeatmapRebuild  = false;

    // Panneaux inspirés de Hazel Engine (The Cherno)
    // -------------------------------------------------------------------
    // ERGONOMIE : seuls les panneaux réellement nécessaires au workflow
    // courant (modéliser / naviguer / lancer un calcul) sont visibles par
    // défaut. Tous les autres restent accessibles à tout moment via le
    // menu "Affichage > Fenêtres d'Outils" (cases à cocher) sans être
    // imposés à l'écran dès le lancement.
    // -------------------------------------------------------------------
    SceneHierarchyPanel sceneHierarchyPanel;
    ContentBrowserPanel contentBrowserPanel;
    stabileo::ui::StructuralModelPanel structuralModelPanel;
    bool showSceneHierarchy    = true;  // Essentiel : navigation dans le modèle
    bool showEntityProperties  = true;  // Essentiel : édition de l'objet sélectionné
    bool showContentBrowser    = false; // Secondaire : accessible via Affichage > Fenêtres d'Outils
    bool showSimulationToolbar = true;  // Essentiel : barre flottante légère (Résoudre / Déformée / Diagrammes...)
    bool showStructuralModeler = true;  // Essentiel : outil de modélisation principal (Robot CAO)

    // Mode Plein Écran (F11)
    bool isFullscreen             = false;
    bool pendingToggleFullscreen  = false;

    // Visibilité individuelle de chaque onglet / fenêtre dockable (style Visual Studio 2026 / Hazelnut)
    //
    // Par défaut, seuls les panneaux nécessaires à la prise en main immédiate
    // (modélisation + affichage de base) sont ouverts. Les panneaux d'analyse
    // avancée, catalogues, tables et journaux sont fermés au démarrage pour
    // dégager la vue 3D, mais restent à un clic dans Affichage > Fenêtres
    // d'Outils (ou via les menus Résultats / Calcul EF / Outils qui les
    // ouvrent automatiquement à la demande).
    bool showViewport3D       = false; // Désactivé : la scène 3D s'affiche directement plein écran sans superposition
    bool showStructureExplorer = false; // Intégré dans la Hiérarchie de Scène Hazel
    bool showDisplayLayers    = true;   // Essentiel : bascule grille / axes / calques d'affichage
    bool showSectionPlanes    = false;  // Secondaire (outil avancé) : Affichage > Fenêtres d'Outils
    bool showDeformedResults  = false;  // Secondaire : panneau d'options (l'aperçu se pilote depuis la barre de simulation)
    bool showDiagramsResults  = false;  // Secondaire : idem, ouvert automatiquement depuis "Résultats" si besoin
    bool showHeatmapResults   = false;  // Secondaire : idem
    bool showInspector        = false;  // Remplacé par le panneau Propriétés Hazel avec DrawVec3Control
    bool showCppCatalog       = false;  // Secondaire : Fichier > Nouveau Modèle couvre l'usage courant
    bool showJsonCatalog      = false;  // Secondaire : Fichier > Ouvrir Catalogue JSON (Ctrl+O) l'ouvre à la demande
    bool showDxfImporter      = false;  // Secondaire : Fichier > Importer DXF (Ctrl+I) l'ouvre à la demande
    bool showTableNodes       = false;  // Secondaire : pertinent seulement après un calcul EF
    bool showTableElements    = false;  // Secondaire : idem
    bool showTableReactions   = false;  // Secondaire : idem
    bool showSolverLog        = false;  // Secondaire : Calcul EF > Afficher le Journal l'ouvre à la demande
    bool showDemoImGui        = false;
    bool showCSharpScripting  = false;  // Secondaire (avancé) : s'ouvre automatiquement si un script est chargé

private:
    void drawMainMenuBar(model::Structure& structure, Camera& camera,
                         const glm::vec3& boundsMin, const glm::vec3& boundsMax,
                         RenderState& state, int fps);
    void drawHazelSimulationToolbar(model::Structure& structure, Camera& camera,
                                    const glm::vec3& boundsMin, const glm::vec3& boundsMax, RenderState& state);
    void drawQuickToolbar(Camera& camera, const glm::vec3& boundsMin, const glm::vec3& boundsMax, RenderState& state);

    // Fenêtre centrale : Vue 3D dockable avec FBO OpenGL
    void drawViewportWindow(Camera& camera, GLuint textureId,
                            const glm::vec3& boundsMin, const glm::vec3& boundsMax,
                            RenderState& state);

    // Fenêtres d'outils et de documents dockables
    void drawStructureExplorerWindow(model::Structure& structure, RenderState& state);
    void drawDisplayLayersWindow(RenderState& state);
    void drawSectionPlanesWindow(RenderState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax, Camera& camera);
    void drawDeformedResultsWindow(RenderState& state);
    void drawDiagramsResultsWindow(RenderState& state);
    void drawHeatmapResultsWindow(RenderState& state);
    void drawInspectorWindow(RenderState& state, const model::Structure& structure);
    void drawCppCatalogWindow();
    void drawJsonCatalogWindow();
    void drawDxfImporterWindow();
    void drawTableNodesWindow(const model::Structure& structure);
    void drawTableElementsWindow(const model::Structure& structure);
    void drawTableReactionsWindow(const model::Structure& structure);
    void drawSolverLogWindow(const model::Structure& structure);

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
