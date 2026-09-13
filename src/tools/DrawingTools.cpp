// =============================================================================
//  DrawingTools.cpp — Implémentation des outils de dessin interactifs
// =============================================================================

#include "tools/DrawingTools.h"

namespace stabileo::tools {

DrawingToolsManager::DrawingToolsManager(structural::ModelDatabase& db, commands::CommandManager& cmdMgr)
    : db_(db), cmdMgr_(cmdMgr) {}

void DrawingToolsManager::setMode(ToolMode mode) {
    currentMode_ = mode;
    tempPoints_.clear();
}

void DrawingToolsManager::onCancel() {
    tempPoints_.clear();
}

bool DrawingToolsManager::isDrawing() const {
    return !tempPoints_.empty();
}

std::string DrawingToolsManager::getPromptMessage() const {
    switch (currentMode_) {
        case ToolMode::Select:
            return "Sélection : Cliquez sur un élément pour éditer ses propriétés.";
        case ToolMode::DrawColumn:
            return tempPoints_.empty() ? "Poteau : Cliquez pour définir la base (Point A)"
                                       : "Poteau : Cliquez pour définir la tête (Point B)";
        case ToolMode::DrawBeam:
            return tempPoints_.empty() ? "Poutre : Cliquez pour définir le début (Point A)"
                                       : "Poutre : Cliquez pour définir la fin (Point B)";
        case ToolMode::DrawTruss:
            return tempPoints_.empty() ? "Treillis : Cliquez sur le début (Point A)"
                                       : "Treillis : Cliquez sur la fin (Point B)";
        case ToolMode::DrawSlab:
            return "Dalle : Cliquez successivement sur les sommets. Cliquez près du premier sommet pour fermer le contour.";
    }
    return "";
}

void DrawingToolsManager::onClick(const glm::dvec3& snappedPoint) {
    if (currentMode_ == ToolMode::Select) {
        return;
    }

    // 1. Outils Barres (Poteau, Poutre, Treillis) : 2 clics
    if (currentMode_ == ToolMode::DrawColumn || currentMode_ == ToolMode::DrawBeam || currentMode_ == ToolMode::DrawTruss) {
        if (tempPoints_.empty()) {
            glm::dvec3 p1 = tempPoints_[0];
            glm::dvec3 p2 = snappedPoint;
            glm::dvec3 diff = p2 - p1;
            if (glm::dot(diff, diff) > 1e-4) {
                structural::MemberType mType = structural::MemberType::Generic;
                if (currentMode_ == ToolMode::DrawColumn) mType = structural::MemberType::Column;
                else if (currentMode_ == ToolMode::DrawBeam) mType = structural::MemberType::Beam;
                else if (currentMode_ == ToolMode::DrawTruss) mType = structural::MemberType::Truss;

                cmdMgr_.executeCommand(std::make_unique<commands::CreateMemberCommand>(
                    db_, p1, p2, mType, activeSectionId_, activeMaterialId_));
            }
            tempPoints_.clear();
        }
        return;
    }

    // 2. Outil Dalle / Panneau : Polygone fermé
    if (currentMode_ == ToolMode::DrawSlab) {
        if (tempPoints_.empty()) {
            tempPoints_.push_back(snappedPoint);
        } else {
            // Vérifier si l'utilisateur clique sur le premier point pour fermer la dalle
            glm::dvec3 pFirst = tempPoints_[0];
            glm::dvec3 dClose = snappedPoint - pFirst;
            if (tempPoints_.size() >= 3 && glm::dot(dClose, dClose) < 0.25) { // < 50 cm
                // Fermer le contour et créer la dalle
                cmdMgr_.executeCommand(std::make_unique<commands::CreatePanelCommand>(
                    db_, tempPoints_, activeThickness_, activeMaterialId_, structural::PanelType::Slab));
                tempPoints_.clear();
            } else {
                tempPoints_.push_back(snappedPoint);
            }
        }
    }
}

} // namespace stabileo::tools
