#pragma once
// =============================================================================
//  DxfImporter.h — Importateur de fichiers AutoCAD DXF pour StabileoViewer
// =============================================================================

#include "scene/StructureModel.h"
#include <string>
#include <vector>

namespace scene {

enum class DxfUnit {
    Meters,       // Facteur 1.0
    Millimeters,  // Facteur 0.001
    Centimeters,  // Facteur 0.01
    Inches        // Facteur 0.0254
};

enum class DxfUpAxis {
    Y_Up,         // X -> X, Y -> Y, Z -> Z (Élévation 2D standard)
    Z_Up_To_Y_Up  // X -> X, Y -> -Z, Z -> Y (Modèle 3D standard avec Z vertical)
};

struct DxfImportOptions {
    DxfUnit   unit              = DxfUnit::Meters;
    DxfUpAxis upAxis            = DxfUpAxis::Y_Up;
    float     snapTolerance     = 0.005f; // 5 mm tolérance de fusion des nœuds
    bool      autoSupportGround = true;   // Créer automatiquement des appuis sur les nœuds au sol
    bool      addGravityLoad    = true;   // Ajouter une charge test pour permettre le calcul FEA immédiat
};

/// Analyse un fichier DXF et le convertit en model::Structure
model::Structure loadDxf(const std::string& filepath, const DxfImportOptions& options = {});

/// Génère deux fichiers DXF de démonstration (portique industriel et ferme treillis)
void generateSampleDxfFiles(const std::string& targetDir);

} // namespace scene
