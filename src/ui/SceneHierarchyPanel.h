#pragma once
// =============================================================================
//  SceneHierarchyPanel.h — Panneau de Hiérarchie et Propriétés inspiré de Hazel
// =============================================================================

#include "scene/StructureModel.h"
#include "render/StructureRenderer.h"
#include "ui/HazelUI.h"
#include <string>

enum class SelectionType {
    None,
    Node,
    Element
};

class SceneHierarchyPanel {
public:
    SceneHierarchyPanel() = default;
    explicit SceneHierarchyPanel(model::Structure* structure) : context_(structure) {}

    void setContext(model::Structure* structure) {
        context_ = structure;
        selectedType_ = SelectionType::None;
        selectedId_ = -1;
    }

    void onImGuiRender(RenderState& state, bool* p_openHierarchy = nullptr, bool* p_openProperties = nullptr);

    SelectionType getSelectedType() const { return selectedType_; }
    int getSelectedId() const { return selectedId_; }

    void selectNode(int id) {
        selectedType_ = SelectionType::Node;
        selectedId_ = id;
    }

    void selectElement(int id) {
        selectedType_ = SelectionType::Element;
        selectedId_ = id;
    }

    void clearSelection() {
        selectedType_ = SelectionType::None;
        selectedId_ = -1;
    }

private:
    void drawHierarchy(RenderState& state, bool* p_open = nullptr);
    void drawProperties(RenderState& state, bool* p_open = nullptr);

    void drawNodeComponents(model::Node& node, RenderState& state);
    void drawElementComponents(model::Element& elem, RenderState& state);

    model::Structure* context_ = nullptr;
    SelectionType     selectedType_ = SelectionType::None;
    int               selectedId_ = -1;

    char searchFilter_[128] = "";
};
