#pragma once
// =============================================================================
//  ViewCube.h — Widget 3D interactif d'orientation spatiale (style Autodesk Robot)
// =============================================================================

#include "core/Camera.h"
#include <imgui.h>

namespace ui {

class ViewCube {
public:
    ViewCube() = default;

    /// Dessine le ViewCube interactif en superposition 3D.
    /// Retourne true si une interaction utilisateur a déclenché un changement de vue.
    bool draw(Camera& camera, const glm::vec3& boundsMin, const glm::vec3& boundsMax, float posX = -1.0f, float posY = -1.0f);

    float cubeSize       = 95.0f; // Dimension en pixels
    bool  showCompass    = true;  // Anneau boussole Nord/Sud/Est/Ouest
    bool  showHomeButton = true;  // Bouton Maison pour recadrer la scène
    bool  showViewType   = true;  // Bascule Perspective / Orthographique

private:
    int hoveredPart_ = -1; // -1: aucun, 0..5: faces, 6..13: coins
};

} // namespace ui
