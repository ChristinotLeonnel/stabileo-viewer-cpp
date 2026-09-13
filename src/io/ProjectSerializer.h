#pragma once
// =============================================================================
//  ProjectSerializer.h — Sauvegarde et chargement de projet (.tsa / .json)
// =============================================================================

#include "structural/ModelDatabase.h"
#include <string>

namespace stabileo::io {

class ProjectSerializer {
public:
    static bool saveToFile(const structural::ModelDatabase& db, const std::string& filePath);
    static bool loadFromFile(structural::ModelDatabase& db, const std::string& filePath);
};

} // namespace stabileo::io
