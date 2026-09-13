// =============================================================================
//  CommandManager.cpp — Gestionnaire d'historique Undo / Redo
// =============================================================================

#include "commands/CommandManager.h"

namespace stabileo::commands {

bool CommandManager::executeCommand(std::unique_ptr<ICommand> cmd) {
    if (!cmd) return false;
    if (cmd->execute()) {
        undoStack_.push_back(std::move(cmd));
        redoStack_.clear();

        if (undoStack_.size() > maxHistory_) {
            undoStack_.erase(undoStack_.begin());
        }
        return true;
    }
    return false;
}

bool CommandManager::undo() {
    if (undoStack_.empty()) return false;

    auto cmd = std::move(undoStack_.back());
    undoStack_.pop_back();

    bool ok = cmd->undo();
    redoStack_.push_back(std::move(cmd));
    return ok;
}

bool CommandManager::redo() {
    if (redoStack_.empty()) return false;

    auto cmd = std::move(redoStack_.back());
    redoStack_.pop_back();

    bool ok = cmd->execute();
    undoStack_.push_back(std::move(cmd));
    return ok;
}

std::string CommandManager::getUndoName() const {
    if (undoStack_.empty()) return "";
    return undoStack_.back()->getName();
}

std::string CommandManager::getRedoName() const {
    if (redoStack_.empty()) return "";
    return redoStack_.back()->getName();
}

void CommandManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
}

} // namespace stabileo::commands
