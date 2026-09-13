#pragma once
// =============================================================================
//  ModelDatabase.h — Registre central du modèle structural (style Robot)
// =============================================================================

#include "structural/StructuralTypes.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace stabileo::structural {

class ModelDatabase {
public:
    ModelDatabase();

    // --- Nœuds ---
    EntityId addNode(double x, double y, double z);
    bool removeNode(EntityId id);
    Node* getNode(EntityId id);
    const Node* getNode(EntityId id) const;
    const std::unordered_map<EntityId, Node>& getNodes() const { return nodes_; }
    EntityId findOrCreateNode(double x, double y, double z, double tolerance = 1e-4);

    // --- Membres (Poutres, Poteaux, Treillis) ---
    EntityId addMember(EntityId startNodeId, EntityId endNodeId, MemberType type,
                       EntityId sectionId = INVALID_ID, EntityId materialId = INVALID_ID);
    bool removeMember(EntityId id);
    Member* getMember(EntityId id);
    const Member* getMember(EntityId id) const;
    const std::unordered_map<EntityId, Member>& getMembers() const { return members_; }

    // --- Panneaux (Dalles, Voiles) ---
    EntityId addPanel(const std::vector<EntityId>& boundaryNodeIds, double thickness,
                      EntityId materialId, PanelType type = PanelType::Slab);
    bool removePanel(EntityId id);
    Panel* getPanel(EntityId id);
    const Panel* getPanel(EntityId id) const;
    const std::unordered_map<EntityId, Panel>& getPanels() const { return panels_; }

    // --- Matériaux & Sections ---
    EntityId addMaterial(const Material& mat);
    Material* getMaterial(EntityId id);
    const Material* getMaterial(EntityId id) const;
    const std::unordered_map<EntityId, Material>& getMaterials() const { return materials_; }

    EntityId addSection(const Section& sec);
    Section* getSection(EntityId id);
    const Section* getSection(EntityId id) const;
    const std::unordered_map<EntityId, Section>& getSections() const { return sections_; }

    // --- Appuis ---
    EntityId addSupport(EntityId nodeId, const SupportCondition& cond);
    bool removeSupport(EntityId id);
    Support* getSupport(EntityId id);
    const Support* getSupport(EntityId id) const;
    const std::unordered_map<EntityId, Support>& getSupports() const { return supports_; }

    // --- Cas de charge & Charges ---
    EntityId addLoadCase(const std::string& name, LoadCaseNature nature, bool selfWeight = true);
    LoadCase* getLoadCase(EntityId id);
    const std::unordered_map<EntityId, LoadCase>& getLoadCases() const { return loadCases_; }

    EntityId addLoad(const Load& load);
    bool removeLoad(EntityId id);
    const std::unordered_map<EntityId, Load>& getLoads() const { return loads_; }

    // --- Combinaisons ---
    EntityId addCombination(const LoadCombination& combo);
    const std::unordered_map<EntityId, LoadCombination>& getCombinations() const { return combinations_; }

    // --- Utilitaires & Gestion de révision ---
    void clear();
    uint64_t getRevision() const { return revision_; }
    void markModified() { ++revision_; }

    // Initialise les matériaux et sections standards par défaut (Béton C25/30, Acier S355, Poteau 30x30, Poutre 25x50)
    void initStandardCatalogs();

private:
    EntityId nextId_ = 1;
    uint64_t revision_ = 0;

    std::unordered_map<EntityId, Node>            nodes_;
    std::unordered_map<EntityId, Member>          members_;
    std::unordered_map<EntityId, Panel>           panels_;
    std::unordered_map<EntityId, Material>        materials_;
    std::unordered_map<EntityId, Section>         sections_;
    std::unordered_map<EntityId, Support>         supports_;
    std::unordered_map<EntityId, LoadCase>        loadCases_;
    std::unordered_map<EntityId, Load>            loads_;
    std::unordered_map<EntityId, LoadCombination> combinations_;
};

} // namespace stabileo::structural
