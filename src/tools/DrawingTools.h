#pragma once
// =============================================================================
//  DrawingTools.h — Machines à états de dessin interactif (Poteau, Poutre, Dalle)
// =============================================================================

#include "tools/SnapEngine.h"
#include "commands/CommandManager.h"
#include "commands/StructuralCommands.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace stabileo::tools {

enum class ToolMode {
    Select,
    DrawColumn, // 2 clics : A -> B
    DrawBeam,   // 2 clics : A -> B
    DrawTruss,  // 2 clics : A -> B
    DrawSlab    // N clics : contour polygonal fermé
};

class DrawingToolsManager {
public:
    DrawingToolsManager(structural::ModelDatabase& db, commands::CommandManager& cmdMgr);

    void setMode(ToolMode mode);
    ToolMode getMode() const { return currentMode_; }

    // Déclenche un clic 3D (avec point d'accrochage)
    void onClick(const glm::dvec3& snappedPoint);
    void onCancel();

    // Propriétés de l'outil courant
    bool isDrawing() const;
    std::string getPromptMessage() const;

    // Points temporaires en cours de dessin (pour aperçu visuel dans le viewport)
    const std::vector<glm::dvec3>& getTemporaryPoints() const { return tempPoints_; }

    // Paramètres actifs pour les nouveaux éléments créés
    void setActiveSection(structural::EntityId id) { activeSectionId_ = id; }
    void setActiveMaterial(structural::EntityId id) { activeMaterialId_ = id; }
    void setActiveThickness(double t) { activeThickness_ = t; }

    structural::EntityId getActiveSection() const { return activeSectionId_; }
    structural::EntityId getActiveMaterial() const { return activeMaterialId_; }
    double getActiveThickness() const { return activeThickness_; }

private:
    structural::ModelDatabase& db_;
    commands::CommandManager&  cmdMgr_;

    ToolMode currentMode_ = ToolMode::Select;
    std::vector<glm::dvec3> tempPoints_;

    structural::EntityId activeSectionId_  = structural::INVALID_ID;
    structural::EntityId activeMaterialId_ = structural::INVALID_ID;
    double activeThickness_ = 0.15; // 15 cm par défaut
};

} // namespace stabileo::tools
