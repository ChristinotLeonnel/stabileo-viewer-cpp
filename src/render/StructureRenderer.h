#pragma once
// =============================================================================
//  StructureRenderer.h — Pipeline de rendu complet pour la structure
// =============================================================================

#include "core/Shader.h"
#include "core/Mesh.h"
#include "core/Camera.h"
#include "core/Grid.h"
#include "scene/StructureModel.h"
#include "scene/DiagramMesh.h"
#include <vector>
#include <memory>

// ---- État d'affichage (contrôlé par l'UI) ----
struct RenderState {
    bool showNodes       = true;
    bool showElements    = true;
    bool showProfiles3D  = true;
    bool showSupports    = true;
    bool showLoads       = true;
    bool showGrid        = true;
    bool showLocalAxes   = false;
    bool showReactions   = true;

    bool showDeformed    = false;
    float deformScale    = 100.0f;
    bool animateDeformed = false;

    bool showDiagram     = false;
    scene::DiagramType diagramType = scene::DiagramType::My;
    float diagramScale   = 1.0f;

    bool showHeatmap     = false;
    int  colormapType    = 0;    // 0=Turbo, 1=Viridis

    int  selectedNodeId    = -1;
    int  selectedElementId = -1;
};

// ---- Mesh instancié avec sa matrice model et sa couleur ----
struct MeshInstance {
    Mesh      mesh;
    glm::mat4 model{1.0f};
    glm::vec3 color{0.7f};
    float     alpha = 1.0f;
};

class StructureRenderer {
public:
    void init();

    /// Reconstruit tous les meshes pour une structure donnée.
    void rebuild(const model::Structure& structure);

    /// Reconstruit les meshes de la déformée (appelé quand le scale change).
    void rebuildDeformed(const model::Structure& structure, float scale);

    /// Reconstruit les meshes des diagrammes.
    void rebuildDiagrams(const model::Structure& structure,
                         scene::DiagramType type, float scale);

    /// Reconstruit les meshes heatmap.
    void rebuildHeatmap(const model::Structure& structure);

    /// Dessine la scène complète.
    void draw(const Camera& camera, const RenderState& state, float time);

    Grid grid;

private:
    // Shaders
    Shader phongShader_;
    Shader heatmapShader_;
    Shader flatShader_;

    // Mesh de référence pour les nœuds (sphère unitaire)
    Mesh nodeSphere_;

    // Meshes de la scène
    std::vector<MeshInstance> elementMeshes_;
    std::vector<MeshInstance> deformedMeshes_;
    std::vector<MeshInstance> supportMeshes_;
    std::vector<MeshInstance> loadMeshes_;
    std::vector<MeshInstance> diagramFills_;
    std::vector<MeshInstance> diagramLines_;
    std::vector<MeshInstance> heatmapMeshes_;

    // Nœuds : position + couleur
    struct NodeInfo {
        glm::vec3 pos;
        glm::vec3 color;
        int id;
    };
    std::vector<NodeInfo> nodeInfos_;

    // Réactions
    std::vector<MeshInstance> reactionMeshes_;

    void drawPhongInstances(const std::vector<MeshInstance>& instances,
                            const Camera& cam) const;
};
