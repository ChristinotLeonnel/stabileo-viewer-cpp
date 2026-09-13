// =============================================================================
//  SnapEngine.cpp — Moteur d'accrochage 3D
// =============================================================================

#include "tools/SnapEngine.h"
#include <cmath>
#include <limits>

namespace stabileo::tools {

glm::dvec3 SnapEngine::snapToGridOnly(const glm::dvec3& rawPos) const {
    if (gridSpacing_ <= 1e-4) return rawPos;

    double sx = std::round(rawPos.x / gridSpacing_) * gridSpacing_;
    double sy = std::round(rawPos.y / gridSpacing_) * gridSpacing_;
    double sz = currentZ_; // Aligné sur l'altitude de travail active
    return glm::dvec3(sx, sy, sz);
}

SnapResult SnapEngine::snapPoint(const glm::dvec3& rawPos, const structural::ModelDatabase& db,
                                 double maxSnapDistance) const {
    SnapResult result;
    double bestDistSq = maxSnapDistance * maxSnapDistance;

    // 1. Accrochage aux Nœuds existants (Priorité maximale)
    if (snapNodes_) {
        for (const auto& [id, node] : db.getNodes()) {
            double distSq = glm::dot(node.position - rawPos, node.position - rawPos);
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                result.snapped = true;
                result.type = SnapType::Node;
                result.point = node.position;
                result.targetId = id;
                result.description = "Nœud #" + std::to_string(id);
            }
        }
    }

    // 2. Accrochage aux Milieux de barres
    if (snapMidpoints_) {
        for (const auto& [id, mem] : db.getMembers()) {
            const auto* n1 = db.getNode(mem.startNodeId);
            const auto* n2 = db.getNode(mem.endNodeId);
            if (n1 && n2) {
                glm::dvec3 mid = (n1->position + n2->position) * 0.5;
                double distSq = glm::dot(mid - rawPos, mid - rawPos);
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    result.snapped = true;
                    result.type = SnapType::MemberMidpoint;
                    result.point = mid;
                    result.targetId = id;
                    result.description = "Milieu Barre #" + std::to_string(id);
                }
            }
        }
    }

    // 3. Accrochage à la Grille (si aucun objet proche)
    if (!result.snapped && snapGrid_) {
        glm::dvec3 gridPt = snapToGridOnly(rawPos);
        double distSq = glm::dot(gridPt - rawPos, gridPt - rawPos);
        if (distSq <= (gridSpacing_ * gridSpacing_ * 1.5)) {
            result.snapped = true;
            result.type = SnapType::Grid;
            result.point = gridPt;
            result.description = "Grille (" + std::to_string(gridPt.x) + ", " +
                                 std::to_string(gridPt.y) + ", " + std::to_string(gridPt.z) + ")";
        }
    }

    if (!result.snapped) {
        result.point = rawPos;
    }

    return result;
}

} // namespace stabileo::tools
