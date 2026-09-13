#pragma once
// =============================================================================
//  CommandManager.h — Gestionnaire d'historique Undo / Redo
// =============================================================================

#include "commands/ICommand.h"
#include <memory>
#include <vector>
#include <string>

namespace stabileo::commands {

class CommandManager {
public:
    CommandManager() = default;

    bool executeCommand(std::unique_ptr<ICommand> cmd);
    bool undo();
    bool redo();

    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }

    std::string getUndoName() const;
    std::string getRedoName() const;

    const std::vector<std::unique_ptr<ICommand>>& getUndoStack() const { return undoStack_; }
    void clear();

private:
    std::vector<std::unique_ptr<ICommand>> undoStack_;
    std::vector<std::unique_ptr<ICommand>> redoStack_;
    size_t maxHistory_ = 100;
};

} // namespace stabileo::commands
