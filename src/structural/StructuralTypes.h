#pragma once
// =============================================================================
//  StructuralTypes.h — Types et structures de données métier (style Robot)
// =============================================================================

#include "structural/EntityId.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace stabileo::structural {

// =============================================================================
//  1. Nœud
// =============================================================================
struct Node {
    EntityId id = INVALID_ID;
    glm::dvec3 position{0.0};
    EntityId supportId = INVALID_ID;

    // Connectivité topologique
    std::vector<EntityId> connectedMemberIds;
    std::vector<EntityId> connectedPanelIds;
};

// =============================================================================
//  2. Membre (Poteau, Poutre, Treillis)
// =============================================================================
enum class MemberType {
    Generic,
    Beam,       // Poutre
    Column,     // Poteau
    Truss       // Treillis (articulé aux 2 bouts)
};

struct EndReleases {
    bool rx = false; // Torsion libre
    bool ry = false; // Moment fléchissant y libre
    bool rz = false; // Moment fléchissant z libre
};

struct Member {
    EntityId id = INVALID_ID;
    EntityId startNodeId = INVALID_ID;
    EntityId endNodeId   = INVALID_ID;
    MemberType type      = MemberType::Generic;

    EntityId sectionId  = INVALID_ID;
    EntityId materialId = INVALID_ID;

    double rollAngleDeg = 0.0; // Angle Gamma d'orientation de la section
    EndReleases startReleases;
    EndReleases endReleases;

    glm::dvec3 startOffset{0.0};
    glm::dvec3 endOffset{0.0};

    std::string storeyName = "Niveau 1";
    std::string groupName  = "Structure Principale";
};

// =============================================================================
//  3. Panneau (Dalle, Voile, Diaphragme)
// =============================================================================
enum class PanelType {
    Slab,        // Dalle de plancher
    Wall,        // Voile vertical
    CurtainWall  // Panneau de bardage
};

enum class FeModelType {
    Shell,       // Coque 3D (Membrane + Flexion)
    Membrane,    // Membrane pure
    PlateRigid   // Diaphragme rigide
};

struct Panel {
    EntityId id = INVALID_ID;
    PanelType type = PanelType::Slab;
    FeModelType feModel = FeModelType::Shell;

    std::vector<EntityId> boundaryNodeIds;
    std::vector<std::vector<EntityId>> openingNodeIds;

    double thickness = 0.15; // en mètres
    EntityId materialId = INVALID_ID;
    double targetMeshSize = 0.50; // taille cible des mailles EF en mètres
};

// =============================================================================
//  4. Conditions d'Appui
// =============================================================================
struct SupportCondition {
    bool tx = true, ty = true, tz = true;
    bool rx = false, ry = false, rz = false;

    // Raideurs élastiques si appui semi-rigide ou sol élastique [kN/m ou kN·m/rad]
    double ktx = 0.0, kty = 0.0, ktz = 0.0;
    double krx = 0.0, kry = 0.0, krz = 0.0;
};

struct Support {
    EntityId id = INVALID_ID;
    EntityId nodeId = INVALID_ID;
    SupportCondition condition;
};

// =============================================================================
//  5. Matériau & Section
// =============================================================================
enum class MaterialCategory { Concrete, Steel, Timber, Generic };

struct Material {
    EntityId id = INVALID_ID;
    std::string name = "C25/30";
    MaterialCategory category = MaterialCategory::Concrete;

    double E   = 31.0e9;   // Module de Young (Pa)
    double nu  = 0.2;      // Poisson
    double rho = 2500.0;   // Masse volumique (kg/m³)
    double fck = 25.0e6;   // Résistance caractéristique (Pa)
};

struct Section {
    EntityId id = INVALID_ID;
    std::string name = "POT 30x30";
    enum class Shape { Rectangle, Circle, IPE, HEA, Tube } shape = Shape::Rectangle;

    double width  = 0.30; // b (m)
    double height = 0.30; // h (m)

    double A  = 0.09;      // Aire (m²)
    double Iy = 6.75e-4;   // Inertie axe fort (m⁴)
    double Iz = 6.75e-4;   // Inertie axe faible (m⁴)
    double It = 1.14e-3;   // Inertie de torsion (m⁴)
};

// =============================================================================
//  6. Cas de Charges, Charges et Combinaisons
// =============================================================================
enum class LoadCaseNature {
    Dead,       // G : Charges permanentes et poids propre
    Live,       // Q : Charges d'exploitation
    Wind,       // W : Vent
    Snow,       // S : Neige
    Seismic,    // E : Séisme
    Accidental  // A : Accidentel
};

struct LoadCase {
    EntityId id = INVALID_ID;
    std::string name = "G - Poids propre";
    LoadCaseNature nature = LoadCaseNature::Dead;
    bool includeSelfWeight = true;
};

enum class LoadType {
    NodalForce,
    MemberUniform,
    MemberTrapezoidal,
    PanelUniform
};

struct Load {
    EntityId id = INVALID_ID;
    EntityId caseId = INVALID_ID;
    LoadType type = LoadType::MemberUniform;

    EntityId targetEntityId = INVALID_ID; // ID du nœud, de la barre ou du panneau

    glm::dvec3 value1{0.0}; // Force ou wStart [kN ou kN/m ou kN/m²]
    glm::dvec3 value2{0.0}; // Moment ou wEnd
    bool isLocal = false;   // Repère local ou global
};

struct CombinationFactor {
    EntityId caseId = INVALID_ID;
    double factor = 1.35;
};

struct LoadCombination {
    EntityId id = INVALID_ID;
    std::string name = "101 : ELU (1.35G + 1.5Q)";
    enum class Type { ULS, SLS, Seismic, Accidental } type = Type::ULS;
    std::vector<CombinationFactor> factors;
};

} // namespace stabileo::structural
