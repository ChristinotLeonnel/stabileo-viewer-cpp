#pragma once
// =============================================================================
//  LinearSolver.h — Solveur d'analyse linéaire élastique 3D par éléments finis
// =============================================================================

#include "scene/StructureModel.h"

namespace solver {

/// Résout le système linéaire global [K]{U} = {F} par la méthode des déplacements (Direct Stiffness Method).
/// 
/// Remplit :
///   - node.displacement (ux, uy, uz en mètres) et node.rotation (rx, ry, rz en radians)
///   - element.N, element.Vy, element.Vz, element.My, element.Mz, element.T le long des stations
///   - element.stressRatio (taux de contrainte équivalente de von Mises sigma / fy)
///   - structure.reactions (forces et moments de réaction aux appuis)
///   - structure.hasResults = true
///
/// Retourne false si la structure est hypostatique / singulière (mécanisme), true si la résolution a réussi.
bool solveLinearStatic(model::Structure& structure);

/// Exécute un test de validation analytique rigoureux (poutre bi-appuyée avec charge répartie)
/// et compare la flèche maximale numérique avec la formule exacte 5*w*L^4 / (384*E*I).
/// Retourne true si l'erreur relative est inférieure à 0.1%.
bool runValidationTest();

} // namespace solver
