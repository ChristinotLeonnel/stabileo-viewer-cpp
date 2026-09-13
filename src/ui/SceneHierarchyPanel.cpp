// =============================================================================
//  SceneHierarchyPanel.cpp — Panneau de Hiérarchie et Propriétés inspiré de Hazel
// =============================================================================

#include "ui/SceneHierarchyPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <algorithm>

void SceneHierarchyPanel::onImGuiRender(RenderState& state, bool* p_openHierarchy, bool* p_openProperties) {
    if (!p_openHierarchy || *p_openHierarchy) {
        drawHierarchy(state, p_openHierarchy);
    }
    if (!p_openProperties || *p_openProperties) {
        drawProperties(state, p_openProperties);
    }
}

void SceneHierarchyPanel::drawHierarchy(RenderState& state, bool* p_open) {
    if (p_open && !*p_open) return;
    if (!ImGui::Begin("Hiérarchie de Scène###SceneHierarchy", p_open)) {
        ImGui::End();
        return;
    }

    if (!context_) {
        ImGui::TextDisabled("Aucun modèle de structure actif.");
        ImGui::End();
        return;
    }

    // Barre de recherche de composants
    ImGui::PushItemWidth(-1);
    ImGui::InputTextWithHint("##FilterHierarchy", "Rechercher nœud ou barre...", searchFilter_, sizeof(searchFilter_));
    ImGui::PopItemWidth();
    ImGui::Spacing();

    std::string filterLower = searchFilter_;
    std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

    // Noeud Racine de la Structure
    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    bool rootOpen = ImGui::TreeNodeEx("StructureRoot", rootFlags, "Structure : %s", context_->name.c_str());

    // Clic droit dans le vide pour menu contextuel
    if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Ajouter un nouveau nœud")) {
            int newId = context_->nodes.empty() ? 1 : context_->nodes.back().id + 1;
            context_->nodes.push_back(model::Node{ newId, glm::vec3(0.0f, 0.0f, 0.0f) });
            selectNode(newId);
        }
        ImGui::EndPopup();
    }

    if (rootOpen) {
        // --- Branche Nœuds ---
        char nodesLabel[64];
        snprintf(nodesLabel, sizeof(nodesLabel), "Nœuds (%zu)", context_->nodes.size());
        if (ImGui::TreeNodeEx("NodesBranch", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", nodesLabel)) {
            for (auto& node : context_->nodes) {
                char nodeName[64];
                snprintf(nodeName, sizeof(nodeName), "Nœud #%d", node.id);

                if (!filterLower.empty()) {
                    std::string nNameLower = nodeName;
                    std::transform(nNameLower.begin(), nNameLower.end(), nNameLower.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
                    if (nNameLower.find(filterLower) == std::string::npos) continue;
                }

                // Détecter si ce nœud a un appui ou une charge
                bool hasSupport = false;
                for (const auto& sup : context_->supports) {
                    if (sup.nodeId == node.id) { hasSupport = true; break; }
                }
                bool hasLoad = false;
                for (const auto& ld : context_->nodalLoads) {
                    if (ld.nodeId == node.id) { hasLoad = true; break; }
                }

                char fullItemLabel[128];
                snprintf(fullItemLabel, sizeof(fullItemLabel), "%s %s%s (%.1f, %.1f, %.1f)",
                         nodeName,
                         hasSupport ? "[▲]" : "",
                         hasLoad ? "[↓]" : "",
                         node.position.x, node.position.y, node.position.z);

                ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedType_ == SelectionType::Node && selectedId_ == node.id) {
                    nodeFlags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::TreeNodeEx((void*)(uintptr_t)(node.id + 10000), nodeFlags, "%s", fullItemLabel);

                if (ImGui::IsItemClicked()) {
                    selectNode(node.id);
                    state.selectedNodeId = node.id;
                    state.selectedElementId = -1;
                }

                // Menu contextuel sur le nœud
                if (ImGui::BeginPopupContextItem()) {
                    selectNode(node.id);
                    if (ImGui::MenuItem("Centrer la caméra")) {
                        // Action centrer
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Supprimer ce nœud")) {
                        // Supprimer le noeud
                        int targetId = node.id;
                        context_->nodes.erase(std::remove_if(context_->nodes.begin(), context_->nodes.end(),
                            [targetId](const model::Node& n) { return n.id == targetId; }), context_->nodes.end());
                        clearSelection();
                        ImGui::EndPopup();
                        break;
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::TreePop();
        }

        // --- Branche Éléments / Barres ---
        char elementsLabel[64];
        snprintf(elementsLabel, sizeof(elementsLabel), "Éléments de Structure (%zu)", context_->elements.size());
        if (ImGui::TreeNodeEx("ElementsBranch", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", elementsLabel)) {
            for (auto& elem : context_->elements) {
                char elemName[128];
                const model::Section* sec = context_->findSection(elem.sectionId);
                const char* secName = sec ? sec->name.c_str() : "IPE";
                snprintf(elemName, sizeof(elemName), "Barre #%d  (N%d -> N%d)  [%s]",
                         elem.id, elem.nodeI, elem.nodeJ, secName);

                if (!filterLower.empty()) {
                    std::string eNameLower = elemName;
                    std::transform(eNameLower.begin(), eNameLower.end(), eNameLower.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
                    if (eNameLower.find(filterLower) == std::string::npos) continue;
                }

                ImGuiTreeNodeFlags elemFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedType_ == SelectionType::Element && selectedId_ == elem.id) {
                    elemFlags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::TreeNodeEx((void*)(uintptr_t)(elem.id + 50000), elemFlags, "%s", elemName);

                if (ImGui::IsItemClicked()) {
                    selectElement(elem.id);
                    state.selectedElementId = elem.id;
                    state.selectedNodeId = -1;
                }

                // Menu contextuel sur l'élément
                if (ImGui::BeginPopupContextItem()) {
                    selectElement(elem.id);
                    if (ImGui::MenuItem("Inverser sens I -> J")) {
                        std::swap(elem.nodeI, elem.nodeJ);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Supprimer cet élément")) {
                        int targetId = elem.id;
                        context_->elements.erase(std::remove_if(context_->elements.begin(), context_->elements.end(),
                            [targetId](const model::Element& e) { return e.id == targetId; }), context_->elements.end());
                        clearSelection();
                        ImGui::EndPopup();
                        break;
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    ImGui::End();
}

void SceneHierarchyPanel::drawProperties(RenderState& state, bool* p_open) {
    if (p_open && !*p_open) return;
    if (!ImGui::Begin("Propriétés###EntityProperties", p_open)) {
        ImGui::End();
        return;
    }

    if (!context_) {
        ImGui::TextDisabled("Aucun composant à inspecter.");
        ImGui::End();
        return;
    }

    if (selectedType_ == SelectionType::Node) {
        model::Node* node = context_->findNode(selectedId_);
        if (node) {
            drawNodeComponents(*node, state);
        } else {
            clearSelection();
            ImGui::TextDisabled("Nœud introuvable.");
        }
    } else if (selectedType_ == SelectionType::Element) {
        model::Element* elem = const_cast<model::Element*>(context_->findElement(selectedId_));
        if (elem) {
            drawElementComponents(*elem, state);
        } else {
            clearSelection();
            ImGui::TextDisabled("Élément introuvable.");
        }
    } else {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Aucune entité sélectionnée.");
        ImGui::TextWrapped("Sélectionnez un Nœud ou un Élément dans la Hiérarchie de Scène pour inspecter et éditer ses composants.");
    }

    ImGui::End();
}

void SceneHierarchyPanel::drawNodeComponents(model::Node& node, RenderState& /*state*/) {
    // 1. Tag Component (En-tête Hazel)
    char nodeTag[64];
    snprintf(nodeTag, sizeof(nodeTag), "Nœud #%d", node.id);
    if (Hazel::UI::BeginComponent("Composant : Tag & Identifiant")) {
        ImGui::Text("Type d'entité : Nœud de structure");
        ImGui::Text("Identifiant ID : %d", node.id);
        Hazel::UI::EndComponent();
    }

    // 2. Transform Component (Position 3D avec DrawVec3Control signature de The Cherno !)
    if (Hazel::UI::BeginComponent("Composant : Coordonnées 3D (Position)")) {
        if (Hazel::UI::DrawVec3Control("Position (m)", node.position, 0.0f, 100.0f, 0.05f)) {
            // Mise à jour de la boîte englobante ou recalcul si nécessaire
        }
        Hazel::UI::EndComponent();
    }

    // 3. Support Component (Conditions aux limites)
    model::Support* supFound = nullptr;
    for (auto& s : context_->supports) {
        if (s.nodeId == node.id) { supFound = &s; break; }
    }

    if (Hazel::UI::BeginComponent("Composant : Conditions aux Limites (Appui)")) {
        bool isSupported = (supFound != nullptr);
        if (ImGui::Checkbox("Nœud appuyé", &isSupported)) {
            if (isSupported && !supFound) {
                context_->supports.push_back(model::Support{ node.id, model::SupportType::PINNED });
                for (auto& s : context_->supports) { if (s.nodeId == node.id) { supFound = &s; break; } }
            } else if (!isSupported && supFound) {
                int targetId = node.id;
                context_->supports.erase(std::remove_if(context_->supports.begin(), context_->supports.end(),
                    [targetId](const model::Support& s) { return s.nodeId == targetId; }), context_->supports.end());
                supFound = nullptr;
            }
        }

        if (supFound) {
            const char* typeNames[] = { "Encastrement (Fixed)", "Rotule (Pinned)", "Glissière X", "Glissière Y", "Glissière Z", "Ressort" };
            int currentType = static_cast<int>(supFound->type);
            if (ImGui::Combo("Type d'appui", &currentType, typeNames, IM_ARRAYSIZE(typeNames))) {
                supFound->type = static_cast<model::SupportType>(currentType);
            }

            ImGui::Text("Blocage des rotations :");
            ImGui::Checkbox("Rx", &supFound->blockRx); ImGui::SameLine();
            ImGui::Checkbox("Ry", &supFound->blockRy); ImGui::SameLine();
            ImGui::Checkbox("Rz", &supFound->blockRz);
        }
        Hazel::UI::EndComponent();
    }

    // 4. Nodal Load Component
    model::NodalLoad* loadFound = nullptr;
    for (auto& l : context_->nodalLoads) {
        if (l.nodeId == node.id) { loadFound = &l; break; }
    }

    if (Hazel::UI::BeginComponent("Composant : Charge Nodale Appliquée")) {
        bool hasLoad = (loadFound != nullptr);
        if (ImGui::Checkbox("Appliquer une charge sur ce nœud", &hasLoad)) {
            if (hasLoad && !loadFound) {
                context_->nodalLoads.push_back(model::NodalLoad{ node.id, glm::vec3(0.0f, -10.0f, 0.0f), glm::vec3(0.0f) });
                for (auto& l : context_->nodalLoads) { if (l.nodeId == node.id) { loadFound = &l; break; } }
            } else if (!hasLoad && loadFound) {
                int targetId = node.id;
                context_->nodalLoads.erase(std::remove_if(context_->nodalLoads.begin(), context_->nodalLoads.end(),
                    [targetId](const model::NodalLoad& l) { return l.nodeId == targetId; }), context_->nodalLoads.end());
                loadFound = nullptr;
            }
        }

        if (loadFound) {
            Hazel::UI::DrawVec3Control("Force F (kN)", loadFound->force, 0.0f, 100.0f, 0.5f);
            Hazel::UI::DrawVec3Control("Moment M (kN.m)", loadFound->moment, 0.0f, 100.0f, 0.5f);
        }
        Hazel::UI::EndComponent();
    }

    // 5. FEA Results Component (Déplacements et Réactions)
    if (context_->hasResults) {
        if (Hazel::UI::BeginComponent("Composant : Résultats Éléments Finis")) {
            glm::vec3 dispMm = node.displacement * 1000.0f;
            glm::vec3 rotMrad = node.rotation * 1000.0f;
            Hazel::UI::DrawVec3Control("Déplacement (mm)", dispMm, 0.0f, 110.0f, 0.0f);
            Hazel::UI::DrawVec3Control("Rotation (mrad)", rotMrad, 0.0f, 110.0f, 0.0f);

            // Réactions
            for (const auto& r : context_->reactions) {
                if (r.nodeId == node.id) {
                    glm::vec3 rf = r.force;
                    glm::vec3 rm = r.moment;
                    Hazel::UI::DrawVec3Control("Réaction F (kN)", rf, 0.0f, 110.0f, 0.0f);
                    Hazel::UI::DrawVec3Control("Réaction M (kN.m)", rm, 0.0f, 110.0f, 0.0f);
                    break;
                }
            }
            Hazel::UI::EndComponent();
        }
    }
}

void SceneHierarchyPanel::drawElementComponents(model::Element& elem, RenderState& /*state*/) {
    // 1. Tag Component
    if (Hazel::UI::BeginComponent("Composant : Tag & Identifiant")) {
        ImGui::Text("Type d'entité : Élément Filaire (Poutre/Barre)");
        ImGui::Text("Identifiant ID : %d", elem.id);
        ImGui::Text("Connectivité : Nœud %d -> Nœud %d", elem.nodeI, elem.nodeJ);
        Hazel::UI::EndComponent();
    }

    // 2. Géométrie & Connectivité
    const model::Node* nI = context_->findNode(elem.nodeI);
    const model::Node* nJ = context_->findNode(elem.nodeJ);
    float length = (nI && nJ) ? glm::length(nJ->position - nI->position) : 0.0f;

    if (Hazel::UI::BeginComponent("Composant : Géométrie & Articulations")) {
        ImGui::Text("Longueur de barre : %.3f m", length);
        Hazel::UI::DrawFloatControl("Roulis (deg)", elem.rollAngle, 0.0f, 100.0f, 1.0f, -180.0f, 180.0f);
        ImGui::Checkbox("Barre de Treillis (Truss, N seul)", &elem.isTruss);
        ImGui::Checkbox("Rotule au début (Hinge I)", &elem.hingeStart); ImGui::SameLine();
        ImGui::Checkbox("Rotule à la fin (Hinge J)", &elem.hingeEnd);
        Hazel::UI::EndComponent();
    }

    // 3. Section & Matériau
    const model::Section* sec = context_->findSection(elem.sectionId);
    if (Hazel::UI::BeginComponent("Composant : Section & Profilé")) {
        if (sec) {
            ImGui::Text("Profilé : %s", sec->name.c_str());
            ImGui::Text("Hauteur h = %.1f mm | Largeur b = %.1f mm", sec->h * 1000.0f, sec->b * 1000.0f);
            ImGui::Text("Inertie Iy = %.1f cm⁴ | Iz = %.1f cm⁴", sec->Iy * 1e8f, sec->Iz * 1e8f);
            ImGui::Text("Aire A = %.2f cm²", sec->A * 1e4f);
            ImGui::Text("Acier : fy = %.0f MPa | E = %.0f GPa", sec->fy, sec->E / 1000.0f);
        } else {
            ImGui::TextDisabled("Aucune section assignée.");
        }
        Hazel::UI::EndComponent();
    }

    // 4. Résultats Internes EF (Hazel Engine Inspector)
    if (context_->hasResults && !elem.N.empty()) {
        if (Hazel::UI::BeginComponent("Composant : Sollicitations Internes EF")) {
            float nMax = *std::max_element(elem.N.begin(), elem.N.end());
            float nMin = *std::min_element(elem.N.begin(), elem.N.end());
            float nAbs = std::max(std::abs(nMax), std::abs(nMin));

            float myMax = !elem.My.empty() ? *std::max_element(elem.My.begin(), elem.My.end()) : 0.0f;
            float mzMax = !elem.Mz.empty() ? *std::max_element(elem.Mz.begin(), elem.Mz.end()) : 0.0f;
            float vyMax = !elem.Vy.empty() ? *std::max_element(elem.Vy.begin(), elem.Vy.end()) : 0.0f;

            ImGui::Text("Effort Normal N : Min = %.2f kN | Max = %.2f kN (|N|max = %.2f kN)", nMin, nMax, nAbs);
            ImGui::Text("Effort Tranchant Vy max : %.2f kN", vyMax);
            ImGui::Text("Moment Fléchissant My max : %.2f kN.m", myMax);
            ImGui::Text("Moment Fléchissant Mz max : %.2f kN.m", mzMax);

            if (!elem.stressRatio.empty()) {
                float maxRatio = *std::max_element(elem.stressRatio.begin(), elem.stressRatio.end());
                ImGui::Separator();
                ImGui::Text("Ratio d'utilisation Eurocode 3 : %.1f %%", maxRatio * 100.0f);

                ImVec4 barColor = (maxRatio > 1.0f) ? ImVec4(0.9f, 0.15f, 0.15f, 1.0f) :
                                  (maxRatio > 0.8f) ? ImVec4(0.95f, 0.70f, 0.10f, 1.0f) :
                                                      ImVec4(0.20f, 0.80f, 0.30f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                char overlay[32];
                snprintf(overlay, sizeof(overlay), "%.1f %%", maxRatio * 100.0f);
                ImGui::ProgressBar(maxRatio, ImVec2(-1, 20.0f), overlay);
                ImGui::PopStyleColor();
            }
            Hazel::UI::EndComponent();
        }
    }
}
