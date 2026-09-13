#pragma once
// =============================================================================
//  BuildingGenerator.h — Générateur paramétrique R+2 et passerelle vers model::Structure
// =============================================================================

#include "structural/ModelDatabase.h"
#include "scene/StructureModel.h"

namespace stabileo::structural {

class BuildingGenerator {
public:
    /// Génère le bâtiment complet R+2 (15 m × 10 m, Z = 0, 3, 6, 9 m, Poteaux 30x30, Poutres 25x50, Dalles 15cm, C25/30)
    static void generateBuildingRPlus2(ModelDatabase& db);

    /// Convertit le modèle de données métier ModelDatabase vers la structure de rendu et de calcul model::Structure
    static void syncToLegacyStructure(const ModelDatabase& db, model::Structure& outStructure);
};

} // namespace stabileo::structural
