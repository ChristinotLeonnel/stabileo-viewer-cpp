#pragma once
// =============================================================================
//  DiagramMesh.h — Maillages ruban 3D pour les diagrammes d'efforts internes
// =============================================================================

#include "core/Mesh.h"
#include "scene/StructureModel.h"

namespace scene {

enum class DiagramType { N, Vy, Vz, My, Mz, T };

/// Crée le mesh ruban d'un diagramme d'effort le long d'un élément.
/// Le ruban s'étend depuis l'axe neutre dans la direction perpendiculaire.
/// scale : échelle d'affichage du diagramme.
Mesh createDiagramRibbon(
    const model::Element& element,
    const model::Node& nI, const model::Node& nJ,
    DiagramType type, float scale);

/// Crée la ligne de contour du diagramme (pour le bord opaque).
Mesh createDiagramOutline(
    const model::Element& element,
    const model::Node& nI, const model::Node& nJ,
    DiagramType type, float scale);

/// Retourne les valeurs du diagramme demandé pour un élément.
const std::vector<float>& getDiagramValues(
    const model::Element& element, DiagramType type);

} // namespace scene
