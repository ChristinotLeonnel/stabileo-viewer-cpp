// =============================================================================
//  StructuralModelPanel.cpp — Implémentation du panneau de modélisation CAO/FEM
// =============================================================================

#include "ui/StructuralModelPanel.h"
#include "structural/BuildingGenerator.h"
#include "io/ProjectSerializer.h"
#include <imgui.h>
#include <iostream>

namespace stabileo::ui {

StructuralModelPanel::StructuralModelPanel()
    : drawingTools_(db_, cmdMgr_) {}

void StructuralModelPanel::syncStructure(model::Structure& outStructure) {
    structural::BuildingGenerator::syncToLegacyStructure(db_, outStructure);
}

void StructuralModelPanel::draw(model::Structure& activeStructure, bool* p_open) {
    if (p_open && !*p_open) return;

    if (!ImGui::Begin("Modélisation Structurale (Robot CAO)###StructuralModeler", p_open)) {
        ImGui::End();
        return;
    }

    // Bandeau d'en-tête
    ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "Workflow de Modélisation Structurale & Génie Civil");
    ImGui::TextDisabled("Géométrie → Sections → Matériaux → Appuis → Charges → Combinaisons → Bâtiment R+2");
    ImGui::Separator();

    // Barre d'outils Undo / Redo & Synchronisation
    if (ImGui::Button(" ↩ Annuler ") && cmdMgr_.canUndo()) {
        cmdMgr_.undo();
        syncStructure(activeStructure);
        statusMessage_ = "Annulé : " + cmdMgr_.getRedoName();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Annuler la dernière action (%s)", cmdMgr_.getUndoName().c_str());

    ImGui::SameLine();
    if (ImGui::Button(" ↪ Rétablir ") && cmdMgr_.canRedo()) {
        cmdMgr_.redo();
        syncStructure(activeStructure);
        statusMessage_ = "Rétabli : " + cmdMgr_.getUndoName();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rétablir l'action annulée (%s)", cmdMgr_.getRedoName().c_str());

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.50f, 0.25f, 0.85f));
    if (ImGui::Button(" Mettre à jour la Vue 3D ")) {
        syncStructure(activeStructure);
        statusMessage_ = "Vue 3D synchronisée (" + std::to_string(db_.getNodes().size()) + " nœuds, " +
                         std::to_string(db_.getMembers().size()) + " barres, " +
                         std::to_string(db_.getPanels().size()) + " panneaux).";
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Onglets principaux du workflow Robot Structural Analysis
    if (ImGui::BeginTabBar("WorkflowTabs")) {
        if (ImGui::BeginTabItem("1. Géométrie & Dessin")) {
            drawGeometryTab(activeStructure);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("2. Sections")) {
            drawSectionsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("3. Matériaux")) {
            drawMaterialsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("4. Appuis")) {
            drawSupportsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("5. Charges & Combinaisons")) {
            drawLoadsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("6. Bâtiment R+2 (15x10m)")) {
            drawBuildingR2Tab(activeStructure);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("7. Projet (.tsa)")) {
            drawProjectIoTab(activeStructure);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Statut : %s", statusMessage_.c_str());

    ImGui::End();
}

void StructuralModelPanel::drawGeometryTab(model::Structure& structure) {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Mode d'Outil Actif :");

    tools::ToolMode current = drawingTools_.getMode();
    if (ImGui::RadioButton("Sélection", current == tools::ToolMode::Select)) drawingTools_.setMode(tools::ToolMode::Select);
    ImGui::SameLine();
    if (ImGui::RadioButton("Poteau", current == tools::ToolMode::DrawColumn)) drawingTools_.setMode(tools::ToolMode::DrawColumn);
    ImGui::SameLine();
    if (ImGui::RadioButton("Poutre", current == tools::ToolMode::DrawBeam)) drawingTools_.setMode(tools::ToolMode::DrawBeam);
    ImGui::SameLine();
    if (ImGui::RadioButton("Treillis", current == tools::ToolMode::DrawTruss)) drawingTools_.setMode(tools::ToolMode::DrawTruss);
    ImGui::SameLine();
    if (ImGui::RadioButton("Dalle / Panneau", current == tools::ToolMode::DrawSlab)) drawingTools_.setMode(tools::ToolMode::DrawSlab);

    ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Consigne : %s", drawingTools_.getPromptMessage().c_str());
    ImGui::Separator();

    // Saisie numérique directe (Coordonnées précises)
    if (ImGui::CollapsingHeader("Saisie Numérique Directe de Nœud (X, Y, Z)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SetNextItemWidth(90);
        ImGui::InputDouble("X (m)", &inputNodeX_, 0.5, 1.0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::InputDouble("Y (m)", &inputNodeY_, 0.5, 1.0, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::InputDouble("Z (m)", &inputNodeZ_, 0.5, 1.0, "%.2f");
        ImGui::SameLine();

        if (ImGui::Button("Créer Nœud")) {
            cmdMgr_.executeCommand(std::make_unique<commands::CreateNodeCommand>(
                db_, inputNodeX_, inputNodeY_, inputNodeZ_));
            syncStructure(structure);
            statusMessage_ = "Nœud créé en (" + std::to_string(inputNodeX_) + ", " +
                             std::to_string(inputNodeY_) + ", " + std::to_string(inputNodeZ_) + ")";
        }
    }

    // Tracé direct par coordonnées (Point Début -> Point Fin)
    if (ImGui::CollapsingHeader("Créer Barre par Coordonnées Début / Fin", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Combo("Type", &inputBarType_, "Poteau (Column)\0Poutre (Beam)\0Treillis (Truss)\0");

        ImGui::Text("Point A (Début) :");
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("Ax", &inputBarStartX_, 0.5, 1.0, "%.2f"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("Ay", &inputBarStartY_, 0.5, 1.0, "%.2f"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("Az", &inputBarStartZ_, 0.5, 1.0, "%.2f");

        ImGui::Text("Point B (Fin) :");
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("Bx", &inputBarEndX_, 0.5, 1.0, "%.2f"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("By", &inputBarEndY_, 0.5, 1.0, "%.2f"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("Bz", &inputBarEndZ_, 0.5, 1.0, "%.2f"); ImGui::SameLine();

        if (ImGui::Button("Générer la Barre")) {
            structural::MemberType mType = structural::MemberType::Generic;
            if (inputBarType_ == 0) mType = structural::MemberType::Column;
            else if (inputBarType_ == 1) mType = structural::MemberType::Beam;
            else if (inputBarType_ == 2) mType = structural::MemberType::Truss;

            cmdMgr_.executeCommand(std::make_unique<commands::CreateMemberCommand>(
                db_, glm::dvec3(inputBarStartX_, inputBarStartY_, inputBarStartZ_),
                glm::dvec3(inputBarEndX_, inputBarEndY_, inputBarEndZ_), mType));
            syncStructure(structure);
            statusMessage_ = "Barre créée entre Point A et Point B.";
        }
    }

    // Paramètres d'accrochage (Snap) et Grille
    if (ImGui::CollapsingHeader("Paramètres de la Grille & Accrochage (Snap)")) {
        float sp = static_cast<float>(snapEngine_.getGridSpacing());
        if (ImGui::SliderFloat("Espacement Grille (m)", &sp, 0.25f, 5.0f, "%.2f m")) {
            snapEngine_.setGridSpacing(sp);
        }

        double alt = snapEngine_.getGridAltitude();
        if (ImGui::InputDouble("Altitude Plan de Travail Z (m)", &alt, 0.5, 3.0, "%.2f m")) {
            snapEngine_.setGridAltitude(alt);
        }

        bool sG = snapEngine_.isSnapToGrid();
        if (ImGui::Checkbox("Accrochage Grille", &sG)) snapEngine_.setSnapToGrid(sG);
        ImGui::SameLine();
        bool sN = snapEngine_.isSnapToNodes();
        if (ImGui::Checkbox("Accrochage Nœuds", &sN)) snapEngine_.setSnapToNodes(sN);
        ImGui::SameLine();
        bool sM = snapEngine_.isSnapToMidpoints();
        if (ImGui::Checkbox("Accrochage Milieux", &sM)) snapEngine_.setSnapToMidpoints(sM);
    }
}

void StructuralModelPanel::drawSectionsTab() {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Catalogue des Profilés & Sections Transversales");
    ImGui::Separator();

    for (const auto& [id, sec] : db_.getSections()) {
        ImGui::BulletText("ID #%llu : %s (b=%.2f m, h=%.2f m, A=%.4f m², Iy=%.2e m⁴)",
                          id, sec.name.c_str(), sec.width, sec.height, sec.A, sec.Iy);
    }

    ImGui::Spacing();
    static char newSecName[64] = "NOUV 30x60";
    static double newWidth = 0.30, newHeight = 0.60;
    ImGui::InputText("Nom", newSecName, sizeof(newSecName));
    ImGui::InputDouble("Largeur b (m)", &newWidth, 0.05, 0.10, "%.2f");
    ImGui::InputDouble("Hauteur h (m)", &newHeight, 0.05, 0.10, "%.2f");

    if (ImGui::Button("Ajouter cette Section au Catalogue")) {
        structural::Section s;
        s.name = newSecName;
        s.width = newWidth;
        s.height = newHeight;
        s.A = newWidth * newHeight;
        s.Iy = newWidth * newHeight * newHeight * newHeight / 12.0;
        s.Iz = newHeight * newWidth * newWidth * newWidth / 12.0;
        s.It = s.Iy * 0.4;
        db_.addSection(s);
        statusMessage_ = "Section " + std::string(newSecName) + " ajoutée.";
    }
}

void StructuralModelPanel::drawMaterialsTab() {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Catalogue des Matériaux");
    ImGui::Separator();

    for (const auto& [id, mat] : db_.getMaterials()) {
        ImGui::BulletText("ID #%llu : %s (E = %.1f GPa, nu = %.2f, rho = %.0f kg/m³, fck = %.1f MPa)",
                          id, mat.name.c_str(), mat.E / 1e9, mat.nu, mat.rho, mat.fck / 1e6);
    }
}

void StructuralModelPanel::drawSupportsTab() {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Conditions aux Limites & Appuis");
    ImGui::Separator();

    ImGui::Text("Nombre d'appuis définis : %zu", db_.getSupports().size());

    static int targetNodeId = 1;
    ImGui::InputInt("Nœud Cible ID", &targetNodeId);

    static bool fixTx = true, fixTy = true, fixTz = true;
    static bool fixRx = true, fixRy = true, fixRz = true;

    ImGui::Checkbox("Bloquer Tx", &fixTx); ImGui::SameLine();
    ImGui::Checkbox("Bloquer Ty", &fixTy); ImGui::SameLine();
    ImGui::Checkbox("Bloquer Tz", &fixTz);

    ImGui::Checkbox("Bloquer Rx", &fixRx); ImGui::SameLine();
    ImGui::Checkbox("Bloquer Ry", &fixRy); ImGui::SameLine();
    ImGui::Checkbox("Bloquer Rz", &fixRz);

    if (ImGui::Button("Assigner l'Appui au Nœud")) {
        structural::SupportCondition cond;
        cond.tx = fixTx; cond.ty = fixTy; cond.tz = fixTz;
        cond.rx = fixRx; cond.ry = fixRy; cond.rz = fixRz;

        cmdMgr_.executeCommand(std::make_unique<commands::AssignSupportCommand>(
            db_, static_cast<structural::EntityId>(targetNodeId), cond));
        statusMessage_ = "Appui assigné au nœud #" + std::to_string(targetNodeId);
    }
}

void StructuralModelPanel::drawLoadsTab() {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Cas de Charges & Combinaisons Réglementaires");
    ImGui::Separator();

    ImGui::Text("Cas de Charges définis :");
    for (const auto& [id, lc] : db_.getLoadCases()) {
        ImGui::BulletText("ID #%llu : %s (Poids propre inclus: %s)",
                          id, lc.name.c_str(), lc.includeSelfWeight ? "Oui" : "Non");
    }

    ImGui::Spacing();
    ImGui::Text("Combinaisons d'Actions :");
    for (const auto& [id, combo] : db_.getCombinations()) {
        ImGui::BulletText("ID #%llu : %s", id, combo.name.c_str());
    }
}

void StructuralModelPanel::drawBuildingR2Tab(model::Structure& structure) {
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "Générateur Paramétrique Bâtiment R+2");
    ImGui::TextWrapped("Génère automatiquement la structure complète selon les spécifications :");
    ImGui::BulletText("Dimensions au sol : 15.00 m × 10.00 m (3 travées X × 2 travées Y)");
    ImGui::BulletText("4 Niveaux : Z = 0.00 m, 3.00 m, 6.00 m, 9.00 m");
    ImGui::BulletText("Poteaux : Béton 30 × 30 cm (36 poteaux au total)");
    ImGui::BulletText("Poutres : Béton 25 × 50 cm (75 m de poutres par niveau)");
    ImGui::BulletText("Dalles : Dalles pleines 15 cm avec charge permanente et exploitation");
    ImGui::BulletText("Fondations : 12 appuis encastrés parfaits au niveau Z = 0");
    ImGui::BulletText("Matériau : Béton C25/30 (E = 31 GPa, fck = 25 MPa)");
    ImGui::BulletText("Combinaisons : ELU (1.35G + 1.5Q) et ELS (1.0G + 1.0Q)");

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.55f, 0.28f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.68f, 0.35f, 1.00f));
    if (ImGui::Button(" GÉNÉRER LE BÂTIMENT R+2 EN 1 CLIC ", ImVec2(-1, 38))) {
        structural::BuildingGenerator::generateBuildingRPlus2(db_);
        syncStructure(structure);
        statusMessage_ = "Bâtiment R+2 généré avec succès (48 nœuds, 81 barres, 18 dalles) !";
    }
    ImGui::PopStyleColor(2);
}

void StructuralModelPanel::drawProjectIoTab(model::Structure& structure) {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Persistance de Projet (.tsa / .json)");
    ImGui::Separator();

    ImGui::InputText("Nom du Fichier", projectPath_, sizeof(projectPath_));

    if (ImGui::Button(" 💾 Enregistrer Projet (.tsa) ")) {
        if (io::ProjectSerializer::saveToFile(db_, projectPath_)) {
            statusMessage_ = "Projet enregistré dans : " + std::string(projectPath_);
        } else {
            statusMessage_ = "Erreur lors de l'enregistrement du projet !";
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(" 📂 Ouvrir Projet (.tsa) ")) {
        if (io::ProjectSerializer::loadFromFile(db_, projectPath_)) {
            syncStructure(structure);
            statusMessage_ = "Projet chargé depuis : " + std::string(projectPath_);
        } else {
            statusMessage_ = "Erreur lors du chargement du fichier !";
        }
    }
}

} // namespace stabileo::ui
