// =============================================================================
//  DxfImporter.cpp — Importateur de fichiers AutoCAD DXF pour StabileoViewer
// =============================================================================

#include "scene/DxfImporter.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <cctype>

namespace scene {

namespace {

// Helper pour convertir une chaîne en minuscules sans espaces
static std::string normalizeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return result;
}

// Clé de grille spatiale pour la fusion rapide des nœuds (Node Snapping)
struct GridKey {
    int64_t x, y, z;
    bool operator==(const GridKey& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct GridKeyHash {
    size_t operator()(const GridKey& k) const {
        size_t h1 = std::hash<int64_t>{}(k.x);
        size_t h2 = std::hash<int64_t>{}(k.y);
        size_t h3 = std::hash<int64_t>{}(k.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class NodeWelder {
public:
    explicit NodeWelder(float tol) : tol_(std::max(tol, 0.0001f)), invTol_(1.0f / tol_) {}

    int getOrAddNode(const glm::vec3& pos, std::vector<model::Node>& nodes) {
        int64_t cx = static_cast<int64_t>(std::floor(pos.x * invTol_));
        int64_t cy = static_cast<int64_t>(std::floor(pos.y * invTol_));
        int64_t cz = static_cast<int64_t>(std::floor(pos.z * invTol_));

        // Recherche dans les cellules voisines (3x3x3)
        for (int64_t dx = -1; dx <= 1; ++dx) {
            for (int64_t dy = -1; dy <= 1; ++dy) {
                for (int64_t dz = -1; dz <= 1; ++dz) {
                    GridKey key{cx + dx, cy + dy, cz + dz};
                    auto it = grid_.find(key);
                    if (it != grid_.end()) {
                        for (int nodeId : it->second) {
                            const auto& existingNode = nodes[static_cast<size_t>(nodeId - 1)];
                            if (glm::distance(existingNode.position, pos) <= tol_) {
                                return nodeId;
                            }
                        }
                    }
                }
            }
        }

        // Nouveau nœud
        int newId = static_cast<int>(nodes.size()) + 1;
        model::Node node;
        node.id = newId;
        node.position = pos;
        node.displacement = glm::vec3(0.0f);
        node.rotation = glm::vec3(0.0f);
        nodes.push_back(node);

        GridKey key{cx, cy, cz};
        grid_[key].push_back(newId);
        return newId;
    }

private:
    float tol_;
    float invTol_;
    std::unordered_map<GridKey, std::vector<int>, GridKeyHash> grid_;
};

// Analyse du nom de calque pour créer ou attribuer la bonne section transversale
static int getOrCreateSectionForLayer(const std::string& layer,
                                      model::Structure& st,
                                      std::unordered_map<std::string, int>& layerToSectionId) {
    std::string norm = normalizeString(layer);
    auto it = layerToSectionId.find(norm);
    if (it != layerToSectionId.end()) {
        return it->second;
    }

    int nextId = static_cast<int>(st.sections.size()) + 1;
    model::Section sec;
    sec.id = nextId;
    sec.fy = 250.0f; // Acier S250 / S355 standard

    if (norm.find("hea300") != std::string::npos) {
        sec.type = model::SectionType::HEA; sec.name = "HEA 300";
        sec.h = 0.290f; sec.b = 0.300f; sec.tw = 0.0085f; sec.tf = 0.014f;
    } else if (norm.find("hea240") != std::string::npos || norm.find("poteau") != std::string::npos || norm.find("col") != std::string::npos) {
        sec.type = model::SectionType::HEA; sec.name = "HEA 240";
        sec.h = 0.230f; sec.b = 0.240f; sec.tw = 0.0075f; sec.tf = 0.012f;
    } else if (norm.find("hea200") != std::string::npos) {
        sec.type = model::SectionType::HEA; sec.name = "HEA 200";
        sec.h = 0.190f; sec.b = 0.200f; sec.tw = 0.0065f; sec.tf = 0.010f;
    } else if (norm.find("ipe400") != std::string::npos) {
        sec.type = model::SectionType::IPE; sec.name = "IPE 400";
        sec.h = 0.400f; sec.b = 0.180f; sec.tw = 0.0086f; sec.tf = 0.0135f;
    } else if (norm.find("ipe300") != std::string::npos || norm.find("poutre") != std::string::npos || norm.find("beam") != std::string::npos || norm.find("rafter") != std::string::npos) {
        sec.type = model::SectionType::IPE; sec.name = "IPE 300";
        sec.h = 0.300f; sec.b = 0.150f; sec.tw = 0.0071f; sec.tf = 0.0107f;
    } else if (norm.find("ipe200") != std::string::npos) {
        sec.type = model::SectionType::IPE; sec.name = "IPE 200";
        sec.h = 0.200f; sec.b = 0.100f; sec.tw = 0.0056f; sec.tf = 0.0085f;
    } else if (norm.find("tube") != std::string::npos || norm.find("chs") != std::string::npos) {
        sec.type = model::SectionType::TUBE_CIRC; sec.name = "Tube D168.3x6.3";
        sec.d = 0.1683f; sec.t = 0.0063f;
    } else if (norm.find("diag") != std::string::npos || norm.find("brace") != std::string::npos || norm.find("treillis") != std::string::npos) {
        sec.type = model::SectionType::TUBE_RECT; sec.name = "SHS 100x5";
        sec.d = 0.100f; sec.d2 = 0.100f; sec.t = 0.005f;
    } else {
        // Section par défaut
        sec.type = model::SectionType::IPE;
        sec.name = layer.empty() ? "Section Defaut IPE 240" : ("Section " + layer);
        sec.h = 0.240f; sec.b = 0.120f; sec.tw = 0.0062f; sec.tf = 0.0098f;
    }

    sec.d = sec.h; sec.t = sec.tw; sec.d2 = sec.b;
    sec.A = sec.h * sec.b * 0.45f;
    sec.Iy = (sec.b * std::pow(sec.h, 3.0f)) / 12.0f;
    sec.Iz = (sec.h * std::pow(sec.b, 3.0f)) / 12.0f;

    st.sections.push_back(sec);
    layerToSectionId[norm] = nextId;
    return nextId;
}

} // namespace

model::Structure loadDxf(const std::string& filepath, const DxfImportOptions& options) {
    model::Structure st;
    std::filesystem::path p(filepath);
    st.name = p.stem().string();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[DxfImporter] Impossible d'ouvrir le fichier : " << filepath << "\n";
        return st;
    }

    float scale = 1.0f;
    switch (options.unit) {
        case DxfUnit::Meters:       scale = 1.0f; break;
        case DxfUnit::Millimeters:  scale = 0.001f; break;
        case DxfUnit::Centimeters:  scale = 0.01f; break;
        case DxfUnit::Inches:       scale = 0.0254f; break;
    }

    auto transformPoint = [&](float x, float y, float z) -> glm::vec3 {
        x *= scale;
        y *= scale;
        z *= scale;
        if (options.upAxis == DxfUpAxis::Z_Up_To_Y_Up) {
            // Dans AutoCAD 3D, Z est la hauteur verticale.
            // On convertit vers le repère moteur : X -> X, Y -> -Z, Z -> Y
            return glm::vec3(x, z, -y);
        } else {
            // Élévation 2D standard : X -> X, Y -> Y (hauteur), Z -> Z
            return glm::vec3(x, y, z);
        }
    };

    // Matériau acier standard par défaut (E = 210 GPa, nu = 0.3, rho = 78.5 kN/m³)
    model::Material mat;
    mat.id = 1;
    mat.name = "Acier S355";
    mat.E = 210000.0f; // [MPa]
    mat.nu = 0.3f;
    mat.rho = 78.5f;   // [kN/m³]
    mat.fy = 355.0f;   // [MPa]
    st.materials.push_back(mat);

    std::unordered_map<std::string, int> layerToSectionId;
    NodeWelder welder(options.snapTolerance);

    std::string line;
    bool inEntities = false;
    std::string currentEntity;
    std::string currentLayer = "0";

    // Variables pour LINE
    float x1 = 0, y1 = 0, z1 = 0;
    float x2 = 0, y2 = 0, z2 = 0;
    bool hasP1 = false, hasP2 = false;

    // Variables pour LWPOLYLINE
    std::vector<glm::vec2> lwVertices;
    bool lwClosed = false;
    float lwElevation = 0.0f;

    // Variables pour POLYLINE 3D
    std::vector<glm::vec3> poly3DVertices;
    bool inPolyline3D = false;
    // Calque de la POLYLINE elle-même, capturé avant d'être écrasé par les
    // sous-entités VERTEX (qui réinitialisent currentLayer à "0" à chaque
    // nouvelle entité). Sans cela, tous les éléments 3DPOLYLINE se voyaient
    // attribuer la section par défaut du calque "0" au lieu de leur vrai calque.
    std::string polylineLayer = "0";

    auto finalizeCurrentEntity = [&]() {
        if (currentEntity == "LINE" || currentEntity == "3DLINE") {
            if (hasP1 && hasP2) {
                glm::vec3 pA = transformPoint(x1, y1, z1);
                glm::vec3 pB = transformPoint(x2, y2, z2);
                if (glm::distance(pA, pB) > options.snapTolerance) {
                    int nI = welder.getOrAddNode(pA, st.nodes);
                    int nJ = welder.getOrAddNode(pB, st.nodes);
                    if (nI != nJ) {
                        model::Element elem;
                        elem.id = static_cast<int>(st.elements.size()) + 1;
                        elem.nodeI = nI;
                        elem.nodeJ = nJ;
                        elem.materialId = 1;
                        elem.sectionId = getOrCreateSectionForLayer(currentLayer, st, layerToSectionId);
                        elem.rollAngle = 0.0f;
                        elem.isTruss = (normalizeString(currentLayer).find("treillis") != std::string::npos ||
                                        normalizeString(currentLayer).find("truss") != std::string::npos ||
                                        normalizeString(currentLayer).find("diag") != std::string::npos);
                        st.elements.push_back(elem);
                    }
                }
            }
        } else if (currentEntity == "LWPOLYLINE") {
            if (lwVertices.size() >= 2) {
                int secId = getOrCreateSectionForLayer(currentLayer, st, layerToSectionId);
                for (size_t i = 0; i + 1 < lwVertices.size(); ++i) {
                    glm::vec3 pA = transformPoint(lwVertices[i].x, lwVertices[i].y, lwElevation);
                    glm::vec3 pB = transformPoint(lwVertices[i + 1].x, lwVertices[i + 1].y, lwElevation);
                    if (glm::distance(pA, pB) > options.snapTolerance) {
                        int nI = welder.getOrAddNode(pA, st.nodes);
                        int nJ = welder.getOrAddNode(pB, st.nodes);
                        if (nI != nJ) {
                            model::Element elem;
                            elem.id = static_cast<int>(st.elements.size()) + 1;
                            elem.nodeI = nI;
                            elem.nodeJ = nJ;
                            elem.materialId = 1;
                            elem.sectionId = secId;
                            st.elements.push_back(elem);
                        }
                    }
                }
                if (lwClosed && lwVertices.size() >= 3) {
                    glm::vec3 pA = transformPoint(lwVertices.back().x, lwVertices.back().y, lwElevation);
                    glm::vec3 pB = transformPoint(lwVertices.front().x, lwVertices.front().y, lwElevation);
                    if (glm::distance(pA, pB) > options.snapTolerance) {
                        int nI = welder.getOrAddNode(pA, st.nodes);
                        int nJ = welder.getOrAddNode(pB, st.nodes);
                        if (nI != nJ) {
                            model::Element elem;
                            elem.id = static_cast<int>(st.elements.size()) + 1;
                            elem.nodeI = nI;
                            elem.nodeJ = nJ;
                            elem.materialId = 1;
                            elem.sectionId = secId;
                            st.elements.push_back(elem);
                        }
                    }
                }
            }
        }
    };

    while (std::getline(file, line)) {
        // Enlève les \r de fin de ligne
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Le format DXF est constitué de paires : code (int) puis valeur (string)
        std::string codeStr = line;
        // Supprime les espaces éventuels
        codeStr.erase(0, codeStr.find_first_not_of(" \t"));
        codeStr.erase(codeStr.find_last_not_of(" \t") + 1);
        if (codeStr.empty()) continue;

        int code = 0;
        try {
            code = std::stoi(codeStr);
        } catch (...) {
            continue;
        }

        std::string value;
        if (!std::getline(file, value)) break;
        if (!value.empty() && value.back() == '\r') value.pop_back();
        // Trim value
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (code == 0) {
            if (value == "SECTION") {
                // Section suivante
            } else if (value == "ENDSEC") {
                finalizeCurrentEntity();
                currentEntity.clear();
                inEntities = false;
                inPolyline3D = false;
            } else if (value == "EOF") {
                break;
            } else {
                // Nouvelle entité
                finalizeCurrentEntity();
                currentEntity = value;
                currentLayer = "0";
                hasP1 = hasP2 = false;
                x1 = y1 = z1 = x2 = y2 = z2 = 0.0f;
                lwVertices.clear();
                lwClosed = false;
                lwElevation = 0.0f;

                if (value == "POLYLINE") {
                    inPolyline3D = true;
                    poly3DVertices.clear();
                    polylineLayer = "0";
                } else if (value == "SEQEND") {
                    if (inPolyline3D && poly3DVertices.size() >= 2) {
                        int secId = getOrCreateSectionForLayer(polylineLayer, st, layerToSectionId);
                        for (size_t i = 0; i + 1 < poly3DVertices.size(); ++i) {
                            glm::vec3 pA = poly3DVertices[i];
                            glm::vec3 pB = poly3DVertices[i + 1];
                            if (glm::distance(pA, pB) > options.snapTolerance) {
                                int nI = welder.getOrAddNode(pA, st.nodes);
                                int nJ = welder.getOrAddNode(pB, st.nodes);
                                if (nI != nJ) {
                                    model::Element elem;
                                    elem.id = static_cast<int>(st.elements.size()) + 1;
                                    elem.nodeI = nI;
                                    elem.nodeJ = nJ;
                                    elem.materialId = 1;
                                    elem.sectionId = secId;
                                    st.elements.push_back(elem);
                                }
                            }
                        }
                    }
                    inPolyline3D = false;
                }
            }
        } else if (code == 2 && value == "ENTITIES") {
            inEntities = true;
        } else if (inEntities) {
            if (code == 8) {
                currentLayer = value;
                if (currentEntity == "POLYLINE") polylineLayer = value;
            } else if (currentEntity == "LINE" || currentEntity == "3DLINE") {
                if (code == 10) { x1 = std::stof(value); hasP1 = true; }
                else if (code == 20) { y1 = std::stof(value); hasP1 = true; }
                else if (code == 30) { z1 = std::stof(value); }
                else if (code == 11) { x2 = std::stof(value); hasP2 = true; }
                else if (code == 21) { y2 = std::stof(value); hasP2 = true; }
                else if (code == 31) { z2 = std::stof(value); }
            } else if (currentEntity == "LWPOLYLINE") {
                if (code == 70) {
                    int flag = std::stoi(value);
                    lwClosed = (flag & 1) != 0;
                } else if (code == 38) {
                    lwElevation = std::stof(value);
                } else if (code == 10) {
                    lwVertices.push_back(glm::vec2(std::stof(value), 0.0f));
                } else if (code == 20) {
                    if (!lwVertices.empty()) {
                        lwVertices.back().y = std::stof(value);
                    }
                }
            } else if (inPolyline3D && currentEntity == "VERTEX") {
                static float vx = 0, vy = 0, vz = 0;
                if (code == 10) vx = std::stof(value);
                else if (code == 20) vy = std::stof(value);
                else if (code == 30) {
                    vz = std::stof(value);
                    poly3DVertices.push_back(transformPoint(vx, vy, vz));
                }
            }
        }
    }
    finalizeCurrentEntity();

    // S'assurer qu'au moins une section par défaut existe
    if (st.sections.empty()) {
        getOrCreateSectionForLayer("Default", st, layerToSectionId);
    }

    // Détection des appuis au sol
    if (options.autoSupportGround && !st.nodes.empty()) {
        float minY = st.nodes[0].position.y;
        for (const auto& n : st.nodes) {
            minY = std::min(minY, n.position.y);
        }

        for (const auto& n : st.nodes) {
            if (std::abs(n.position.y - minY) <= options.snapTolerance * 2.0f) {
                model::Support sup;
                sup.nodeId = n.id;
                sup.type = model::SupportType::FIXED;
                sup.blockRx = sup.blockRy = sup.blockRz = true;
                st.supports.push_back(sup);
            }
        }
    }

    // Chargement test si demandé pour permettre la résolution FEA
    if (options.addGravityLoad && !st.elements.empty()) {
        // Ajout d'une charge uniformément répartie descendante de 12 kN/m sur les éléments non verticaux
        for (const auto& elem : st.elements) {
            const auto* nI = st.findNode(elem.nodeI);
            const auto* nJ = st.findNode(elem.nodeJ);
            if (nI && nJ) {
                glm::vec3 dir = glm::normalize(nJ->position - nI->position);
                // Si l'élément n'est pas un poteau vertical pur (|dir.y| < 0.9)
                if (std::abs(dir.y) < 0.9f) {
                    model::DistributedLoad dl;
                    dl.elementId = elem.id;
                    dl.wStart = glm::vec3(0.0f, -12.0f, 0.0f); // -12 kN/m
                    dl.wEnd   = glm::vec3(0.0f, -12.0f, 0.0f);
                    st.distributedLoads.push_back(dl);
                }
            }
        }

        // Si aucun élément horizontal n'a été trouvé, ajouter une charge nodale sur les nœuds les plus hauts
        if (st.distributedLoads.empty() && !st.nodes.empty()) {
            float maxY = st.nodes[0].position.y;
            for (const auto& n : st.nodes) maxY = std::max(maxY, n.position.y);
            for (const auto& n : st.nodes) {
                if (std::abs(n.position.y - maxY) <= options.snapTolerance * 2.0f) {
                    model::NodalLoad nl;
                    nl.nodeId = n.id;
                    nl.force = glm::vec3(0.0f, -25.0f, 0.0f); // -25 kN
                    nl.moment = glm::vec3(0.0f);
                    st.nodalLoads.push_back(nl);
                }
            }
        }
    }

    std::cout << "[DxfImporter] Importation réussie : " << st.name
              << " (" << st.nodes.size() << " nœuds, "
              << st.elements.size() << " éléments, "
              << st.supports.size() << " appuis)\n";

    return st;
}

void generateSampleDxfFiles(const std::string& targetDir) {
    std::filesystem::create_directories(targetDir);

    // 1. Portique Industriel avec Poteaux HEA 240 et Traverse IPE 300
    std::string framePath = targetDir + "/industrial-portal-frame.dxf";
    if (!std::filesystem::exists(framePath)) {
        std::ofstream f(framePath);
        if (f.is_open()) {
            f << "0\nSECTION\n2\nENTITIES\n";

            auto writeLine = [&](const std::string& layer, float x1, float y1, float z1, float x2, float y2, float z2) {
                f << "0\nLINE\n8\n" << layer << "\n";
                f << "10\n" << x1 << "\n20\n" << y1 << "\n30\n" << z1 << "\n";
                f << "11\n" << x2 << "\n21\n" << y2 << "\n31\n" << z2 << "\n";
            };

            // Poteau gauche (HEA 240)
            writeLine("HEA240", 0.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f);
            // Poteau droit (HEA 240)
            writeLine("HEA240", 8.0f, 0.0f, 0.0f, 8.0f, 5.0f, 0.0f);
            // Rafter gauche vers faîtage (IPE 300)
            writeLine("IPE300", 0.0f, 5.0f, 0.0f, 4.0f, 6.5f, 0.0f);
            // Rafter droit vers faîtage (IPE 300)
            writeLine("IPE300", 4.0f, 6.5f, 0.0f, 8.0f, 5.0f, 0.0f);
            // Entrait / tirant horizontal (SHS)
            writeLine("SHS100", 0.0f, 5.0f, 0.0f, 8.0f, 5.0f, 0.0f);

            f << "0\nENDSEC\n0\nEOF\n";
        }
    }

    // 2. Ferme de Toiture Treillis Warren (Warren Roof Truss)
    std::string trussPath = targetDir + "/warren-roof-truss.dxf";
    if (!std::filesystem::exists(trussPath)) {
        std::ofstream f(trussPath);
        if (f.is_open()) {
            f << "0\nSECTION\n2\nENTITIES\n";

            auto writeLine = [&](const std::string& layer, float x1, float y1, float z1, float x2, float y2, float z2) {
                f << "0\nLINE\n8\n" << layer << "\n";
                f << "10\n" << x1 << "\n20\n" << y1 << "\n30\n" << z1 << "\n";
                f << "11\n" << x2 << "\n21\n" << y2 << "\n31\n" << z2 << "\n";
            };

            float L = 12.0f; // 12 m portée
            int nBays = 6;
            float dx = L / nBays; // 2 m par panneau
            float H = 2.2f;       // 2.2 m hauteur

            // Corde inférieure
            for (int i = 0; i < nBays; ++i) {
                writeLine("IPE200", i * dx, 0.0f, 0.0f, (i + 1) * dx, 0.0f, 0.0f);
            }
            // Corde supérieure
            for (int i = 0; i < nBays; ++i) {
                writeLine("IPE240", i * dx, H, 0.0f, (i + 1) * dx, H, 0.0f);
            }
            // Montants verticaux et diagonales
            for (int i = 0; i <= nBays; ++i) {
                writeLine("SHS100", i * dx, 0.0f, 0.0f, i * dx, H, 0.0f);
            }
            for (int i = 0; i < nBays; ++i) {
                if (i % 2 == 0) {
                    writeLine("TREILLIS", i * dx, 0.0f, 0.0f, (i + 1) * dx, H, 0.0f);
                } else {
                    writeLine("TREILLIS", i * dx, H, 0.0f, (i + 1) * dx, 0.0f, 0.0f);
                }
            }

            f << "0\nENDSEC\n0\nEOF\n";
        }
    }
}

} // namespace scene
