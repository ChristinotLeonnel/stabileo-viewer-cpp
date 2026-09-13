#pragma once
// =============================================================================
//  ProfileExtruder.h — Génération de maillages 3D de profilés structurels
// =============================================================================

#include "core/Mesh.h"
#include "scene/StructureModel.h"
#include <vector>

namespace scene {

/// Génère un contour 2D (Y, Z) pour un type de section donné.
/// Retourne les points et normales 2D du contour fermé.
void generateSectionContour(
    const model::Section& section,
    std::vector<glm::vec2>& outPoints,
    std::vector<glm::vec2>& outNormals);

/// Calcule le repère local d'un élément (xLocal, yLocal, zLocal).
/// xLocal = direction de la barre, yLocal = axe fort, zLocal = axe faible.
void computeLocalFrame(
    const glm::vec3& posI, const glm::vec3& posJ,
    float rollAngleDeg,
    glm::vec3& outX, glm::vec3& outY, glm::vec3& outZ);

/// Extrude un profilé le long d'un élément (droit ou déformé).
/// axisPoints : positions le long de l'axe neutre (≥ 2 points).
Mesh extrudeProfile(
    const model::Section& section,
    const std::vector<glm::vec3>& axisPoints,
    const std::vector<glm::vec3>& axisUp,
    const std::vector<glm::vec3>& axisFwd);

/// Extrude un profilé le long de la déformée (Hermite).
/// scale : facteur d'amplification de la déformation.
Mesh extrudeProfileDeformed(
    const model::Section& section,
    const model::Element& element,
    const model::Node& nI, const model::Node& nJ,
    float scale, int numStations = 20);

} // namespace scene
