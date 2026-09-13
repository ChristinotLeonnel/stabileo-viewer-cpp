// =============================================================================
//  ProfileExtruder.cpp — Contours de sections et extrusion 3D
// =============================================================================

#include "scene/ProfileExtruder.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>

namespace scene {

// ---- Contour 2D d'un profilé I (IPE / HEA / HEB) ----
// Sens horaire dans le plan (Y_local = droite, Z_local = haut)
// Y = axe faible (horizontal), Z = axe fort (vertical)
static void generateIContour(float h, float b, float tw, float tf,
                              std::vector<glm::vec2>& pts,
                              std::vector<glm::vec2>& nrm) {
    float hh = h * 0.5f;
    float hb = b * 0.5f;
    float htw = tw * 0.5f;

    // Contour fermé (sens horaire vu de l'avant)
    // Semelle supérieure
    pts.push_back({ -hb,  hh });
    pts.push_back({  hb,  hh });
    pts.push_back({  hb,  hh - tf });
    pts.push_back({  htw, hh - tf });
    // Âme
    pts.push_back({  htw, -hh + tf });
    // Semelle inférieure
    pts.push_back({  hb,  -hh + tf });
    pts.push_back({  hb,  -hh });
    pts.push_back({ -hb,  -hh });
    pts.push_back({ -hb,  -hh + tf });
    pts.push_back({ -htw, -hh + tf });
    // Âme (côté gauche)
    pts.push_back({ -htw,  hh - tf });
    pts.push_back({ -hb,   hh - tf });

    // Normales 2D (perpendiculaires au segment suivant)
    int n = static_cast<int>(pts.size());
    nrm.resize(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        glm::vec2 edge = pts[static_cast<size_t>(next)] - pts[static_cast<size_t>(i)];
        glm::vec2 perp = glm::normalize(glm::vec2(edge.y, -edge.x));
        nrm[static_cast<size_t>(i)] = perp;
    }
}

// ---- Contour 2D d'un tube rectangulaire ----
static void generateRectTubeContour(float width, float height, float t,
                                     std::vector<glm::vec2>& pts,
                                     std::vector<glm::vec2>& nrm) {
    float hw = width * 0.5f, hh = height * 0.5f;
    // Contour extérieur
    pts.push_back({-hw,  hh});
    pts.push_back({ hw,  hh});
    pts.push_back({ hw, -hh});
    pts.push_back({-hw, -hh});

    int n = static_cast<int>(pts.size());
    nrm.resize(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        glm::vec2 edge = pts[static_cast<size_t>(next)] - pts[static_cast<size_t>(i)];
        nrm[static_cast<size_t>(i)] = glm::normalize(glm::vec2(edge.y, -edge.x));
    }
}

// ---- Contour 2D d'un tube circulaire ----
static void generateCircTubeContour(float diameter, int sectors,
                                     std::vector<glm::vec2>& pts,
                                     std::vector<glm::vec2>& nrm) {
    float r = diameter * 0.5f;
    pts.resize(static_cast<size_t>(sectors));
    nrm.resize(static_cast<size_t>(sectors));
    for (int i = 0; i < sectors; ++i) {
        float theta = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(sectors);
        float c = std::cos(theta), s = std::sin(theta);
        pts[static_cast<size_t>(i)] = {c * r, s * r};
        nrm[static_cast<size_t>(i)] = {c, s};
    }
}

// ---- Contour 2D d'une cornière L ----
static void generateAngleLContour(float h, float b, float t,
                                   std::vector<glm::vec2>& pts,
                                   std::vector<glm::vec2>& nrm) {
    // L simple (branche verticale h, branche horizontale b, épaisseur t)
    pts.push_back({0, 0});
    pts.push_back({b, 0});
    pts.push_back({b, t});
    pts.push_back({t, t});
    pts.push_back({t, h});
    pts.push_back({0, h});

    // Centre sur le centre de gravité approximatif
    glm::vec2 cg(0.0f);
    for (auto& p : pts) cg += p;
    cg /= static_cast<float>(pts.size());
    for (auto& p : pts) p -= cg;

    int n = static_cast<int>(pts.size());
    nrm.resize(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        glm::vec2 edge = pts[static_cast<size_t>(next)] - pts[static_cast<size_t>(i)];
        nrm[static_cast<size_t>(i)] = glm::normalize(glm::vec2(edge.y, -edge.x));
    }
}

// ---- Contour 2D d'un profilé en U (UPN / Channel) ----
static void generateUContour(float h, float b, float tw, float tf,
                             std::vector<glm::vec2>& pts,
                             std::vector<glm::vec2>& nrm) {
    float hh = h * 0.5f;
    float hb = b * 0.5f;
    pts.push_back({ -hb,  hh });
    pts.push_back({  hb,  hh });
    pts.push_back({  hb,  hh - tf });
    pts.push_back({ -hb + tw, hh - tf });
    pts.push_back({ -hb + tw, -hh + tf });
    pts.push_back({  hb, -hh + tf });
    pts.push_back({  hb, -hh });
    pts.push_back({ -hb, -hh });

    int n = static_cast<int>(pts.size());
    nrm.resize(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        glm::vec2 edge = pts[static_cast<size_t>(next)] - pts[static_cast<size_t>(i)];
        nrm[static_cast<size_t>(i)] = glm::normalize(glm::vec2(edge.y, -edge.x));
    }
}

// ---- Contour 2D d'un profilé en T (TEE) ----
static void generateTContour(float h, float b, float tw, float tf,
                             std::vector<glm::vec2>& pts,
                             std::vector<glm::vec2>& nrm) {
    float hh = h * 0.5f;
    float hb = b * 0.5f;
    float htw = tw * 0.5f;
    pts.push_back({ -hb,  hh });
    pts.push_back({  hb,  hh });
    pts.push_back({  hb,  hh - tf });
    pts.push_back({  htw, hh - tf });
    pts.push_back({  htw, -hh });
    pts.push_back({ -htw, -hh });
    pts.push_back({ -htw, hh - tf });
    pts.push_back({ -hb,  hh - tf });

    int n = static_cast<int>(pts.size());
    nrm.resize(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        glm::vec2 edge = pts[static_cast<size_t>(next)] - pts[static_cast<size_t>(i)];
        nrm[static_cast<size_t>(i)] = glm::normalize(glm::vec2(edge.y, -edge.x));
    }
}

// ---- Fonction publique : génération du contour ----
void generateSectionContour(
    const model::Section& section,
    std::vector<glm::vec2>& outPoints,
    std::vector<glm::vec2>& outNormals)
{
    outPoints.clear();
    outNormals.clear();

    switch (section.type) {
        case model::SectionType::IPE:
        case model::SectionType::IPN:
        case model::SectionType::HEA:
        case model::SectionType::HEB:
        case model::SectionType::HEM:
            generateIContour(section.h, section.b, section.tw, section.tf,
                             outPoints, outNormals);
            break;
        case model::SectionType::UPN:
            generateUContour(section.h, section.b, section.tw, section.tf,
                             outPoints, outNormals);
            break;
        case model::SectionType::TEE:
            generateTContour(section.h, section.b, section.tw, section.tf,
                             outPoints, outNormals);
            break;
        case model::SectionType::TUBE_RECT:
            generateRectTubeContour(section.d > 0.0f ? section.d : section.b,
                                    section.d2 > 0.0f ? section.d2 : section.h,
                                    section.t > 0.0f ? section.t : section.tw,
                                    outPoints, outNormals);
            break;
        case model::SectionType::TUBE_CIRC:
            generateCircTubeContour(section.d > 0.0f ? section.d : section.h, 24, outPoints, outNormals);
            break;
        case model::SectionType::ANGLE_L:
            generateAngleLContour(section.h, section.b, section.tw,
                                  outPoints, outNormals);
            break;
        case model::SectionType::RECT_SOLID:
            generateRectTubeContour(section.b, section.h, 0.0f,
                                    outPoints, outNormals);
            break;
        case model::SectionType::CIRC_SOLID:
            generateCircTubeContour(section.d > 0.0f ? section.d : section.h, 24, outPoints, outNormals);
            break;
    }
}

// ---- Repère local d'un élément ----
void computeLocalFrame(
    const glm::vec3& posI, const glm::vec3& posJ,
    float rollAngleDeg,
    glm::vec3& outX, glm::vec3& outY, glm::vec3& outZ)
{
    outX = glm::normalize(posJ - posI);

    // Vecteur "up" global
    glm::vec3 globalUp(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(outX, globalUp)) > 0.99f)
        globalUp = glm::vec3(0.0f, 0.0f, 1.0f);

    outZ = glm::normalize(glm::cross(outX, globalUp));
    outY = glm::cross(outZ, outX);

    // Appliquer le roulis
    if (std::abs(rollAngleDeg) > 0.01f) {
        float rad = glm::radians(rollAngleDeg);
        float c = std::cos(rad), s = std::sin(rad);
        glm::vec3 newY = c * outY + s * outZ;
        glm::vec3 newZ = -s * outY + c * outZ;
        outY = newY;
        outZ = newZ;
    }
}

// ---- Extrusion droite ----
Mesh extrudeProfile(
    const model::Section& section,
    const std::vector<glm::vec3>& axisPoints,
    const std::vector<glm::vec3>& axisUp,
    const std::vector<glm::vec3>& axisFwd)
{
    std::vector<glm::vec2> contour, normals2D;
    generateSectionContour(section, contour, normals2D);
    if (contour.empty()) return Mesh{};

    return Mesh::createExtruded(contour, normals2D, axisPoints, axisUp, axisFwd);
}

// ---- Hermite shape functions ----
static glm::vec3 hermiteInterp(
    float s, float L,
    const glm::vec3& dI, const glm::vec3& rI,
    const glm::vec3& dJ, const glm::vec3& rJ)
{
    float s2 = s * s;
    float s3 = s2 * s;
    float N1 = 1.0f - 3.0f * s2 + 2.0f * s3;
    float N2 = (s - 2.0f * s2 + s3) * L;
    float N3 = 3.0f * s2 - 2.0f * s3;
    float N4 = (-s2 + s3) * L;
    return N1 * dI + N2 * rI + N3 * dJ + N4 * rJ;
}

// ---- Extrusion déformée ----
Mesh extrudeProfileDeformed(
    const model::Section& section,
    const model::Element& element,
    const model::Node& nI, const model::Node& nJ,
    float scale, int numStations)
{
    glm::vec3 posI = nI.position;
    glm::vec3 posJ = nJ.position;
    float L = glm::length(posJ - posI);
    if (L < 1e-6f) return Mesh{};

    glm::vec3 localX, localY, localZ;
    computeLocalFrame(posI, posJ, element.rollAngle, localX, localY, localZ);

    // Construire les points d'axe déformé
    std::vector<glm::vec3> axisPoints, axisUp, axisFwd;
    axisPoints.reserve(static_cast<size_t>(numStations));
    axisUp.reserve(static_cast<size_t>(numStations));
    axisFwd.reserve(static_cast<size_t>(numStations));

    for (int i = 0; i < numStations; ++i) {
        float s = static_cast<float>(i) / static_cast<float>(numStations - 1);

        // Position non déformée
        glm::vec3 p0 = posI + localX * (s * L);

        // Déplacement interpolé (Hermite pour transverse, linéaire pour axial)
        glm::vec3 disp = hermiteInterp(s, L, nI.displacement, nI.rotation,
                                        nJ.displacement, nJ.rotation);

        axisPoints.push_back(p0 + disp * scale);
        axisUp.push_back(localY);
    }

    // Calculer la tangente à chaque station
    for (int i = 0; i < numStations; ++i) {
        glm::vec3 fwd;
        if (i == 0)
            fwd = axisPoints[1] - axisPoints[0];
        else if (i == numStations - 1)
            fwd = axisPoints[static_cast<size_t>(i)] - axisPoints[static_cast<size_t>(i - 1)];
        else
            fwd = axisPoints[static_cast<size_t>(i + 1)] - axisPoints[static_cast<size_t>(i - 1)];
        axisFwd.push_back(glm::normalize(fwd));
    }

    return extrudeProfile(section, axisPoints, axisUp, axisFwd);
}

} // namespace scene
