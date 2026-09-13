#pragma once
// =============================================================================
//  DemoModels.h — Modèles de démonstration et structures de modélisation Stabileo
// =============================================================================

#include "scene/StructureModel.h"

namespace scene {

// ---- 1. Structures de Base & Poutres ----
model::Structure createDemoPortalFrame();
model::Structure createDemoSpaceTruss();
model::Structure createDemoContinuousBeam();

// ---- 2. Bâtiments & Tours ----
model::Structure createDemo3DBuilding();
model::Structure createDemoDiagridTower();
model::Structure createDemoTower3D();

// ---- 3. Ponts & Passerelles ----
model::Structure createDemoSuspensionBridge();
model::Structure createDemoCableStayedBridge();
model::Structure createDemoWarrenTruss();

// ---- 4. Dômes, Arcs & Grandes Portées ----
model::Structure createDemoGeodesicDome();
model::Structure createDemoThreeHingeArch();
model::Structure createDemoNaveIndustrial();

// ---- 5. Génie Civil & Structures Spéciales ----
model::Structure createDemoOffshorePlatform();
model::Structure createDemoPipeRack();
model::Structure createDemoGridSlab();

} // namespace scene
