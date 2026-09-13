// =============================================================================
//  ModelDatabase.cpp — Implémentation du registre structural
// =============================================================================

#include "structural/ModelDatabase.h"
#include <cmath>
#include <algorithm>

namespace stabileo::structural {

ModelDatabase::ModelDatabase() {
    initStandardCatalogs();
}

void ModelDatabase::initStandardCatalogs() {
    // 1. Matériau Béton C25/30
    Material c25;
    c25.id = nextId_++;
    c25.name = "Béton C25/30";
    c25.category = MaterialCategory::Concrete;
    c25.E = 31.0e9;
    c25.nu = 0.2;
    c25.rho = 2500.0;
    c25.fck = 25.0e6;
    materials_[c25.id] = c25;

    // 2. Matériau Acier S355
    Material s355;
    s355.id = nextId_++;
    s355.name = "Acier S355";
    s355.category = MaterialCategory::Steel;
    s355.E = 210.0e9;
    s355.nu = 0.3;
    s355.rho = 7850.0;
    s355.fck = 355.0e6;
    materials_[s355.id] = s355;

    // 3. Section Poteau 30x30 cm
    Section col30;
    col30.id = nextId_++;
    col30.name = "Poteau 30x30 cm";
    col30.shape = Section::Shape::Rectangle;
    col30.width = 0.30;
    col30.height = 0.30;
    col30.A = 0.09;
    col30.Iy = 0.30 * 0.30 * 0.30 * 0.30 / 12.0;
    col30.Iz = col30.Iy;
    col30.It = 1.14e-3;
    sections_[col30.id] = col30;

    // 4. Section Poutre 25x50 cm
    Section beam25x50;
    beam25x50.id = nextId_++;
    beam25x50.name = "Poutre 25x50 cm";
    beam25x50.shape = Section::Shape::Rectangle;
    beam25x50.width = 0.25;
    beam25x50.height = 0.50;
    beam25x50.A = 0.125;
    beam25x50.Iy = 0.25 * 0.50 * 0.50 * 0.50 / 12.0;
    beam25x50.Iz = 0.50 * 0.25 * 0.25 * 0.25 / 12.0;
    beam25x50.It = 1.85e-3;
    sections_[beam25x50.id] = beam25x50;

    // 5. Cas de charges par défaut (G et Q)
    addLoadCase("G - Poids propre & Permanente", LoadCaseNature::Dead, true);
    addLoadCase("Q - Exploitation", LoadCaseNature::Live, false);
}

EntityId ModelDatabase::addNode(double x, double y, double z) {
    EntityId id = nextId_++;
    Node node;
    node.id = id;
    node.position = glm::dvec3(x, y, z);
    nodes_[id] = node;
    markModified();
    return id;
}

EntityId ModelDatabase::findOrCreateNode(double x, double y, double z, double tolerance) {
    glm::dvec3 target(x, y, z);
    double tolSq = tolerance * tolerance;
    for (const auto& [id, node] : nodes_) {
        double distSq = glm::dot(node.position - target, node.position - target);
        if (distSq <= tolSq) {
            return id;
        }
    }
    return addNode(x, y, z);
}

bool ModelDatabase::removeNode(EntityId id) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) return false;

    // Nettoyer les barres connectées
    std::vector<EntityId> membersToRemove = it->second.connectedMemberIds;
    for (EntityId mId : membersToRemove) {
        removeMember(mId);
    }

    if (it->second.supportId != INVALID_ID) {
        removeSupport(it->second.supportId);
    }

    nodes_.erase(it);
    markModified();
    return true;
}

Node* ModelDatabase::getNode(EntityId id) {
    auto it = nodes_.find(id);
    return (it != nodes_.end()) ? &it->second : nullptr;
}

const Node* ModelDatabase::getNode(EntityId id) const {
    auto it = nodes_.find(id);
    return (it != nodes_.end()) ? &it->second : nullptr;
}

EntityId ModelDatabase::addMember(EntityId startNodeId, EntityId endNodeId, MemberType type,
                                 EntityId sectionId, EntityId materialId) {
    if (startNodeId == endNodeId) return INVALID_ID;
    if (nodes_.find(startNodeId) == nodes_.end() || nodes_.find(endNodeId) == nodes_.end()) {
        return INVALID_ID;
    }

    EntityId id = nextId_++;
    Member m;
    m.id = id;
    m.startNodeId = startNodeId;
    m.endNodeId = endNodeId;
    m.type = type;

    // Assigner sections et matériaux par défaut si non spécifiés
    if (sectionId == INVALID_ID && !sections_.empty()) {
        m.sectionId = (type == MemberType::Column) ? sections_.begin()->first : std::next(sections_.begin())->first;
    } else {
        m.sectionId = sectionId;
    }

    if (materialId == INVALID_ID && !materials_.empty()) {
        m.materialId = materials_.begin()->first;
    } else {
        m.materialId = materialId;
    }

    members_[id] = m;
    nodes_[startNodeId].connectedMemberIds.push_back(id);
    nodes_[endNodeId].connectedMemberIds.push_back(id);

    markModified();
    return id;
}

bool ModelDatabase::removeMember(EntityId id) {
    auto it = members_.find(id);
    if (it == members_.end()) return false;

    // Détacher des nœuds d'extrémités
    auto* n1 = getNode(it->second.startNodeId);
    if (n1) {
        auto& v = n1->connectedMemberIds;
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
    }
    auto* n2 = getNode(it->second.endNodeId);
    if (n2) {
        auto& v = n2->connectedMemberIds;
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
    }

    members_.erase(it);
    markModified();
    return true;
}

Member* ModelDatabase::getMember(EntityId id) {
    auto it = members_.find(id);
    return (it != members_.end()) ? &it->second : nullptr;
}

const Member* ModelDatabase::getMember(EntityId id) const {
    auto it = members_.find(id);
    return (it != members_.end()) ? &it->second : nullptr;
}

EntityId ModelDatabase::addPanel(const std::vector<EntityId>& boundaryNodeIds, double thickness,
                                EntityId materialId, PanelType type) {
    if (boundaryNodeIds.size() < 3) return INVALID_ID;

    EntityId id = nextId_++;
    Panel p;
    p.id = id;
    p.boundaryNodeIds = boundaryNodeIds;
    p.thickness = thickness;
    p.type = type;
    p.materialId = (materialId != INVALID_ID) ? materialId : (!materials_.empty() ? materials_.begin()->first : INVALID_ID);

    panels_[id] = p;
    for (EntityId nId : boundaryNodeIds) {
        auto* n = getNode(nId);
        if (n) n->connectedPanelIds.push_back(id);
    }

    markModified();
    return id;
}

bool ModelDatabase::removePanel(EntityId id) {
    auto it = panels_.find(id);
    if (it == panels_.end()) return false;

    for (EntityId nId : it->second.boundaryNodeIds) {
        auto* n = getNode(nId);
        if (n) {
            auto& v = n->connectedPanelIds;
            v.erase(std::remove(v.begin(), v.end(), id), v.end());
        }
    }

    panels_.erase(it);
    markModified();
    return true;
}

Panel* ModelDatabase::getPanel(EntityId id) {
    auto it = panels_.find(id);
    return (it != panels_.end()) ? &it->second : nullptr;
}

const Panel* ModelDatabase::getPanel(EntityId id) const {
    auto it = panels_.find(id);
    return (it != panels_.end()) ? &it->second : nullptr;
}

EntityId ModelDatabase::addMaterial(const Material& mat) {
    EntityId id = nextId_++;
    Material m = mat;
    m.id = id;
    materials_[id] = m;
    markModified();
    return id;
}

Material* ModelDatabase::getMaterial(EntityId id) {
    auto it = materials_.find(id);
    return (it != materials_.end()) ? &it->second : nullptr;
}

const Material* ModelDatabase::getMaterial(EntityId id) const {
    auto it = materials_.find(id);
    return (it != materials_.end()) ? &it->second : nullptr;
}

EntityId ModelDatabase::addSection(const Section& sec) {
    EntityId id = nextId_++;
    Section s = sec;
    s.id = id;
    sections_[id] = s;
    markModified();
    return id;
}

Section* ModelDatabase::getSection(EntityId id) {
    auto it = sections_.find(id);
    return (it != sections_.end()) ? &it->second : nullptr;
}

const Section* ModelDatabase::getSection(EntityId id) const {
    auto it = sections_.find(id);
    return (it != sections_.end()) ? &it->second : nullptr;
}

EntityId ModelDatabase::addSupport(EntityId nodeId, const SupportCondition& cond) {
    auto* node = getNode(nodeId);
    if (!node) return INVALID_ID;

    EntityId id = nextId_++;
    Support supp;
    supp.id = id;
    supp.nodeId = nodeId;
    supp.condition = cond;

    supports_[id] = supp;
    node->supportId = id;
    markModified();
    return id;
}

bool ModelDatabase::removeSupport(EntityId id) {
    auto it = supports_.find(id);
    if (it == supports_.end()) return false;

    auto* n = getNode(it->second.nodeId);
    if (n && n->supportId == id) {
        n->supportId = INVALID_ID;
    }

    supports_.erase(it);
    markModified();
    return true;
}

Support* ModelDatabase::getSupport(EntityId id) {
    auto it = supports_.find(id);
    return (it != supports_.end()) ? &it->second : nullptr;
}

const Support* ModelDatabase::getSupport(EntityId id) const {
    auto it = supports_.find(id);
    return (it != supports_.end()) ? &it->second : nullptr;
}

EntityId ModelDatabase::addLoadCase(const std::string& name, LoadCaseNature nature, bool selfWeight) {
    EntityId id = nextId_++;
    LoadCase lc;
    lc.id = id;
    lc.name = name;
    lc.nature = nature;
    lc.includeSelfWeight = selfWeight;
    loadCases_[id] = lc;
    markModified();
    return id;
}

LoadCase* ModelDatabase::getLoadCase(EntityId id) {
    auto it = loadCases_.find(id);
    return (it != loadCases_.end()) ? &it->second : nullptr;
}

EntityId ModelDatabase::addLoad(const Load& load) {
    EntityId id = nextId_++;
    Load l = load;
    l.id = id;
    loads_[id] = l;
    markModified();
    return id;
}

bool ModelDatabase::removeLoad(EntityId id) {
    auto it = loads_.find(id);
    if (it == loads_.end()) return false;
    loads_.erase(it);
    markModified();
    return true;
}

EntityId ModelDatabase::addCombination(const LoadCombination& combo) {
    EntityId id = nextId_++;
    LoadCombination c = combo;
    c.id = id;
    combinations_[id] = c;
    markModified();
    return id;
}

void ModelDatabase::clear() {
    nodes_.clear();
    members_.clear();
    panels_.clear();
    materials_.clear();
    sections_.clear();
    supports_.clear();
    loadCases_.clear();
    loads_.clear();
    combinations_.clear();
    nextId_ = 1;
    initStandardCatalogs();
    markModified();
}

} // namespace stabileo::structural
