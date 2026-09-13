#pragma once
// =============================================================================
//  StructuralCommands.h — Commandes concrètes de modélisation structurale
// =============================================================================

#include "commands/ICommand.h"
#include "structural/ModelDatabase.h"
#include <glm/glm.hpp>
#include <vector>

namespace stabileo::commands {

using structural::EntityId;

// 1. Commande de création de nœud
class CreateNodeCommand : public ICommand {
public:
    CreateNodeCommand(structural::ModelDatabase& db, double x, double y, double z);
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Créer Nœud"; }
    EntityId getCreatedNodeId() const { return createdNodeId_; }

private:
    structural::ModelDatabase& db_;
    double x_, y_, z_;
    EntityId createdNodeId_ = structural::INVALID_ID;
};

// 2. Commande de création de membre (Poteau / Poutre / Treillis) avec snap/findOrCreateNode
class CreateMemberCommand : public ICommand {
public:
    CreateMemberCommand(structural::ModelDatabase& db,
                        const glm::dvec3& startPos, const glm::dvec3& endPos,
                        structural::MemberType type,
                        EntityId sectionId = structural::INVALID_ID,
                        EntityId materialId = structural::INVALID_ID);
    bool execute() override;
    bool undo() override;
    std::string getName() const override;
    EntityId getCreatedMemberId() const { return createdMemberId_; }

private:
    structural::ModelDatabase& db_;
    glm::dvec3 startPos_, endPos_;
    structural::MemberType type_;
    EntityId sectionId_, materialId_;

    EntityId n1Id_ = structural::INVALID_ID;
    EntityId n2Id_ = structural::INVALID_ID;
    EntityId createdMemberId_ = structural::INVALID_ID;
    bool createdN1_ = false;
    bool createdN2_ = false;
};

// 3. Commande de création de dalle / panneau
class CreatePanelCommand : public ICommand {
public:
    CreatePanelCommand(structural::ModelDatabase& db,
                       const std::vector<glm::dvec3>& polygonPoints,
                       double thickness = 0.15,
                       EntityId materialId = structural::INVALID_ID,
                       structural::PanelType type = structural::PanelType::Slab);
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Créer Dalle / Panneau"; }

private:
    structural::ModelDatabase& db_;
    std::vector<glm::dvec3> points_;
    double thickness_;
    EntityId materialId_;
    structural::PanelType type_;

    std::vector<EntityId> nodeIds_;
    std::vector<bool> createdNodes_;
    EntityId createdPanelId_ = structural::INVALID_ID;
};

// 4. Commande d'assignation d'appui
class AssignSupportCommand : public ICommand {
public:
    AssignSupportCommand(structural::ModelDatabase& db, EntityId nodeId,
                         const structural::SupportCondition& condition);
    bool execute() override;
    bool undo() override;
    std::string getName() const override { return "Assigner Appui"; }

private:
    structural::ModelDatabase& db_;
    EntityId nodeId_;
    structural::SupportCondition condition_;
    EntityId prevSupportId_ = structural::INVALID_ID;
    structural::SupportCondition prevCondition_;
    EntityId newSupportId_ = structural::INVALID_ID;
};

} // namespace stabileo::commands
