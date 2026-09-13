#pragma once
// =============================================================================
//  StructureModel.h — Modèle de données structurel complet
//
//  Définit toutes les entités d'un modèle d'analyse de structure :
//  nœuds, éléments (barres/poutres), sections transversales, appuis,
//  chargements et résultats d'analyse (efforts internes, déplacements).
// =============================================================================

#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace model {

// ---- Nœud ----
struct Node {
    int id = 0;
    glm::vec3 position{0.0f};
    // Résultats (remplis après analyse)
    glm::vec3 displacement{0.0f};   // [m]
    glm::vec3 rotation{0.0f};       // [rad]
};

// ---- Matériau ----
struct Material {
    int id = 1;
    std::string name = "Acero A36";
    float E   = 200000.0f; // [MPa]
    float nu  = 0.3f;
    float rho = 78.5f;     // [kN/m³]
    float fy  = 250.0f;    // [MPa]
};

// ---- Type de section transversale ----
enum class SectionType {
    IPE,          // Profilé I européen (IPE)
    IPN,          // Profilé I normalisé (IPN)
    HEA,          // Profilé H large (HEA)
    HEB,          // Profilé H lourd  (HEB)
    HEM,          // Profilé H renforcé (HEM)
    UPN,          // Profilé U (channel)
    TUBE_RECT,    // Tube rectangulaire creux (RHS / SHS)
    TUBE_CIRC,    // Tube circulaire creux (CHS)
    ANGLE_L,      // Cornière L
    TEE,          // Profilé en T
    RECT_SOLID,   // Rectangle plein
    CIRC_SOLID    // Cercle plein
};

// ---- Section transversale ----
struct Section {
    int id = 0;
    SectionType type = SectionType::IPE;
    std::string name = "IPE 300";

    // Paramètres géométriques des profilés I / H
    float h  = 0.300f;    // Hauteur totale [m]
    float b  = 0.150f;    // Largeur des semelles [m]
    float tw = 0.0071f;   // Épaisseur de l'âme [m]
    float tf = 0.0107f;   // Épaisseur des semelles [m]

    // Paramètres tubes
    float d  = 0.0f;      // Diamètre extérieur ou largeur [m]
    float t  = 0.0f;      // Épaisseur de paroi [m]
    float d2 = 0.0f;      // Hauteur (tube rect) [m]

    // Matériau
    float fy = 235.0f;    // Limite élastique [MPa]
    float E  = 210000.0f; // Module de Young [MPa]

    // Caractéristiques mécaniques
    float Iy = 8356e-8f;  // Moment d'inertie axe fort [m⁴]
    float Iz = 604e-8f;   // Moment d'inertie axe faible [m⁴]
    float A  = 53.8e-4f;  // Aire de la section [m²]

    // Rayon du filet de raccordement pour le rendu (optionnel)
    float r = 0.015f;
};

// ---- Élément (barre / poutre / câble) ----
struct Element {
    int id = 0;
    int nodeI = 0;          // ID du nœud de début
    int nodeJ = 0;          // ID du nœud de fin
    int sectionId = 0;
    int materialId = 1;
    float rollAngle = 0.0f; // Angle de roulis [degrés]
    bool isTruss = false;   // true = barre articulée (effort normal seul)
    bool isCable = false;   // true = câble (traction seule)
    bool hingeStart = false;
    bool hingeEnd   = false;

    // Résultats aux stations le long de l'élément (s ∈ [0, 1])
    std::vector<float> stations;
    std::vector<float> N;           // Effort normal [kN]
    std::vector<float> Vy;          // Effort tranchant y [kN]
    std::vector<float> Vz;          // Effort tranchant z [kN]
    std::vector<float> My;          // Moment fléchissant y [kN·m]
    std::vector<float> Mz;          // Moment fléchissant z [kN·m]
    std::vector<float> T;           // Torsion [kN·m]
    std::vector<float> stressRatio; // σ / fy (0..1+)
};

// ---- Type d'appui ----
enum class SupportType {
    FIXED,        // Encastrement parfait
    PINNED,       // Rotule (translations bloquées)
    ROLLER_X,     // Translations Y,Z bloquées ; X libre
    ROLLER_Y,     // Translations X,Z bloquées ; Y libre
    ROLLER_Z,     // Translations X,Y bloquées ; Z libre
    SPRING        // Appui élastique
};

// ---- Appui / Condition aux limites ----
struct Support {
    int nodeId = 0;
    SupportType type = SupportType::FIXED;
    bool blockRx = false, blockRy = false, blockRz = false;
    glm::vec3 springK{0.0f};  // Raideurs des ressorts [kN/m]
};

// ---- Chargement nodal ----
struct NodalLoad {
    int nodeId = 0;
    glm::vec3 force{0.0f};    // [kN]
    glm::vec3 moment{0.0f};   // [kN·m]
};

// ---- Charge répartie sur un élément ----
struct DistributedLoad {
    int elementId = 0;
    glm::vec3 wStart{0.0f};   // Intensité au début [kN/m] (coords locales)
    glm::vec3 wEnd{0.0f};     // Intensité à la fin [kN/m] (coords locales)
};

// ---- Réaction d'appui (résultat) ----
struct Reaction {
    int nodeId = 0;
    glm::vec3 force{0.0f};    // [kN]
    glm::vec3 moment{0.0f};   // [kN·m]
};

// ---- Structure complète ----
struct Structure {
    std::string name = "Sans titre";
    std::vector<Node>            nodes;
    std::vector<Element>         elements;
    std::vector<Section>         sections;
    std::vector<Material>        materials;
    std::vector<Support>         supports;
    std::vector<NodalLoad>       nodalLoads;
    std::vector<DistributedLoad> distributedLoads;
    std::vector<Reaction>        reactions;
    bool hasResults = false;

    // ---- Recherche par ID ----
    const Node* findNode(int id) const {
        for (auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    }
    Node* findNode(int id) {
        for (auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    }
    const Section* findSection(int id) const {
        for (auto& s : sections) if (s.id == id) return &s;
        return nullptr;
    }
    const Material* findMaterial(int id) const {
        for (auto& m : materials) if (m.id == id) return &m;
        return nullptr;
    }
    const Element* findElement(int id) const {
        for (auto& e : elements) if (e.id == id) return &e;
        return nullptr;
    }

    // ---- Boîte englobante de la scène ----
    void computeBounds(glm::vec3& outMin, glm::vec3& outMax) const {
        if (nodes.empty()) { outMin = outMax = glm::vec3(0.0f); return; }
        outMin = outMax = nodes[0].position;
        for (auto& n : nodes) {
            outMin = glm::min(outMin, n.position);
            outMax = glm::max(outMax, n.position);
        }
    }
};

} // namespace model
