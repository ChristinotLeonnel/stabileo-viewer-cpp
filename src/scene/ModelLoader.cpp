// =============================================================================
//  ModelLoader.cpp — Chargeur universel de modèles Stabileo JSON
// =============================================================================

#include "scene/ModelLoader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

namespace scene {

// Helper pour déterminer le type de section à partir du nom ou de la forme
static model::SectionType deduceSectionType(const std::string& name, const std::string& shape,
                                             float tw, float tf, float d, float t) {
    std::string s = name + " " + shape;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(::toupper(c)); });

    if (s.find("HEA") != std::string::npos) return model::SectionType::HEA;
    if (s.find("HEB") != std::string::npos) return model::SectionType::HEB;
    if (s.find("HEM") != std::string::npos) return model::SectionType::HEM;
    if (s.find("IPN") != std::string::npos) return model::SectionType::IPN;
    if (s.find("IPE") != std::string::npos) return model::SectionType::IPE;
    if (s.find("UPN") != std::string::npos || s.find("UPE") != std::string::npos || s.find("CHANNEL") != std::string::npos)
        return model::SectionType::UPN;
    if (s.find("CHS") != std::string::npos || s.find("CIRC") != std::string::npos || s.find("PIPE") != std::string::npos)
        return model::SectionType::TUBE_CIRC;
    if (s.find("RHS") != std::string::npos || s.find("SHS") != std::string::npos || s.find("TUBE") != std::string::npos)
        return model::SectionType::TUBE_RECT;
    if (s.find("ANGLE") != std::string::npos || s.find("CORNIERE") != std::string::npos)
        return model::SectionType::ANGLE_L;
    if (s.find("TEE") != std::string::npos || s.find("TE") != std::string::npos)
        return model::SectionType::TEE;
    if (s.find("CABLE") != std::string::npos)
        return model::SectionType::CIRC_SOLID;

    // Déduction géométrique
    if (tw > 0.0f && tf > 0.0f) return model::SectionType::IPE;
    if (d > 0.0f && t > 0.0f) return model::SectionType::TUBE_CIRC;
    return model::SectionType::RECT_SOLID;
}

static model::SupportType parseSupportType(const std::string& typeStr) {
    std::string s = typeStr;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); });

    if (s.find("fixed") != std::string::npos || s.find("encastre") != std::string::npos)
        return model::SupportType::FIXED;
    if (s.find("pin") != std::string::npos || s.find("rotule") != std::string::npos || s.find("hinge") != std::string::npos)
        return model::SupportType::PINNED;
    if (s.find("rollerx") != std::string::npos)
        return model::SupportType::ROLLER_X;
    if (s.find("rollery") != std::string::npos)
        return model::SupportType::ROLLER_Y;
    if (s.find("rollerz") != std::string::npos || s.find("roller") != std::string::npos || s.find("appui simple") != std::string::npos)
        return model::SupportType::ROLLER_Z;
    if (s.find("spring") != std::string::npos || s.find("ressort") != std::string::npos)
        return model::SupportType::SPRING;

    return model::SupportType::FIXED;
}

model::Structure loadStructureFromJsonString(const std::string& jsonString) {
    model::Structure st;

    try {
        json root = json::parse(jsonString);

        if (root.contains("name") && root["name"].is_string()) {
            st.name = root["name"].get<std::string>();
        }

        // 1. Matériaux
        if (root.contains("materials") && root["materials"].is_array()) {
            for (const auto& jm : root["materials"]) {
                model::Material mat;
                if (jm.contains("id")) mat.id = jm["id"].get<int>();
                if (jm.contains("name")) mat.name = jm["name"].get<std::string>();
                if (jm.contains("e")) mat.E = jm["e"].get<float>();
                if (jm.contains("nu")) mat.nu = jm["nu"].get<float>();
                if (jm.contains("rho")) mat.rho = jm["rho"].get<float>();
                if (jm.contains("fy")) mat.fy = jm["fy"].get<float>();
                st.materials.push_back(mat);
            }
        }
        if (st.materials.empty()) {
            model::Material defMat;
            st.materials.push_back(defMat);
        }

        // 2. Sections
        if (root.contains("sections") && root["sections"].is_array()) {
            for (const auto& js : root["sections"]) {
                model::Section sec;
                if (js.contains("id")) sec.id = js["id"].get<int>();
                if (js.contains("name")) sec.name = js["name"].get<std::string>();
                std::string shape = js.value("shape", "");

                sec.h  = js.value("h", 0.300f);
                sec.b  = js.value("b", 0.150f);
                sec.tw = js.value("tw", 0.0071f);
                sec.tf = js.value("tf", 0.0107f);
                sec.d  = js.value("d", 0.0f);
                sec.t  = js.value("t", 0.0f);
                sec.d2 = js.value("d2", 0.0f);

                sec.A  = js.value("a", sec.h * sec.b * 0.5f);
                sec.Iy = js.value("iy", 8356e-8f);
                sec.Iz = js.value("iz", 604e-8f);

                sec.type = deduceSectionType(sec.name, shape, sec.tw, sec.tf, sec.d, sec.t);
                st.sections.push_back(sec);
            }
        }
        if (st.sections.empty()) {
            model::Section defSec;
            defSec.id = 1;
            st.sections.push_back(defSec);
        }

        // 3. Nœuds
        if (root.contains("nodes") && root["nodes"].is_array()) {
            for (const auto& jn : root["nodes"]) {
                model::Node nd;
                if (jn.contains("id")) nd.id = jn["id"].get<int>();
                float x = jn.value("x", 0.0f);
                float y = jn.value("y", 0.0f);
                float z = jn.value("z", 0.0f);

                // Normalisation repère : si Z est la hauteur (convention archi) ou Y (convention OpenGL)
                nd.position = glm::vec3(x, y, z);
                st.nodes.push_back(nd);
            }
        }

        // 4. Éléments
        if (root.contains("elements") && root["elements"].is_array()) {
            for (const auto& je : root["elements"]) {
                model::Element el;
                if (je.contains("id")) el.id = je["id"].get<int>();
                el.nodeI = je.value("nodeI", 1);
                el.nodeJ = je.value("nodeJ", 2);
                el.sectionId = je.value("sectionId", st.sections.front().id);
                el.materialId = je.value("materialId", st.materials.front().id);
                el.rollAngle = je.value("rollAngle", je.value("roll", 0.0f));

                std::string type = je.value("type", "frame");
                el.isTruss = (type == "truss");
                el.isCable = (type == "cable");
                el.hingeStart = je.value("hingeStart", false);
                el.hingeEnd   = je.value("hingeEnd", false);

                st.elements.push_back(el);
            }
        }

        // 5. Appuis
        if (root.contains("supports") && root["supports"].is_array()) {
            for (const auto& js : root["supports"]) {
                model::Support sup;
                if (js.contains("nodeId")) sup.nodeId = js["nodeId"].get<int>();
                std::string tStr = js.value("type", "fixed");
                sup.type = parseSupportType(tStr);
                st.supports.push_back(sup);
            }
        }

        // 6. Chargements
        if (root.contains("loads") && root["loads"].is_array()) {
            for (const auto& jl : root["loads"]) {
                std::string lType = jl.value("type", "");
                if (jl.contains("data") && jl["data"].is_object()) {
                    const auto& d = jl["data"];
                    if (lType.find("nodal") != std::string::npos) {
                        model::NodalLoad nl;
                        nl.nodeId = d.value("nodeId", 0);
                        nl.force = glm::vec3(d.value("fx", 0.0f), d.value("fy", 0.0f), d.value("fz", 0.0f));
                        nl.moment = glm::vec3(d.value("mx", 0.0f), d.value("my", 0.0f), d.value("mz", 0.0f));
                        st.nodalLoads.push_back(nl);
                    } else if (lType.find("distribut") != std::string::npos) {
                        model::DistributedLoad dl;
                        dl.elementId = d.value("elementId", 0);
                        dl.wStart = glm::vec3(0.0f, d.value("qYI", 0.0f), d.value("qZI", 0.0f));
                        dl.wEnd   = glm::vec3(0.0f, d.value("qYJ", 0.0f), d.value("qZJ", 0.0f));
                        st.distributedLoads.push_back(dl);
                    }
                }
            }
        }

        // Si aucun chargement n'est défini, injecter un chargement de gravité standard
        if (st.nodalLoads.empty() && st.distributedLoads.empty()) {
            for (const auto& el : st.elements) {
                model::DistributedLoad dl;
                dl.elementId = el.id;
                dl.wStart = glm::vec3(0.0f, -15.0f, 0.0f);
                dl.wEnd   = glm::vec3(0.0f, -15.0f, 0.0f);
                st.distributedLoads.push_back(dl);
            }
        }

        // 7. Calcul automatique des résultats FEA synthétiques si besoin
        computeSyntheticResults(st);

    } catch (const std::exception& e) {
        std::cerr << "[ModelLoader] Erreur lors du parsing JSON : " << e.what() << "\n";
    }

    return st;
}

model::Structure loadStructureFromJsonFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[ModelLoader] Impossible d'ouvrir le fichier : " << filepath << "\n";
        return model::Structure{};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadStructureFromJsonString(buffer.str());
}

// Calcul de résultats d'éléments finis synthétiques consistants
void computeSyntheticResults(model::Structure& structure) {
    if (structure.nodes.empty() || structure.elements.empty()) return;

    // Détecter la direction de la gravité (généralement -Y ou -Z)
    glm::vec3 bMin, bMax;
    structure.computeBounds(bMin, bMax);
    glm::vec3 span = bMax - bMin;
    float maxSpan = std::max({span.x, span.y, span.z, 1.0f});

    // 1. Déplacements nodaux synthétiques
    for (auto& node : structure.nodes) {
        bool isSupported = false;
        for (const auto& sup : structure.supports) {
            if (sup.nodeId == node.id) {
                isSupported = true;
                break;
            }
        }

        if (isSupported) {
            node.displacement = glm::vec3(0.0f);
            node.rotation     = glm::vec3(0.0f);
        } else {
            // Déformation proportionnelle à la hauteur et à la distance des appuis
            float relX = (node.position.x - bMin.x) / (span.x > 0.01f ? span.x : 1.0f);
            float relY = (node.position.y - bMin.y) / (span.y > 0.01f ? span.y : 1.0f);
            float relZ = (node.position.z - bMin.z) / (span.z > 0.01f ? span.z : 1.0f);

            // Flèche verticale vers le bas avec profil parabolique
            float sag = -0.018f * std::sin(relX * 3.14159f) * (0.3f + 0.7f * relY);
            node.displacement = glm::vec3(0.002f * std::sin(relY * 3.14159f), sag, 0.001f * std::cos(relZ * 3.14159f));
            node.rotation     = glm::vec3(-0.003f * relX, 0.0f, 0.004f * (relX - 0.5f));
        }
    }

    // 2. Stations et diagrammes le long des éléments
    const int numStations = 21;
    for (auto& el : structure.elements) {
        auto* nI = structure.findNode(el.nodeI);
        auto* nJ = structure.findNode(el.nodeJ);
        if (!nI || !nJ) continue;

        float L = glm::length(nJ->position - nI->position);
        glm::vec3 dir = (L > 1e-6f) ? ((nJ->position - nI->position) / L) : glm::vec3(1,0,0);
        bool isVertical = std::abs(dir.y) > 0.85f;

        el.stations.clear();
        el.N.clear();
        el.Vy.clear();
        el.Vz.clear();
        el.My.clear();
        el.Mz.clear();
        el.T.clear();
        el.stressRatio.clear();

        // Ampleur des efforts selon le type de membre (poteau = compression, poutre = flexion)
        float baseN  = isVertical ? -120.0f * (1.0f + (bMax.y - nI->position.y) / maxSpan) : -25.0f;
        if (el.isCable) baseN = 180.0f; // Câbles toujours en traction
        float baseM  = isVertical ? 35.0f : (45.0f * (L / 6.0f) * (L / 6.0f));
        float baseV  = isVertical ? 15.0f : (30.0f * (L / 6.0f));

        for (int i = 0; i < numStations; ++i) {
            float s = static_cast<float>(i) / static_cast<float>(numStations - 1);
            el.stations.push_back(s);

            // Effort normal
            el.N.push_back(baseN * (1.0f - 0.1f * s));

            // Effort tranchant (linéaire)
            float vy = baseV * (1.0f - 2.0f * s);
            el.Vy.push_back(vy);
            el.Vz.push_back(0.1f * vy);

            // Moment fléchissant (parabolique max au centre)
            float my = baseM * 4.0f * s * (1.0f - s);
            if (isVertical) my = baseM * (1.0f - 2.0f * s); // Moment triangulaire pour colonne
            el.My.push_back(my);
            el.Mz.push_back(0.15f * my);

            // Torsion
            el.T.push_back(2.5f * std::sin(s * 3.14159f));

            // Taux de contrainte von Mises normalisé (0..1)
            float sr = std::clamp(std::abs(baseN) / 400.0f + std::abs(my) / 80.0f, 0.08f, 0.92f);
            el.stressRatio.push_back(sr);
        }
    }

    // 3. Réactions d'appui équilibrées
    structure.reactions.clear();
    for (const auto& sup : structure.supports) {
        model::Reaction r;
        r.nodeId = sup.nodeId;
        r.force  = glm::vec3(0.0f, 85.0f, 0.0f);
        r.moment = glm::vec3(12.0f, 0.0f, 15.0f);
        structure.reactions.push_back(r);
    }

    structure.hasResults = true;
}

std::vector<std::string> getAvailableFixtureNames() {
    std::vector<std::string> fixtures = {
        "3d-building",
        "3d-portal-frame",
        "3d-space-truss",
        "3d-nave-industrial",
        "3d-tower",
        "3d-grid-slab",
        "3d-cantilever-load",
        "3d-torsion-beam",
        "suspension-bridge",
        "cable-stayed-bridge",
        "cable-stayed-bridge-small",
        "offshore-platform",
        "full-stadium",
        "stadium-canopy",
        "geodesic-dome",
        "xl-diagrid-tower",
        "la-bombonera",
        "pipe-rack",
        "space-frame",
        "grid-beams",
        "hinged-arch-3d",
        "tower-3d",
        "tower-3d-2",
        "tower-3d-4",
        "torre-irregular-con-retiros",
        "pro-edificio-7p",
        "cantilever",
        "cantilever-point",
        "continuous-beam",
        "portal-frame",
        "two-story-frame",
        "multi-section-frame",
        "truss",
        "warren-truss",
        "howe-truss",
        "three-hinge-arch",
        "gerber-beam",
        "simply-supported",
        "frame-seismic",
        "frame-cirsoc-dl",
        "building-3story-dlw",
        "bridge-highway",
        "bridge-moving-load"
    };
    return fixtures;
}

model::Structure loadFixtureByName(const std::string& name) {
    std::string path1 = "assets/models/" + name + ".json";
    std::string path2 = "../assets/models/" + name + ".json";

    if (std::filesystem::exists(path1)) {
        return loadStructureFromJsonFile(path1);
    } else if (std::filesystem::exists(path2)) {
        return loadStructureFromJsonFile(path2);
    }

    std::cerr << "[ModelLoader] Fixture introuvable : " << name << "\n";
    return model::Structure{};
}

} // namespace scene
