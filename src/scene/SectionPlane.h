#pragma once
// =============================================================================
//  SectionPlane.h — Plans de coupe dynamiques (Section Cuts & Slicing)
//  Inspiré d'Autodesk Robot Structural Analysis pour l'inspection des structures
// =============================================================================

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace scene {

struct SectionPlanes {
    // Coupe en X (longitudinale / files)
    bool  clipX = false;
    float posX  = 0.0f;
    int   dirX  = 1; // 1: garde X <= posX, -1: garde X >= posX

    // Coupe en Y (hauteur / niveaux / étages)
    bool  clipY = false;
    float posY  = 0.0f;
    int   dirY  = 1; // 1: garde Y <= posY, -1: garde Y >= posY

    // Coupe en Z (transversale / travées)
    bool  clipZ = false;
    float posZ  = 0.0f;
    int   dirZ  = 1; // 1: garde Z <= posZ, -1: garde Z >= posZ

    // Mode tranche d'épaisseur (isole un niveau spécifique entre [pos - e, pos])
    bool  sliceMode = false;
    float sliceThickness = 3.5f; // épaisseur de tranche en mètres (hauteur d'un étage)

    // Affichage visuel du plan de coupe
    bool  showPlaneGizmo = true;

    /// Indique si au moins un plan de coupe est actif
    bool active() const { return clipX || clipY || clipZ; }

    /// Teste si un nœud 3D est visible
    bool isPointVisible(const glm::vec3& p) const {
        if (clipX) {
            if (sliceMode) {
                if (std::abs(p.x - posX) > sliceThickness * 0.5f) return false;
            } else {
                if (dirX > 0 && p.x > posX) return false;
                if (dirX < 0 && p.x < posX) return false;
            }
        }
        if (clipY) {
            if (sliceMode) {
                if (std::abs(p.y - posY) > sliceThickness * 0.5f) return false;
            } else {
                if (dirY > 0 && p.y > posY) return false;
                if (dirY < 0 && p.y < posY) return false;
            }
        }
        if (clipZ) {
            if (sliceMode) {
                if (std::abs(p.z - posZ) > sliceThickness * 0.5f) return false;
            } else {
                if (dirZ > 0 && p.z > posZ) return false;
                if (dirZ < 0 && p.z < posZ) return false;
            }
        }
        return true;
    }

    /// Teste si une barre 3D (reliant p1 à p2) est visible
    bool isElementVisible(const glm::vec3& p1, const glm::vec3& p2) const {
        if (clipX) {
            if (sliceMode) {
                float midX = (p1.x + p2.x) * 0.5f;
                if (std::abs(midX - posX) > sliceThickness * 0.5f) return false;
            } else {
                if (dirX > 0 && p1.x > posX && p2.x > posX) return false;
                if (dirX < 0 && p1.x < posX && p2.x < posX) return false;
            }
        }
        if (clipY) {
            if (sliceMode) {
                float midY = (p1.y + p2.y) * 0.5f;
                if (std::abs(midY - posY) > sliceThickness * 0.5f) return false;
            } else {
                if (dirY > 0 && p1.y > posY && p2.y > posY) return false;
                if (dirY < 0 && p1.y < posY && p2.y < posY) return false;
            }
        }
        if (clipZ) {
            if (sliceMode) {
                float midZ = (p1.z + p2.z) * 0.5f;
                if (std::abs(midZ - posZ) > sliceThickness * 0.5f) return false;
            } else {
                if (dirZ > 0 && p1.z > posZ && p2.z > posZ) return false;
                if (dirZ < 0 && p1.z < posZ && p2.z < posZ) return false;
            }
        }
        return true;
    }
};

} // namespace scene
