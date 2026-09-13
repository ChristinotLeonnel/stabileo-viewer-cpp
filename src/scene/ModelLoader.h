#pragma once
// =============================================================================
//  ModelLoader.h — Chargeur universel de modèles Stabileo JSON
// =============================================================================

#include "scene/StructureModel.h"
#include <string>
#include <vector>

namespace scene {

/// Charge une structure Stabileo depuis un fichier JSON (compatibilité totale avec les fixtures).
model::Structure loadStructureFromJsonFile(const std::string& filepath);

/// Charge une structure Stabileo depuis une chaîne JSON brute.
model::Structure loadStructureFromJsonString(const std::string& jsonString);

/// Calcule des résultats FEA synthétiques réalistes (déplacements, N, V, M, contraintes, réactions)
/// pour toute structure importée qui ne contient pas de stations pré-calculées.
void computeSyntheticResults(model::Structure& structure);

/// Récupère la liste des noms de toutes les fixtures de démonstration disponibles dans assets/models.
std::vector<std::string> getAvailableFixtureNames();

/// Charge une fixture par son nom court (ex: "3d-building", "suspension-bridge").
model::Structure loadFixtureByName(const std::string& name);

} // namespace scene
