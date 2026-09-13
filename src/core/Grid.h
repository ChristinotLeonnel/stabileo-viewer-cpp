#pragma once
// =============================================================================
//  Grid.h — Grille de référence au sol et trièdre d'axes
// =============================================================================

#include "core/Mesh.h"
#include "core/Shader.h"

class Grid {
public:
    /// Génère la grille au plan XZ (sol structurel Y=0).
    void init(float halfSize = 50.0f, float spacing = 1.0f);

    /// Dessine la grille (appeler après avoir activé le shader flat).
    void draw(const Shader& shader) const;

    /// Génère le trièdre d'orientation (X rouge, Y vert, Z bleu).
    void initAxesTriad();

    /// Dessine le trièdre dans un coin de l'écran (viewport indépendant).
    void drawAxesTriad(const Shader& shader,
                       const glm::mat4& viewRotOnly,
                       float vpW, float vpH) const;

private:
    Mesh gridMesh_;
    Mesh axesMesh_;
};
