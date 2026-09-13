// =============================================================================
//  ViewCube.cpp — Widget 3D interactif d'orientation spatiale (style Autodesk Robot)
// =============================================================================

#include "ui/ViewCube.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace ui {

namespace {

// Test d'appartenance d'un point dans un quadrilatère convexe 2D
static bool pointInQuad(const ImVec2& p, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d) {
    auto cross = [](const ImVec2& o, const ImVec2& u, const ImVec2& v) {
        return (u.x - o.x) * (v.y - o.y) - (u.y - o.y) * (v.x - o.x);
    };
    float c1 = cross(a, b, p);
    float c2 = cross(b, c, p);
    float c3 = cross(c, d, p);
    float c4 = cross(d, a, p);
    bool hasNeg = (c1 < 0) || (c2 < 0) || (c3 < 0) || (c4 < 0);
    bool hasPos = (c1 > 0) || (c2 > 0) || (c3 > 0) || (c4 > 0);
    return !(hasNeg && hasPos);
}

struct FaceDef {
    int v[4];
    glm::vec3 normal;
    const char* label;
    int faceId; // 0: Face, 1: Arrière, 2: Haut, 3: Bas, 4: Droite, 5: Gauche
};

} // namespace

bool ViewCube::draw(Camera& camera, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    // Position du centre du ViewCube dans le coin supérieur droit
    float marginX = 90.0f;
    float marginY = 85.0f;
    ImVec2 center(vp->Pos.x + vp->Size.x - marginX, vp->Pos.y + marginY);
    float s = cubeSize * 0.38f;

    // Matrice d'orientation caméra (rotation pure)
    glm::mat3 viewRot = glm::mat3(camera.getViewMatrix());

    // 8 sommets du cube unité [-1, 1]³
    const glm::vec3 localVertices[8] = {
        {-1.0f, -1.0f, -1.0f}, // 0: LBB
        { 1.0f, -1.0f, -1.0f}, // 1: RBB
        { 1.0f,  1.0f, -1.0f}, // 2: RTB
        {-1.0f,  1.0f, -1.0f}, // 3: LTB
        {-1.0f, -1.0f,  1.0f}, // 4: LBF
        { 1.0f, -1.0f,  1.0f}, // 5: RBF
        { 1.0f,  1.0f,  1.0f}, // 6: RTF
        {-1.0f,  1.0f,  1.0f}  // 7: LTF
    };

    // Projection 3D vers l'écran 2D
    glm::vec3 camVertices[8];
    ImVec2 screenVertices[8];
    for (int i = 0; i < 8; ++i) {
        camVertices[i] = viewRot * (localVertices[i] * s);
        screenVertices[i] = ImVec2(center.x + camVertices[i].x, center.y - camVertices[i].y);
    }

    // Définition des 6 faces du cube
    const FaceDef faces[6] = {
        {{4, 5, 6, 7}, { 0.0f,  0.0f,  1.0f}, "FACE",    0}, // Face (Z+)
        {{1, 0, 3, 2}, { 0.0f,  0.0f, -1.0f}, "ARRIERE", 1}, // Arrière (Z-)
        {{7, 6, 2, 3}, { 0.0f,  1.0f,  0.0f}, "HAUT",    2}, // Haut / Plan (Y+)
        {{0, 1, 5, 4}, { 0.0f, -1.0f,  0.0f}, "BAS",     3}, // Bas (Y-)
        {{5, 1, 2, 6}, { 1.0f,  0.0f,  0.0f}, "DROITE",  4}, // Droite (X+)
        {{0, 4, 7, 3}, {-1.0f,  0.0f,  0.0f}, "GAUCHE",  5}  // Gauche (X-)
    };

    // Tri des faces visibles (normal.z > 0 en repère caméra)
    struct VisibleFace {
        int index;
        float depth;
    };
    std::vector<VisibleFace> visibleFaces;

    for (int i = 0; i < 6; ++i) {
        glm::vec3 camNormal = viewRot * faces[i].normal;
        if (camNormal.z > 0.02f) { // Face orientée vers la caméra
            float avgZ = 0.0f;
            for (int k = 0; k < 4; ++k) avgZ += camVertices[faces[i].v[k]].z;
            avgZ *= 0.25f;
            visibleFaces.push_back({i, avgZ});
        }
    }

    // Tri peintre : du fond vers l'avant
    std::sort(visibleFaces.begin(), visibleFaces.end(), [](const VisibleFace& a, const VisibleFace& b) {
        return a.depth < b.depth;
    });

    ImVec2 mousePos = io.MousePos;
    int clickedFace = -1;
    int hoveredFace = -1;
    bool actionTriggered = false;

    // Détection du survol de face
    for (auto it = visibleFaces.rbegin(); it != visibleFaces.rend(); ++it) {
        const auto& f = faces[it->index];
        const ImVec2& a = screenVertices[f.v[0]];
        const ImVec2& b = screenVertices[f.v[1]];
        const ImVec2& c = screenVertices[f.v[2]];
        const ImVec2& d = screenVertices[f.v[3]];

        if (pointInQuad(mousePos, a, b, c, d)) {
            hoveredFace = it->index;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                clickedFace = it->index;
            }
            break;
        }
    }

    // 1. Dessin de l'anneau boussole (Compass Ring) sous le cube
    if (showCompass) {
        float ringRadius = s * 1.55f;
        int ringSegments = 36;
        std::vector<ImVec2> ringPts(ringSegments);
        for (int i = 0; i < ringSegments; ++i) {
            float th = (static_cast<float>(i) / ringSegments) * 2.0f * 3.14159265f;
            glm::vec3 worldPt(std::sin(th) * ringRadius, -s * 1.25f, std::cos(th) * ringRadius);
            glm::vec3 camPt = viewRot * worldPt;
            ringPts[i] = ImVec2(center.x + camPt.x, center.y - camPt.y);
        }
        for (int i = 0; i < ringSegments; ++i) {
            drawList->AddLine(ringPts[i], ringPts[(i + 1) % ringSegments], IM_COL32(70, 85, 110, 160), 1.5f);
        }

        // Points cardinaux Nord, Sud, Est, Ouest
        struct Cardinal { const char* name; glm::vec3 pos; ImU32 col; };
        Cardinal cards[4] = {
            {"N", { 0.0f, -s * 1.25f,  ringRadius * 1.08f}, IM_COL32(240, 70, 70, 240)},
            {"S", { 0.0f, -s * 1.25f, -ringRadius * 1.08f}, IM_COL32(180, 190, 205, 200)},
            {"E", { ringRadius * 1.08f, -s * 1.25f,  0.0f}, IM_COL32(180, 190, 205, 200)},
            {"O", {-ringRadius * 1.08f, -s * 1.25f,  0.0f}, IM_COL32(180, 190, 205, 200)}
        };

        for (const auto& c : cards) {
            glm::vec3 camPt = viewRot * c.pos;
            ImVec2 scrPt(center.x + camPt.x, center.y - camPt.y);
            ImVec2 textSize = ImGui::CalcTextSize(c.name);
            drawList->AddText(ImVec2(scrPt.x - textSize.x * 0.5f, scrPt.y - textSize.y * 0.5f), c.col, c.name);
        }
    }

    // 2. Dessin des faces visibles du cube
    for (const auto& vf : visibleFaces) {
        const auto& f = faces[vf.index];
        const ImVec2& a = screenVertices[f.v[0]];
        const ImVec2& b = screenVertices[f.v[1]];
        const ImVec2& c = screenVertices[f.v[2]];
        const ImVec2& d = screenVertices[f.v[3]];

        bool isHovered = (hoveredFace == vf.index);

        // Ombrage selon la normale (éclairage directionnel virtuel)
        glm::vec3 camNorm = viewRot * f.normal;
        float light = std::clamp(camNorm.z * 0.35f + (camNorm.y * 0.45f) + 0.45f, 0.25f, 1.0f);

        ImU32 faceColor;
        if (isHovered) {
            faceColor = IM_COL32(40, 130, 230, 235); // Bleu azur électrique survol
        } else {
            int r = static_cast<int>(55.0f * light + 20.0f);
            int g = static_cast<int>(68.0f * light + 25.0f);
            int bl = static_cast<int>(88.0f * light + 35.0f);
            faceColor = IM_COL32(r, g, bl, 220);
        }

        drawList->AddQuadFilled(a, b, c, d, faceColor);
        drawList->AddQuad(a, b, c, d, IM_COL32(140, 165, 195, 220), 1.5f);

        // Texte de la face centrée
        ImVec2 faceCenter((a.x + b.x + c.x + d.x) * 0.25f, (a.y + b.y + c.y + d.y) * 0.25f);
        ImVec2 txtSize = ImGui::CalcTextSize(f.label);
        ImU32 txtColor = isHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(220, 230, 245, 240);
        drawList->AddText(ImVec2(faceCenter.x - txtSize.x * 0.5f, faceCenter.y - txtSize.y * 0.5f), txtColor, f.label);
    }

    // 3. Dessin et détection des coins pour les vues isométriques (Axonométrie)
    int hoveredCorner = -1;
    for (int i = 0; i < 8; ++i) {
        if (camVertices[i].z > -s * 0.5f) { // coin orienté vers nous
            float dist = glm::distance(glm::vec2(mousePos.x, mousePos.y), glm::vec2(screenVertices[i].x, screenVertices[i].y));
            if (dist < 9.0f) {
                hoveredCorner = i;
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    camera.setIsoCorner(i);
                    actionTriggered = true;
                }
                break;
            }
        }
    }

    for (int i = 0; i < 8; ++i) {
        if (camVertices[i].z > -s * 0.5f) {
            bool isCornerHov = (hoveredCorner == i);
            ImU32 dotColor = isCornerHov ? IM_COL32(255, 200, 40, 255) : IM_COL32(180, 200, 230, 160);
            drawList->AddCircleFilled(screenVertices[i], isCornerHov ? 5.5f : 3.0f, dotColor);
        }
    }

    // Traitement du clic sur face
    if (clickedFace >= 0) {
        switch (faces[clickedFace].faceId) {
            case 0: camera.setFrontView();  break;
            case 1: camera.setBackView();   break;
            case 2: camera.setTopView();    break;
            case 3: camera.setBottomView(); break;
            case 4: camera.setRightView();  break;
            case 5: camera.setLeftView();   break;
        }
        actionTriggered = true;
    }

    // 4. Contrôles annexes du ViewCube (Bouton Home et Bascule Persp/Ortho)
    ImGui::SetNextWindowPos(ImVec2(center.x - 48.0f, center.y + s * 1.55f + 12.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("##ViewCubeControls", nullptr, flags)) {
        if (showHomeButton) {
            if (ImGui::Button("Recadrer (F)")) {
                camera.fitToScene(boundsMin, boundsMax);
                actionTriggered = true;
            }
            ImGui::SameLine();
        }

        if (showViewType) {
            const char* typeLabel = camera.orthographic ? "Ortho [2D]" : "Persp [3D]";
            if (ImGui::Button(typeLabel)) {
                camera.orthographic = !camera.orthographic;
                actionTriggered = true;
            }
        }
    }
    ImGui::End();

    // Flèches de rotation en plan (+/-90°)
    ImGui::SetNextWindowPos(ImVec2(center.x - 55.0f, center.y - s * 1.75f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.70f);
    if (ImGui::Begin("##ViewCubeRotate", nullptr, flags)) {
        if (ImGui::Button("< -90°")) {
            camera.rotateYaw(-90.0f);
            actionTriggered = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+90° >")) {
            camera.rotateYaw(90.0f);
            actionTriggered = true;
        }
    }
    ImGui::End();

    return actionTriggered;
}

} // namespace ui
