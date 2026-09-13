// =============================================================================
//  SupportGizmos.cpp — Géométrie 3D des appuis structurels
// =============================================================================

#include "scene/SupportGizmos.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace scene {

// Génère un mesh combiné pour le gizmo d'un appui
Mesh createSupportGizmo(model::SupportType type, float size) {
    switch (type) {

    // ---- Encastrement : boîte épaisse hachurée ----
    case model::SupportType::FIXED:
        return Mesh::createBox(size * 1.6f, size * 0.4f, size * 1.6f);

    // ---- Rotule : cône pointant vers le haut ----
    case model::SupportType::PINNED:
        return Mesh::createCone(size * 0.5f, size * 0.8f, 16);

    // ---- Appui simple (rouleaux) : cône + base plate ----
    case model::SupportType::ROLLER_X:
    case model::SupportType::ROLLER_Y:
    case model::SupportType::ROLLER_Z: {
        // On crée un cône comme le pinned — la distinction visuelle
        // (trait horizontal sous la base) est gérée par le renderer
        // en ajoutant une ligne au sol.
        std::vector<Vertex> verts;
        std::vector<uint32_t> idx;

        // Cône
        int sectors = 16;
        float coneR = size * 0.45f;
        float coneH = size * 0.7f;
        float slope = coneR / coneH;

        uint32_t tip = 0;
        verts.push_back({{0, coneH, 0}, {0, 1, 0}});

        for (int j = 0; j <= sectors; ++j) {
            float t = glm::two_pi<float>() * static_cast<float>(j) / static_cast<float>(sectors);
            float c = std::cos(t), s = std::sin(t);
            glm::vec3 n = glm::normalize(glm::vec3(c, slope, s));
            verts.push_back({{c * coneR, 0, s * coneR}, n});
        }
        for (int j = 0; j < sectors; ++j) {
            idx.insert(idx.end(), {tip,
                static_cast<uint32_t>(j + 2),
                static_cast<uint32_t>(j + 1)});
        }

        // Petite barre de base (plaque)
        uint32_t baseOff = static_cast<uint32_t>(verts.size());
        float pw = size * 0.7f, ph = size * 0.08f;
        // Top face
        verts.push_back({{-pw, -ph, -pw}, {0, -1, 0}});
        verts.push_back({{ pw, -ph, -pw}, {0, -1, 0}});
        verts.push_back({{ pw, -ph,  pw}, {0, -1, 0}});
        verts.push_back({{-pw, -ph,  pw}, {0, -1, 0}});
        idx.insert(idx.end(), {baseOff, baseOff+1, baseOff+2,
                               baseOff, baseOff+2, baseOff+3});

        Mesh m;
        m.upload(verts, idx);
        return m;
    }

    // ---- Ressort : hélice ----
    case model::SupportType::SPRING: {
        std::vector<Vertex> verts;
        std::vector<uint32_t> idx;

        int coils = 4;
        int segsPerCoil = 16;
        int totalSegs = coils * segsPerCoil;
        float springH = size * 1.2f;
        float springR = size * 0.25f;
        float tubeR = size * 0.04f;
        int tubeSectors = 6;

        // Génère un tube le long de l'hélice
        for (int i = 0; i <= totalSegs; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(totalSegs);
            float angle = glm::two_pi<float>() * static_cast<float>(coils) * t;
            float y = t * springH;

            glm::vec3 center(std::cos(angle) * springR, y, std::sin(angle) * springR);

            // Tangente de l'hélice
            float dangle = glm::two_pi<float>() * static_cast<float>(coils);
            glm::vec3 tangent = glm::normalize(glm::vec3(
                -std::sin(angle) * springR * dangle,
                springH,
                std::cos(angle) * springR * dangle
            ));

            glm::vec3 up(0, 1, 0);
            if (std::abs(glm::dot(tangent, up)) > 0.99f) up = glm::vec3(1, 0, 0);
            glm::vec3 binormal = glm::normalize(glm::cross(tangent, up));
            glm::vec3 normal = glm::cross(binormal, tangent);

            for (int j = 0; j < tubeSectors; ++j) {
                float phi = glm::two_pi<float>() * static_cast<float>(j) / static_cast<float>(tubeSectors);
                glm::vec3 n = std::cos(phi) * normal + std::sin(phi) * binormal;
                verts.push_back({center + n * tubeR, n});
            }
        }

        // Indices
        for (int i = 0; i < totalSegs; ++i) {
            int ringA = i * tubeSectors;
            int ringB = (i + 1) * tubeSectors;
            for (int j = 0; j < tubeSectors; ++j) {
                int jn = (j + 1) % tubeSectors;
                uint32_t a = static_cast<uint32_t>(ringA + j);
                uint32_t b = static_cast<uint32_t>(ringB + j);
                uint32_t c = static_cast<uint32_t>(ringB + jn);
                uint32_t d = static_cast<uint32_t>(ringA + jn);
                idx.insert(idx.end(), {a, b, c, a, c, d});
            }
        }

        Mesh m;
        m.upload(verts, idx);
        return m;
    }

    default:
        return Mesh::createBox(size, size, size);
    }
}

} // namespace scene
