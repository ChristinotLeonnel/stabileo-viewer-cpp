#pragma once
// =============================================================================
//  SupportGizmos.h — Génération des gizmos 3D pour les appuis
// =============================================================================

#include "core/Mesh.h"
#include "scene/StructureModel.h"

namespace scene {

/// Crée le mesh 3D d'un gizmo d'appui (encastrement, rotule, appui simple, ressort).
Mesh createSupportGizmo(model::SupportType type, float size = 0.3f);

} // namespace scene
