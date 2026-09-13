#pragma once
// =============================================================================
//  LoadGizmos.h — Flèches 3D pour forces, moments et charges réparties
// =============================================================================

#include "core/Mesh.h"
#include "scene/StructureModel.h"
#include <vector>

namespace scene {

/// Crée une flèche 3D pour une force nodale (direction + magnitude).
Mesh createForceArrow(const glm::vec3& direction, float magnitude, float maxMag);

/// Crée un arc fléché 3D pour un moment.
Mesh createMomentArc(const glm::vec3& axis, float magnitude, float maxMag);

/// Crée un peigne de flèches pour une charge répartie sur un élément.
Mesh createDistributedLoadComb(
    const model::DistributedLoad& load,
    const glm::vec3& posI, const glm::vec3& posJ,
    const glm::vec3& localY, const glm::vec3& localZ,
    float maxMag, int numArrows = 8);

} // namespace scene
