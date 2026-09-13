// =============================================================================
//  LinearSolver.cpp — Solveur d'analyse linéaire élastique 3D par éléments finis
// =============================================================================

#include "solver/LinearSolver.h"
#include "scene/ProfileExtruder.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>

#include <cmath>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace solver {

bool solveLinearStatic(model::Structure& structure) {
    if (structure.nodes.empty() || structure.elements.empty()) {
        std::cerr << "[LinearSolver] Structure vide, résolution impossible.\n";
        return false;
    }

    const int numNodes = static_cast<int>(structure.nodes.size());
    const int numDofs  = numNodes * 6;

    // 1. Table de correspondance ID Nœud -> Index [0 .. numNodes - 1]
    std::unordered_map<int, int> nodeMap;
    for (int i = 0; i < numNodes; ++i) {
        nodeMap[structure.nodes[static_cast<size_t>(i)].id] = i;
    }

    // 2. Conditions aux limites et appuis
    std::vector<bool>   isFixed(static_cast<size_t>(numDofs), false);
    std::vector<double> springK(static_cast<size_t>(numDofs), 0.0);

    for (const auto& sup : structure.supports) {
        auto it = nodeMap.find(sup.nodeId);
        if (it == nodeMap.end()) continue;
        int base = it->second * 6;

        switch (sup.type) {
            case model::SupportType::FIXED:
                for (int k = 0; k < 6; ++k) isFixed[static_cast<size_t>(base + k)] = true;
                break;
            case model::SupportType::PINNED:
                for (int k = 0; k < 3; ++k) isFixed[static_cast<size_t>(base + k)] = true;
                break;
            case model::SupportType::ROLLER_X:
                isFixed[static_cast<size_t>(base + 1)] = true; // Bloque Y
                isFixed[static_cast<size_t>(base + 2)] = true; // Bloque Z
                break;
            case model::SupportType::ROLLER_Y:
                isFixed[static_cast<size_t>(base + 0)] = true; // Bloque X
                isFixed[static_cast<size_t>(base + 2)] = true; // Bloque Z
                break;
            case model::SupportType::ROLLER_Z:
                isFixed[static_cast<size_t>(base + 0)] = true; // Bloque X
                isFixed[static_cast<size_t>(base + 1)] = true; // Bloque Y
                break;
            case model::SupportType::SPRING:
                springK[static_cast<size_t>(base + 0)] += sup.springK.x;
                springK[static_cast<size_t>(base + 1)] += sup.springK.y;
                springK[static_cast<size_t>(base + 2)] += sup.springK.z;
                break;
        }

        if (sup.blockRx) isFixed[static_cast<size_t>(base + 3)] = true;
        if (sup.blockRy) isFixed[static_cast<size_t>(base + 4)] = true;
        if (sup.blockRz) isFixed[static_cast<size_t>(base + 5)] = true;
    }

    // Détection de modèle plan (2D dans le plan X-Y) :
    // Si tous les nœuds et chargements sont dans le plan X-Y (Z ≈ 0),
    // les DDLs hors-plan (uz, rx, ry) sont automatiquement contraints à zéro.
    bool isPlanarXY = true;
    for (const auto& nd : structure.nodes) {
        if (std::abs(nd.position.z) > 1e-4f) { isPlanarXY = false; break; }
    }
    if (isPlanarXY) {
        for (const auto& nl : structure.nodalLoads) {
            if (std::abs(nl.force.z) > 1e-4f || std::abs(nl.moment.x) > 1e-4f || std::abs(nl.moment.y) > 1e-4f) {
                isPlanarXY = false; break;
            }
        }
    }
    if (isPlanarXY) {
        for (const auto& dl : structure.distributedLoads) {
            if (std::abs(dl.wStart.z) > 1e-4f || std::abs(dl.wEnd.z) > 1e-4f) {
                isPlanarXY = false; break;
            }
        }
    }

    if (isPlanarXY) {
        for (int i = 0; i < numNodes; ++i) {
            isFixed[static_cast<size_t>(i * 6 + 2)] = true; // uz = 0
            isFixed[static_cast<size_t>(i * 6 + 3)] = true; // rx = 0
            isFixed[static_cast<size_t>(i * 6 + 4)] = true; // ry = 0
        }
    }

    // 3. Assemblage du vecteur de charge global {F}
    Eigen::VectorXd F_global = Eigen::VectorXd::Zero(numDofs);

    for (const auto& nl : structure.nodalLoads) {
        auto it = nodeMap.find(nl.nodeId);
        if (it == nodeMap.end()) continue;
        int base = it->second * 6;
        F_global[base + 0] += static_cast<double>(nl.force.x);
        F_global[base + 1] += static_cast<double>(nl.force.y);
        F_global[base + 2] += static_cast<double>(nl.force.z);
        F_global[base + 3] += static_cast<double>(nl.moment.x);
        F_global[base + 4] += static_cast<double>(nl.moment.y);
        F_global[base + 5] += static_cast<double>(nl.moment.z);
    }

    // Structures auxiliaires pour stocker les matrices d'éléments
    struct ElementData {
        Eigen::Matrix<double, 12, 12> T;
        Eigen::Matrix<double, 12, 12> Kl;
        Eigen::Matrix<double, 12, 1>  fl_fe;
        glm::vec3 localX, localY, localZ;
        double L;
        double E_kPa, A, Iy, Iz, J, fy;
        int i1, i2;
    };
    std::vector<ElementData> elemDataList;
    elemDataList.reserve(structure.elements.size());

    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(structure.elements.size() * 144);

    // 4. Assemblage de chaque élément (Poutre Euler-Bernoulli 3D / Treillis)
    for (const auto& el : structure.elements) {
        auto itI = nodeMap.find(el.nodeI);
        auto itJ = nodeMap.find(el.nodeJ);
        if (itI == nodeMap.end() || itJ == nodeMap.end()) continue;

        int i1 = itI->second;
        int i2 = itJ->second;

        const auto& p1 = structure.nodes[static_cast<size_t>(i1)].position;
        const auto& p2 = structure.nodes[static_cast<size_t>(i2)].position;

        double L = static_cast<double>(glm::length(p2 - p1));
        if (L < 1e-6) continue;

        // Repère local
        glm::vec3 lx, ly, lz;
        scene::computeLocalFrame(p1, p2, el.rollAngle, lx, ly, lz);

        // Matrice de rotation 3x3
        Eigen::Matrix3d Lambda;
        Lambda << lx.x, lx.y, lx.z,
                  ly.x, ly.y, ly.z,
                  lz.x, lz.y, lz.z;

        // Matrice de transformation 12x12
        Eigen::Matrix<double, 12, 12> T = Eigen::Matrix<double, 12, 12>::Zero();
        T.block<3, 3>(0, 0) = Lambda;
        T.block<3, 3>(3, 3) = Lambda;
        T.block<3, 3>(6, 6) = Lambda;
        T.block<3, 3>(9, 9) = Lambda;

        // Propriétés de section et matériau
        const auto* sec = structure.findSection(el.sectionId);
        const auto* mat = structure.findMaterial(el.materialId);

        double E_MPa = (sec && sec->E > 0.0f) ? static_cast<double>(sec->E) : (mat ? static_cast<double>(mat->E) : 210000.0);
        double E_kPa = E_MPa * 1000.0; // Conversion MPa -> kN/m²

        double nu = (mat && mat->nu > 0.0f) ? static_cast<double>(mat->nu) : 0.3;
        double G_kPa = E_kPa / (2.0 * (1.0 + nu));

        double A  = sec ? static_cast<double>(sec->A)  : 0.005;
        double Iy = sec ? static_cast<double>(sec->Iy) : 1e-4;
        double Iz = sec ? static_cast<double>(sec->Iz) : 1e-4;
        double fy = sec ? static_cast<double>(sec->fy) : 250.0;

        // Inertie de torsion J
        double J = (sec && sec->type == model::SectionType::TUBE_CIRC) ? (Iy + Iz) : (0.15 * (Iy + Iz));

        // Matrice de rigidité locale Kl (12x12)
        Eigen::Matrix<double, 12, 12> Kl = Eigen::Matrix<double, 12, 12>::Zero();

        double EA_L = E_kPa * A / L;
        Kl(0, 0) =  EA_L; Kl(0, 6) = -EA_L;
        Kl(6, 0) = -EA_L; Kl(6, 6) =  EA_L;

        if (!el.isTruss) {
            double GJ_L = G_kPa * J / L;
            Kl(3, 3) =  GJ_L; Kl(3, 9) = -GJ_L;
            Kl(9, 3) = -GJ_L; Kl(9, 9) =  GJ_L;

            // Flexion dans le plan local x-y (autour de z_l, inertie Iz) : dofs 1, 5, 7, 11
            double EIz_L  = E_kPa * Iz / L;
            double EIz_L2 = EIz_L / L;
            double EIz_L3 = EIz_L2 / L;

            Kl(1, 1)   =  12.0 * EIz_L3; Kl(1, 5)   =   6.0 * EIz_L2; Kl(1, 7)   = -12.0 * EIz_L3; Kl(1, 11)  =   6.0 * EIz_L2;
            Kl(5, 1)   =   6.0 * EIz_L2; Kl(5, 5)   =   4.0 * EIz_L;  Kl(5, 7)   =  -6.0 * EIz_L2; Kl(5, 11)  =   2.0 * EIz_L;
            Kl(7, 1)   = -12.0 * EIz_L3; Kl(7, 5)   =  -6.0 * EIz_L2; Kl(7, 7)   =  12.0 * EIz_L3; Kl(7, 11)  =  -6.0 * EIz_L2;
            Kl(11, 1)  =   6.0 * EIz_L2; Kl(11, 5)  =   2.0 * EIz_L;  Kl(11, 7)  =  -6.0 * EIz_L2; Kl(11, 11) =   4.0 * EIz_L;

            // Flexion dans le plan local x-z (autour de y_l, inertie Iy) : dofs 2, 4, 8, 10
            double EIy_L  = E_kPa * Iy / L;
            double EIy_L2 = EIy_L / L;
            double EIy_L3 = EIy_L2 / L;

            Kl(2, 2)   =  12.0 * EIy_L3; Kl(2, 4)   =  -6.0 * EIy_L2; Kl(2, 8)   = -12.0 * EIy_L3; Kl(2, 10)  =  -6.0 * EIy_L2;
            Kl(4, 2)   =  -6.0 * EIy_L2; Kl(4, 4)   =   4.0 * EIy_L;  Kl(4, 8)   =   6.0 * EIy_L2; Kl(4, 10)  =   2.0 * EIy_L;
            Kl(8, 2)   = -12.0 * EIy_L3; Kl(8, 4)   =   6.0 * EIy_L2; Kl(8, 8)   =  12.0 * EIy_L3; Kl(8, 10)  =   6.0 * EIy_L2;
            Kl(10, 2)  =  -6.0 * EIy_L2; Kl(10, 4)  =   2.0 * EIy_L;  Kl(10, 8)  =   6.0 * EIy_L2; Kl(10, 10) =   4.0 * EIy_L;
        }

        // Matrice élémentaire globale : Kg = T^T * Kl * T
        Eigen::Matrix<double, 12, 12> Kg = T.transpose() * Kl * T;

        // Insertion dans les triplets globaux
        int dofs[12] = {
            i1 * 6 + 0, i1 * 6 + 1, i1 * 6 + 2, i1 * 6 + 3, i1 * 6 + 4, i1 * 6 + 5,
            i2 * 6 + 0, i2 * 6 + 1, i2 * 6 + 2, i2 * 6 + 3, i2 * 6 + 4, i2 * 6 + 5
        };

        for (int r = 0; r < 12; ++r) {
            for (int c = 0; c < 12; ++c) {
                triplets.emplace_back(dofs[r], dofs[c], Kg(r, c));
            }
        }

        // Charges réparties équivalentes (fixed-end forces)
        Eigen::Matrix<double, 12, 1> fl_fe = Eigen::Matrix<double, 12, 1>::Zero();

        for (const auto& dl : structure.distributedLoads) {
            if (dl.elementId != el.id) continue;

            // La charge peut être spécifiée en repère global ou local : projection sur le repère local
            glm::vec3 w_avg = (dl.wStart + dl.wEnd) * 0.5f;
            Eigen::Vector3d wg(w_avg.x, w_avg.y, w_avg.z);
            Eigen::Vector3d wl = Lambda * wg;

            double wx = wl.x();
            double wy = wl.y();
            double wz = wl.z();

            // Traction axiale répartie
            fl_fe(0) += wx * L * 0.5;
            fl_fe(6) += wx * L * 0.5;

            // Flexion en Y local
            fl_fe(1)  += wy * L * 0.5;
            fl_fe(5)  += wy * L * L / 12.0;
            fl_fe(7)  += wy * L * 0.5;
            fl_fe(11) += -wy * L * L / 12.0;

            // Flexion en Z local
            fl_fe(2)  += wz * L * 0.5;
            fl_fe(4)  += -wz * L * L / 12.0;
            fl_fe(8)  += wz * L * 0.5;
            fl_fe(10) += wz * L * L / 12.0;
        }

        if (fl_fe.squaredNorm() > 1e-12) {
            Eigen::Matrix<double, 12, 1> fg_fe = T.transpose() * fl_fe;
            for (int r = 0; r < 12; ++r) {
                F_global[dofs[r]] += fg_fe(r);
            }
        }

        ElementData ed;
        ed.T = T;
        ed.Kl = Kl;
        ed.fl_fe = fl_fe;
        ed.localX = lx; ed.localY = ly; ed.localZ = lz;
        ed.L = L;
        ed.E_kPa = E_kPa; ed.A = A; ed.Iy = Iy; ed.Iz = Iz; ed.J = J; ed.fy = fy;
        ed.i1 = i1; ed.i2 = i2;
        elemDataList.push_back(ed);
    }

    // Ajout des raideurs d'appuis élastiques
    for (int d = 0; d < numDofs; ++d) {
        if (springK[static_cast<size_t>(d)] > 0.0) {
            triplets.emplace_back(d, d, springK[static_cast<size_t>(d)]);
        }
    }

    // Construction de la matrice de rigidité globale creuse K
    Eigen::SparseMatrix<double> K_global(numDofs, numDofs);
    K_global.setFromTriplets(triplets.begin(), triplets.end());

    // Détection des DDLs libres sans aucune raideur connectée (ex: nœuds non connectés ou DDLs singuliers)
    for (int d = 0; d < numDofs; ++d) {
        if (!isFixed[static_cast<size_t>(d)]) {
            double diag = K_global.coeff(d, d);
            if (diag <= 1e-12) {
                isFixed[static_cast<size_t>(d)] = true;
            }
        }
    }

    // 5. Application des conditions aux limites par réduction du système (DDL libres)
    std::vector<int> globalToFree(static_cast<size_t>(numDofs), -1);
    std::vector<int> freeToGlobal;
    freeToGlobal.reserve(static_cast<size_t>(numDofs));

    for (int d = 0; d < numDofs; ++d) {
        if (!isFixed[static_cast<size_t>(d)]) {
            globalToFree[static_cast<size_t>(d)] = static_cast<int>(freeToGlobal.size());
            freeToGlobal.push_back(d);
        }
    }

    const int nFree = static_cast<int>(freeToGlobal.size());
    if (nFree == 0) {
        std::cerr << "[LinearSolver] Erreur : Aucun DDL libre dans la structure.\n";
        return false;
    }

    Eigen::VectorXd F_free(nFree);
    for (int i = 0; i < nFree; ++i) {
        F_free[i] = F_global[freeToGlobal[static_cast<size_t>(i)]];
    }

    std::vector<Eigen::Triplet<double>> freeTriplets;
    freeTriplets.reserve(triplets.size());

    for (int k = 0; k < K_global.outerSize(); ++k) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(K_global, k); it; ++it) {
            int r = static_cast<int>(it.row());
            int c = static_cast<int>(it.col());
            if (!isFixed[static_cast<size_t>(r)] && !isFixed[static_cast<size_t>(c)]) {
                freeTriplets.emplace_back(globalToFree[static_cast<size_t>(r)],
                                         globalToFree[static_cast<size_t>(c)],
                                         it.value());
            }
        }
    }

    Eigen::SparseMatrix<double> K_free(nFree, nFree);
    K_free.setFromTriplets(freeTriplets.begin(), freeTriplets.end());

    // 6. Résolution K_ff * U_f = F_f via décomposition Cholesky creuse LDLT
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
    solver.compute(K_free);

    if (solver.info() != Eigen::Success) {
        std::cerr << "[LinearSolver] Décomposition échouée : structure singulière / hypostatique (mécanisme).\n";
        return false;
    }

    Eigen::VectorXd U_free = solver.solve(F_free);
    if (solver.info() != Eigen::Success || !U_free.allFinite()) {
        std::cerr << "[LinearSolver] Échec de la résolution du système linéaire (instabilité ou singularité).\n";
        return false;
    }

    // 7. Reconstruction du vecteur de déplacements complet {U}
    Eigen::VectorXd U_global = Eigen::VectorXd::Zero(numDofs);
    for (int i = 0; i < nFree; ++i) {
        U_global[freeToGlobal[static_cast<size_t>(i)]] = U_free[i];
    }

    // 8. Report des déplacements et rotations dans chaque model::Node
    for (int i = 0; i < numNodes; ++i) {
        auto& nd = structure.nodes[static_cast<size_t>(i)];
        nd.displacement = glm::vec3(
            static_cast<float>(U_global[i * 6 + 0]),
            static_cast<float>(U_global[i * 6 + 1]),
            static_cast<float>(U_global[i * 6 + 2])
        );
        nd.rotation = glm::vec3(
            static_cast<float>(U_global[i * 6 + 3]),
            static_cast<float>(U_global[i * 6 + 4]),
            static_cast<float>(U_global[i * 6 + 5])
        );
    }

    // 9. Calcul des réactions d'appui : R = K_global * U_global - F_applied
    Eigen::VectorXd R_global = K_global * U_global - F_global;

    structure.reactions.clear();
    for (const auto& sup : structure.supports) {
        auto it = nodeMap.find(sup.nodeId);
        if (it == nodeMap.end()) continue;
        int base = it->second * 6;

        model::Reaction react;
        react.nodeId = sup.nodeId;
        react.force  = glm::vec3(
            static_cast<float>(R_global[base + 0]),
            static_cast<float>(R_global[base + 1]),
            static_cast<float>(R_global[base + 2])
        );
        react.moment = glm::vec3(
            static_cast<float>(R_global[base + 3]),
            static_cast<float>(R_global[base + 4]),
            static_cast<float>(R_global[base + 5])
        );
        structure.reactions.push_back(react);
    }

    // 10. Recombinaison des efforts internes locaux et interpolation le long des barres
    for (size_t eIdx = 0; eIdx < structure.elements.size(); ++eIdx) {
        auto& el = structure.elements[eIdx];
        if (eIdx >= elemDataList.size()) break;
        const auto& ed = elemDataList[eIdx];

        // Déplacements globaux de l'élément
        Eigen::Matrix<double, 12, 1> ug;
        for (int k = 0; k < 6; ++k) {
            ug(k)     = U_global[ed.i1 * 6 + k];
            ug(k + 6) = U_global[ed.i2 * 6 + k];
        }

        // Déplacements locaux
        Eigen::Matrix<double, 12, 1> ul = ed.T * ug;

        // Forces aux extrémités en repère local : f_local = Kl * ul - fl_fe
        Eigen::Matrix<double, 12, 1> fl_end = ed.Kl * ul - ed.fl_fe;

        // Efforts au début de la barre (x = 0)
        double N0   = -fl_end(0);  // Traction positive
        double Vy0  =  fl_end(1);
        double Vz0  =  fl_end(2);
        double T0   = -fl_end(3);
        double My0  = -fl_end(4);
        double Mz0  = -fl_end(5);

        // Charge répartie locale moyenne sur l'élément
        double wx = 0.0, wy = 0.0, wz = 0.0;
        for (const auto& dl : structure.distributedLoads) {
            if (dl.elementId == el.id) {
                glm::vec3 w_avg = (dl.wStart + dl.wEnd) * 0.5f;
                Eigen::Vector3d wg(w_avg.x, w_avg.y, w_avg.z);
                Eigen::Vector3d wl = ed.T.block<3, 3>(0, 0) * wg;
                wx += wl.x();
                wy += wl.y();
                wz += wl.z();
            }
        }

        // Échantillonnage aux stations (par défaut 21 stations si non initialisé)
        if (el.stations.empty()) {
            const int numStations = 21;
            el.stations.resize(numStations);
            for (int i = 0; i < numStations; ++i) {
                el.stations[static_cast<size_t>(i)] = static_cast<float>(i) / static_cast<float>(numStations - 1);
            }
        }

        const size_t nSt = el.stations.size();
        el.N.resize(nSt);
        el.Vy.resize(nSt);
        el.Vz.resize(nSt);
        el.My.resize(nSt);
        el.Mz.resize(nSt);
        el.T.resize(nSt);
        el.stressRatio.resize(nSt);

        const auto* sec = structure.findSection(el.sectionId);
        double secH = sec ? static_cast<double>(sec->h) : 0.3;
        double secB = sec ? static_cast<double>(sec->b) : 0.15;

        for (size_t s = 0; s < nSt; ++s) {
            double x = static_cast<double>(el.stations[s]) * ed.L;

            // Interpolation analytique exacte d'Euler-Bernoulli
            double N_x  = N0 - wx * x;
            double Vy_x = Vy0 - wy * x;
            double Vz_x = Vz0 - wz * x;
            double T_x  = T0;
            double Mz_x = Mz0 + Vy0 * x - 0.5 * wy * x * x;
            double My_x = My0 - Vz0 * x + 0.5 * wz * x * x;

            el.N[s]  = static_cast<float>(N_x);
            el.Vy[s] = static_cast<float>(Vy_x);
            el.Vz[s] = static_cast<float>(Vz_x);
            el.My[s] = static_cast<float>(My_x);
            el.Mz[s] = static_cast<float>(Mz_x);
            el.T[s]  = static_cast<float>(T_x);

            // Contrainte équivalente et taux d'utilisation sigma / fy
            double sigma_axial  = std::abs(N_x) / (ed.A > 0.0 ? ed.A : 0.005);
            double sigma_flex_z = std::abs(Mz_x) * (secH * 0.5) / (ed.Iz > 0.0 ? ed.Iz : 1e-4);
            double sigma_flex_y = std::abs(My_x) * (secB * 0.5) / (ed.Iy > 0.0 ? ed.Iy : 1e-4);
            double sigma_total_kPa = sigma_axial + sigma_flex_z + sigma_flex_y;
            double sigma_total_MPa = sigma_total_kPa / 1000.0;

            double ratio = sigma_total_MPa / (ed.fy > 0.0 ? ed.fy : 250.0);
            el.stressRatio[s] = static_cast<float>(std::clamp(ratio, 0.0, 5.0));
        }
    }

    structure.hasResults = true;
    return true;
}

// =============================================================================
//  Test de Validation Analytique (Poutre bi-appuyée avec charge répartie)
// =============================================================================
bool runValidationTest() {
    model::Structure beam;
    beam.name = "Test Validation Analytique — Poutre bi-appuyée";

    // Section IPE 300
    model::Section sec;
    sec.id = 1;
    sec.name = "IPE 300";
    sec.h = 0.300f; sec.b = 0.150f; sec.tw = 0.0071f; sec.tf = 0.0107f;
    sec.A = 53.8e-4f;
    sec.Iz = 8356e-8f; // Inertie axe fort [m⁴]
    sec.Iy = 604e-8f;
    sec.E = 210000.0f; // [MPa] = 2.1e8 kPa
    sec.fy = 235.0f;
    beam.sections.push_back(sec);

    // Longueur totale L = 6.0 m, discrétisée en 2 éléments (nœud central au milieu)
    const float L = 6.0f;
    beam.nodes = {
        {1, {0.0f, 0.0f, 0.0f}},
        {2, {L * 0.5f, 0.0f, 0.0f}},
        {3, {L, 0.0f, 0.0f}}
    };

    beam.elements = {
        {1, 1, 2, 1},
        {2, 2, 3, 1}
    };

    // Appuis : Rotule à x=0, Appui simple (glissière X) à x=L
    beam.supports = {
        {1, model::SupportType::PINNED},
        {3, model::SupportType::ROLLER_X}
    };

    // Charge répartie uniforme vers le bas q = -10.0 kN/m
    const float q = -10.0f;
    model::DistributedLoad dl1; dl1.elementId = 1; dl1.wStart = {0, q, 0}; dl1.wEnd = {0, q, 0};
    model::DistributedLoad dl2; dl2.elementId = 2; dl2.wStart = {0, q, 0}; dl2.wEnd = {0, q, 0};
    beam.distributedLoads = {dl1, dl2};

    // Résolution EF
    bool ok = solveLinearStatic(beam);
    if (!ok) {
        std::cerr << "[ValidationTest] Échec de la résolution numérique.\n";
        return false;
    }

    // Flèche numérique au milieu (nœud 2)
    float v_fem = std::abs(beam.nodes[1].displacement.y);

    // Flèche théorique analytique exacte : delta_max = 5 * |q| * L^4 / (384 * E * Iz)
    double E_kPa = static_cast<double>(sec.E) * 1000.0;
    double Iz = static_cast<double>(sec.Iz);
    double v_exact = (5.0 * std::abs(static_cast<double>(q)) * std::pow(static_cast<double>(L), 4.0)) /
                     (384.0 * E_kPa * Iz);

    double relError = std::abs(static_cast<double>(v_fem) - v_exact) / v_exact;

    std::cout << "\n======================================================\n";
    std::cout << "  VALIDATION DU SOLVEUR ÉLÉMENTS FINIS 3D (DSM)\n";
    std::cout << "======================================================\n";
    std::cout << "  Flèche analytique exacte 5wL^4/(384EI) : " << (v_exact * 1000.0) << " mm\n";
    std::cout << "  Flèche calculée par le solveur EF       : " << (v_fem * 1000.0) << " mm\n";
    std::cout << "  Erreur relative                         : " << (relError * 100.0) << " %\n";

    if (relError < 0.001) { // Moins de 0.1% d'erreur
        std::cout << "  --> [SUCCÈS] Le solveur est validé avec une précision exacte !\n";
        std::cout << "======================================================\n\n";
        std::cout.flush();
        return true;
    } else {
        std::cerr << "  --> [ÉCHEC] Écart supérieur à la tolérance autorisée.\n";
        std::cout << "======================================================\n\n";
        std::cerr.flush();
        std::cout.flush();
        return false;
    }
}

} // namespace solver
