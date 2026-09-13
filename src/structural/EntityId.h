#pragma once
// =============================================================================
//  EntityId.h — Identifiants uniques pour le modèle structurel
// =============================================================================

#include <cstdint>

namespace stabileo::structural {

using EntityId = uint64_t;
constexpr EntityId INVALID_ID = 0;

enum class StructuralType {
    Node,
    Member,
    Panel,
    Support,
    Material,
    Section,
    LoadCase,
    Load,
    Combination
};

} // namespace stabileo::structural
