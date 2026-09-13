// =============================================================================
//  DemoModels.cpp — Implémentation des modèles de modélisation Stabileo en C++
// =============================================================================

#include "scene/DemoModels.h"
#include "scene/ModelLoader.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <vector>
#include <algorithm>

namespace scene {

// Helper pour créer les sections standard
static model::Section makeSection(int id, model::SectionType type, const std::string& name,
                                  float h, float b, float tw, float tf, float fy = 250.0f) {
    model::Section s;
    s.id = id;
    s.type = type;
    s.name = name;
    s.h = h; s.b = b; s.tw = tw; s.tf = tf;
    s.d = h; s.t = tw; s.d2 = b;
    s.fy = fy;
    s.A = h * b * 0.45f;
    s.Iy = (b * std::pow(h, 3.0f)) / 12.0f;
    s.Iz = (h * std::pow(b, 3.0f)) / 12.0f;
    return s;
}

// =============================================================================
//  1. Portique 2D — 2 poteaux + 1 traverse
// =============================================================================
model::Structure createDemoPortalFrame() {
    // Si la fixture existe sur disque, on peut la charger directement
    model::Structure s = loadFixtureByName("3d-portal-frame");
    if (!s.nodes.empty()) return s;

    s.name = "Portique 2D — IPE 300 / HEA 240";
    s.sections.push_back(makeSection(1, model::SectionType::HEA, "HEA 240", 0.230f, 0.240f, 0.0075f, 0.012f));
    s.sections.push_back(makeSection(2, model::SectionType::IPE, "IPE 300", 0.300f, 0.150f, 0.0071f, 0.0107f));

    float H = 4.5f, L = 7.0f;
    s.nodes = {
        {1, {0, 0, 0}},
        {2, {0, H, 0}},
        {3, {L, H, 0}},
        {4, {L, 0, 0}}
    };

    s.elements = {
        {1, 1, 2, 1}, // Poteau G
        {2, 2, 3, 2}, // Traverse
        {3, 4, 3, 1}  // Poteau D
    };

    s.supports = {
        {1, model::SupportType::FIXED},
        {4, model::SupportType::FIXED}
    };

    model::DistributedLoad dl;
    dl.elementId = 2;
    dl.wStart = glm::vec3(0, -25.0f, 0);
    dl.wEnd   = glm::vec3(0, -25.0f, 0);
    s.distributedLoads.push_back(dl);

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  2. Treillis Spatial 3D (Pyramide)
// =============================================================================
model::Structure createDemoSpaceTruss() {
    model::Structure s = loadFixtureByName("3d-space-truss");
    if (!s.nodes.empty()) return s;

    s.name = "Treillis Spatial 3D — Pyramide";
    s.sections.push_back(makeSection(1, model::SectionType::TUBE_CIRC, "CHS 88.9x5", 0.0889f, 0.0889f, 0.005f, 0.005f));

    float b = 3.0f, h = 4.0f;
    s.nodes = {
        {1, {-b, 0, -b}},
        {2, { b, 0, -b}},
        {3, { b, 0,  b}},
        {4, {-b, 0,  b}},
        {5, { 0, h,  0}}  // Sommet
    };

    s.elements = {
        {1, 1, 2, 1, 1, 0, true},
        {2, 2, 3, 1, 1, 0, true},
        {3, 3, 4, 1, 1, 0, true},
        {4, 4, 1, 1, 1, 0, true},
        {5, 1, 5, 1, 1, 0, true},
        {6, 2, 5, 1, 1, 0, true},
        {7, 3, 5, 1, 1, 0, true},
        {8, 4, 5, 1, 1, 0, true}
    };

    s.supports = {
        {1, model::SupportType::PINNED},
        {2, model::SupportType::ROLLER_X},
        {3, model::SupportType::ROLLER_Z},
        {4, model::SupportType::PINNED}
    };

    model::NodalLoad nl;
    nl.nodeId = 5;
    nl.force = glm::vec3(0, -150.0f, 0);
    s.nodalLoads.push_back(nl);

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  3. Poutre Continue 4 travées
// =============================================================================
model::Structure createDemoContinuousBeam() {
    model::Structure s = loadFixtureByName("continuous-beam");
    if (!s.nodes.empty()) return s;

    s.name = "Poutre Continue 4 Travées (24 m)";
    s.sections.push_back(makeSection(1, model::SectionType::IPE, "IPE 360", 0.360f, 0.170f, 0.008f, 0.0127f));

    float span = 6.0f;
    for (int i = 0; i <= 4; ++i) {
        s.nodes.push_back({i + 1, {static_cast<float>(i) * span, 2.0f, 0.0f}});
        if (i == 0) s.supports.push_back({i + 1, model::SupportType::PINNED});
        else        s.supports.push_back({i + 1, model::SupportType::ROLLER_X});
    }

    for (int i = 0; i < 4; ++i) {
        model::Element el;
        el.id = i + 1;
        el.nodeI = i + 1;
        el.nodeJ = i + 2;
        el.sectionId = 1;
        s.elements.push_back(el);

        model::DistributedLoad dl;
        dl.elementId = el.id;
        dl.wStart = glm::vec3(0, -30.0f, 0);
        dl.wEnd   = glm::vec3(0, -30.0f, 0);
        s.distributedLoads.push_back(dl);
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  4. Pont Suspendu (Suspension Bridge)
// =============================================================================
model::Structure createDemoSuspensionBridge() {
    model::Structure s = loadFixtureByName("suspension-bridge");
    if (!s.nodes.empty()) return s;

    s.name = "Pont Suspendu 3D (Suspension Bridge)";
    s.sections.push_back(makeSection(1, model::SectionType::CIRC_SOLID, "Cable Principal D280", 0.280f, 0.280f, 0.280f, 0.280f, 1600.0f));
    s.sections.push_back(makeSection(2, model::SectionType::HEB, "HEB 600 (Pylônes)", 0.600f, 0.300f, 0.0155f, 0.030f));
    s.sections.push_back(makeSection(3, model::SectionType::IPE, "IPE 400 (Tablier)", 0.400f, 0.180f, 0.0086f, 0.0135f));
    s.sections.push_back(makeSection(4, model::SectionType::CIRC_SOLID, "Suspente D35", 0.035f, 0.035f, 0.035f, 0.035f, 1200.0f));

    float totalL = 72.0f;
    float towerH = 18.0f;
    float deckY  = 4.0f;
    float width  = 6.0f;
    int numPanels = 18;
    float dx = totalL / static_cast<float>(numPanels);

    int nId = 1;
    int eId = 1;

    // Deux lignes de tablier (gauche Z=-width/2, droite Z=+width/2)
    std::vector<int> deckLeft, deckRight;
    for (int i = 0; i <= numPanels; ++i) {
        float x = -totalL * 0.5f + static_cast<float>(i) * dx;
        s.nodes.push_back({nId, {x, deckY, -width * 0.5f}});
        deckLeft.push_back(nId++);
        s.nodes.push_back({nId, {x, deckY,  width * 0.5f}});
        deckRight.push_back(nId++);
    }

    // Appuis aux extrémités du tablier
    s.supports.push_back({deckLeft.front(), model::SupportType::PINNED});
    s.supports.push_back({deckRight.front(), model::SupportType::PINNED});
    s.supports.push_back({deckLeft.back(), model::SupportType::ROLLER_X});
    s.supports.push_back({deckRight.back(), model::SupportType::ROLLER_X});

    // Poutres longitudinales et transversales du tablier
    for (size_t i = 0; i < deckLeft.size() - 1; ++i) {
        s.elements.push_back({eId++, deckLeft[i], deckLeft[i+1], 3});
        s.elements.push_back({eId++, deckRight[i], deckRight[i+1], 3});
        s.elements.push_back({eId++, deckLeft[i], deckRight[i], 3}); // Entretoise
    }
    s.elements.push_back({eId++, deckLeft.back(), deckRight.back(), 3});

    // Pylônes verticaux aux positions 1/4 et 3/4
    float xT1 = -totalL * 0.25f;
    float xT2 =  totalL * 0.25f;

    auto addTower = [&](float tx) {
        // Bases pylône
        int bL = nId++; s.nodes.push_back({bL, {tx, 0, -width * 0.5f}});
        int bR = nId++; s.nodes.push_back({bR, {tx, 0,  width * 0.5f}});
        s.supports.push_back({bL, model::SupportType::FIXED});
        s.supports.push_back({bR, model::SupportType::FIXED});

        // Sommets pylône
        int tL = nId++; s.nodes.push_back({tL, {tx, towerH, -width * 0.5f}});
        int tR = nId++; s.nodes.push_back({tR, {tx, towerH,  width * 0.5f}});

        // Poteaux et croisillons de pylône
        s.elements.push_back({eId++, bL, tL, 2});
        s.elements.push_back({eId++, bR, tR, 2});
        s.elements.push_back({eId++, tL, tR, 2});
        return std::make_pair(tL, tR);
    };

    auto [t1L, t1R] = addTower(xT1);
    auto [t2L, t2R] = addTower(xT2);

    // Câbles principaux paraboliques et suspentes
    std::vector<int> cableLeft, cableRight;
    for (int i = 0; i <= numPanels; ++i) {
        float x = -totalL * 0.5f + static_cast<float>(i) * dx;
        float yCable = deckY + 1.5f;

        if (x < xT1) {
            float t = (x - (-totalL * 0.5f)) / (xT1 - (-totalL * 0.5f));
            yCable = deckY + 1.0f + t * (towerH - (deckY + 1.0f));
        } else if (x <= xT2) {
            float u = (x - xT1) / (xT2 - xT1);
            float sag = 10.0f;
            yCable = towerH - 4.0f * sag * u * (1.0f - u);
        } else {
            float t = (x - xT2) / (totalL * 0.5f - xT2);
            yCable = towerH - t * (towerH - (deckY + 1.0f));
        }

        int cL = nId++; s.nodes.push_back({cL, {x, yCable, -width * 0.5f}});
        cableLeft.push_back(cL);
        int cR = nId++; s.nodes.push_back({cR, {x, yCable,  width * 0.5f}});
        cableRight.push_back(cR);

        // Suspente verticale vers le tablier
        if (std::abs(x - xT1) > 0.5f && std::abs(x - xT2) > 0.5f) {
            s.elements.push_back({eId++, deckLeft[static_cast<size_t>(i)], cL, 4, 1, 0, false, true});
            s.elements.push_back({eId++, deckRight[static_cast<size_t>(i)], cR, 4, 1, 0, false, true});
        }
    }

    // Segments de câbles principaux
    for (size_t i = 0; i < cableLeft.size() - 1; ++i) {
        s.elements.push_back({eId++, cableLeft[i], cableLeft[i+1], 1, 1, 0, false, true});
        s.elements.push_back({eId++, cableRight[i], cableRight[i+1], 1, 1, 0, false, true});
    }

    // Ancrages des câbles au sol
    s.supports.push_back({cableLeft.front(), model::SupportType::FIXED});
    s.supports.push_back({cableRight.front(), model::SupportType::FIXED});
    s.supports.push_back({cableLeft.back(), model::SupportType::FIXED});
    s.supports.push_back({cableRight.back(), model::SupportType::FIXED});

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  5. Pont à Haubans (Cable-Stayed Bridge)
// =============================================================================
model::Structure createDemoCableStayedBridge() {
    model::Structure s = loadFixtureByName("cable-stayed-bridge");
    if (!s.nodes.empty()) return s;

    s.name = "Pont à Haubans (Cable-Stayed Bridge)";
    s.sections.push_back(makeSection(1, model::SectionType::HEB, "Pylône Central HEB 800", 0.800f, 0.300f, 0.0175f, 0.033f));
    s.sections.push_back(makeSection(2, model::SectionType::IPE, "Tablier IPE 450", 0.450f, 0.190f, 0.0094f, 0.0146f));
    s.sections.push_back(makeSection(3, model::SectionType::CIRC_SOLID, "Haubans Acier D45", 0.045f, 0.045f, 0.045f, 0.045f, 1600.0f));

    float L = 60.0f;
    float pylonH = 22.0f;
    float deckY = 3.0f;
    float width = 5.0f;
    int nId = 1, eId = 1;

    // Pylône central
    int pBaseL = nId++; s.nodes.push_back({pBaseL, {0, 0, -width * 0.5f}});
    int pBaseR = nId++; s.nodes.push_back({pBaseR, {0, 0,  width * 0.5f}});
    int pTopL  = nId++; s.nodes.push_back({pTopL,  {0, pylonH, -width * 0.5f}});
    int pTopR  = nId++; s.nodes.push_back({pTopR,  {0, pylonH,  width * 0.5f}});

    s.supports.push_back({pBaseL, model::SupportType::FIXED});
    s.supports.push_back({pBaseR, model::SupportType::FIXED});
    s.elements.push_back({eId++, pBaseL, pTopL, 1});
    s.elements.push_back({eId++, pBaseR, pTopR, 1});
    s.elements.push_back({eId++, pTopL, pTopR, 1});

    // Tablier
    int nP = 14;
    float dx = L / static_cast<float>(nP);
    std::vector<int> deckL, deckR;

    for (int i = 0; i <= nP; ++i) {
        float x = -L * 0.5f + static_cast<float>(i) * dx;
        int dL = nId++; s.nodes.push_back({dL, {x, deckY, -width * 0.5f}});
        int dR = nId++; s.nodes.push_back({dR, {x, deckY,  width * 0.5f}});
        deckL.push_back(dL);
        deckR.push_back(dR);

        if (i > 0) {
            s.elements.push_back({eId++, deckL[static_cast<size_t>(i-1)], dL, 2});
            s.elements.push_back({eId++, deckR[static_cast<size_t>(i-1)], dR, 2});
            s.elements.push_back({eId++, dL, dR, 2});
        }

        // Haubans rayonnants depuis le pylône
        if (std::abs(x) > 3.0f && std::abs(x) < L * 0.48f) {
            s.elements.push_back({eId++, pTopL, dL, 3, 1, 0, false, true});
            s.elements.push_back({eId++, pTopR, dR, 3, 1, 0, false, true});
        }
    }

    s.supports.push_back({deckL.front(), model::SupportType::PINNED});
    s.supports.push_back({deckR.front(), model::SupportType::PINNED});
    s.supports.push_back({deckL.back(), model::SupportType::ROLLER_X});
    s.supports.push_back({deckR.back(), model::SupportType::ROLLER_X});

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  6. Bâtiment 3D Multi-étages (3D Building)
// =============================================================================
model::Structure createDemo3DBuilding() {
    model::Structure s = loadFixtureByName("3d-building");
    if (!s.nodes.empty()) return s;

    s.name = "Bâtiment 3D Industriel (4 étages)";
    s.sections.push_back(makeSection(1, model::SectionType::HEB, "Poteaux HEB 300", 0.300f, 0.300f, 0.011f, 0.019f));
    s.sections.push_back(makeSection(2, model::SectionType::IPE, "Poutres IPE 360", 0.360f, 0.170f, 0.008f, 0.0127f));
    s.sections.push_back(makeSection(3, model::SectionType::ANGLE_L, "Diagonales L 100x10", 0.100f, 0.100f, 0.010f, 0.010f));

    int nx = 4, nz = 3, ny = 4;
    float dx = 5.0f, dz = 5.0f, dy = 3.5f;
    int nId = 1, eId = 1;

    // Matrice de nœuds 3D [ix][iy][iz]
    std::vector<std::vector<std::vector<int>>> grid(
        static_cast<size_t>(nx + 1),
        std::vector<std::vector<int>>(static_cast<size_t>(ny + 1), std::vector<int>(static_cast<size_t>(nz + 1), 0))
    );

    for (int iy = 0; iy <= ny; ++iy) {
        for (int ix = 0; ix <= nx; ++ix) {
            for (int iz = 0; iz <= nz; ++iz) {
                int id = nId++;
                s.nodes.push_back({id, {static_cast<float>(ix) * dx, static_cast<float>(iy) * dy, static_cast<float>(iz) * dz}});
                grid[static_cast<size_t>(ix)][static_cast<size_t>(iy)][static_cast<size_t>(iz)] = id;

                if (iy == 0) {
                    s.supports.push_back({id, model::SupportType::FIXED});
                }
            }
        }
    }

    // Poteaux (verticaux) et poutres (X et Z)
    for (int iy = 0; iy < ny; ++iy) {
        for (int ix = 0; ix <= nx; ++ix) {
            for (int iz = 0; iz <= nz; ++iz) {
                // Poteau vertical
                s.elements.push_back({eId++, grid[static_cast<size_t>(ix)][static_cast<size_t>(iy)][static_cast<size_t>(iz)],
                                             grid[static_cast<size_t>(ix)][static_cast<size_t>(iy+1)][static_cast<size_t>(iz)], 1});
            }
        }
    }

    for (int iy = 1; iy <= ny; ++iy) {
        for (int ix = 0; ix <= nx; ++ix) {
            for (int iz = 0; iz <= nz; ++iz) {
                // Poutres longitudinales X
                if (ix < nx) {
                    s.elements.push_back({eId++, grid[static_cast<size_t>(ix)][static_cast<size_t>(iy)][static_cast<size_t>(iz)],
                                                 grid[static_cast<size_t>(ix+1)][static_cast<size_t>(iy)][static_cast<size_t>(iz)], 2});
                }
                // Poutres transversales Z
                if (iz < nz) {
                    s.elements.push_back({eId++, grid[static_cast<size_t>(ix)][static_cast<size_t>(iy)][static_cast<size_t>(iz)],
                                                 grid[static_cast<size_t>(ix)][static_cast<size_t>(iy)][static_cast<size_t>(iz+1)], 2});
                }
            }
        }
        // Contreventement en croix sur les baies extérieures
        s.elements.push_back({eId++, grid[0][static_cast<size_t>(iy-1)][0], grid[1][static_cast<size_t>(iy)][0], 3, 1, 0, true});
        s.elements.push_back({eId++, grid[1][static_cast<size_t>(iy-1)][0], grid[0][static_cast<size_t>(iy)][0], 3, 1, 0, true});
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  7. Tour Diagrid (XL Diagrid Tower)
// =============================================================================
model::Structure createDemoDiagridTower() {
    model::Structure s = loadFixtureByName("xl-diagrid-tower");
    if (!s.nodes.empty()) return s;

    s.name = "Tour Diagrid Spatiale (XL Tower)";
    s.sections.push_back(makeSection(1, model::SectionType::TUBE_CIRC, "Diagrid CHS 355x12", 0.355f, 0.355f, 0.012f, 0.012f));
    s.sections.push_back(makeSection(2, model::SectionType::IPE, "Ceintures IPE 300", 0.300f, 0.150f, 0.0071f, 0.0107f));

    int numLevels = 8;
    int numSides = 8;
    float levelH = 4.0f;
    float radius = 7.0f;
    int nId = 1, eId = 1;

    std::vector<std::vector<int>> rings(static_cast<size_t>(numLevels + 1));

    for (int lev = 0; lev <= numLevels; ++lev) {
        float y = static_cast<float>(lev) * levelH;
        // Légère conicité
        float r = radius * (1.0f - 0.25f * (static_cast<float>(lev) / static_cast<float>(numLevels)));

        for (int i = 0; i < numSides; ++i) {
            float angle = static_cast<float>(i) * glm::two_pi<float>() / static_cast<float>(numSides);
            int id = nId++;
            s.nodes.push_back({id, {r * std::cos(angle), y, r * std::sin(angle)}});
            rings[static_cast<size_t>(lev)].push_back(id);

            if (lev == 0) {
                s.supports.push_back({id, model::SupportType::FIXED});
            }
        }
    }

    // Connexions de la grille losangique Diagrid
    for (int lev = 0; lev < numLevels; ++lev) {
        for (int i = 0; i < numSides; ++i) {
            int nextI = (i + 1) % numSides;
            int prevI = (i - 1 + numSides) % numSides;

            int nCurr = rings[static_cast<size_t>(lev)][static_cast<size_t>(i)];
            int nNext = rings[static_cast<size_t>(lev)][static_cast<size_t>(nextI)];

            // Poutre circonférentielle de ceinture
            s.elements.push_back({eId++, nCurr, nNext, 2});

            // Diagonales croisées
            int nUpNext = rings[static_cast<size_t>(lev+1)][static_cast<size_t>(nextI)];
            int nUpPrev = rings[static_cast<size_t>(lev+1)][static_cast<size_t>(prevI)];
            s.elements.push_back({eId++, nCurr, nUpNext, 1});
            s.elements.push_back({eId++, nCurr, nUpPrev, 1});
        }
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  8. Pylône de Transmission 3D (Tower 3D)
// =============================================================================
model::Structure createDemoTower3D() {
    model::Structure s = loadFixtureByName("tower-3d");
    if (!s.nodes.empty()) return s;

    s.name = "Pylône Haute Tension 3D (Transmission Tower)";
    s.sections.push_back(makeSection(1, model::SectionType::ANGLE_L, "Montants L 120x12", 0.120f, 0.120f, 0.012f, 0.012f));
    s.sections.push_back(makeSection(2, model::SectionType::ANGLE_L, "Croisillons L 80x8", 0.080f, 0.080f, 0.008f, 0.008f));

    float heights[] = {0.0f, 6.0f, 13.0f, 20.0f, 26.0f, 32.0f};
    float widths[]  = {6.0f, 4.2f,  3.0f,  2.2f,  2.0f,  1.8f};
    int nLevels = 5;
    int nId = 1, eId = 1;

    std::vector<std::vector<int>> lvl(static_cast<size_t>(nLevels + 1));

    for (int l = 0; l <= nLevels; ++l) {
        float w = widths[l] * 0.5f;
        float y = heights[l];

        int n1 = nId++; s.nodes.push_back({n1, {-w, y, -w}});
        int n2 = nId++; s.nodes.push_back({n2, { w, y, -w}});
        int n3 = nId++; s.nodes.push_back({n3, { w, y,  w}});
        int n4 = nId++; s.nodes.push_back({n4, {-w, y,  w}});
        lvl[static_cast<size_t>(l)] = {n1, n2, n3, n4};

        if (l == 0) {
            s.supports.push_back({n1, model::SupportType::PINNED});
            s.supports.push_back({n2, model::SupportType::PINNED});
            s.supports.push_back({n3, model::SupportType::PINNED});
            s.supports.push_back({n4, model::SupportType::PINNED});
        }
    }

    // Montants et croisillons en X
    for (int l = 0; l < nLevels; ++l) {
        for (int i = 0; i < 4; ++i) {
            int nextI = (i + 1) % 4;
            int bA = lvl[static_cast<size_t>(l)][static_cast<size_t>(i)];
            int bB = lvl[static_cast<size_t>(l)][static_cast<size_t>(nextI)];
            int tA = lvl[static_cast<size_t>(l+1)][static_cast<size_t>(i)];
            int tB = lvl[static_cast<size_t>(l+1)][static_cast<size_t>(nextI)];

            s.elements.push_back({eId++, bA, tA, 1}); // Montant
            s.elements.push_back({eId++, tA, tB, 2}); // Ceinture
            s.elements.push_back({eId++, bA, tB, 2, 1, 0, true}); // Diagonale 1
            s.elements.push_back({eId++, bB, tA, 2, 1, 0, true}); // Diagonale 2
        }
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  9. Dôme Géodésique (Geodesic Dome)
// =============================================================================
model::Structure createDemoGeodesicDome() {
    model::Structure s = loadFixtureByName("geodesic-dome");
    if (!s.nodes.empty()) return s;

    s.name = "Dôme Géodésique Réticulé (Geodesic Dome)";
    s.sections.push_back(makeSection(1, model::SectionType::TUBE_CIRC, "CHS 76.1x4 (Barres Dôme)", 0.0761f, 0.0761f, 0.004f, 0.004f));

    float R = 12.0f;
    int ringsCount = 5;
    int sectors = 12;
    int nId = 1, eId = 1;

    std::vector<std::vector<int>> ringNodes(static_cast<size_t>(ringsCount + 1));

    // Nœud central au sommet
    int apexId = nId++;
    s.nodes.push_back({apexId, {0, R, 0}});
    ringNodes[0].push_back(apexId);

    // Anneaux concentriques
    for (int r = 1; r <= ringsCount; ++r) {
        float phi = static_cast<float>(r) * (glm::pi<float>() * 0.45f) / static_cast<float>(ringsCount);
        float y = R * std::cos(phi);
        float rXY = R * std::sin(phi);

        for (int i = 0; i < sectors; ++i) {
            float theta = static_cast<float>(i) * glm::two_pi<float>() / static_cast<float>(sectors);
            int id = nId++;
            s.nodes.push_back({id, {rXY * std::cos(theta), y, rXY * std::sin(theta)}});
            ringNodes[static_cast<size_t>(r)].push_back(id);

            if (r == ringsCount) {
                s.supports.push_back({id, model::SupportType::PINNED});
            }
        }
    }

    // Reliure du sommet
    for (int i = 0; i < sectors; ++i) {
        s.elements.push_back({eId++, apexId, ringNodes[1][static_cast<size_t>(i)], 1, 1, 0, true});
    }

    // Triangulation entre anneaux
    for (int r = 1; r <= ringsCount; ++r) {
        for (int i = 0; i < sectors; ++i) {
            int nextI = (i + 1) % sectors;
            int nA = ringNodes[static_cast<size_t>(r)][static_cast<size_t>(i)];
            int nB = ringNodes[static_cast<size_t>(r)][static_cast<size_t>(nextI)];
            s.elements.push_back({eId++, nA, nB, 1, 1, 0, true}); // Horizontal

            if (r < ringsCount) {
                int nNextRing = ringNodes[static_cast<size_t>(r+1)][static_cast<size_t>(i)];
                int nNextRingNext = ringNodes[static_cast<size_t>(r+1)][static_cast<size_t>(nextI)];
                s.elements.push_back({eId++, nA, nNextRing, 1, 1, 0, true});
                s.elements.push_back({eId++, nA, nNextRingNext, 1, 1, 0, true});
            }
        }
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  10. Plateforme Offshore (Jacket Platform)
// =============================================================================
model::Structure createDemoOffshorePlatform() {
    model::Structure s = loadFixtureByName("offshore-platform");
    if (!s.nodes.empty()) return s;

    s.name = "Plateforme Offshore (Jacket Platform)";
    s.sections.push_back(makeSection(1, model::SectionType::TUBE_CIRC, "Jambes Tubulaires D1200x40", 1.200f, 1.200f, 0.040f, 0.040f));
    s.sections.push_back(makeSection(2, model::SectionType::TUBE_CIRC, "Contreventements K D600x20", 0.600f, 0.600f, 0.020f, 0.020f));
    s.sections.push_back(makeSection(3, model::SectionType::HEB, "Poutres Tablier HEB 600", 0.600f, 0.300f, 0.0155f, 0.030f));

    float elevations[] = {0.0f, 8.0f, 17.0f, 26.0f, 32.0f};
    float halfSpreads[] = {8.0f, 6.8f,  5.6f,  4.5f,  4.5f};
    int nLevels = 4;
    int nId = 1, eId = 1;

    std::vector<std::vector<int>> lvl(static_cast<size_t>(nLevels + 1));

    for (int l = 0; l <= nLevels; ++l) {
        float hs = halfSpreads[l];
        float y = elevations[l];

        int n1 = nId++; s.nodes.push_back({n1, {-hs, y, -hs}});
        int n2 = nId++; s.nodes.push_back({n2, { hs, y, -hs}});
        int n3 = nId++; s.nodes.push_back({n3, { hs, y,  hs}});
        int n4 = nId++; s.nodes.push_back({n4, {-hs, y,  hs}});
        lvl[static_cast<size_t>(l)] = {n1, n2, n3, n4};

        if (l == 0) {
            s.supports.push_back({n1, model::SupportType::FIXED});
            s.supports.push_back({n2, model::SupportType::FIXED});
            s.supports.push_back({n3, model::SupportType::FIXED});
            s.supports.push_back({n4, model::SupportType::FIXED});
        }
    }

    // Jambes inclinées et contreventement K
    for (int l = 0; l < nLevels; ++l) {
        for (int i = 0; i < 4; ++i) {
            int nextI = (i + 1) % 4;
            int bA = lvl[static_cast<size_t>(l)][static_cast<size_t>(i)];
            int bB = lvl[static_cast<size_t>(l)][static_cast<size_t>(nextI)];
            int tA = lvl[static_cast<size_t>(l+1)][static_cast<size_t>(i)];
            int tB = lvl[static_cast<size_t>(l+1)][static_cast<size_t>(nextI)];

            s.elements.push_back({eId++, bA, tA, 1}); // Jambe principale
            s.elements.push_back({eId++, tA, tB, 2}); // Ceinture horizontale

            // Contreventement K avec nœud central
            glm::vec3 posBA = s.nodes[static_cast<size_t>(bA-1)].position;
            glm::vec3 posBB = s.nodes[static_cast<size_t>(bB-1)].position;
            int midNode = nId++;
            s.nodes.push_back({midNode, (posBA + posBB) * 0.5f});

            s.elements.push_back({eId++, tA, midNode, 2});
            s.elements.push_back({eId++, tB, midNode, 2});
        }
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  11. Hangar Industriel (Nave Industrial)
// =============================================================================
model::Structure createDemoNaveIndustrial() {
    model::Structure s = loadFixtureByName("3d-nave-industrial");
    if (!s.nodes.empty()) return s;

    s.name = "Hangar Industriel 3D (Nave Industrial)";
    s.sections.push_back(makeSection(1, model::SectionType::HEA, "Poteaux HEA 260", 0.250f, 0.260f, 0.0075f, 0.0125f));
    s.sections.push_back(makeSection(2, model::SectionType::IPE, "Arbalétriers IPE 300", 0.300f, 0.150f, 0.0071f, 0.0107f));
    s.sections.push_back(makeSection(3, model::SectionType::UPN, "Pannes UPN 160", 0.160f, 0.065f, 0.0075f, 0.0105f));

    float span = 14.0f;
    float eaveH = 5.0f;
    float ridgeH = 7.5f;
    float bayL = 5.5f;
    int numBays = 4;
    int nId = 1, eId = 1;

    std::vector<int> colLeft, colRight, ridgeNodes;

    for (int b = 0; b <= numBays; ++b) {
        float z = static_cast<float>(b) * bayL;

        int nFootL = nId++; s.nodes.push_back({nFootL, {-span * 0.5f, 0, z}});
        int nEaveL = nId++; s.nodes.push_back({nEaveL, {-span * 0.5f, eaveH, z}});
        int nRidge = nId++; s.nodes.push_back({nRidge, {0, ridgeH, z}});
        int nEaveR = nId++; s.nodes.push_back({nEaveR, { span * 0.5f, eaveH, z}});
        int nFootR = nId++; s.nodes.push_back({nFootR, { span * 0.5f, 0, z}});

        s.supports.push_back({nFootL, model::SupportType::PINNED});
        s.supports.push_back({nFootR, model::SupportType::PINNED});

        // Portique transversal
        s.elements.push_back({eId++, nFootL, nEaveL, 1});
        s.elements.push_back({eId++, nEaveL, nRidge, 2});
        s.elements.push_back({eId++, nRidge, nEaveR, 2});
        s.elements.push_back({eId++, nFootR, nEaveR, 1});

        colLeft.push_back(nEaveL);
        colRight.push_back(nEaveR);
        ridgeNodes.push_back(nRidge);
    }

    // Pannes longitudinales
    for (int b = 0; b < numBays; ++b) {
        s.elements.push_back({eId++, colLeft[static_cast<size_t>(b)], colLeft[static_cast<size_t>(b+1)], 3});
        s.elements.push_back({eId++, colRight[static_cast<size_t>(b)], colRight[static_cast<size_t>(b+1)], 3});
        s.elements.push_back({eId++, ridgeNodes[static_cast<size_t>(b)], ridgeNodes[static_cast<size_t>(b+1)], 3});
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  12. Arc Parabolique 3D (Three-Hinge Arch)
// =============================================================================
model::Structure createDemoThreeHingeArch() {
    model::Structure s = loadFixtureByName("three-hinge-arch");
    if (!s.nodes.empty()) return s;

    s.name = "Arc Parabolique 3D (Three-Hinge Arch)";
    s.sections.push_back(makeSection(1, model::SectionType::HEB, "Arc HEB 400", 0.400f, 0.300f, 0.0135f, 0.024f));
    s.sections.push_back(makeSection(2, model::SectionType::IPE, "Tablier IPE 300", 0.300f, 0.150f, 0.0071f, 0.0107f));
    s.sections.push_back(makeSection(3, model::SectionType::TUBE_CIRC, "Potelets CHS 114x6", 0.1143f, 0.1143f, 0.006f, 0.006f));

    float span = 30.0f;
    float rise = 7.5f;
    float width = 4.0f;
    int numSeg = 12;
    float dx = span / static_cast<float>(numSeg);
    int nId = 1, eId = 1;

    for (int side = -1; side <= 1; side += 2) {
        float z = static_cast<float>(side) * width * 0.5f;
        std::vector<int> archNodes, deckNodes;

        for (int i = 0; i <= numSeg; ++i) {
            float x = -span * 0.5f + static_cast<float>(i) * dx;
            float yArch = 4.0f * rise * (1.0f - (x * x) / (span * span * 0.25f));
            yArch = std::max(0.0f, yArch);

            int aId = nId++; s.nodes.push_back({aId, {x, yArch, z}});
            archNodes.push_back(aId);

            int dId = nId++; s.nodes.push_back({dId, {x, rise + 1.0f, z}});
            deckNodes.push_back(dId);

            // Potelets verticaux
            if (i > 0 && i < numSeg) {
                s.elements.push_back({eId++, aId, dId, 3});
            }
        }

        s.supports.push_back({archNodes.front(), model::SupportType::PINNED});
        s.supports.push_back({archNodes.back(), model::SupportType::PINNED});

        for (size_t i = 0; i < archNodes.size() - 1; ++i) {
            s.elements.push_back({eId++, archNodes[i], archNodes[i+1], 1});
            s.elements.push_back({eId++, deckNodes[i], deckNodes[i+1], 2});
        }
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  13. Grille Spatiale de Plancher (Grid Slab)
// =============================================================================
model::Structure createDemoGridSlab() {
    model::Structure s = loadFixtureByName("3d-grid-slab");
    if (!s.nodes.empty()) return s;

    s.name = "Plancher Réticulé 3D (Grid Slab)";
    s.sections.push_back(makeSection(1, model::SectionType::IPE, "Poutrelles IPE 270", 0.270f, 0.135f, 0.0066f, 0.0102f));

    int nx = 6, nz = 6;
    float dx = 2.5f, dz = 2.5f;
    int nId = 1, eId = 1;

    std::vector<std::vector<int>> g(static_cast<size_t>(nx + 1), std::vector<int>(static_cast<size_t>(nz + 1), 0));

    for (int ix = 0; ix <= nx; ++ix) {
        for (int iz = 0; iz <= nz; ++iz) {
            int id = nId++;
            s.nodes.push_back({id, {static_cast<float>(ix) * dx, 3.0f, static_cast<float>(iz) * dz}});
            g[static_cast<size_t>(ix)][static_cast<size_t>(iz)] = id;

            // Appuis simples sur le pourtour
            if (ix == 0 || ix == nx || iz == 0 || iz == nz) {
                s.supports.push_back({id, model::SupportType::PINNED});
            }
        }
    }

    for (int ix = 0; ix <= nx; ++ix) {
        for (int iz = 0; iz <= nz; ++iz) {
            if (ix < nx) s.elements.push_back({eId++, g[static_cast<size_t>(ix)][static_cast<size_t>(iz)], g[static_cast<size_t>(ix+1)][static_cast<size_t>(iz)], 1});
            if (iz < nz) s.elements.push_back({eId++, g[static_cast<size_t>(ix)][static_cast<size_t>(iz)], g[static_cast<size_t>(ix)][static_cast<size_t>(iz+1)], 1});
        }
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  14. Pipe-Rack Industriel (Petrochemical Pipe-Rack)
// =============================================================================
model::Structure createDemoPipeRack() {
    model::Structure s = loadFixtureByName("pipe-rack");
    if (!s.nodes.empty()) return s;

    s.name = "Pipe-Rack Industriel 3 Niveaux";
    s.sections.push_back(makeSection(1, model::SectionType::HEB, "Poteaux HEB 240", 0.240f, 0.240f, 0.010f, 0.017f));
    s.sections.push_back(makeSection(2, model::SectionType::IPE, "Traverses IPE 300", 0.300f, 0.150f, 0.0071f, 0.0107f));
    s.sections.push_back(makeSection(3, model::SectionType::TUBE_RECT, "Longerons RHS 150x100", 0.150f, 0.100f, 0.006f, 0.006f));

    float levels[] = {0.0f, 3.5f, 6.0f, 8.5f};
    int nLev = 3;
    float bentW = 6.0f;
    float baySpacing = 6.0f;
    int numBays = 4;
    int nId = 1, eId = 1;

    std::vector<std::vector<std::pair<int, int>>> bents(static_cast<size_t>(numBays + 1));

    for (int b = 0; b <= numBays; ++b) {
        float z = static_cast<float>(b) * baySpacing;

        int footL = nId++; s.nodes.push_back({footL, {-bentW * 0.5f, 0, z}});
        int footR = nId++; s.nodes.push_back({footR, { bentW * 0.5f, 0, z}});
        s.supports.push_back({footL, model::SupportType::FIXED});
        s.supports.push_back({footR, model::SupportType::FIXED});

        int prevL = footL, prevR = footR;

        for (int l = 1; l <= nLev; ++l) {
            float y = levels[l];
            int nodeL = nId++; s.nodes.push_back({nodeL, {-bentW * 0.5f, y, z}});
            int nodeR = nId++; s.nodes.push_back({nodeR, { bentW * 0.5f, y, z}});

            s.elements.push_back({eId++, prevL, nodeL, 1}); // Poteau G
            s.elements.push_back({eId++, prevR, nodeR, 1}); // Poteau D
            s.elements.push_back({eId++, nodeL, nodeR, 2}); // Traverse

            bents[static_cast<size_t>(b)].push_back({nodeL, nodeR});
            prevL = nodeL; prevR = nodeR;
        }
    }

    // Longerons reliant les portiques
    for (int b = 0; b < numBays; ++b) {
        for (int l = 0; l < nLev; ++l) {
            auto [currL, currR] = bents[static_cast<size_t>(b)][static_cast<size_t>(l)];
            auto [nextL, nextR] = bents[static_cast<size_t>(b+1)][static_cast<size_t>(l)];
            s.elements.push_back({eId++, currL, nextL, 3});
            s.elements.push_back({eId++, currR, nextR, 3});
        }
    }

    computeSyntheticResults(s);
    return s;
}

// =============================================================================
//  15. Pont Treillis Warren (Warren Truss)
// =============================================================================
model::Structure createDemoWarrenTruss() {
    model::Structure s = loadFixtureByName("warren-truss");
    if (!s.nodes.empty()) return s;

    s.name = "Pont Treillis Warren 3D (Warren Truss)";
    s.sections.push_back(makeSection(1, model::SectionType::HEB, "Membrures HEB 280", 0.280f, 0.280f, 0.0105f, 0.018f));
    s.sections.push_back(makeSection(2, model::SectionType::TUBE_RECT, "Diagonales RHS 160x160", 0.160f, 0.160f, 0.008f, 0.008f));
    s.sections.push_back(makeSection(3, model::SectionType::IPE, "Entretoises IPE 330", 0.330f, 0.160f, 0.0075f, 0.0115f));

    float panelL = 4.0f;
    float trussH = 4.5f;
    float width  = 5.0f;
    int numPanels = 6;
    int nId = 1, eId = 1;

    for (int side = -1; side <= 1; side += 2) {
        float z = static_cast<float>(side) * width * 0.5f;
        std::vector<int> botNodes, topNodes;

        for (int i = 0; i <= numPanels; ++i) {
            int id = nId++;
            s.nodes.push_back({id, {static_cast<float>(i) * panelL, 2.0f, z}});
            botNodes.push_back(id);
        }

        for (int i = 0; i < numPanels; ++i) {
            int id = nId++;
            s.nodes.push_back({id, {(static_cast<float>(i) + 0.5f) * panelL, 2.0f + trussH, z}});
            topNodes.push_back(id);
        }

        // Membrure inférieure
        for (size_t i = 0; i < botNodes.size() - 1; ++i)
            s.elements.push_back({eId++, botNodes[i], botNodes[i+1], 1});

        // Membrure supérieure
        for (size_t i = 0; i < topNodes.size() - 1; ++i)
            s.elements.push_back({eId++, topNodes[i], topNodes[i+1], 1});

        // Diagonales en zig-zag Warren
        for (int i = 0; i < numPanels; ++i) {
            s.elements.push_back({eId++, botNodes[static_cast<size_t>(i)], topNodes[static_cast<size_t>(i)], 2, 1, 0, true});
            s.elements.push_back({eId++, topNodes[static_cast<size_t>(i)], botNodes[static_cast<size_t>(i+1)], 2, 1, 0, true});
        }

        s.supports.push_back({botNodes.front(), model::SupportType::PINNED});
        s.supports.push_back({botNodes.back(), model::SupportType::ROLLER_X});
    }

    computeSyntheticResults(s);
    return s;
}

} // namespace scene
