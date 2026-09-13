#pragma once
// =============================================================================
//  SnapEngine.h — Moteur d'accrochage 3D (Grille, Nœuds, Barres, Milieux)
// =============================================================================

#include "structural/ModelDatabase.h"
#include <glm/glm.hpp>

namespace stabileo::tools {

enum class SnapType {
    None,
    Grid,          // Intersection grille
    Node,          // Nœud existant
    MemberMidpoint // Milieu de barre
};

struct SnapResult {
    bool snapped = false;
    SnapType type = SnapType::None;
    glm::dvec3 point{0.0};
    structural::EntityId targetId = structural::INVALID_ID;
    std::string description;
};

class SnapEngine {
public:
    SnapEngine() = default;

    void setGridSpacing(double spacing) { gridSpacing_ = spacing; }
    double getGridSpacing() const { return gridSpacing_; }

    void setGridAltitude(double z) { currentZ_ = z; }
    double getGridAltitude() const { return currentZ_; }

    void setSnapToGrid(bool enabled) { snapGrid_ = enabled; }
    void setSnapToNodes(bool enabled) { snapNodes_ = enabled; }
    void setSnapToMidpoints(bool enabled) { snapMidpoints_ = enabled; }

    bool isSnapToGrid() const { return snapGrid_; }
    bool isSnapToNodes() const { return snapNodes_; }
    bool isSnapToMidpoints() const { return snapMidpoints_; }

    // Projette les coordonnées d'un point dans l'espace vers le point d'accrochage le plus proche
    SnapResult snapPoint(const glm::dvec3& rawPos, const structural::ModelDatabase& db,
                         double maxSnapDistance = 0.50) const;

    // Arrondit un point sur la grille 3D courante
    glm::dvec3 snapToGridOnly(const glm::dvec3& rawPos) const;

private:
    double gridSpacing_ = 1.0; // Grille de 1 m par défaut
    double currentZ_ = 0.0;    // Plan de travail Z courant
    bool snapGrid_ = true;
    bool snapNodes_ = true;
    bool snapMidpoints_ = true;
};

} // namespace stabileo::tools
