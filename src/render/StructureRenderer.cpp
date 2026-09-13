// =============================================================================
//  StructureRenderer.cpp — Pipeline de rendu coordonné
// =============================================================================

#include "render/StructureRenderer.h"
#include "render/ShaderSources.h"
#include "scene/ProfileExtruder.h"
#include "scene/SupportGizmos.h"
#include "scene/LoadGizmos.h"
#include "scene/DiagramMesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>

void StructureRenderer::init() {
    phongShader_.compile(shaders::PHONG_VERT, shaders::PHONG_FRAG);
    heatmapShader_.compile(shaders::HEATMAP_VERT, shaders::HEATMAP_FRAG);
    flatShader_.compile(shaders::FLAT_VERT, shaders::FLAT_FRAG);

    grid.init(30.0f, 1.0f);
    grid.initAxesTriad();

    nodeSphere_ = Mesh::createSphere(1.0f, 16, 12);
}

// ---- Couleurs par défaut ----
static const glm::vec3 COL_ELEMENT  {0.45f, 0.55f, 0.70f};
static const glm::vec3 COL_DEFORMED {1.00f, 0.55f, 0.15f};
static const glm::vec3 COL_NODE     {0.20f, 0.75f, 0.90f};
static const glm::vec3 COL_SUPPORT  {0.40f, 0.80f, 0.40f};
static const glm::vec3 COL_LOAD     {0.95f, 0.30f, 0.30f};
static const glm::vec3 COL_REACTION {0.30f, 0.90f, 0.40f};
static const glm::vec3 COL_SELECTED {1.00f, 0.95f, 0.20f};

// ---- Rebuild complet ----
void StructureRenderer::rebuild(const model::Structure& structure) {
    elementMeshes_.clear();
    deformedMeshes_.clear();
    supportMeshes_.clear();
    loadMeshes_.clear();
    diagramFills_.clear();
    diagramLines_.clear();
    heatmapMeshes_.clear();
    nodeInfos_.clear();
    reactionMeshes_.clear();

    // ---- Nœuds ----
    for (auto& nd : structure.nodes) {
        nodeInfos_.push_back({nd.position, COL_NODE, nd.id});
    }

    // ---- Éléments (profilés 3D extrudés) ----
    for (auto& elem : structure.elements) {
        auto* nI = structure.findNode(elem.nodeI);
        auto* nJ = structure.findNode(elem.nodeJ);
        auto* sec = structure.findSection(elem.sectionId);
        if (!nI || !nJ || !sec) continue;

        glm::vec3 localX, localY, localZ;
        scene::computeLocalFrame(nI->position, nJ->position, elem.rollAngle,
                                  localX, localY, localZ);

        std::vector<glm::vec3> axis = {nI->position, nJ->position};
        std::vector<glm::vec3> up   = {localY, localY};
        std::vector<glm::vec3> fwd  = {localX, localX};

        MeshInstance mi;
        mi.mesh = scene::extrudeProfile(*sec, axis, up, fwd);
        mi.model = glm::mat4(1.0f);
        mi.color = COL_ELEMENT;
        mi.id    = elem.id;
        elementMeshes_.push_back(std::move(mi));
    }

    // ---- Appuis ----
    for (auto& sup : structure.supports) {
        auto* nd = structure.findNode(sup.nodeId);
        if (!nd) continue;

        float gizmoSize = 0.25f;
        MeshInstance mi;
        mi.mesh = scene::createSupportGizmo(sup.type, gizmoSize);

        glm::mat4 m = glm::translate(glm::mat4(1.0f), nd->position);
        // Décaler vers le bas pour que le gizmo soit sous le nœud
        m = glm::translate(m, glm::vec3(0, -gizmoSize * 0.5f, 0));
        mi.model = m;
        mi.color = COL_SUPPORT;
        supportMeshes_.push_back(std::move(mi));
    }

    // ---- Charges nodales ----
    float maxForce = 1.0f;
    for (auto& ld : structure.nodalLoads)
        maxForce = std::max(maxForce, glm::length(ld.force));

    for (auto& ld : structure.nodalLoads) {
        auto* nd = structure.findNode(ld.nodeId);
        if (!nd) continue;

        if (glm::length(ld.force) > 0.01f) {
            glm::vec3 dir = glm::normalize(ld.force);
            float mag = glm::length(ld.force);

            MeshInstance mi;
            mi.mesh = scene::createForceArrow(dir, mag, maxForce);

            // Orienter la flèche dans la direction de la force
            glm::vec3 up(0, 1, 0);
            glm::mat4 rot(1.0f);
            if (std::abs(glm::dot(dir, up) + 1.0f) < 0.01f) {
                rot = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1,0,0));
            } else if (std::abs(glm::dot(dir, up) - 1.0f) > 0.01f) {
                glm::vec3 axis = glm::normalize(glm::cross(up, dir));
                float angle = std::acos(std::clamp(glm::dot(up, dir), -1.0f, 1.0f));
                rot = glm::rotate(glm::mat4(1.0f), angle, axis);
            }

            mi.model = glm::translate(glm::mat4(1.0f), nd->position) * rot;
            mi.color = COL_LOAD;
            loadMeshes_.push_back(std::move(mi));
        }
    }

    // ---- Charges réparties ----
    float maxDistLoad = 1.0f;
    for (auto& dl : structure.distributedLoads)
        maxDistLoad = std::max(maxDistLoad, std::max(glm::length(dl.wStart), glm::length(dl.wEnd)));

    for (auto& dl : structure.distributedLoads) {
        auto* elem = structure.findElement(dl.elementId);
        if (!elem) continue;
        auto* nI = structure.findNode(elem->nodeI);
        auto* nJ = structure.findNode(elem->nodeJ);
        if (!nI || !nJ) continue;

        glm::vec3 lx, ly, lz;
        scene::computeLocalFrame(nI->position, nJ->position, elem->rollAngle, lx, ly, lz);

        MeshInstance mi;
        mi.mesh = scene::createDistributedLoadComb(dl, nI->position, nJ->position,
                                                    ly, lz, maxDistLoad, 8);
        mi.model = glm::mat4(1.0f);
        mi.color = COL_LOAD;
        loadMeshes_.push_back(std::move(mi));
    }

    // ---- Réactions ----
    float maxReaction = 1.0f;
    for (auto& r : structure.reactions)
        maxReaction = std::max(maxReaction, glm::length(r.force));

    for (auto& r : structure.reactions) {
        auto* nd = structure.findNode(r.nodeId);
        if (!nd || glm::length(r.force) < 0.01f) continue;

        glm::vec3 dir = glm::normalize(r.force);

        MeshInstance mi;
        mi.mesh = scene::createForceArrow(dir, glm::length(r.force), maxReaction);

        glm::vec3 up(0, 1, 0);
        glm::mat4 rot(1.0f);
        if (std::abs(glm::dot(dir, up) + 1.0f) < 0.01f)
            rot = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1,0,0));
        else if (std::abs(glm::dot(dir, up) - 1.0f) > 0.01f) {
            glm::vec3 axis = glm::normalize(glm::cross(up, dir));
            float angle = std::acos(std::clamp(glm::dot(up, dir), -1.0f, 1.0f));
            rot = glm::rotate(glm::mat4(1.0f), angle, axis);
        }
        mi.model = glm::translate(glm::mat4(1.0f), nd->position - dir * 0.5f) * rot;
        mi.color = COL_REACTION;
        reactionMeshes_.push_back(std::move(mi));
    }
}

void StructureRenderer::rebuildDeformed(const model::Structure& structure, float scale) {
    deformedMeshes_.clear();
    if (!structure.hasResults) return;

    for (auto& elem : structure.elements) {
        auto* nI = structure.findNode(elem.nodeI);
        auto* nJ = structure.findNode(elem.nodeJ);
        auto* sec = structure.findSection(elem.sectionId);
        if (!nI || !nJ || !sec) continue;

        MeshInstance mi;
        mi.mesh = scene::extrudeProfileDeformed(*sec, elem, *nI, *nJ, scale, 20);
        mi.model = glm::mat4(1.0f);
        mi.color = COL_DEFORMED;
        mi.alpha = 0.85f;
        deformedMeshes_.push_back(std::move(mi));
    }
}

void StructureRenderer::rebuildDiagrams(const model::Structure& structure,
                                         scene::DiagramType type, float scale) {
    diagramFills_.clear();
    diagramLines_.clear();
    if (!structure.hasResults) return;

    for (auto& elem : structure.elements) {
        auto* nI = structure.findNode(elem.nodeI);
        auto* nJ = structure.findNode(elem.nodeJ);
        if (!nI || !nJ) continue;

        const auto& vals = scene::getDiagramValues(elem, type);
        if (vals.empty()) continue;

        // Remplissage semi-transparent
        MeshInstance fill;
        fill.mesh = scene::createDiagramRibbon(elem, *nI, *nJ, type, scale);
        fill.model = glm::mat4(1.0f);
        fill.alpha = 0.45f;
        diagramFills_.push_back(std::move(fill));

        // Contour opaque
        MeshInstance line;
        line.mesh = scene::createDiagramOutline(elem, *nI, *nJ, type, scale);
        line.model = glm::mat4(1.0f);
        line.alpha = 1.0f;
        diagramLines_.push_back(std::move(line));
    }
}

void StructureRenderer::rebuildHeatmap(const model::Structure& structure) {
    heatmapMeshes_.clear();
    if (!structure.hasResults) return;

    for (auto& elem : structure.elements) {
        auto* nI = structure.findNode(elem.nodeI);
        auto* nJ = structure.findNode(elem.nodeJ);
        auto* sec = structure.findSection(elem.sectionId);
        if (!nI || !nJ || !sec) continue;
        if (elem.stressRatio.empty() || elem.stations.empty()) continue;

        // Générer un mesh heatmap par extrusion multi-stations
        int nSt = static_cast<int>(elem.stations.size());
        glm::vec3 localX, localY, localZ;
        scene::computeLocalFrame(nI->position, nJ->position, elem.rollAngle,
                                  localX, localY, localZ);

        std::vector<glm::vec2> contour, normals2D;
        scene::generateSectionContour(*sec, contour, normals2D);
        if (contour.empty()) continue;

        float L = glm::length(nJ->position - nI->position);
        int nContour = static_cast<int>(contour.size());

        std::vector<HeatmapVertex> verts;
        std::vector<uint32_t> idx;

        glm::vec3 right = glm::normalize(glm::cross(localX, localY));
        glm::vec3 up = glm::cross(right, localX);

        for (int s = 0; s < nSt; ++s) {
            float t = elem.stations[static_cast<size_t>(s)];
            glm::vec3 center = nI->position + localX * (t * L);
            float val = elem.stressRatio[static_cast<size_t>(s)];

            for (int c = 0; c < nContour; ++c) {
                glm::vec3 pos = center + right * contour[static_cast<size_t>(c)].x
                                       + up * contour[static_cast<size_t>(c)].y;
                glm::vec3 nrm = glm::normalize(
                    right * normals2D[static_cast<size_t>(c)].x +
                    up * normals2D[static_cast<size_t>(c)].y);
                verts.push_back({pos, nrm, val});
            }
        }

        for (int s = 0; s < nSt - 1; ++s) {
            int rA = s * nContour;
            int rB = (s + 1) * nContour;
            for (int c = 0; c < nContour; ++c) {
                int cn = (c + 1) % nContour;
                uint32_t a = static_cast<uint32_t>(rA + c);
                uint32_t b = static_cast<uint32_t>(rB + c);
                uint32_t bn = static_cast<uint32_t>(rB + cn);
                uint32_t an = static_cast<uint32_t>(rA + cn);
                idx.insert(idx.end(), {a, b, bn, a, bn, an});
            }
        }

        MeshInstance mi;
        mi.mesh.uploadHeatmap(verts, idx);
        mi.model = glm::mat4(1.0f);
        heatmapMeshes_.push_back(std::move(mi));
    }
}

// ---- Helper : dessiner des instances Phong ----
void StructureRenderer::drawPhongInstances(const std::vector<MeshInstance>& instances,
                                            const Camera& /*cam*/, int selectedId) const {
    for (auto& mi : instances) {
        phongShader_.setMat4("uModel", mi.model);
        glm::mat3 nm = glm::inverseTranspose(glm::mat3(mi.model));
        phongShader_.setMat3("uNormalMatrix", nm);
        bool selected = (selectedId >= 0 && mi.id == selectedId);
        phongShader_.setVec3("uObjectColor", selected ? COL_SELECTED : mi.color);
        phongShader_.setFloat("uAlpha", mi.alpha);
        mi.mesh.draw();
    }
}

// ---- Dessin principal ----
void StructureRenderer::draw(const Camera& camera, const RenderState& state, float time) {
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix();
    glm::vec3 camPos = camera.getPosition();
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.4f, 0.8f, 0.5f));

    // ---- Grille ----
    if (state.showGrid) {
        flatShader_.use();
        flatShader_.setMat4("uView", view);
        flatShader_.setMat4("uProjection", proj);
        flatShader_.setMat4("uModel", glm::mat4(1.0f));
        flatShader_.setFloat("uAlpha", 0.5f);
        grid.draw(flatShader_);
    }

    // ---- Éléments (Phong) ----
    if (state.showElements) {
        phongShader_.use();
        phongShader_.setMat4("uView", view);
        phongShader_.setMat4("uProjection", proj);
        phongShader_.setVec3("uLightDir", lightDir);
        phongShader_.setVec3("uViewPos", camPos);

        if (state.showHeatmap && !heatmapMeshes_.empty()) {
            // Heatmap mode
            heatmapShader_.use();
            heatmapShader_.setMat4("uView", view);
            heatmapShader_.setMat4("uProjection", proj);
            heatmapShader_.setVec3("uLightDir", lightDir);
            heatmapShader_.setVec3("uViewPos", camPos);
            heatmapShader_.setInt("uColormapType", state.colormapType);
            for (auto& mi : heatmapMeshes_) {
                heatmapShader_.setMat4("uModel", mi.model);
                glm::mat3 nm = glm::inverseTranspose(glm::mat3(mi.model));
                heatmapShader_.setMat3("uNormalMatrix", nm);
                mi.mesh.draw();
            }
        } else if (state.showProfiles3D) {
            phongShader_.use();
            phongShader_.setMat4("uView", view);
            phongShader_.setMat4("uProjection", proj);
            phongShader_.setVec3("uLightDir", lightDir);
            phongShader_.setVec3("uViewPos", camPos);
            drawPhongInstances(elementMeshes_, camera, state.selectedElementId);
        }
    }

    // ---- Déformée ----
    if (state.showDeformed && !deformedMeshes_.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        phongShader_.use();
        phongShader_.setMat4("uView", view);
        phongShader_.setMat4("uProjection", proj);
        phongShader_.setVec3("uLightDir", lightDir);
        phongShader_.setVec3("uViewPos", camPos);
        drawPhongInstances(deformedMeshes_, camera);
        glDisable(GL_BLEND);
    }

    // ---- Appuis ----
    if (state.showSupports) {
        phongShader_.use();
        phongShader_.setMat4("uView", view);
        phongShader_.setMat4("uProjection", proj);
        phongShader_.setVec3("uLightDir", lightDir);
        phongShader_.setVec3("uViewPos", camPos);
        drawPhongInstances(supportMeshes_, camera);
    }

    // ---- Charges ----
    if (state.showLoads) {
        phongShader_.use();
        phongShader_.setMat4("uView", view);
        phongShader_.setMat4("uProjection", proj);
        phongShader_.setVec3("uLightDir", lightDir);
        phongShader_.setVec3("uViewPos", camPos);
        drawPhongInstances(loadMeshes_, camera);
    }

    // ---- Réactions ----
    if (state.showReactions) {
        phongShader_.use();
        phongShader_.setMat4("uView", view);
        phongShader_.setMat4("uProjection", proj);
        phongShader_.setVec3("uLightDir", lightDir);
        phongShader_.setVec3("uViewPos", camPos);
        drawPhongInstances(reactionMeshes_, camera);
    }

    // ---- Diagrammes ----
    if (state.showDiagram && !diagramFills_.empty()) {
        flatShader_.use();
        flatShader_.setMat4("uView", view);
        flatShader_.setMat4("uProjection", proj);

        // Remplissage translucide
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (auto& mi : diagramFills_) {
            flatShader_.setMat4("uModel", mi.model);
            flatShader_.setFloat("uAlpha", mi.alpha);
            mi.mesh.draw();
        }
        glDisable(GL_BLEND);

        // Contour
        glLineWidth(2.0f);
        for (auto& mi : diagramLines_) {
            flatShader_.setMat4("uModel", mi.model);
            flatShader_.setFloat("uAlpha", 1.0f);
            mi.mesh.draw();
        }
        glLineWidth(1.0f);
    }

    // ---- Nœuds (sphères) ----
    if (state.showNodes) {
        phongShader_.use();
        phongShader_.setMat4("uView", view);
        phongShader_.setMat4("uProjection", proj);
        phongShader_.setVec3("uLightDir", lightDir);
        phongShader_.setVec3("uViewPos", camPos);
        phongShader_.setFloat("uAlpha", 1.0f);

        float nodeRadius = 0.06f;
        for (auto& ni : nodeInfos_) {
            bool selected = (ni.id == state.selectedNodeId);
            // Le nœud sélectionné est agrandi pour rester bien visible :
            // un simple changement de couleur est difficile à repérer sur
            // une sphère aussi petite.
            float radius = selected ? nodeRadius * 1.8f : nodeRadius;

            glm::mat4 m = glm::translate(glm::mat4(1.0f), ni.pos);
            m = glm::scale(m, glm::vec3(radius));
            phongShader_.setMat4("uModel", m);
            glm::mat3 nm = glm::inverseTranspose(glm::mat3(m));
            phongShader_.setMat3("uNormalMatrix", nm);

            phongShader_.setVec3("uObjectColor", selected ? COL_SELECTED : ni.color);
            nodeSphere_.draw();
        }
    }

    // ---- Trièdre d'axes (coin bas-gauche) ----
    {
        glm::mat4 viewRotOnly = glm::mat4(glm::mat3(view));
        float vpW = static_cast<float>(camera.aspectRatio * 600.0f);
        float vpH = 600.0f;
        grid.drawAxesTriad(flatShader_, viewRotOnly, vpW, vpH);
    }
}
