// =============================================================================
//  ProjectSerializer.cpp — Implémentation JSON de la persistance de projet
// =============================================================================

#include "io/ProjectSerializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace stabileo::io {

bool ProjectSerializer::saveToFile(const structural::ModelDatabase& db, const std::string& filePath) {
    try {
        json root;
        root["format"] = "StabileoProject";
        root["version"] = "2026.1";

        // 1. Matériaux
        json jMaterials = json::array();
        for (const auto& [id, m] : db.getMaterials()) {
            json jm;
            jm["id"] = m.id;
            jm["name"] = m.name;
            jm["category"] = static_cast<int>(m.category);
            jm["E"] = m.E;
            jm["nu"] = m.nu;
            jm["rho"] = m.rho;
            jm["fck"] = m.fck;
            jMaterials.push_back(jm);
        }
        root["materials"] = jMaterials;

        // 2. Sections
        json jSections = json::array();
        for (const auto& [id, s] : db.getSections()) {
            json js;
            js["id"] = s.id;
            js["name"] = s.name;
            js["shape"] = static_cast<int>(s.shape);
            js["width"] = s.width;
            js["height"] = s.height;
            js["A"] = s.A;
            js["Iy"] = s.Iy;
            js["Iz"] = s.Iz;
            js["It"] = s.It;
            jSections.push_back(js);
        }
        root["sections"] = jSections;

        // 3. Nœuds
        json jNodes = json::array();
        for (const auto& [id, n] : db.getNodes()) {
            json jn;
            jn["id"] = n.id;
            jn["x"] = n.position.x;
            jn["y"] = n.position.y;
            jn["z"] = n.position.z;
            jNodes.push_back(jn);
        }
        root["nodes"] = jNodes;

        // 4. Membres
        json jMembers = json::array();
        for (const auto& [id, m] : db.getMembers()) {
            json jm;
            jm["id"] = m.id;
            jm["startNode"] = m.startNodeId;
            jm["endNode"] = m.endNodeId;
            jm["type"] = static_cast<int>(m.type);
            jm["sectionId"] = m.sectionId;
            jm["materialId"] = m.materialId;
            jm["rollAngle"] = m.rollAngleDeg;
            jm["hingeStartRy"] = m.startReleases.ry;
            jm["hingeStartRz"] = m.startReleases.rz;
            jm["hingeEndRy"] = m.endReleases.ry;
            jm["hingeEndRz"] = m.endReleases.rz;
            jMembers.push_back(jm);
        }
        root["members"] = jMembers;

        // 5. Panneaux (Dalles)
        json jPanels = json::array();
        for (const auto& [id, p] : db.getPanels()) {
            json jp;
            jp["id"] = p.id;
            jp["type"] = static_cast<int>(p.type);
            jp["thickness"] = p.thickness;
            jp["materialId"] = p.materialId;
            jp["nodes"] = p.boundaryNodeIds;
            jPanels.push_back(jp);
        }
        root["panels"] = jPanels;

        // 6. Appuis
        json jSupports = json::array();
        for (const auto& [id, s] : db.getSupports()) {
            json js;
            js["id"] = s.id;
            js["nodeId"] = s.nodeId;
            js["tx"] = s.condition.tx;
            js["ty"] = s.condition.ty;
            js["tz"] = s.condition.tz;
            js["rx"] = s.condition.rx;
            js["ry"] = s.condition.ry;
            js["rz"] = s.condition.rz;
            jSupports.push_back(js);
        }
        root["supports"] = jSupports;

        std::ofstream file(filePath);
        if (!file.is_open()) return false;
        file << root.dump(2);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ProjectSerializer] Erreur lors de l'écriture : " << e.what() << std::endl;
        return false;
    }
}

bool ProjectSerializer::loadFromFile(structural::ModelDatabase& db, const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        json root;
        file >> root;

        db.clear();

        // BUGFIX : sectionId/materialId étaient bien écrits par saveToFile (voir
        // jm["sectionId"], jm["materialId"], jp["materialId"]) mais jamais relus
        // ici : les tables de correspondance ancien -> nouvel EntityId n'existaient
        // pas, et addMember()/addPanel() étaient appelés sans section/matériau,
        // ce qui réaffectait silencieusement les valeurs par défaut à chaque
        // rechargement de projet (perte des affectations section/matériau).
        std::unordered_map<uint64_t, structural::EntityId> oldToNewMaterials;
        std::unordered_map<uint64_t, structural::EntityId> oldToNewSections;

        // 1. Matériaux
        if (root.contains("materials")) {
            for (const auto& jm : root["materials"]) {
                uint64_t oldId = jm.value("id", 0);
                structural::Material m;
                m.name = jm.value("name", "Béton C25/30");
                m.category = static_cast<structural::MaterialCategory>(jm.value("category", 0));
                m.E = jm.value("E", 31.0e9);
                m.nu = jm.value("nu", 0.2);
                m.rho = jm.value("rho", 2500.0);
                m.fck = jm.value("fck", 25.0e6);
                structural::EntityId newId = db.addMaterial(m);
                oldToNewMaterials[oldId] = newId;
            }
        }

        // 2. Sections
        if (root.contains("sections")) {
            for (const auto& js : root["sections"]) {
                uint64_t oldId = js.value("id", 0);
                structural::Section s;
                s.name = js.value("name", "POT 30x30");
                s.shape = static_cast<structural::Section::Shape>(js.value("shape", 0));
                s.width = js.value("width", 0.30);
                s.height = js.value("height", 0.30);
                s.A = js.value("A", 0.09);
                s.Iy = js.value("Iy", 6.75e-4);
                s.Iz = js.value("Iz", 6.75e-4);
                s.It = js.value("It", 1.14e-3);
                structural::EntityId newId = db.addSection(s);
                oldToNewSections[oldId] = newId;
            }
        }

        // 3. Nœuds
        std::unordered_map<uint64_t, structural::EntityId> oldToNewNodes;
        if (root.contains("nodes")) {
            for (const auto& jn : root["nodes"]) {
                uint64_t oldId = jn.value("id", 0);
                double x = jn.value("x", 0.0);
                double y = jn.value("y", 0.0);
                double z = jn.value("z", 0.0);
                structural::EntityId newId = db.addNode(x, y, z);
                oldToNewNodes[oldId] = newId;
            }
        }

        // 4. Membres
        if (root.contains("members")) {
            for (const auto& jm : root["members"]) {
                uint64_t sNode = jm.value("startNode", 0);
                uint64_t eNode = jm.value("endNode", 0);
                auto type = static_cast<structural::MemberType>(jm.value("type", 0));

                uint64_t oldSectionId = jm.value("sectionId", 0);
                uint64_t oldMaterialId = jm.value("materialId", 0);
                structural::EntityId sectionId = oldToNewSections.count(oldSectionId)
                    ? oldToNewSections[oldSectionId] : structural::INVALID_ID;
                structural::EntityId materialId = oldToNewMaterials.count(oldMaterialId)
                    ? oldToNewMaterials[oldMaterialId] : structural::INVALID_ID;

                if (oldToNewNodes.count(sNode) && oldToNewNodes.count(eNode)) {
                    structural::EntityId mId = db.addMember(oldToNewNodes[sNode], oldToNewNodes[eNode],
                                                            type, sectionId, materialId);
                    auto* mem = db.getMember(mId);
                    if (mem) {
                        mem->rollAngleDeg = jm.value("rollAngle", 0.0);
                        mem->startReleases.ry = jm.value("hingeStartRy", false);
                        mem->startReleases.rz = jm.value("hingeStartRz", false);
                        mem->endReleases.ry = jm.value("hingeEndRy", false);
                        mem->endReleases.rz = jm.value("hingeEndRz", false);
                    }
                }
            }
        }

        // 5. Panneaux
        if (root.contains("panels")) {
            for (const auto& jp : root["panels"]) {
                std::vector<uint64_t> oldNodeList = jp.value("nodes", std::vector<uint64_t>{});
                std::vector<structural::EntityId> newNodes;
                for (uint64_t oldN : oldNodeList) {
                    if (oldToNewNodes.count(oldN)) {
                        newNodes.push_back(oldToNewNodes[oldN]);
                    }
                }
                double thick = jp.value("thickness", 0.15);
                auto pType = static_cast<structural::PanelType>(jp.value("type", 0));
                uint64_t oldMaterialId = jp.value("materialId", 0);
                structural::EntityId materialId = oldToNewMaterials.count(oldMaterialId)
                    ? oldToNewMaterials[oldMaterialId] : structural::INVALID_ID;
                db.addPanel(newNodes, thick, materialId, pType);
            }
        }

        // 6. Appuis
        if (root.contains("supports")) {
            for (const auto& js : root["supports"]) {
                uint64_t oldN = js.value("nodeId", 0);
                if (oldToNewNodes.count(oldN)) {
                    structural::SupportCondition cond;
                    cond.tx = js.value("tx", true);
                    cond.ty = js.value("ty", true);
                    cond.tz = js.value("tz", true);
                    cond.rx = js.value("rx", false);
                    cond.ry = js.value("ry", false);
                    cond.rz = js.value("rz", false);
                    db.addSupport(oldToNewNodes[oldN], cond);
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ProjectSerializer] Erreur lors de la lecture : " << e.what() << std::endl;
        return false;
    }
}

} // namespace stabileo::io
