#pragma once
// =============================================================================
//  StructuralModelPanel.h — Panneau de modélisation CAO/FEM (Workflow style Robot)
// =============================================================================

#include "structural/ModelDatabase.h"
#include "commands/CommandManager.h"
#include "tools/SnapEngine.h"
#include "tools/DrawingTools.h"
#include "scene/StructureModel.h"

namespace stabileo::ui {

class StructuralModelPanel {
public:
    StructuralModelPanel();

    void draw(model::Structure& activeStructure, bool* p_open = nullptr);

    structural::ModelDatabase& getDatabase() { return db_; }
    const structural::ModelDatabase& getDatabase() const { return db_; }

    commands::CommandManager& getCommandManager() { return cmdMgr_; }
    tools::SnapEngine& getSnapEngine() { return snapEngine_; }
    tools::DrawingToolsManager& getDrawingTools() { return drawingTools_; }

    // Synchronise vers le modèle de rendu et de calcul
    void syncStructure(model::Structure& outStructure);

private:
    void drawGeometryTab(model::Structure& structure);
    void drawSectionsTab();
    void drawMaterialsTab();
    void drawSupportsTab();
    void drawLoadsTab();
    void drawBuildingR2Tab(model::Structure& structure);
    void drawProjectIoTab(model::Structure& structure);

    structural::ModelDatabase   db_;
    commands::CommandManager    cmdMgr_;
    tools::SnapEngine           snapEngine_;
    tools::DrawingToolsManager  drawingTools_;

    // Champs de saisie numérique directe de nœuds
    double inputNodeX_ = 0.0;
    double inputNodeY_ = 0.0;
    double inputNodeZ_ = 0.0;

    // Saisie rapide barre
    double inputBarStartX_ = 0.0, inputBarStartY_ = 0.0, inputBarStartZ_ = 0.0;
    double inputBarEndX_   = 5.0, inputBarEndY_   = 0.0, inputBarEndZ_   = 0.0;
    int    inputBarType_   = 0; // 0=Column, 1=Beam, 2=Truss

    char projectPath_[256] = "mon_batiment.tsa";
    std::string statusMessage_ = "Prêt. Modélisation active.";
};

} // namespace stabileo::ui
