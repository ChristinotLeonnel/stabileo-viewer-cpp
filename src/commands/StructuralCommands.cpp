// =============================================================================
//  StructuralCommands.cpp — Implémentation des commandes concrètes
// =============================================================================

#include "commands/StructuralCommands.h"

namespace stabileo::commands {

// =============================================================================
//  1. CreateNodeCommand
// =============================================================================
CreateNodeCommand::CreateNodeCommand(structural::ModelDatabase& db, double x, double y, double z)
    : db_(db), x_(x), y_(y), z_(z) {}

bool CreateNodeCommand::execute() {
    createdNodeId_ = db_.addNode(x_, y_, z_);
    return (createdNodeId_ != structural::INVALID_ID);
}

bool CreateNodeCommand::undo() {
    if (createdNodeId_ == structural::INVALID_ID) return false;
    bool ok = db_.removeNode(createdNodeId_);
    createdNodeId_ = structural::INVALID_ID;
    return ok;
}

// =============================================================================
//  2. CreateMemberCommand
// =============================================================================
CreateMemberCommand::CreateMemberCommand(structural::ModelDatabase& db,
                                         const glm::dvec3& startPos, const glm::dvec3& endPos,
                                         structural::MemberType type,
                                         EntityId sectionId, EntityId materialId)
    : db_(db), startPos_(startPos), endPos_(endPos), type_(type),
      sectionId_(sectionId), materialId_(materialId) {}

std::string CreateMemberCommand::getName() const {
    switch (type_) {
        case structural::MemberType::Column: return "Créer Poteau";
        case structural::MemberType::Beam:   return "Créer Poutre";
        case structural::MemberType::Truss:  return "Créer Barre Treillis";
        default: return "Créer Membre";
    }
}

bool CreateMemberCommand::execute() {
    glm::dvec3 diff = endPos_ - startPos_;
    if (glm::dot(diff, diff) < 1e-6) return false;

    // Détecter si les nœuds existent déjà à moins de 1 mm
    EntityId existing1 = db_.findOrCreateNode(startPos_.x, startPos_.y, startPos_.z, 1e-3);
    EntityId existing2 = db_.findOrCreateNode(endPos_.x, endPos_.y, endPos_.z, 1e-3);

    n1Id_ = existing1;
    n2Id_ = existing2;

    createdMemberId_ = db_.addMember(n1Id_, n2Id_, type_, sectionId_, materialId_);
    return (createdMemberId_ != structural::INVALID_ID);
}

bool CreateMemberCommand::undo() {
    if (createdMemberId_ == structural::INVALID_ID) return false;
    db_.removeMember(createdMemberId_);
    createdMemberId_ = structural::INVALID_ID;

    // Si les nœuds n'ont plus aucune barre connectée et ont été créés par cette commande
    auto* n1 = db_.getNode(n1Id_);
    if (n1 && n1->connectedMemberIds.empty() && n1->supportId == structural::INVALID_ID) {
        db_.removeNode(n1Id_);
    }
    auto* n2 = db_.getNode(n2Id_);
    if (n2 && n2->connectedMemberIds.empty() && n2->supportId == structural::INVALID_ID) {
        db_.removeNode(n2Id_);
    }

    return true;
}

// =============================================================================
//  3. CreatePanelCommand
// =============================================================================
CreatePanelCommand::CreatePanelCommand(structural::ModelDatabase& db,
                                       const std::vector<glm::dvec3>& polygonPoints,
                                       double thickness, EntityId materialId,
                                       structural::PanelType type)
    : db_(db), points_(polygonPoints), thickness_(thickness),
      materialId_(materialId), type_(type) {}

bool CreatePanelCommand::execute() {
    if (points_.size() < 3) return false;

    nodeIds_.clear();
    for (const auto& pt : points_) {
        EntityId nId = db_.findOrCreateNode(pt.x, pt.y, pt.z, 1e-3);
        nodeIds_.push_back(nId);
    }

    createdPanelId_ = db_.addPanel(nodeIds_, thickness_, materialId_, type_);
    return (createdPanelId_ != structural::INVALID_ID);
}

bool CreatePanelCommand::undo() {
    if (createdPanelId_ == structural::INVALID_ID) return false;
    bool ok = db_.removePanel(createdPanelId_);
    createdPanelId_ = structural::INVALID_ID;
    return ok;
}

// =============================================================================
//  4. AssignSupportCommand
// =============================================================================
AssignSupportCommand::AssignSupportCommand(structural::ModelDatabase& db, EntityId nodeId,
                                           const structural::SupportCondition& condition)
    : db_(db), nodeId_(nodeId), condition_(condition) {}

bool AssignSupportCommand::execute() {
    auto* node = db_.getNode(nodeId_);
    if (!node) return false;

    if (node->supportId != structural::INVALID_ID) {
        prevSupportId_ = node->supportId;
        auto* s = db_.getSupport(prevSupportId_);
        if (s) prevCondition_ = s->condition;
        db_.removeSupport(prevSupportId_);
    }

    newSupportId_ = db_.addSupport(nodeId_, condition_);
    return (newSupportId_ != structural::INVALID_ID);
}

bool AssignSupportCommand::undo() {
    if (newSupportId_ != structural::INVALID_ID) {
        db_.removeSupport(newSupportId_);
        newSupportId_ = structural::INVALID_ID;
    }
    if (prevSupportId_ != structural::INVALID_ID) {
        db_.addSupport(nodeId_, prevCondition_);
    }
    return true;
}

} // namespace stabileo::commands
