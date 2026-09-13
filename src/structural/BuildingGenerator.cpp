// =============================================================================
//  BuildingGenerator.cpp — Implémentation du générateur R+2 et de la synchronisation
// =============================================================================

#include "structural/BuildingGenerator.h"
#include <iostream>

namespace stabileo::structural {

void BuildingGenerator::generateBuildingRPlus2(ModelDatabase& db) {
    db.clear();

    // 1. Matériau C25/30
    Material c25;
    c25.name = "Béton C25/30";
    c25.category = MaterialCategory::Concrete;
    c25.E = 31.0e9;   // 31 GPa
    c25.nu = 0.2;
    c25.rho = 2500.0; // 25 kN/m³
    c25.fck = 25.0e6;
    EntityId matId = db.addMaterial(c25);

    // 2. Sections Poteau 30x30 et Poutre 25x50
    Section colSec;
    colSec.name = "POT 30x30";
    colSec.shape = Section::Shape::Rectangle;
    colSec.width = 0.30;
    colSec.height = 0.30;
    colSec.A = 0.09;
    colSec.Iy = 0.30 * 0.30 * 0.30 * 0.30 / 12.0;
    colSec.Iz = colSec.Iy;
    colSec.It = 1.14e-3;
    EntityId colSecId = db.addSection(colSec);

    Section beamSec;
    beamSec.name = "POU 25x50";
    beamSec.shape = Section::Shape::Rectangle;
    beamSec.width = 0.25;
    beamSec.height = 0.50;
    beamSec.A = 0.125;
    beamSec.Iy = 0.25 * 0.50 * 0.50 * 0.50 / 12.0; // 0.002604 m⁴
    beamSec.Iz = 0.50 * 0.25 * 0.25 * 0.25 / 12.0; // 0.000651 m⁴
    beamSec.It = 1.85e-3;
    EntityId beamSecId = db.addSection(beamSec);

    // 3. Niveaux : Z = 0, 3, 6, 9 m
    const std::vector<double> levels = {0.0, 3.0, 6.0, 9.0};

    // Grille de poteaux (3 travées en X de 5m -> X=0, 5, 10, 15m ; 2 travées en Y de 5m -> Y=0, 5, 10m)
    const std::vector<double> xGrid = {0.0, 5.0, 10.0, 15.0};
    const std::vector<double> yGrid = {0.0, 5.0, 10.0};

    // Matrice de nœuds par étage [lvl][ix][iy]
    std::vector<std::vector<std::vector<EntityId>>> gridNodes(levels.size(),
        std::vector<std::vector<EntityId>>(xGrid.size(), std::vector<EntityId>(yGrid.size(), INVALID_ID)));

    for (size_t lvl = 0; lvl < levels.size(); ++lvl) {
        for (size_t ix = 0; ix < xGrid.size(); ++ix) {
            for (size_t iy = 0; iy < yGrid.size(); ++iy) {
                EntityId nId = db.addNode(xGrid[ix], yGrid[iy], levels[lvl]);
                gridNodes[lvl][ix][iy] = nId;

                // Fondations au niveau 0 : Encastrements complets
                if (lvl == 0) {
                    SupportCondition encastrement;
                    encastrement.tx = encastrement.ty = encastrement.tz = true;
                    encastrement.rx = encastrement.ry = encastrement.rz = true;
                    db.addSupport(nId, encastrement);
                }
            }
        }
    }

    // 4. Poteaux verticaux (reliant niveau k à niveau k+1)
    for (size_t lvl = 0; lvl < levels.size() - 1; ++lvl) {
        for (size_t ix = 0; ix < xGrid.size(); ++ix) {
            for (size_t iy = 0; iy < yGrid.size(); ++iy) {
                EntityId nBottom = gridNodes[lvl][ix][iy];
                EntityId nTop    = gridNodes[lvl + 1][ix][iy];
                db.addMember(nBottom, nTop, MemberType::Column, colSecId, matId);
            }
        }
    }

    // 5. Poutres longitudinales (selon X) et transversales (selon Y) à chaque étage (Z = 3, 6, 9m)
    for (size_t lvl = 1; lvl < levels.size(); ++lvl) {
        // Poutres selon X
        for (size_t iy = 0; iy < yGrid.size(); ++iy) {
            for (size_t ix = 0; ix < xGrid.size() - 1; ++ix) {
                EntityId n1 = gridNodes[lvl][ix][iy];
                EntityId n2 = gridNodes[lvl][ix + 1][iy];
                db.addMember(n1, n2, MemberType::Beam, beamSecId, matId);
            }
        }

        // Poutres selon Y
        for (size_t ix = 0; ix < xGrid.size(); ++ix) {
            for (size_t iy = 0; iy < yGrid.size() - 1; ++iy) {
                EntityId n1 = gridNodes[lvl][ix][iy];
                EntityId n2 = gridNodes[lvl][ix][iy + 1];
                db.addMember(n1, n2, MemberType::Beam, beamSecId, matId);
            }
        }

        // 6. Dalles pleines (panneaux 15 cm) sur chaque maille de plancher
        for (size_t ix = 0; ix < xGrid.size() - 1; ++ix) {
            for (size_t iy = 0; iy < yGrid.size() - 1; ++iy) {
                std::vector<EntityId> slabNodes = {
                    gridNodes[lvl][ix][iy],
                    gridNodes[lvl][ix + 1][iy],
                    gridNodes[lvl][ix + 1][iy + 1],
                    gridNodes[lvl][ix][iy + 1]
                };
                db.addPanel(slabNodes, 0.15, matId, PanelType::Slab);
            }
        }
    }

    // 7. Cas de charges & Combinaisons
    EntityId caseG = db.addLoadCase("G - Charges Permanentes (Poids propre + 2.5 kN/m²)", LoadCaseNature::Dead, true);
    EntityId caseQ = db.addLoadCase("Q - Charges d'Exploitation (1.5 kN/m²)", LoadCaseNature::Live, false);

    LoadCombination elu;
    elu.name = "101 : ELU Fondamentale (1.35G + 1.5Q)";
    elu.type = LoadCombination::Type::ULS;
    elu.factors.push_back({caseG, 1.35});
    elu.factors.push_back({caseQ, 1.50});
    db.addCombination(elu);

    LoadCombination els;
    els.name = "102 : ELS Quasi-permanent (1.0G + 1.0Q)";
    els.type = LoadCombination::Type::SLS;
    els.factors.push_back({caseG, 1.00});
    els.factors.push_back({caseQ, 1.00});
    db.addCombination(els);
}

void BuildingGenerator::syncToLegacyStructure(const ModelDatabase& db, model::Structure& outStructure) {
    outStructure.name = "Bâtiment R+2 (15m x 10m - C25/30)";
    outStructure.nodes.clear();
    outStructure.elements.clear();
    outStructure.sections.clear();
    outStructure.materials.clear();
    outStructure.supports.clear();
    outStructure.nodalLoads.clear();
    outStructure.distributedLoads.clear();

    // Map EntityId -> index interne
    std::unordered_map<EntityId, int> nodeIdMap;
    std::unordered_map<EntityId, int> sectionIdMap;
    std::unordered_map<EntityId, int> materialIdMap;

    // 1. Matériaux
    for (const auto& [id, m] : db.getMaterials()) {
        model::Material mat;
        mat.id = static_cast<int>(id);
        mat.name = m.name;
        mat.E = static_cast<float>(m.E / 1e6); // Pa -> MPa
        mat.nu = static_cast<float>(m.nu);
        mat.rho = static_cast<float>(m.rho * 9.81 / 1000.0); // kg/m³ -> kN/m³
        mat.fy = static_cast<float>(m.fck / 1e6);
        outStructure.materials.push_back(mat);
        materialIdMap[id] = static_cast<int>(outStructure.materials.size() - 1);
    }

    // 2. Sections
    for (const auto& [id, s] : db.getSections()) {
        model::Section sec;
        sec.id = static_cast<int>(id);
        sec.name = s.name;
        sec.type = model::SectionType::RECT_SOLID;
        sec.b = static_cast<float>(s.width);
        sec.h = static_cast<float>(s.height);
        sec.A = static_cast<float>(s.A);
        sec.Iy = static_cast<float>(s.Iy);
        sec.Iz = static_cast<float>(s.Iz);
        sec.E = 31000.0f; // MPa
        outStructure.sections.push_back(sec);
        sectionIdMap[id] = static_cast<int>(outStructure.sections.size() - 1);
    }

    // 3. Nœuds
    for (const auto& [id, n] : db.getNodes()) {
        model::Node node;
        node.id = static_cast<int>(id);
        node.position = glm::vec3(static_cast<float>(n.position.x),
                                  static_cast<float>(n.position.y),
                                  static_cast<float>(n.position.z));
        outStructure.nodes.push_back(node);
        nodeIdMap[id] = static_cast<int>(outStructure.nodes.size() - 1);
    }

    // 4. Appuis
    for (const auto& [id, s] : db.getSupports()) {
        model::Support supp;
        supp.nodeId = static_cast<int>(s.nodeId);
        supp.type = model::SupportType::FIXED;
        supp.blockRx = s.condition.rx;
        supp.blockRy = s.condition.ry;
        supp.blockRz = s.condition.rz;
        outStructure.supports.push_back(supp);
    }

    // 5. Éléments (Barres)
    int elemIdx = 1;
    for (const auto& [id, m] : db.getMembers()) {
        model::Element elem;
        elem.id = elemIdx++;
        elem.nodeI = static_cast<int>(m.startNodeId);
        elem.nodeJ = static_cast<int>(m.endNodeId);
        elem.sectionId = static_cast<int>(m.sectionId);
        elem.materialId = static_cast<int>(m.materialId);
        elem.rollAngle = static_cast<float>(m.rollAngleDeg);
        elem.hingeStart = m.startReleases.ry || m.startReleases.rz;
        elem.hingeEnd = m.endReleases.ry || m.endReleases.rz;

        outStructure.elements.push_back(elem);

        // Charge répartie descendante par défaut sur les poutres (poids + plancher)
        if (m.type == MemberType::Beam) {
            model::DistributedLoad dLoad;
            dLoad.elementId = elem.id;
            // qz = -18 kN/m en repère global/local
            dLoad.wStart = glm::vec3(0.0f, -18.0f, 0.0f);
            dLoad.wEnd   = glm::vec3(0.0f, -18.0f, 0.0f);
            outStructure.distributedLoads.push_back(dLoad);
        }
    }
}

} // namespace stabileo::structural
